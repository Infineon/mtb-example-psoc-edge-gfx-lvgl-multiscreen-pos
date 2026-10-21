/*******************************************************************************
* File Name        : lv_port_disp.c
*
* Description      : This file provides implementation of low level display
*                    device driver for LVGL.
*
* Related Document : See README.md
*
********************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "lv_port_disp.h"
#include "lv_draw_sw.h"
#include <stdbool.h>
#include <string.h>
#include "cy_graphics.h"


#if LV_COLOR_DEPTH == 16
    #define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
#elif LV_COLOR_DEPTH == 32
    #define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_ARGB8888))
#endif


/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Two full-screen buffers are always allocated: LVGL renders into the back
 * buffer while the display controller scans out the front one, which is what
 * keeps these video-mode panels tear-free. Each buffer is
 * MY_DISP_HOR_RES x MY_DISP_VER_RES x BYTE_PER_PIXEL - 832 x 480 x 2 =
 * 798,720 B (~780 KB) on the default 4.3-inch panel, so ~1560 KB in total. */
CY_SECTION(".cy_gpu_buf") LV_ATTRIBUTE_MEM_ALIGN uint8_t disp_buf1[MY_DISP_HOR_RES *
                                               MY_DISP_VER_RES * BYTE_PER_PIXEL];
CY_SECTION(".cy_gpu_buf") LV_ATTRIBUTE_MEM_ALIGN uint8_t disp_buf2[MY_DISP_HOR_RES *
                                               MY_DISP_VER_RES * BYTE_PER_PIXEL];

/* Frame buffers used by GFXSS to render UI */
void *frame_buffer1 = &disp_buf1;
void *frame_buffer2 = &disp_buf2;

cy_stc_gfx_context_t gfx_context;


/*******************************************************************************
* Function Name: disp_flush
********************************************************************************
* Summary:
*  Flush the content of the internal buffer to the specific area on the display.
*  You can use DMA or any hardware acceleration to do this operation in the
*  background but 'lv_display_flush_ready()' has to be called when finished.
*
* Parameters:
*  *disp_drv: Pointer to the display driver structure to be registered by HAL.
*  *area: Pointer to the area of the screen (not used).
*  *color_p: Pointer to the frame buffer address.
*
* Return:
*  None
*
*******************************************************************************/
static void LV_ATTRIBUTE_FAST_MEM disp_flush(lv_display_t *disp_drv,
                                             const lv_area_t *area,
                                             uint8_t *color_p)
{
    CY_UNUSED_PARAMETER(area);

    /* Drain any stale vsync notification that arrived while LVGL was rendering.
     * Without this, the wait below can consume an OLD vsync that fired before
     * Set_FrameBuffer - meaning LVGL starts writing to the previous front buffer
     * while the DC is still reading it, causing visible flicker. */
    ulTaskNotifyTake(pdTRUE, 0);

    Cy_GFXSS_Set_FrameBuffer((GFXSS_Type*) GFXSS, (uint32_t*) color_p,
            &gfx_context);

    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY))
    {
        /* Inform the graphics library that you are ready with the flushing */
        lv_display_flush_ready(disp_drv);
    }
}


/*******************************************************************************
* Function Name: lv_port_disp_init
********************************************************************************
* Summary:
*  Initialization function for display devices supported by LVGL.
*  Registers the LVGL display with the two full-screen frame buffers in
*  LV_DISPLAY_RENDER_MODE_FULL, so every refresh hands `disp_flush()` a whole
*  rendered screen and flushing only reprograms the frame-buffer address.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void lv_port_disp_init(void)
{
    memset(disp_buf1, 0, sizeof(disp_buf1));
    memset(disp_buf2, 0, sizeof(disp_buf2));

    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);

    lv_display_set_flush_cb(disp, disp_flush);

    lv_tick_set_cb(xTaskGetTickCount);

    /* Use explicit stride to prevent stride_is_auto recalculation.
     * Buffer is MY_DISP_HOR_RES(832) wide for VG-Lite 16px alignment,
     * but logical display is ACTUAL_DISP_HOR_RES(800).
     * Without explicit stride, lv_refr.c recalculates stride from buf_area width (800)
     * instead of buffer width (832), causing garbled display output. */
    uint32_t buf_stride = MY_DISP_HOR_RES * LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_NATIVE);

    lv_display_set_buffers_with_stride(disp, disp_buf1, disp_buf2, sizeof(disp_buf1),
                                       buf_stride, LV_DISPLAY_RENDER_MODE_FULL);

    /* Set the display resolution to match the physical resolution so that
     * LVGL renders the UI within the limits of the actual display resolution.
     */
    lv_display_set_resolution(disp, ACTUAL_DISP_HOR_RES, ACTUAL_DISP_VER_RES);

    Cy_GFXSS_Clear_DC_Interrupt((GFXSS_Type*) GFXSS, &gfx_context);
}


/* [] END OF FILE */

/*******************************************************************************
* File Name        : ui_layout.h
*
* Description      : Responsive layout helpers. Turns the runtime display
*                    resolution into a size class and derives paddings, fonts
*                    and the shared header height from it, so the same UI code
*                    fits the 480x480, 800x480 and 1024x600 panels supported by
*                    this BSP family.
*
* Related Document : See docs/design_and_implementation.md
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

#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* Data Structures
*******************************************************************************/

/* Size class derived from the visible display width. */
typedef enum
{
    UI_SIZE_COMPACT = 0,    /* < UI_BREAKPOINT_COMPACT_PX, e.g. 480x480.   */
    UI_SIZE_REGULAR,        /* e.g. 800x480.                               */
    UI_SIZE_WIDE,           /* >= UI_BREAKPOINT_WIDE_PX, e.g. 1024x600.    */
} ui_size_class_t;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

ui_size_class_t ui_layout_size_class(void);

int32_t ui_layout_width(void);

int32_t ui_layout_height(void);

bool ui_layout_is_square(void);

int32_t ui_layout_header_height(void);

int32_t ui_layout_pad(void);

int32_t ui_layout_radius(void);

const lv_font_t * ui_layout_font_title(void);

const lv_font_t * ui_layout_font_body(void);

const lv_font_t * ui_layout_font_small(void);

int32_t ui_layout_grid_columns(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UI_LAYOUT_H */

/* [] END OF FILE */

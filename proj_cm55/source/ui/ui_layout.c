/*******************************************************************************
* File Name        : ui_layout.c
*
* Description      : Responsive layout helpers. Owns the breakpoint table that
*                    maps the runtime display resolution onto paddings, fonts
*                    and the shared header height.
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

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "ui_layout.h"
#include "ui_config.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* A panel is treated as square when width and height differ by less than this
 * fraction of the width. The 480x480 4-inch TFT is the case we care about. */
#define SQUARE_TOLERANCE_NUM            (10)
#define SQUARE_TOLERANCE_DEN            (100)

/*******************************************************************************
* Data Structures
*******************************************************************************/
typedef struct
{
    int32_t header_height;
    int32_t pad;
    int32_t radius;
    int32_t grid_columns;
} ui_layout_metrics_t;

/*******************************************************************************
* Global Variables
*******************************************************************************/
static const ui_layout_metrics_t metrics[] =
{
    /* UI_SIZE_COMPACT: 480x480. Vertical space is scarce, keep the header
     * low. */
    [UI_SIZE_COMPACT] =
    {
        .header_height = 44, .pad =  8, .radius =  8, .grid_columns = 2
    },
    /* UI_SIZE_REGULAR: 800x480. */
    [UI_SIZE_REGULAR] =
    {
        .header_height = 52, .pad = 12, .radius = 10, .grid_columns = 3
    },
    /* UI_SIZE_WIDE: 1024x600. */
    [UI_SIZE_WIDE] =
    {
        .header_height = 64, .pad = 16, .radius = 12, .grid_columns = 3
    },
};

/*******************************************************************************
* Function Name: current_metrics
********************************************************************************
* Summary:
*  Looks up the metric row for the active size class.
*
* Parameters:
*  None
*
* Return:
*  const ui_layout_metrics_t *: Metric row. Never NULL.
*
*******************************************************************************/
static const ui_layout_metrics_t * current_metrics(void)
{
    return &metrics[ui_layout_size_class()];
}


/*******************************************************************************
* Function Name: ui_layout_width
********************************************************************************
* Summary:
*  Returns the visible width of the default display.
*
* Parameters:
*  None
*
* Return:
*  int32_t: Width in pixels.
*
*******************************************************************************/
int32_t ui_layout_width(void)
{
    return lv_display_get_horizontal_resolution(lv_display_get_default());
}


/*******************************************************************************
* Function Name: ui_layout_height
********************************************************************************
* Summary:
*  Returns the visible height of the default display.
*
* Parameters:
*  None
*
* Return:
*  int32_t: Height in pixels.
*
*******************************************************************************/
int32_t ui_layout_height(void)
{
    return lv_display_get_vertical_resolution(lv_display_get_default());
}


/*******************************************************************************
* Function Name: ui_layout_size_class
********************************************************************************
* Summary:
*  Maps the visible display width onto a size class using the breakpoints from
*  ui_config.h.
*
*  Do not use MY_DISP_HOR_RES for layout: on the 4.3-inch panel that macro is
*  the padded GPU stride (832 px), not the 800 px the user can actually see.
*
* Parameters:
*  None
*
* Return:
*  ui_size_class_t: Size class.
*
*******************************************************************************/
ui_size_class_t ui_layout_size_class(void)
{
    int32_t width = ui_layout_width();

    if (width < UI_BREAKPOINT_COMPACT_PX)
    {
        return UI_SIZE_COMPACT;
    }

    if (width >= UI_BREAKPOINT_WIDE_PX)
    {
        return UI_SIZE_WIDE;
    }

    return UI_SIZE_REGULAR;
}


/*******************************************************************************
* Function Name: ui_layout_is_square
********************************************************************************
* Summary:
*  Reports whether the panel is close enough to 1:1 (for example the 480x480
*  4-inch TFT) that side-by-side layouts should be stacked instead.
*
* Parameters:
*  None
*
* Return:
*  bool: true for (near) square panels.
*
*******************************************************************************/
bool ui_layout_is_square(void)
{
    int32_t width  = ui_layout_width();
    int32_t height = ui_layout_height();
    int32_t delta  = (width > height) ? (width - height) : (height - width);

    return (delta * SQUARE_TOLERANCE_DEN) < (width * SQUARE_TOLERANCE_NUM);
}


/*******************************************************************************
* Function Name: ui_layout_header_height
********************************************************************************
* Summary:
*  Height of the shared header bar for the active size class.
*
* Parameters:
*  None
*
* Return:
*  int32_t: Height in pixels.
*
*******************************************************************************/
int32_t ui_layout_header_height(void)
{
    return current_metrics()->header_height;
}


/*******************************************************************************
* Function Name: ui_layout_pad
********************************************************************************
* Summary:
*  Default padding and flex gap for the active size class.
*
* Parameters:
*  None
*
* Return:
*  int32_t: Padding in pixels.
*
*******************************************************************************/
int32_t ui_layout_pad(void)
{
    return current_metrics()->pad;
}


/*******************************************************************************
* Function Name: ui_layout_radius
********************************************************************************
* Summary:
*  Corner radius for cards and buttons.
*
* Parameters:
*  None
*
* Return:
*  int32_t: Radius in pixels.
*
*******************************************************************************/
int32_t ui_layout_radius(void)
{
    return current_metrics()->radius;
}


/*******************************************************************************
* Function Name: ui_layout_grid_columns
********************************************************************************
* Summary:
*  Number of columns a tile grid should use on this panel.
*
* Parameters:
*  None
*
* Return:
*  int32_t: Column count: 2 on compact panels, 3 otherwise.
*
*******************************************************************************/
int32_t ui_layout_grid_columns(void)
{
    return current_metrics()->grid_columns;
}


/*******************************************************************************
* Function Name: ui_layout_font_title
********************************************************************************
* Summary:
*  Font for page titles and large figures in the active size class. Only fonts
*  enabled in lv_conf.h may be referenced here.
*
* Parameters:
*  None
*
* Return:
*  const lv_font_t *: Font.
*
*******************************************************************************/
const lv_font_t * ui_layout_font_title(void)
{
    switch (ui_layout_size_class())
    {
        case UI_SIZE_COMPACT:
            return &lv_font_montserrat_20;
        case UI_SIZE_WIDE:
            return &lv_font_montserrat_24;
        case UI_SIZE_REGULAR:
        default:
            return &lv_font_montserrat_22;
    }
}


/*******************************************************************************
* Function Name: ui_layout_font_body
********************************************************************************
* Summary:
*  Font for body text and list rows in the active size class.
*
* Parameters:
*  None
*
* Return:
*  const lv_font_t *: Font.
*
*******************************************************************************/
const lv_font_t * ui_layout_font_body(void)
{
    switch (ui_layout_size_class())
    {
        case UI_SIZE_COMPACT:
            return &lv_font_montserrat_14;
        case UI_SIZE_WIDE:
            return &lv_font_montserrat_20;
        case UI_SIZE_REGULAR:
        default:
            return &lv_font_montserrat_16;
    }
}


/*******************************************************************************
* Function Name: ui_layout_font_small
********************************************************************************
* Summary:
*  Font for captions, units and secondary information in the active size class.
*
* Parameters:
*  None
*
* Return:
*  const lv_font_t *: Font.
*
*******************************************************************************/
const lv_font_t * ui_layout_font_small(void)
{
    switch (ui_layout_size_class())
    {
        case UI_SIZE_COMPACT:
            return &lv_font_montserrat_12;
        case UI_SIZE_WIDE:
            return &lv_font_montserrat_16;
        case UI_SIZE_REGULAR:
        default:
            return &lv_font_montserrat_14;
    }
}

/* [] END OF FILE */

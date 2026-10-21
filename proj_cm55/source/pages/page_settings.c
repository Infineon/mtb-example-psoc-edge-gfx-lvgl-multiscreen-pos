/*******************************************************************************
* File Name        : page_settings.c
*
* Description      : Settings page. Exposes the colour scheme, the dark/light
*                    switch and the performance monitor switch, and
*                    reports how the navigation layer is currently configured,
*                    so the behaviour on the panel can be matched to the
*                    build-time options in ui_config.h without reading the
*                    source.
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
#include "ui_pages.h"
#include "ui_nav.h"
#include "ui_theme.h"
#include "ui_header.h"
#include "ui_config.h"
#include "ui_spring_anim.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* lv_label_set_text_fmt() is backed by LVGL's own printf, which is built
 * without floating point support. Ratios are therefore scaled to hundredths
 * and printed as two integers. */
#define RATIO_SCALE                     (100)

/*******************************************************************************
* Global Variables
*******************************************************************************/
static lv_obj_t * theme_switch;
static lv_obj_t * scheme_dropdown;

/*******************************************************************************
* Function Name: theme_switch_event_cb
********************************************************************************
* Summary:
*  Applies the colour mode chosen with the switch.
*
*  Nothing is rebuilt here. ui_theme_toggle() rewrites the shared styles and
*  asks LVGL to re-evaluate them, which is why the switch itself, the header
*  and every page already created all change colour in the same frame.
*
* Parameters:
*  e: Value-changed event.
*
* Return:
*  None
*
*******************************************************************************/
static void theme_switch_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    ui_theme_toggle();
    ui_header_refresh_theme_icon();
}


/*******************************************************************************
* Function Name: theme_switch_delete_cb
********************************************************************************
* Summary:
*  Drops the cached handle when the switch goes away. Pages are kept alive in
*  this code example, but an application that evicts pages to save memory would
*  otherwise leave page_settings_sync_theme_switch() with a dangling pointer.
*
* Parameters:
*  e: LV_EVENT_DELETE event.
*
* Return:
*  None
*
*******************************************************************************/
static void theme_switch_delete_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    theme_switch = NULL;
}


/*******************************************************************************
* Function Name: page_settings_sync_theme_switch
********************************************************************************
* Summary:
*  Realigns the theme switch with the colour mode actually in force. Call it
*  after the theme is changed from anywhere other than the switch itself, for
*  example with SW4 while this page is on screen; a control that disagrees with
*  what the user is looking at is worse than no control at all. Safe to call
*  before the page exists.
*
*  lv_obj_add_state()/lv_obj_remove_state() do not raise LV_EVENT_VALUE_CHANGED,
*  so this cannot loop back into theme_switch_event_cb().
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void page_settings_sync_theme_switch(void)
{
    if (NULL == theme_switch)
    {
        return;
    }

    if (UI_THEME_DARK == ui_theme_get_mode())
    {
        lv_obj_add_state(theme_switch, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_remove_state(theme_switch, LV_STATE_CHECKED);
    }
}


/*******************************************************************************
* Function Name: add_switch_row
********************************************************************************
* Summary:
*  Adds a row carrying a label and a switch.
*
* Parameters:
*  parent: Card object.
*  label: Row text.
*  checked: Initial switch state.
*
* Return:
*  lv_obj_t *: The switch.
*
*******************************************************************************/
static lv_obj_t * add_switch_row(lv_obj_t * parent, const char * label,
                                 bool checked)
{
    lv_obj_t * row = ui_page_add_row(parent, label, "", false);
    lv_obj_t * sw;

    /* Replace the value label with a real control. */
    lv_obj_delete(lv_obj_get_child(row, 1));

    sw = lv_switch_create(row);
    lv_obj_add_style(sw, ui_style_switch_knob(), LV_PART_KNOB);

    if (checked)
    {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }

    return sw;
}


#if LV_USE_SYSMON && LV_USE_PERF_MONITOR
/*******************************************************************************
* Function Name: perf_switch_event_cb
********************************************************************************
* Summary:
*  Shows or hides LVGL's performance monitor overlay. It is hidden at boot;
*  the monitor itself runs either way.
*
* Parameters:
*  e: Value-changed event.
*
* Return:
*  None
*
*******************************************************************************/
static void perf_switch_event_cb(lv_event_t * e)
{
    if (lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED))
    {
        lv_sysmon_show_performance(NULL);
    }
    else
    {
        lv_sysmon_hide_performance(NULL);
    }
}
#endif


/*******************************************************************************
* Function Name: scheme_event_cb
********************************************************************************
* Summary:
*  Applies the colour scheme picked from the list. The dark/light mode is kept.
*
* Parameters:
*  e: Value-changed event.
*
* Return:
*  None
*
*******************************************************************************/
static void scheme_event_cb(lv_event_t * e)
{
    ui_theme_set_scheme((ui_theme_scheme_t)
                        lv_dropdown_get_selected(lv_event_get_target(e)));
}


/*******************************************************************************
* Function Name: add_scheme_row
********************************************************************************
* Summary:
*  Adds a row carrying a label and the colour scheme dropdown.
*
* Parameters:
*  parent: Card object.
*
* Return:
*  lv_obj_t *: The dropdown.
*
*******************************************************************************/
static lv_obj_t * add_scheme_row(lv_obj_t * parent)
{
    lv_obj_t * row = ui_page_add_row(parent, "Theme", "", false);
    lv_obj_t * dropdown;
    lv_obj_t * list;
    uint32_t i;

    /* Replace the value label with a real control. */
    lv_obj_delete(lv_obj_get_child(row, 1));

    dropdown = lv_dropdown_create(row);
    lv_dropdown_clear_options(dropdown);

    for (i = 0U; i < (uint32_t)UI_THEME_SCHEME_COUNT; i++)
    {
        lv_dropdown_add_option(dropdown,
                               ui_theme_scheme_name((ui_theme_scheme_t)i),
                               LV_DROPDOWN_POS_LAST);
    }

    lv_dropdown_set_selected(dropdown, (uint32_t)ui_theme_get_scheme());
    lv_obj_set_width(dropdown, LV_PCT(40));
    /* The stock pressed style animates a recolour over the gradient, and every
     * intermediate colour costs the GPU a new cached gradient. */
    lv_obj_remove_style(dropdown, NULL, LV_STATE_PRESSED);
    lv_obj_add_style(dropdown, ui_style_btn_secondary(), 0);
    ui_page_add_press_style(dropdown);

    /* The list is a separate object on the screen, so it needs the shared
     * styles too, including the highlighted option. */
    list = lv_dropdown_get_list(dropdown);
    lv_obj_remove_style(list, NULL,
                        (lv_style_selector_t)LV_PART_SELECTED |
                        (lv_style_selector_t)LV_STATE_PRESSED);
    lv_obj_add_style(list, ui_style_card(), 0);
    /* LVGL redraws the whole option text in the highlight clipped to one row,
     * so the list and the highlight must use the same font. */
    lv_obj_add_style(list, ui_style_body(), 0);
    lv_obj_add_style(list, ui_style_btn_primary(),
                     (lv_style_selector_t)LV_PART_SELECTED |
                     (lv_style_selector_t)LV_STATE_CHECKED);

    return dropdown;
}


/*******************************************************************************
* Function Name: page_settings_defocus
********************************************************************************
* Summary:
*  Defocus callback. Closes the colour scheme list, which lives on the screen
*  rather than on this page and would otherwise stay open over the next page.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void page_settings_defocus(void)
{
    if (NULL != scheme_dropdown)
    {
        lv_dropdown_close(scheme_dropdown);
    }
}


/*******************************************************************************
* Function Name: page_settings_create
********************************************************************************
* Summary:
*  Builds the settings page.
*
* Parameters:
*  parent: Page container.
*
* Return:
*  None
*
*******************************************************************************/
void page_settings_create(lv_obj_t * parent)
{
    const ui_spring_params_t * spring = ui_spring_default_params();
    int32_t zeta = (int32_t)(ui_spring_damping_ratio(spring) *
                             (float)RATIO_SCALE);
    lv_obj_t * appearance;
    lv_obj_t * motion;
    lv_obj_t * row;
#if LV_USE_SYSMON && LV_USE_PERF_MONITOR
    lv_obj_t * perf_switch;
#endif

    (void)ui_page_add_heading(parent, "Look and feel",
                              "Appearance and motion for this terminal");

    appearance = ui_page_add_card(parent, 0U);
    scheme_dropdown = add_scheme_row(appearance);
    lv_obj_add_event_cb(scheme_dropdown, scheme_event_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    theme_switch = add_switch_row(appearance, "Dark appearance",
                                  (UI_THEME_DARK == ui_theme_get_mode()));
    lv_obj_add_event_cb(theme_switch, theme_switch_event_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(theme_switch, theme_switch_delete_cb,
                        LV_EVENT_DELETE, NULL);

#if LV_USE_SYSMON && LV_USE_PERF_MONITOR
    perf_switch = add_switch_row(appearance, "Performance monitor", false);
    lv_obj_add_event_cb(perf_switch, perf_switch_event_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);
#endif

    /* Grows and scrolls itself, so the page never scrolls and the footer
     * stays put, as on the other pages. */
    motion = ui_page_add_card(parent, 1U);

    row = ui_page_add_row(motion, "Transition duration", "", true);
    lv_label_set_text_fmt(lv_obj_get_child(row, 1), "%d ms",
                          (int)UI_SPRING_DURATION_MS);

    row = ui_page_add_row(motion, "Spring damping ratio", "", true);
    lv_label_set_text_fmt(lv_obj_get_child(row, 1), "%d.%02d",
                          (int)(zeta / RATIO_SCALE),
                          (int)(zeta % RATIO_SCALE));

    row = ui_page_add_row(motion, "Spring stiffness / damping", "", true);
    lv_label_set_text_fmt(lv_obj_get_child(row, 1), "%d / %d",
                          (int)spring->stiffness, (int)spring->damping);

    (void)ui_page_add_nav_footer(parent, "Back", UI_PAGE_HOME, NULL,
                                 UI_PAGE_HOME);
}

/* [] END OF FILE */

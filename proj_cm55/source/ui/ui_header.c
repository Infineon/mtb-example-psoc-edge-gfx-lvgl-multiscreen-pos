/*******************************************************************************
* File Name        : ui_header.c
*
* Description      : Shared status bar widget. Owns the title, the caption, the
*                    theme indicator and the clock. Created once at start-up
*                    and kept alive for the whole application lifetime, which
*                    is what makes it survive both page transitions and theme
*                    changes without being reloaded.
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
#include <stdio.h>

#include "ui_header.h"
#include "ui_theme.h"
#include "ui_layout.h"
#include "ui_config.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define CLOCK_UPDATE_PERIOD_MS          (1000U)

/* The demo clock starts at 09:41 so screenshots look deliberate. */
#define CLOCK_START_HOUR                (9U)
#define CLOCK_START_MINUTE              (41U)

#define SECONDS_PER_MINUTE              (60U)
#define MINUTES_PER_HOUR                (60U)
#define HOURS_PER_DAY                   (24U)

#define CLOCK_TEXT_LEN                  (6U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
static lv_obj_t * header_root;
static lv_obj_t * title_label;
static lv_obj_t * caption_label;
static lv_obj_t * theme_icon;
static lv_obj_t * clock_label;

static uint32_t clock_seconds = ((CLOCK_START_HOUR * MINUTES_PER_HOUR) +
                                  CLOCK_START_MINUTE) * SECONDS_PER_MINUTE;

/*******************************************************************************
* Function Name: clock_timer_cb
********************************************************************************
* Summary:
*  LVGL timer callback. Runs once a second on the LVGL task and advances the
*  simulated wall clock shown on the right of the bar. It keeps running across
*  page transitions, which is the visible proof that the header is not
*  recreated.
*
* Parameters:
*  timer: Timer that fired (unused).
*
* Return:
*  None
*
*******************************************************************************/
static void clock_timer_cb(lv_timer_t * timer)
{
    char text[CLOCK_TEXT_LEN];
    uint32_t hours;
    uint32_t minutes;

    LV_UNUSED(timer);

    clock_seconds = (clock_seconds + 1U) %
                    (HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE);

    hours   = clock_seconds / (MINUTES_PER_HOUR * SECONDS_PER_MINUTE);
    minutes = (clock_seconds / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR;

    (void)snprintf(text, sizeof(text), "%02u:%02u",
                   (unsigned int)hours, (unsigned int)minutes);
    lv_label_set_text(clock_label, text);
}


/*******************************************************************************
* Function Name: ui_header_create
********************************************************************************
* Summary:
*  Builds the shared header bar. Only one header exists; later calls return the
*  existing instance.
*
* Parameters:
*  parent: Parent object: the navigation host screen or lv_layer_top().
*
* Return:
*  lv_obj_t *: Header root object.
*
*******************************************************************************/
lv_obj_t * ui_header_create(lv_obj_t * parent)
{
    lv_obj_t * text_group;
    lv_obj_t * status_group;
    lv_obj_t * wifi_icon;
    lv_obj_t * battery_icon;

    if (NULL != header_root)
    {
        return header_root;
    }

    header_root = lv_obj_create(parent);
    lv_obj_remove_style_all(header_root);
    lv_obj_add_style(header_root, ui_style_header(), 0);
    lv_obj_set_size(header_root, LV_PCT(100), ui_layout_header_height());
    lv_obj_remove_flag(header_root, LV_OBJ_FLAG_SCROLLABLE);
    /* The bar must never eat touches meant for the page underneath. */
    lv_obj_remove_flag(header_root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_flex_flow(header_root, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_root, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Left: title over caption. */
    text_group = lv_obj_create(header_root);
    lv_obj_remove_style_all(text_group);
    lv_obj_set_size(text_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(text_group, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(text_group, 1);
    lv_obj_remove_flag(text_group, LV_OBJ_FLAG_SCROLLABLE);

    title_label = lv_label_create(text_group);
    lv_obj_add_style(title_label, ui_style_title(), 0);
    lv_label_set_text(title_label, "POS Terminal");

    caption_label = lv_label_create(text_group);
    lv_obj_add_style(caption_label, ui_style_muted(), 0);
    lv_label_set_text(caption_label, "");

    /* Right: theme indicator, connectivity, battery, clock. */
    status_group = lv_obj_create(header_root);
    lv_obj_remove_style_all(status_group);
    lv_obj_set_size(status_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(status_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_group, LV_FLEX_ALIGN_END,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(status_group, ui_layout_pad(), 0);
    lv_obj_remove_flag(status_group, LV_OBJ_FLAG_SCROLLABLE);

    theme_icon = lv_label_create(status_group);
    lv_obj_add_style(theme_icon, ui_style_body(), 0);

    wifi_icon = lv_label_create(status_group);
    lv_obj_add_style(wifi_icon, ui_style_muted(), 0);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);

    battery_icon = lv_label_create(status_group);
    lv_obj_add_style(battery_icon, ui_style_muted(), 0);
    lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_3);

    clock_label = lv_label_create(status_group);
    lv_obj_add_style(clock_label, ui_style_body(), 0);
    lv_label_set_text(clock_label, "09:41");

    ui_header_refresh_theme_icon();

    (void)lv_timer_create(clock_timer_cb, CLOCK_UPDATE_PERIOD_MS, NULL);

    return header_root;
}


/*******************************************************************************
* Function Name: ui_header_create_shadow
********************************************************************************
* Summary:
*  Builds the header's drop shadow. A box shadow on the header itself would be
*  painted over by the page area, which is drawn after it, so the shadow is a
*  separate strip that must be created after the page area.
*
* Parameters:
*  parent: Parent object, the same one the header was created on.
*
* Return:
*  None
*
*******************************************************************************/
void ui_header_create_shadow(lv_obj_t * parent)
{
    lv_obj_t * shadow = lv_obj_create(parent);

    lv_obj_remove_style_all(shadow);
    lv_obj_add_style(shadow, ui_style_header_shadow(), 0);
    lv_obj_add_flag(shadow, LV_OBJ_FLAG_FLOATING);
    /* Touches must reach the page underneath. */
    lv_obj_remove_flag(shadow, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(shadow, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(shadow, LV_PCT(100), ui_layout_pad() / 2);
    lv_obj_set_pos(shadow, 0, ui_layout_header_height());
}


/*******************************************************************************
* Function Name: ui_header_get
********************************************************************************
* Summary:
*  Returns the header root object.
*
* Parameters:
*  None
*
* Return:
*  lv_obj_t *: Header root, or NULL if ui_header_create() has not been called
*              yet.
*
*******************************************************************************/
lv_obj_t * ui_header_get(void)
{
    return header_root;
}


/*******************************************************************************
* Function Name: ui_header_set_title
********************************************************************************
* Summary:
*  Updates the title text shown on the left of the bar.
*
* Parameters:
*  title: New title. LVGL keeps its own copy.
*
* Return:
*  None
*
*******************************************************************************/
void ui_header_set_title(const char * title)
{
    if ((NULL != title_label) && (NULL != title))
    {
        lv_label_set_text(title_label, title);
    }
}


/*******************************************************************************
* Function Name: ui_header_set_caption
********************************************************************************
* Summary:
*  Updates the caption text under the title, for example "3 items".
*
* Parameters:
*  text: New caption. LVGL keeps its own copy.
*
* Return:
*  None
*
*******************************************************************************/
void ui_header_set_caption(const char * text)
{
    if ((NULL != caption_label) && (NULL != text))
    {
        lv_label_set_text(caption_label, text);
    }
}


/*******************************************************************************
* Function Name: ui_header_refresh_theme_icon
********************************************************************************
* Summary:
*  Swaps the glyph that indicates the active colour mode after a dark/light
*  change. Only the label text changes; the header is not rebuilt.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void ui_header_refresh_theme_icon(void)
{
    if (NULL == theme_icon)
    {
        return;
    }

    lv_label_set_text(theme_icon, (UI_THEME_DARK == ui_theme_get_mode())
                                  ? LV_SYMBOL_EYE_CLOSE : LV_SYMBOL_EYE_OPEN);
}

/* [] END OF FILE */

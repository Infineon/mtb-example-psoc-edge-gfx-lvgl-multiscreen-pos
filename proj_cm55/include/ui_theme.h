/*******************************************************************************
* File Name        : ui_theme.h
*
* Description      : Runtime colour schemes and dark/light theming for the
*                    whole application.
*
*                    Two things have to change when the mode or scheme changes:
*                      1. the stock LVGL widget chrome, handled by re-running
*                         lv_theme_default_init() with the new dark flag, and
*                      2. the application's own styling, handled by mutating
*                         the shared lv_style_t objects owned by this module.
*
*                    CODING RULE FOR THIS CODE EXAMPLE
*                    Never pass a literal colour to lv_obj_set_style_*(). A
*                    local style set that way is frozen and will not follow the
*                    theme toggle, which is the most common way of breaking
*                    dark mode. Always attach one of the shared styles below.
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

#ifndef UI_THEME_H
#define UI_THEME_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* Data Structures
*******************************************************************************/

/* Selected colour mode. */
typedef enum
{
    UI_THEME_LIGHT = 0,
    UI_THEME_DARK,
} ui_theme_mode_t;

/* Named colour scheme. Each scheme has a light and a dark variant. */
typedef enum
{
    UI_THEME_SCHEME_DEFAULT = 0,
    UI_THEME_SCHEME_CLASSIC,
    UI_THEME_SCHEME_SUNSET,
    UI_THEME_SCHEME_TEAL,
    UI_THEME_SCHEME_COUNT,
} ui_theme_scheme_t;

/* Semantic colour roles. Pages ask for a role, never for a hex value. */
typedef enum
{
    UI_COLOR_BG = 0,        /* Page background.                            */
    UI_COLOR_SURFACE,       /* Card / panel background.                    */
    UI_COLOR_SURFACE_ALT,   /* Alternating list row background.            */
    UI_COLOR_TEXT,          /* Primary text.                               */
    UI_COLOR_MUTED,         /* Secondary text, units, captions.            */
    UI_COLOR_BORDER,        /* Hairlines and dividers.                     */
    UI_COLOR_ACCENT,        /* Brand / call to action.                     */
    UI_COLOR_SUCCESS,       /* Confirmation, paid, in stock.               */
    UI_COLOR_DANGER,        /* Void, remove, error.                        */
    UI_COLOR_ON_ACCENT,     /* Text drawn on an accent fill.               */
    UI_COLOR_COUNT,
} ui_color_role_t;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

void ui_theme_init(ui_theme_mode_t mode);

void ui_theme_set_mode(ui_theme_mode_t mode);

void ui_theme_toggle(void);

ui_theme_mode_t ui_theme_get_mode(void);

void ui_theme_set_scheme(ui_theme_scheme_t scheme);

ui_theme_scheme_t ui_theme_get_scheme(void);

const char * ui_theme_scheme_name(ui_theme_scheme_t scheme);

lv_color_t ui_theme_color(ui_color_role_t role);

/*******************************************************************************
* Shared styles
********************************************************************************
* All returned styles are owned by this module, live for the lifetime of the
* application and are updated in place on a theme change. Attach them with
* lv_obj_add_style(); never modify them from page code.
*******************************************************************************/

/* Opacity used for the pressed button state. An opacity cue rather than a
 *  second colour, so it needs no maintenance when the palette is swapped.
 *  The disabled state is a colour change instead - see ui_style_disabled(). */
#define UI_BTN_PRESSED_OPA              (LV_OPA_70)

lv_style_t * ui_style_page(void);

lv_style_t * ui_style_header(void);

lv_style_t * ui_style_card(void);

lv_style_t * ui_style_row(void);

lv_style_t * ui_style_title(void);

lv_style_t * ui_style_body(void);

lv_style_t * ui_style_muted(void);

lv_style_t * ui_style_amount(void);

lv_style_t * ui_style_accent_text(void);

lv_style_t * ui_style_success_text(void);

lv_style_t * ui_style_btn_primary(void);

lv_style_t * ui_style_selected(void);

lv_style_t * ui_style_btn_secondary(void);

lv_style_t * ui_style_key(void);

lv_style_t * ui_style_disabled(void);

lv_style_t * ui_style_pressed(void);

lv_style_t * ui_style_ring(void);

lv_style_t * ui_style_header_shadow(void);

lv_style_t * ui_style_switch_knob(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UI_THEME_H */

/* [] END OF FILE */

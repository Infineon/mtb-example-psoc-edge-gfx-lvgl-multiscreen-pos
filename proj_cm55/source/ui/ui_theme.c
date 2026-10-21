/*******************************************************************************
* File Name        : ui_theme.c
*
* Description      : Runtime colour schemes and dark/light theming. Owns the
*                    application palettes and every shared lv_style_t used by
*                    the pages and by the shared header.
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
#include "ui_theme.h"
#include "ui_layout.h"
#include "ui_config.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define BORDER_WIDTH_PX                 (1)

/* Border of the selected item in a choice group. */
#define SELECTED_BORDER_WIDTH_PX        (2)

#define THEME_MODE_COUNT                (2U)

/* How far the bottom of a depth gradient moves towards the next role colour. */
#define DEPTH_GRAD_MIX                  (LV_OPA_50)

/* Lighten / darken applied to the accent for the primary button bevel. */
#define ACCENT_BEVEL_LEVEL              (LV_OPA_10)

/* Darken applied to the bottom of the white switch knob. */
#define KNOB_BEVEL_LEVEL                (LV_OPA_20)

/* Drop shadows are black in every palette; a dark palette needs more opacity.
 * The GPU fades its shadow rings from a quarter of this value (see
 * lv_draw_vg_lite_box_shadow.c), so low values all but vanish. */
#define SHADOW_OPA_LIGHT                (LV_OPA_40)
#define SHADOW_OPA_DARK                 (LV_OPA_70)

/* Primary buttons glow in the accent colour, which also shows on a dark
 * background where a black shadow cannot. */
#define GLOW_OPA                        (LV_OPA_60)

#define PRESS_TRANSITION_MS             (80U)

/*******************************************************************************
* Global Variables
*******************************************************************************/

/* Palettes, indexed [scheme][mode]. Apart from the black used for shadows,
 * these are the only literal colours in the application; everything else
 * refers to them through ui_theme_color(). */
static const uint32_t
    palettes[UI_THEME_SCHEME_COUNT][THEME_MODE_COUNT][UI_COLOR_COUNT] =
{
    [UI_THEME_SCHEME_DEFAULT] =
    {
        [UI_THEME_DARK] =
        {
            [UI_COLOR_BG]          = 0x101216U,
            [UI_COLOR_SURFACE]     = 0x1B1E24U,
            [UI_COLOR_SURFACE_ALT] = 0x23272FU,
            [UI_COLOR_TEXT]        = 0xF2F4F8U,
            [UI_COLOR_MUTED]       = 0x9AA1AEU,
            [UI_COLOR_BORDER]      = 0x2E333CU,
            [UI_COLOR_ACCENT]      = UI_COLOR_PRIMARY_HEX,
            [UI_COLOR_SUCCESS]     = UI_COLOR_SECONDARY_HEX,
            [UI_COLOR_DANGER]      = 0xFF453AU,
            [UI_COLOR_ON_ACCENT]   = 0xFFFFFFU,
        },
        [UI_THEME_LIGHT] =
        {
            [UI_COLOR_BG]          = 0xF2F3F7U,
            [UI_COLOR_SURFACE]     = 0xFFFFFFU,
            [UI_COLOR_SURFACE_ALT] = 0xE9EBF0U,
            [UI_COLOR_TEXT]        = 0x14161AU,
            [UI_COLOR_MUTED]       = 0x606774U,
            [UI_COLOR_BORDER]      = 0xD5D8E0U,
            [UI_COLOR_ACCENT]      = 0x0060DFU,
            [UI_COLOR_SUCCESS]     = 0x1C8B3AU,
            [UI_COLOR_DANGER]      = 0xC7261CU,
            [UI_COLOR_ON_ACCENT]   = 0xFFFFFFU,
        },
    },
    [UI_THEME_SCHEME_SUNSET] =
    {
        [UI_THEME_DARK] =
        {
            [UI_COLOR_BG]          = 0x1A1016U,
            [UI_COLOR_SURFACE]     = 0x26171FU,
            [UI_COLOR_SURFACE_ALT] = 0x331F2AU,
            [UI_COLOR_TEXT]        = 0xFBEFE8U,
            [UI_COLOR_MUTED]       = 0xC9A89AU,
            [UI_COLOR_BORDER]      = 0x4A2E3BU,
            [UI_COLOR_ACCENT]      = 0xFF8A4CU,
            [UI_COLOR_SUCCESS]     = 0x5BD46EU,
            [UI_COLOR_DANGER]      = 0xFF6B6BU,
            [UI_COLOR_ON_ACCENT]   = 0x1A1016U,
        },
        [UI_THEME_LIGHT] =
        {
            [UI_COLOR_BG]          = 0xFFF4ECU,
            [UI_COLOR_SURFACE]     = 0xFFFFFFU,
            [UI_COLOR_SURFACE_ALT] = 0xFDE7D9U,
            [UI_COLOR_TEXT]        = 0x2B1A12U,
            [UI_COLOR_MUTED]       = 0x7A5646U,
            [UI_COLOR_BORDER]      = 0xF1CDB8U,
            [UI_COLOR_ACCENT]      = 0xB23A0AU,
            [UI_COLOR_SUCCESS]     = 0x2E7D32U,
            [UI_COLOR_DANGER]      = 0xB91C1CU,
            [UI_COLOR_ON_ACCENT]   = 0xFFFFFFU,
        },
    },
    [UI_THEME_SCHEME_TEAL] =
    {
        [UI_THEME_DARK] =
        {
            [UI_COLOR_BG]          = 0x0B1716U,
            [UI_COLOR_SURFACE]     = 0x122321U,
            [UI_COLOR_SURFACE_ALT] = 0x1A2E2CU,
            [UI_COLOR_TEXT]        = 0xE6F4F2U,
            [UI_COLOR_MUTED]       = 0x93B3AFU,
            [UI_COLOR_BORDER]      = 0x264240U,
            [UI_COLOR_ACCENT]      = 0x2DD4BFU,
            [UI_COLOR_SUCCESS]     = 0x4ADE80U,
            [UI_COLOR_DANGER]      = 0xF87171U,
            [UI_COLOR_ON_ACCENT]   = 0x0B1716U,
        },
        [UI_THEME_LIGHT] =
        {
            [UI_COLOR_BG]          = 0xEEF7F6U,
            [UI_COLOR_SURFACE]     = 0xFFFFFFU,
            [UI_COLOR_SURFACE_ALT] = 0xDDEFECU,
            [UI_COLOR_TEXT]        = 0x0F2624U,
            [UI_COLOR_MUTED]       = 0x4E6B68U,
            [UI_COLOR_BORDER]      = 0xC4DEDAU,
            [UI_COLOR_ACCENT]      = 0x00695CU,
            [UI_COLOR_SUCCESS]     = 0x2E7D32U,
            [UI_COLOR_DANGER]      = 0xC62828U,
            [UI_COLOR_ON_ACCENT]   = 0xFFFFFFU,
        },
    },
    [UI_THEME_SCHEME_CLASSIC] =
    {
        [UI_THEME_DARK] =
        {
            [UI_COLOR_BG]          = 0x1C1C1CU,
            [UI_COLOR_SURFACE]     = 0x2B2B2BU,
            [UI_COLOR_SURFACE_ALT] = 0x3A3A3AU,
            [UI_COLOR_TEXT]        = 0xFFFFFFU,
            [UI_COLOR_MUTED]       = 0xC0C0C0U,
            [UI_COLOR_BORDER]      = 0x808080U,
            [UI_COLOR_ACCENT]      = 0x20B8B8U,
            [UI_COLOR_SUCCESS]     = 0x4CC24CU,
            [UI_COLOR_DANGER]      = 0xFF6060U,
            [UI_COLOR_ON_ACCENT]   = 0x000000U,
        },
        [UI_THEME_LIGHT] =
        {
            [UI_COLOR_BG]          = 0xC0C0C0U,
            [UI_COLOR_SURFACE]     = 0xD4D0C8U,
            [UI_COLOR_SURFACE_ALT] = 0xFFFFFFU,
            [UI_COLOR_TEXT]        = 0x000000U,
            [UI_COLOR_MUTED]       = 0x404040U,
            [UI_COLOR_BORDER]      = 0x808080U,
            [UI_COLOR_ACCENT]      = 0x000080U,
            [UI_COLOR_SUCCESS]     = 0x006400U,
            [UI_COLOR_DANGER]      = 0xB00000U,
            [UI_COLOR_ON_ACCENT]   = 0xFFFFFFU,
        },
    },
};

static const char * const scheme_names[UI_THEME_SCHEME_COUNT] =
{
    [UI_THEME_SCHEME_DEFAULT] = "Default",
    [UI_THEME_SCHEME_CLASSIC] = "Classic",
    [UI_THEME_SCHEME_SUNSET]  = "Sunset",
    [UI_THEME_SCHEME_TEAL]    = "Teal",
};

static const uint32_t * active_palette =
    palettes[UI_THEME_SCHEME_DEFAULT][UI_THEME_DARK];
static ui_theme_mode_t   active_mode    = UI_THEME_DARK;
static ui_theme_scheme_t active_scheme  = UI_THEME_SCHEME_DEFAULT;
static bool              styles_ready   = false;

/* Shared styles. Mutated in place on every theme change. */
static lv_style_t style_page;
static lv_style_t style_header;
static lv_style_t style_card;
static lv_style_t style_row;
static lv_style_t style_title;
static lv_style_t style_body;
static lv_style_t style_muted;
static lv_style_t style_amount;
static lv_style_t style_accent_text;
static lv_style_t style_success_text;
static lv_style_t style_btn_primary;
static lv_style_t style_btn_secondary;
static lv_style_t style_key;
static lv_style_t style_selected;
static lv_style_t style_disabled;
static lv_style_t style_pressed;
static lv_style_t style_ring;
static lv_style_t style_header_shadow;
static lv_style_t style_switch_knob;

static lv_style_transition_dsc_t press_transition;

/*******************************************************************************
* Function Name: colour
********************************************************************************
* Summary:
*  Shorthand for looking up a role in the active palette.
*
* Parameters:
*  role: Colour role.
*
* Return:
*  lv_color_t: Colour value.
*
*******************************************************************************/
static lv_color_t colour(ui_color_role_t role)
{
    return lv_color_hex(active_palette[role]);
}


/*******************************************************************************
* Function Name: blend
********************************************************************************
* Summary:
*  Second stop of a depth gradient: one role leaning towards another. Deriving
*  it from the roles is what gives every scheme the same depth for free.
*
* Parameters:
*  from: Role the gradient starts from.
*  to: Role the second stop leans towards.
*
* Return:
*  lv_color_t: Blended colour.
*
*******************************************************************************/
static lv_color_t blend(ui_color_role_t from, ui_color_role_t to)
{
    return lv_color_mix(colour(to), colour(from), DEPTH_GRAD_MIX);
}


/*******************************************************************************
* Function Name: set_depth_gradient
********************************************************************************
* Summary:
*  Gives a style the surface depth gradient with the lighter stop on top, so
*  every scheme and mode reads as lit from above whichever of SURFACE and
*  SURFACE_ALT is the lighter one.
*
* Parameters:
*  style: Style to update.
*
* Return:
*  None
*
*******************************************************************************/
static void set_depth_gradient(lv_style_t * style)
{
    lv_color_t base = colour(UI_COLOR_SURFACE);
    lv_color_t step = blend(UI_COLOR_SURFACE, UI_COLOR_SURFACE_ALT);
    bool step_lighter = lv_color_luminance(step) > lv_color_luminance(base);

    lv_style_set_bg_color(style, step_lighter ? step : base);
    lv_style_set_bg_grad_color(style, step_lighter ? base : step);
}


/*******************************************************************************
* Function Name: styles_create
********************************************************************************
* Summary:
*  Initialises every shared style and sets the geometry properties that do not
*  depend on the colour mode. Called once.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
static void styles_create(void)
{
    static const lv_style_prop_t press_props[] =
    {
        LV_STYLE_TRANSLATE_Y, LV_STYLE_SHADOW_OFFSET_Y, LV_STYLE_SHADOW_WIDTH,
        LV_STYLE_PROP_INV,
    };

    int32_t pad    = ui_layout_pad();
    int32_t radius = ui_layout_radius();
    int32_t depth  = pad / 4;

    /* Used both on press and on release, so the button eases in and out. */
    lv_style_transition_dsc_init(&press_transition, press_props,
                                 lv_anim_path_ease_out, PRESS_TRANSITION_MS,
                                 0U, NULL);

    lv_style_init(&style_page);
    lv_style_set_border_width(&style_page, 0);
    lv_style_set_radius(&style_page, 0);
    lv_style_set_pad_all(&style_page, pad);
    lv_style_set_pad_row(&style_page, pad);
    lv_style_set_pad_column(&style_page, pad);

    lv_style_init(&style_header);
    lv_style_set_radius(&style_header, 0);
    lv_style_set_border_width(&style_header, BORDER_WIDTH_PX);
    /* Hairline on the bottom edge only, so the header reads as a bar. */
    lv_style_set_border_side(&style_header, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_pad_hor(&style_header, pad);
    lv_style_set_pad_ver(&style_header, 0);
    lv_style_set_pad_column(&style_header, pad);
    lv_style_set_bg_grad_dir(&style_header, LV_GRAD_DIR_VER);

    lv_style_init(&style_card);
    lv_style_set_radius(&style_card, radius);
    lv_style_set_border_width(&style_card, BORDER_WIDTH_PX);
    lv_style_set_bg_grad_dir(&style_card, LV_GRAD_DIR_VER);
    /* Reaches about one pad beyond the card, i.e. into the gap it sits in. */
    lv_style_set_shadow_width(&style_card, (pad * 3) / 2);
    lv_style_set_shadow_offset_y(&style_card, depth);
    /* Home tiles are cards, so this is their release transition. */
    lv_style_set_transition(&style_card, &press_transition);
    lv_style_set_pad_all(&style_card, pad);
    lv_style_set_pad_row(&style_card, pad / 2);
    lv_style_set_pad_column(&style_card, pad);

    lv_style_init(&style_row);
    lv_style_set_radius(&style_row, radius / 2);
    lv_style_set_border_width(&style_row, 0);
    lv_style_set_pad_hor(&style_row, pad);
    lv_style_set_pad_ver(&style_row, pad / 2);
    lv_style_set_pad_column(&style_row, pad);

    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, ui_layout_font_title());

    lv_style_init(&style_body);
    lv_style_set_text_font(&style_body, ui_layout_font_body());

    lv_style_init(&style_muted);
    lv_style_set_text_font(&style_muted, ui_layout_font_small());

    lv_style_init(&style_amount);
    lv_style_set_text_font(&style_amount, ui_layout_font_title());

    /* Colour-only styles. They carry no font so they can be layered on top of
     * a text style without overriding its size. */
    lv_style_init(&style_accent_text);
    lv_style_init(&style_success_text);

    lv_style_init(&style_btn_primary);
    lv_style_set_radius(&style_btn_primary, radius);
    lv_style_set_border_width(&style_btn_primary, 0);
    lv_style_set_bg_grad_dir(&style_btn_primary, LV_GRAD_DIR_VER);
    lv_style_set_shadow_width(&style_btn_primary, pad);
    /* Centred, so the glow surrounds the button instead of a coloured strip
     * showing under it. */
    lv_style_set_shadow_offset_y(&style_btn_primary, 0);
    lv_style_set_transition(&style_btn_primary, &press_transition);
    lv_style_set_text_font(&style_btn_primary, ui_layout_font_body());
    lv_style_set_pad_hor(&style_btn_primary, pad * 2);
    lv_style_set_pad_ver(&style_btn_primary, pad);

    lv_style_init(&style_btn_secondary);
    lv_style_set_radius(&style_btn_secondary, radius);
    lv_style_set_border_width(&style_btn_secondary, BORDER_WIDTH_PX);
    lv_style_set_bg_grad_dir(&style_btn_secondary, LV_GRAD_DIR_VER);
    lv_style_set_shadow_width(&style_btn_secondary, pad);
    lv_style_set_shadow_offset_y(&style_btn_secondary, depth);
    lv_style_set_transition(&style_btn_secondary, &press_transition);
    lv_style_set_text_font(&style_btn_secondary, ui_layout_font_body());
    lv_style_set_pad_hor(&style_btn_secondary, pad * 2);
    lv_style_set_pad_ver(&style_btn_secondary, pad);

    /* Keys of a button matrix. The widget draws its own LV_PART_ITEMS from the
     * stock theme, which in light mode carries a drop shadow the keypad does
     * not use and which would blur every key on every frame. */
    lv_style_init(&style_key);
    lv_style_set_radius(&style_key, radius);
    lv_style_set_border_width(&style_key, BORDER_WIDTH_PX);
    lv_style_set_shadow_width(&style_key, 0);
    lv_style_set_text_font(&style_key, ui_layout_font_body());

    lv_style_init(&style_selected);
    lv_style_set_border_width(&style_selected, SELECTED_BORDER_WIDTH_PX);
    /* The selected choice reads as pushed in, so it drops its shadow. */
    lv_style_set_shadow_opa(&style_selected, LV_OPA_TRANSP);

    lv_style_init(&style_disabled);
    lv_style_set_bg_grad_dir(&style_disabled, LV_GRAD_DIR_NONE);
    lv_style_set_shadow_opa(&style_disabled, LV_OPA_TRANSP);

    lv_style_init(&style_pressed);
    lv_style_set_translate_y(&style_pressed, depth);
    lv_style_set_shadow_offset_y(&style_pressed, 0);
    lv_style_set_shadow_width(&style_pressed, depth);
    /* Flat while pressed: a gradient fill ignores the pressed opacity cue. */
    lv_style_set_bg_grad_dir(&style_pressed, LV_GRAD_DIR_NONE);
    lv_style_set_transition(&style_pressed, &press_transition);

    lv_style_init(&style_ring);
    lv_style_set_arc_width(&style_ring, pad / 2);
    lv_style_set_arc_rounded(&style_ring, true);

    lv_style_init(&style_header_shadow);
    lv_style_set_bg_opa(&style_header_shadow, LV_OPA_COVER);
    lv_style_set_bg_grad_dir(&style_header_shadow, LV_GRAD_DIR_VER);
    lv_style_set_bg_grad_opa(&style_header_shadow, LV_OPA_TRANSP);

    /* The knob stays white in every palette, so its bevel is fixed here. */
    lv_style_init(&style_switch_knob);
    lv_style_set_bg_grad_dir(&style_switch_knob, LV_GRAD_DIR_VER);
    lv_style_set_bg_grad_color(&style_switch_knob,
                               lv_color_darken(lv_color_white(),
                                               KNOB_BEVEL_LEVEL));
    lv_style_set_shadow_width(&style_switch_knob, pad / 2);
    lv_style_set_shadow_offset_y(&style_switch_knob, depth);

    styles_ready = true;
}


/*******************************************************************************
* Function Name: styles_apply_palette
********************************************************************************
* Summary:
*  Rewrites every colour property of the shared styles from the active palette.
*  Only colours are touched here; geometry is set once in styles_create().
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
static void styles_apply_palette(void)
{
    lv_opa_t shadow_opa = (UI_THEME_DARK == active_mode) ? SHADOW_OPA_DARK
                                                         : SHADOW_OPA_LIGHT;

    lv_style_set_bg_color(&style_page, colour(UI_COLOR_BG));
    lv_style_set_bg_opa(&style_page, LV_OPA_COVER);
    lv_style_set_text_color(&style_page, colour(UI_COLOR_TEXT));

    set_depth_gradient(&style_header);
    lv_style_set_bg_opa(&style_header, LV_OPA_COVER);
    lv_style_set_border_color(&style_header, colour(UI_COLOR_BORDER));
    lv_style_set_text_color(&style_header, colour(UI_COLOR_TEXT));

    set_depth_gradient(&style_card);
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_border_color(&style_card, colour(UI_COLOR_BORDER));
    lv_style_set_text_color(&style_card, colour(UI_COLOR_TEXT));
    lv_style_set_shadow_color(&style_card, lv_color_black());
    lv_style_set_shadow_opa(&style_card, shadow_opa);

    lv_style_set_bg_color(&style_row, colour(UI_COLOR_SURFACE_ALT));
    lv_style_set_bg_opa(&style_row, LV_OPA_COVER);
    lv_style_set_text_color(&style_row, colour(UI_COLOR_TEXT));

    lv_style_set_text_color(&style_title, colour(UI_COLOR_TEXT));
    lv_style_set_text_color(&style_body, colour(UI_COLOR_TEXT));
    lv_style_set_text_color(&style_muted, colour(UI_COLOR_MUTED));
    lv_style_set_text_color(&style_amount, colour(UI_COLOR_ACCENT));
    lv_style_set_text_color(&style_accent_text, colour(UI_COLOR_ACCENT));
    lv_style_set_text_color(&style_success_text, colour(UI_COLOR_SUCCESS));

    lv_style_set_bg_color(&style_btn_primary,
                          lv_color_lighten(colour(UI_COLOR_ACCENT),
                                           ACCENT_BEVEL_LEVEL));
    lv_style_set_bg_grad_color(&style_btn_primary,
                               lv_color_darken(colour(UI_COLOR_ACCENT),
                                               ACCENT_BEVEL_LEVEL));
    lv_style_set_bg_opa(&style_btn_primary, LV_OPA_COVER);
    lv_style_set_text_color(&style_btn_primary, colour(UI_COLOR_ON_ACCENT));
    lv_style_set_shadow_color(&style_btn_primary, colour(UI_COLOR_ACCENT));
    lv_style_set_shadow_opa(&style_btn_primary, GLOW_OPA);

    set_depth_gradient(&style_btn_secondary);
    lv_style_set_bg_opa(&style_btn_secondary, LV_OPA_COVER);
    lv_style_set_border_color(&style_btn_secondary, colour(UI_COLOR_BORDER));
    lv_style_set_text_color(&style_btn_secondary, colour(UI_COLOR_TEXT));
    lv_style_set_shadow_color(&style_btn_secondary, lv_color_black());
    lv_style_set_shadow_opa(&style_btn_secondary, shadow_opa);

    /* Alt surface, not surface: the keys sit on a card that is already the
     * surface colour, so they need the step up to read as separate keys. */
    lv_style_set_bg_color(&style_key, colour(UI_COLOR_SURFACE_ALT));
    lv_style_set_bg_opa(&style_key, LV_OPA_COVER);
    lv_style_set_border_color(&style_key, colour(UI_COLOR_BORDER));
    lv_style_set_text_color(&style_key, colour(UI_COLOR_TEXT));

    lv_style_set_bg_color(&style_selected, colour(UI_COLOR_SURFACE_ALT));
    lv_style_set_bg_grad_color(&style_selected, colour(UI_COLOR_SURFACE_ALT));
    lv_style_set_bg_opa(&style_selected, LV_OPA_COVER);
    lv_style_set_border_color(&style_selected, colour(UI_COLOR_ACCENT));
    lv_style_set_text_color(&style_selected, colour(UI_COLOR_ACCENT));

    /* Border grey as a fill reads as "inert" against both the card and the
     * accent, and muted text keeps the caption legible without inviting a
     * tap. */
    lv_style_set_bg_color(&style_disabled, colour(UI_COLOR_BORDER));
    lv_style_set_bg_opa(&style_disabled, LV_OPA_COVER);
    lv_style_set_border_color(&style_disabled, colour(UI_COLOR_BORDER));
    lv_style_set_text_color(&style_disabled, colour(UI_COLOR_MUTED));

    lv_style_set_arc_color(&style_ring, colour(UI_COLOR_SUCCESS));

    lv_style_set_bg_color(&style_header_shadow, lv_color_black());
    lv_style_set_bg_grad_color(&style_header_shadow, lv_color_black());
    lv_style_set_bg_main_opa(&style_header_shadow, shadow_opa);

    lv_style_set_shadow_color(&style_switch_knob, lv_color_black());
    lv_style_set_shadow_opa(&style_switch_knob, shadow_opa);
}


/*******************************************************************************
* Function Name: ui_theme_init
********************************************************************************
* Summary:
*  Creates the shared styles and applies the starting colour mode. Must be
*  called after lv_port_disp_init(), because the fonts are chosen from the
*  display resolution.
*
* Parameters:
*  mode: Colour mode to start in.
*
* Return:
*  None
*
*******************************************************************************/
void ui_theme_init(ui_theme_mode_t mode)
{
    if (!styles_ready)
    {
        styles_create();
    }

    ui_theme_set_mode(mode);
}


/*******************************************************************************
* Function Name: ui_theme_set_mode
********************************************************************************
* Summary:
*  Applies a colour mode at runtime.
*
*  Ordering matters here. The application styles are updated first, then
*  lv_theme_default_init() re-initialises the stock widget styles in place, and
*  finally lv_obj_report_style_change(NULL) invalidates every live widget once.
*  Nothing is destroyed or rebuilt, which is what lets the shared header keep
*  its state across a theme change.
*
* Parameters:
*  mode: Colour mode to apply.
*
* Return:
*  None
*
*******************************************************************************/
void ui_theme_set_mode(ui_theme_mode_t mode)
{
    lv_display_t * disp = lv_display_get_default();
    lv_theme_t * theme;

    if (!styles_ready)
    {
        styles_create();
    }

    active_mode = mode;
    active_palette = palettes[active_scheme][mode];

    styles_apply_palette();

    theme = lv_theme_default_init(disp,
                                  colour(UI_COLOR_ACCENT),
                                  colour(UI_COLOR_SUCCESS),
                                  (UI_THEME_DARK == mode),
                                  ui_layout_font_body());
    lv_display_set_theme(disp, theme);

    /* lv_theme_default_init() already reports a style change, but only when it
     * decides something changed. Reporting again is cheap and makes the
     * application styles updated above take effect unconditionally. */
    lv_obj_report_style_change(NULL);
}


/*******************************************************************************
* Function Name: ui_theme_toggle
********************************************************************************
* Summary:
*  Switches between dark and light.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void ui_theme_toggle(void)
{
    ui_theme_set_mode((UI_THEME_DARK == active_mode) ? UI_THEME_LIGHT
                                                     : UI_THEME_DARK);
}


/*******************************************************************************
* Function Name: ui_theme_get_mode
********************************************************************************
* Summary:
*  Returns the colour mode currently applied.
*
* Parameters:
*  None
*
* Return:
*  ui_theme_mode_t: Active mode.
*
*******************************************************************************/
ui_theme_mode_t ui_theme_get_mode(void)
{
    return active_mode;
}


/*******************************************************************************
* Function Name: ui_theme_set_scheme
********************************************************************************
* Summary:
*  Applies a colour scheme in the colour mode currently in force. Uses the same
*  in-place mechanism as ui_theme_set_mode(), so nothing is recreated.
*
* Parameters:
*  scheme: Colour scheme to apply.
*
* Return:
*  None
*
*******************************************************************************/
void ui_theme_set_scheme(ui_theme_scheme_t scheme)
{
    if (scheme >= UI_THEME_SCHEME_COUNT)
    {
        return;
    }

    active_scheme = scheme;
    ui_theme_set_mode(active_mode);
}


/*******************************************************************************
* Function Name: ui_theme_get_scheme
********************************************************************************
* Summary:
*  Returns the colour scheme currently applied.
*
* Parameters:
*  None
*
* Return:
*  ui_theme_scheme_t: Active scheme.
*
*******************************************************************************/
ui_theme_scheme_t ui_theme_get_scheme(void)
{
    return active_scheme;
}


/*******************************************************************************
* Function Name: ui_theme_scheme_name
********************************************************************************
* Summary:
*  Returns the display name of a colour scheme.
*
* Parameters:
*  scheme: Colour scheme.
*
* Return:
*  const char *: Name, or "" if the scheme is out of range.
*
*******************************************************************************/
const char * ui_theme_scheme_name(ui_theme_scheme_t scheme)
{
    return (scheme < UI_THEME_SCHEME_COUNT) ? scheme_names[scheme] : "";
}


/*******************************************************************************
* Function Name: ui_theme_color
********************************************************************************
* Summary:
*  Resolves a semantic colour role against the active palette.
*
* Parameters:
*  role: Colour role.
*
* Return:
*  lv_color_t: Colour value.
*
*******************************************************************************/
lv_color_t ui_theme_color(ui_color_role_t role)
{
    if (role >= UI_COLOR_COUNT)
    {
        role = UI_COLOR_TEXT;
    }

    return colour(role);
}

/*******************************************************************************
* Shared style accessors
*******************************************************************************/

/*******************************************************************************
* Function Name: ui_style_page
********************************************************************************
* Summary:
*  Style for the full-bleed page background: no border, no padding.
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_page(void)
{
    return &style_page;
}


/*******************************************************************************
* Function Name: ui_style_header
********************************************************************************
* Summary:
*  Style for the header bar: surface gradient, bottom hairline and horizontal
*  padding.
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_header(void)
{
    return &style_header;
}


/*******************************************************************************
* Function Name: ui_style_card
********************************************************************************
* Summary:
*  Style for a rounded card or panel on the page background.
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_card(void)
{
    return &style_card;
}


/*******************************************************************************
* Function Name: ui_style_row
********************************************************************************
* Summary:
*  Style for a single row inside a list card.
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_row(void)
{
    return &style_row;
}


/*******************************************************************************
* Function Name: ui_style_title
********************************************************************************
* Summary:
*  Style for page and section titles.
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_title(void)
{
    return &style_title;
}


/*******************************************************************************
* Function Name: ui_style_body
********************************************************************************
* Summary:
*  Style for default body text.
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_body(void)
{
    return &style_body;
}


/*******************************************************************************
* Function Name: ui_style_muted
********************************************************************************
* Summary:
*  Style for secondary text: captions, units and helper lines.
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_muted(void)
{
    return &style_muted;
}


/*******************************************************************************
* Function Name: ui_style_amount
********************************************************************************
* Summary:
*  Style for a large monetary figure.
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_amount(void)
{
    return &style_amount;
}


/*******************************************************************************
* Function Name: ui_style_accent_text
********************************************************************************
* Summary:
*  Accent text colour only, no font. Layer it on top of a text style to recolour
*  a label without changing its font:
*
*    lv_obj_add_style(label, ui_style_title(), 0);
*    lv_obj_add_style(label, ui_style_accent_text(), 0);
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_accent_text(void)
{
    return &style_accent_text;
}


/*******************************************************************************
* Function Name: ui_style_success_text
********************************************************************************
* Summary:
*  Success text colour only, no font. Used the same way as
*  ui_style_accent_text().
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_success_text(void)
{
    return &style_success_text;
}


/*******************************************************************************
* Function Name: ui_style_btn_primary
********************************************************************************
* Summary:
*  Style for the primary call-to-action button, with an accent-coloured glow.
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_btn_primary(void)
{
    return &style_btn_primary;
}


/*******************************************************************************
* Function Name: ui_style_btn_secondary
********************************************************************************
* Summary:
*  Style for a secondary (outline) button.
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_btn_secondary(void)
{
    return &style_btn_secondary;
}


/*******************************************************************************
* Function Name: ui_style_key
********************************************************************************
* Summary:
*  Style for a single key of a button matrix. Add it with the LV_PART_ITEMS
*  selector, otherwise the keys keep the stock LVGL theme instead of this
*  palette:
*
*    lv_obj_add_style(keypad, ui_style_key(), LV_PART_ITEMS);
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_key(void)
{
    return &style_key;
}


/*******************************************************************************
* Function Name: ui_style_selected
********************************************************************************
* Summary:
*  Secondary button decoration for the selected choice in a group. Add it with
*  the LV_STATE_CHECKED selector:
*
*    lv_obj_add_style(btn, ui_style_selected(), LV_STATE_CHECKED);
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_selected(void)
{
    return &style_selected;
}


/*******************************************************************************
* Function Name: ui_style_disabled
********************************************************************************
* Summary:
*  Greyed-out decoration for a button that cannot be used yet. Add it with the
*  LV_STATE_DISABLED selector:
*
*    lv_obj_add_style(btn, ui_style_disabled(), LV_STATE_DISABLED);
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_disabled(void)
{
    return &style_disabled;
}


/*******************************************************************************
* Function Name: ui_style_pressed
********************************************************************************
* Summary:
*  Pressed decoration: sinks a raised button or tile into its shadow and drops
*  its gradient, which would otherwise hide UI_BTN_PRESSED_OPA. Add it with
*  ui_page_add_press_style() when the control sits in a content-sized
*  container; otherwise use the LV_STATE_PRESSED selector:
*
*    lv_obj_add_style(btn, ui_style_pressed(), LV_STATE_PRESSED);
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_pressed(void)
{
    return &style_pressed;
}


/*******************************************************************************
* Function Name: ui_style_ring
********************************************************************************
* Summary:
*  Success-coloured arc style for both parts of an lv_arc.
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_ring(void)
{
    return &style_ring;
}


/*******************************************************************************
* Function Name: ui_style_header_shadow
********************************************************************************
* Summary:
*  Style for the fading strip that casts the header shadow onto the page area.
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_header_shadow(void)
{
    return &style_header_shadow;
}


/*******************************************************************************
* Function Name: ui_style_switch_knob
********************************************************************************
* Summary:
*  Raised, bevelled knob for an lv_switch. Add it with the LV_PART_KNOB
*  selector.
*
* Parameters:
*  None
*
* Return:
*  lv_style_t *: Pointer to the shared style.
*
*******************************************************************************/
lv_style_t * ui_style_switch_knob(void)
{
    return &style_switch_knob;
}

/* [] END OF FILE */

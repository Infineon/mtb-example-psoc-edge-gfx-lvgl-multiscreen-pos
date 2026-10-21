/*******************************************************************************
* File Name        : ui_pages.c
*
* Description      : Page registry and the small set of helpers every page
*                    builds on. Keeping the helpers here is what stops the
*                    individual pages from ever touching a literal colour or a
*                    literal pixel size.
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

#include "ui_pages.h"
#include "ui_nav.h"
#include "ui_theme.h"
#include "ui_layout.h"
#include "ui_config.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* Long enough for "$99999.99". */
#define AMOUNT_TEXT_LEN                 (16U)

#define CENTS_PER_UNIT                  (100U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
static uint32_t sale_amount_cents;
static bool sale_is_adhoc;

/* The single source of truth for what pages exist and in which order they are
 * reached by the "next page" button. */
static const ui_page_desc_t page_registry[UI_PAGE_COUNT] =
{
    [UI_PAGE_HOME] =
    {
        .name    = "page_home",
        .title   = "New sale",
        .caption = "Register 1",
        .create_cb = page_home_create,
        .focus_cb  = page_home_refresh,
    },
    [UI_PAGE_BASKET] =
    {
        .name    = "page_basket",
        .title   = "Basket",
        .caption = "Open sale",
        .create_cb = page_basket_create,
        .focus_cb  = page_basket_refresh,
    },
    [UI_PAGE_PAYMENT] =
    {
        .name    = "page_payment",
        .title   = "Payment",
        .caption = "Open sale",
        .create_cb = page_payment_create,
        .focus_cb  = page_payment_refresh,
        .prepare_cb = page_payment_refresh,
    },
    [UI_PAGE_RECEIPT] =
    {
        .name    = "page_receipt",
        .title   = "Receipt",
        .caption = "Sale complete",
        .create_cb = page_receipt_create,
        .focus_cb = page_receipt_refresh,
        .prepare_cb = page_receipt_refresh,
    },
    [UI_PAGE_ADHOC] =
    {
        .name    = "page_adhoc",
        .title   = "Ad hoc",
        .caption = "Keyed sale",
        .create_cb = page_adhoc_create,
    },
    [UI_PAGE_SETTINGS] =
    {
        .name    = "page_settings",
        .title   = "Settings",
        .caption = "Terminal options",
        .create_cb = page_settings_create,
        .defocus_cb = page_settings_defocus,
    },
};

/*******************************************************************************
* Function Name: ui_nav_page_desc
********************************************************************************
* Summary:
*  Looks up a registry entry.
*
* Parameters:
*  id: Page id.
*
* Return:
*  const ui_page_desc_t *: Registry entry, or NULL if the id is out of range.
*
*******************************************************************************/
const ui_page_desc_t * ui_nav_page_desc(ui_page_id_t id)
{
    if (id >= UI_PAGE_COUNT)
    {
        return NULL;
    }

    return &page_registry[id];
}


/*******************************************************************************
* Function Name: ui_sale_arm
********************************************************************************
* Summary:
*  Arms the open sale with the amount to be charged and the route it came from.
*
* Parameters:
*  cents: Amount in cents.
*  adhoc: true when the amount was keyed in rather than scanned.
*
* Return:
*  None
*
*******************************************************************************/
void ui_sale_arm(uint32_t cents, bool adhoc)
{
    sale_amount_cents = cents;
    sale_is_adhoc = adhoc;
}


/*******************************************************************************
* Function Name: ui_sale_is_adhoc
********************************************************************************
* Summary:
*  Reports whether the open sale was keyed in on the ad hoc page. Pages use it
*  to offer a route back that matches the way the operator got here.
*
* Parameters:
*  None
*
* Return:
*  bool: true for an ad hoc sale.
*
*******************************************************************************/
bool ui_sale_is_adhoc(void)
{
    return sale_is_adhoc;
}


/*******************************************************************************
* Function Name: ui_page_amount_text
********************************************************************************
* Summary:
*  Formats a cent amount for display. Shared so that every page showing money
*  - the keypad, the payment page, the receipt - renders it identically.
*
* Parameters:
*  cents: Amount in cents.
*
* Return:
*  const char *: "$12.34" style text in a shared buffer, valid until the next
*                call.
*
*******************************************************************************/
const char * ui_page_amount_text(uint32_t cents)
{
    static char text[AMOUNT_TEXT_LEN];

    (void)snprintf(text, sizeof(text), "$%u.%02u",
                   (unsigned int)(cents / CENTS_PER_UNIT),
                   (unsigned int)(cents % CENTS_PER_UNIT));

    return text;
}


/*******************************************************************************
* Function Name: ui_sale_amount_text
********************************************************************************
* Summary:
*  Formats the open sale amount for display.
*
* Parameters:
*  None
*
* Return:
*  const char *: "$12.34" style text in the ui_page_amount_text() buffer, valid
*                until the next call.
*
*******************************************************************************/
const char * ui_sale_amount_text(void)
{
    return ui_page_amount_text(sale_amount_cents);
}


/*******************************************************************************
* Function Name: nav_btn_event_cb
********************************************************************************
* Summary:
*  Click handler shared by every navigation button. The destination page id is
*  carried in the event user data; the direction is derived from the position
*  of the destination relative to the page currently shown, so "Back" animates
*  the opposite way to "Continue" without either page having to say so.
*
* Parameters:
*  e: Click event.
*
* Return:
*  None
*
*******************************************************************************/
static void nav_btn_event_cb(lv_event_t * e)
{
    ui_page_id_t target = (ui_page_id_t)(uintptr_t)lv_event_get_user_data(e);
    ui_nav_dir_t dir = (target > ui_nav_current()) ? UI_NAV_DIR_FORWARD
                                                   : UI_NAV_DIR_BACK;

    (void)ui_nav_go_to(target, dir);
}


/*******************************************************************************
* Function Name: hold_parent_height_cb
********************************************************************************
* Summary:
*  Fixes the parent at its resting height before the press animation starts.
*  The sink is a translation, and LVGL counts it in the height of a
*  content-sized parent, which would push the cards around it.
*
* Parameters:
*  e: Press event of the control.
*
* Return:
*  None
*
*******************************************************************************/
static void hold_parent_height_cb(lv_event_t * e)
{
    lv_obj_t * parent = lv_obj_get_parent(lv_event_get_target(e));

    lv_obj_set_height(parent, lv_obj_get_height(parent));
}


/*******************************************************************************
* Function Name: ui_page_add_press_style
********************************************************************************
* Summary:
*  Makes a raised control sink into its shadow while pressed, without the
*  content-sized container it sits in growing by the sink depth.
*
* Parameters:
*  obj: Button or other raised control.
*
* Return:
*  None
*
*******************************************************************************/
void ui_page_add_press_style(lv_obj_t * obj)
{
    lv_obj_add_style(obj, ui_style_pressed(), LV_STATE_PRESSED);
    lv_obj_add_event_cb(obj, hold_parent_height_cb, LV_EVENT_PRESSED, NULL);
}


/*******************************************************************************
* Function Name: ui_page_add_button
********************************************************************************
* Summary:
*  Creates a themed, full-width button with a centred caption. Use it for an
*  action button in a ui_page_add_button_row(), or for a footer button whose
*  destination is not a plain page id (ui_page_add_nav_footer() covers that
*  case).
*
* Parameters:
*  parent: Parent container.
*  text: Button caption.
*  primary: true for the accent-filled style.
*  cb: Click handler.
*  user_data: Passed to the handler.
*
* Return:
*  lv_obj_t *: The button. Its caption label is child 0.
*
*******************************************************************************/
lv_obj_t * ui_page_add_button(lv_obj_t * parent, const char * text,
                              bool primary, lv_event_cb_t cb,
                              void * user_data)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_t * label;

    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, primary ? ui_style_btn_primary()
                                  : ui_style_btn_secondary(), 0);
    lv_obj_set_height(btn, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_style_opa(btn, UI_BTN_PRESSED_OPA, LV_STATE_PRESSED);
    ui_page_add_press_style(btn);

    label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);

    return btn;
}


/*******************************************************************************
* Function Name: ui_page_add_button_row
********************************************************************************
* Summary:
*  Creates a row for side-by-side buttons, inset so their shadows show on
*  every side. Every button row uses it, so they all share the same spacing.
*
* Parameters:
*  parent: Card or page container.
*
* Return:
*  lv_obj_t *: The row. Its height is LV_SIZE_CONTENT.
*
*******************************************************************************/
lv_obj_t * ui_page_add_button_row(lv_obj_t * parent)
{
    lv_obj_t * row = lv_obj_create(parent);
    int32_t pad = ui_layout_pad();

    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_hor(row, pad, 0);
    lv_obj_set_style_pad_ver(row, pad / 2, 0);
    lv_obj_set_style_pad_column(row, pad * 2, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    /* Button shadows extend into the parent's padding. */
    lv_obj_add_flag(row, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    return row;
}


/*******************************************************************************
* Function Name: add_nav_button
********************************************************************************
* Summary:
*  Creates a themed button wired to nav_btn_event_cb().
*
* Parameters:
*  parent: Parent container.
*  text: Button label.
*  target: Destination page.
*  primary: true for the accent-filled style.
*
* Return:
*  lv_obj_t *: The button.
*
*******************************************************************************/
static lv_obj_t * add_nav_button(lv_obj_t * parent, const char * text,
                                 ui_page_id_t target, bool primary)
{
    return ui_page_add_button(parent, text, primary, nav_btn_event_cb,
                              (void *)(uintptr_t)target);
}


/*******************************************************************************
* Function Name: ui_page_add_heading
********************************************************************************
* Summary:
*  Adds the standard heading block at the top of a page: a title plus one line
*  of helper text.
*
* Parameters:
*  parent: Page container.
*  title: Heading text.
*  subtitle: Helper text, may be NULL.
*
* Return:
*  lv_obj_t *: The heading container.
*
*******************************************************************************/
lv_obj_t * ui_page_add_heading(lv_obj_t * parent, const char * title,
                               const char * subtitle)
{
    lv_obj_t * box = lv_obj_create(parent);
    lv_obj_t * label;

    lv_obj_remove_style_all(box);
    lv_obj_set_size(box, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(box);
    lv_obj_add_style(label, ui_style_title(), 0);
    lv_label_set_text(label, title);

    if (NULL != subtitle)
    {
        label = lv_label_create(box);
        lv_obj_add_style(label, ui_style_muted(), 0);
        lv_label_set_text(label, subtitle);
    }

    return box;
}


/*******************************************************************************
* Function Name: ui_page_add_card
********************************************************************************
* Summary:
*  Adds a themed card laid out as a column.
*
* Parameters:
*  parent: Page container.
*  grow: Flex grow factor, 0 for content height.
*
* Return:
*  lv_obj_t *: The card.
*
*******************************************************************************/
lv_obj_t * ui_page_add_card(lv_obj_t * parent, uint8_t grow)
{
    lv_obj_t * card = lv_obj_create(parent);

    lv_obj_remove_style_all(card);
    lv_obj_add_style(card, ui_style_card(), 0);
    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_height(card, (0U == grow) ? LV_SIZE_CONTENT : LV_PCT(100));
    lv_obj_set_flex_grow(card, (int32_t)grow);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);

    if (0U == grow)
    {
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    }
    else
    {
        lv_obj_set_scroll_dir(card, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_AUTO);
    }

    return card;
}


/*******************************************************************************
* Function Name: ui_page_add_row
********************************************************************************
* Summary:
*  Adds a "label on the left, value on the right" row to a card.
*
* Parameters:
*  parent: Card object.
*  label: Left text.
*  value: Right text.
*  strong: true to render the value in the body font instead of the muted
*          caption font.
*
* Return:
*  lv_obj_t *: The row.
*
*******************************************************************************/
lv_obj_t * ui_page_add_row(lv_obj_t * parent, const char * label,
                           const char * value, bool strong)
{
    lv_obj_t * row = lv_obj_create(parent);
    lv_obj_t * left;
    lv_obj_t * right;

    lv_obj_remove_style_all(row);
    lv_obj_add_style(row, ui_style_row(), 0);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    left = lv_label_create(row);
    lv_obj_add_style(left, ui_style_body(), 0);
    lv_label_set_text(left, label);
    lv_obj_set_flex_grow(left, 1);

    right = lv_label_create(row);
    lv_obj_add_style(right, strong ? ui_style_body() : ui_style_muted(), 0);
    lv_label_set_text(right, value);

    return row;
}


/*******************************************************************************
* Function Name: ui_page_add_amount_card
********************************************************************************
* Summary:
*  Adds a card that shows one amount in large type under a caption.
*
* Parameters:
*  parent: Page container.
*  caption: Text above the amount.
*
* Return:
*  lv_obj_t *: The amount label, empty until the caller sets it.
*
*******************************************************************************/
lv_obj_t * ui_page_add_amount_card(lv_obj_t * parent, const char * caption)
{
    lv_obj_t * card = ui_page_add_card(parent, 0U);
    lv_obj_t * label;

    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    label = lv_label_create(card);
    lv_obj_add_style(label, ui_style_muted(), 0);
    lv_label_set_text(label, caption);

    label = lv_label_create(card);
    lv_obj_add_style(label, ui_style_amount(), 0);

    return label;
}


/*******************************************************************************
* Function Name: ui_page_add_nav_footer
********************************************************************************
* Summary:
*  Adds the footer row carrying up to two navigation buttons. The footer is a
*  ui_page_add_button_row(), inset further so its buttons line up with the
*  content of the cards above.
*
* Parameters:
*  parent: Page container.
*  back_text: Secondary button text, NULL to omit.
*  back_target: Secondary button destination.
*  forward_text: Primary button text, NULL to omit.
*  forward_target: Primary button destination.
*
* Return:
*  lv_obj_t *: The footer.
*
*******************************************************************************/
lv_obj_t * ui_page_add_nav_footer(lv_obj_t * parent,
                                  const char * back_text,
                                  ui_page_id_t back_target,
                                  const char * forward_text,
                                  ui_page_id_t forward_target)
{
    lv_obj_t * footer = ui_page_add_button_row(parent);

    /* Adds the card padding the footer does not have, so its buttons line up
     * with the button rows and text inside the cards above. */
    lv_obj_set_style_pad_hor(footer, ui_layout_pad() * 2, 0);

    if (NULL != back_text)
    {
        (void)add_nav_button(footer, back_text, back_target, false);
    }

    if (NULL != forward_text)
    {
        (void)add_nav_button(footer, forward_text, forward_target, true);
    }

    return footer;
}

/* [] END OF FILE */

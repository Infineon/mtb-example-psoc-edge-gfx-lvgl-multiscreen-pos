/*******************************************************************************
* File Name        : page_receipt.c
*
* Description      : Receipt page. Confirms the transaction and offers the
*                    usual receipt delivery options.
*
*                    The two delivery buttons deliberately do not navigate:
*                    they show that a page can own local interaction without
*                    involving the navigation layer at all.
*
*                    The approval check is replayed from the focus callback on
*                    every visit, because the page itself is built only once.
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
#include "ui_layout.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define DELIVERY_COUNT                  (2U)

#define DELIVERY_PROMPT                 "Choose how to send the receipt"

/* An ad hoc sale is an amount typed on the keypad, so it has no line items. */
#define ITEMS_UNKNOWN                   "N/A"

/* Ring diameter in title-font lines, matching the two lines beside it. */
#define RING_SIZE_LINES                 (2)
#define RING_VALUE_MAX                  (100)
#define RING_DRAW_MS                    (500U)
#define CHECK_FADE_MS                   (200U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
static lv_obj_t * delivery_label;
static lv_obj_t * delivery_buttons[DELIVERY_COUNT];
static uint32_t delivery_used;
static lv_obj_t * amount_value;
static lv_obj_t * tender_value;
static lv_obj_t * items_value;
static lv_obj_t * check_ring;
static lv_obj_t * check_mark;

/*******************************************************************************
* Function Name: ring_anim_cb
********************************************************************************
* Summary:
*  Animation execution callback that draws the approval ring in.
*
* Parameters:
*  var: The ring.
*  v: Ring value, 0 to RING_VALUE_MAX.
*
* Return:
*  None
*
*******************************************************************************/
static void ring_anim_cb(void * var, int32_t v)
{
    lv_arc_set_value((lv_obj_t *)var, v);
}


/*******************************************************************************
* Function Name: check_anim_cb
********************************************************************************
* Summary:
*  Animation execution callback that fades the check mark in.
*
* Parameters:
*  var: The check mark label.
*  v: Opacity.
*
* Return:
*  None
*
*******************************************************************************/
static void check_anim_cb(void * var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}


/*******************************************************************************
* Function Name: ring_done_cb
********************************************************************************
* Summary:
*  Shows the check mark once the ring has closed.
*
* Parameters:
*  a: Ring animation that completed (unused).
*
* Return:
*  None
*
*******************************************************************************/
static void ring_done_cb(lv_anim_t * a)
{
    lv_anim_t fade;

    LV_UNUSED(a);

    lv_anim_init(&fade);
    lv_anim_set_var(&fade, check_mark);
    lv_anim_set_exec_cb(&fade, check_anim_cb);
    lv_anim_set_values(&fade, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&fade, CHECK_FADE_MS);
    (void)lv_anim_start(&fade);
}


/*******************************************************************************
* Function Name: play_approval
********************************************************************************
* Summary:
*  Restarts the approval animation from an empty ring.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
static void play_approval(void)
{
    lv_anim_t ring;

    if (NULL == check_ring)
    {
        return;
    }

    (void)lv_anim_delete(check_ring, ring_anim_cb);
    (void)lv_anim_delete(check_mark, check_anim_cb);
    lv_arc_set_value(check_ring, 0);
    lv_obj_set_style_opa(check_mark, LV_OPA_TRANSP, 0);

    /* The page is also refreshed while it is built, before it slides in. */
    if (UI_PAGE_RECEIPT != ui_nav_current())
    {
        return;
    }

    lv_anim_init(&ring);
    lv_anim_set_var(&ring, check_ring);
    lv_anim_set_exec_cb(&ring, ring_anim_cb);
    lv_anim_set_values(&ring, 0, RING_VALUE_MAX);
    lv_anim_set_duration(&ring, RING_DRAW_MS);
    lv_anim_set_path_cb(&ring, lv_anim_path_ease_out);
    lv_anim_set_completed_cb(&ring, ring_done_cb);
    (void)lv_anim_start(&ring);
}

/*******************************************************************************
* Function Name: delivery_event_cb
********************************************************************************
* Summary:
*  Acknowledges a receipt delivery choice and takes that option out of play.
*  A receipt should not be printed or emailed twice for the same sale, so the
*  button stays disabled until the page is visited again.
*
* Parameters:
*  e: Click event carrying the confirmation text.
*
* Return:
*  None
*
*******************************************************************************/
static void delivery_event_cb(lv_event_t * e)
{
    const char * text = (const char *)lv_event_get_user_data(e);
    lv_obj_t * btn = lv_event_get_target(e);

    lv_label_set_text(delivery_label, text);
    lv_obj_add_state(btn, LV_STATE_DISABLED);
}


/*******************************************************************************
* Function Name: add_delivery_button
********************************************************************************
* Summary:
*  Creates one receipt delivery option.
*
* Parameters:
*  parent: Row container.
*  text: Button text.
*  ack: Confirmation text shown when pressed.
*
* Return:
*  None
*
*******************************************************************************/
static void add_delivery_button(lv_obj_t * parent, const char * text,
                                const char * ack)
{
    lv_obj_t * btn = ui_page_add_button(parent, text, false, delivery_event_cb,
                                        (void *)(uintptr_t)ack);

    lv_obj_add_style(btn, ui_style_disabled(), LV_STATE_DISABLED);

    delivery_buttons[delivery_used] = btn;
    delivery_used++;
}


/*******************************************************************************
* Function Name: page_receipt_refresh
********************************************************************************
* Summary:
*  Focus callback. Restates the sale on this page: pulls the tender and item
*  count from the pages that own them, puts both delivery options back in play
*  and replays the approval check. Needed on every visit because the page itself
*  is built once and then kept alive. Safe to call before the page exists.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void page_receipt_refresh(void)
{
    uint32_t i;

    for (i = 0U; i < delivery_used; i++)
    {
        lv_obj_remove_state(delivery_buttons[i], LV_STATE_DISABLED);
    }

    if (NULL != delivery_label)
    {
        lv_label_set_text(delivery_label, DELIVERY_PROMPT);
    }

    if (NULL != amount_value)
    {
        lv_label_set_text(amount_value, ui_sale_amount_text());
    }

    if (NULL != tender_value)
    {
        lv_label_set_text(tender_value, page_payment_tender_text());
    }

    if (NULL != items_value)
    {
        if (ui_sale_is_adhoc())
        {
            lv_label_set_text(items_value, ITEMS_UNKNOWN);
        }
        else
        {
            lv_label_set_text_fmt(items_value, "%u",
                                  (unsigned int)page_basket_item_count());
        }
    }

    play_approval();
}


/*******************************************************************************
* Function Name: page_receipt_create
********************************************************************************
* Summary:
*  Builds the receipt page.
*
* Parameters:
*  parent: Page container.
*
* Return:
*  None
*
*******************************************************************************/
void page_receipt_create(lv_obj_t * parent)
{
    int32_t ring_size = RING_SIZE_LINES *
                        lv_font_get_line_height(ui_layout_font_title());
    bool side_by_side = !ui_layout_is_square();
    lv_obj_t * top;
    lv_obj_t * status;
    lv_obj_t * status_text;
    lv_obj_t * summary;
    lv_obj_t * delivery;
    lv_obj_t * row;
    lv_obj_t * label;

    (void)ui_page_add_heading(parent, "Payment approved",
                              "Transaction 4417-002");

    /* Status and delivery share one band so the summary gets the height; a
     * square panel is too narrow for that, so it stacks them. */
    top = lv_obj_create(parent);
    lv_obj_remove_style_all(top);
    lv_obj_set_size(top, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(top, side_by_side ? LV_FLEX_FLOW_ROW
                                           : LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(top, ui_layout_pad(), 0);
    lv_obj_remove_flag(top, LV_OBJ_FLAG_SCROLLABLE);
    /* Card shadows extend into the page gaps. */
    lv_obj_add_flag(top, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    status = ui_page_add_card(top, 0U);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    check_ring = lv_arc_create(status);
    lv_obj_set_size(check_ring, ring_size, ring_size);
    lv_arc_set_rotation(check_ring, 270);
    lv_arc_set_bg_angles(check_ring, 0, 360);
    lv_arc_set_range(check_ring, 0, RING_VALUE_MAX);
    lv_obj_remove_style(check_ring, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(check_ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_style(check_ring, ui_style_ring(), LV_PART_MAIN);
    lv_obj_add_style(check_ring, ui_style_ring(), LV_PART_INDICATOR);
    /* The track is the same colour, just faint. */
    lv_obj_set_style_arc_opa(check_ring, LV_OPA_20, LV_PART_MAIN);

    check_mark = lv_label_create(check_ring);
    lv_obj_add_style(check_mark, ui_style_title(), 0);
    lv_obj_add_style(check_mark, ui_style_success_text(), 0);
    lv_label_set_text(check_mark, LV_SYMBOL_OK);
    lv_obj_center(check_mark);

    status_text = lv_obj_create(status);
    lv_obj_remove_style_all(status_text);
    lv_obj_set_size(status_text, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(status_text, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(status_text, ui_layout_pad() / 2, 0);
    lv_obj_remove_flag(status_text, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(status_text);
    lv_obj_add_style(label, ui_style_title(), 0);
    /* Shared style, not a local colour: a local one would freeze at the mode
     * that was active when the page was built and ignore the theme toggle. */
    lv_obj_add_style(label, ui_style_success_text(), 0);
    lv_label_set_text(label, "Paid");

    amount_value = lv_label_create(status_text);
    lv_obj_add_style(amount_value, ui_style_amount(), 0);

    delivery = ui_page_add_card(top, 0U);

    if (side_by_side)
    {
        lv_obj_set_flex_grow(status, 1);
        lv_obj_set_flex_grow(delivery, 1);
        /* Matches the delivery card, which sets the height of the band. */
        lv_obj_set_height(status, LV_PCT(100));
    }

    row = ui_page_add_button_row(delivery);

    add_delivery_button(row, LV_SYMBOL_COPY "  Print",
                        "Receipt sent to printer");
    add_delivery_button(row, LV_SYMBOL_UPLOAD "  Email", "Receipt emailed");

    delivery_label = lv_label_create(delivery);
    lv_obj_add_style(delivery_label, ui_style_muted(), 0);
    lv_label_set_text(delivery_label, DELIVERY_PROMPT);

    summary = ui_page_add_card(parent, 1U);
    /* Child 1 of a row is its value label; kept so the summary can be restated
     * on every visit. */
    tender_value = lv_obj_get_child(ui_page_add_row(summary, "Tender",
                                                    "", true), 1);
    (void)ui_page_add_row(summary, "Authorisation", "A7C21B", false);
    items_value = lv_obj_get_child(ui_page_add_row(summary, "Items",
                                                   "", false), 1);
    (void)ui_page_add_row(summary, "Register", "1", false);
    (void)ui_page_add_row(summary, "Cashier", "Bob", false);

    page_receipt_refresh();

    (void)ui_page_add_nav_footer(parent,
                                 "Back", UI_PAGE_PAYMENT,
                                 "New sale", UI_PAGE_HOME);
}

/* [] END OF FILE */

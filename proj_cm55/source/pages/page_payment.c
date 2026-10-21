/*******************************************************************************
* File Name        : page_payment.c
*
* Description      : Payment method page. Offers a mutually exclusive choice of
*                    tender and shows the amount due.
*
*                    The selected method is expressed purely through
*                    LV_STATE_CHECKED plus the shared ui_style_selected(), so a
*                    mode or scheme change repaints it correctly without this
*                    page knowing anything about colours.
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

/*******************************************************************************
* Macros
*******************************************************************************/
#define METHOD_COUNT                    (3U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
static const char * const method_names[METHOD_COUNT] =
{
    "Card", "Cash", "Crypto",
};

/* The built-in symbol font has no coin glyph; the lightning bolt is the
 * closest recognisable stand-in for a crypto tender. */
static const char * const method_icons[METHOD_COUNT] =
{
    LV_SYMBOL_SD_CARD, LV_SYMBOL_FILE, LV_SYMBOL_CHARGE,
};

/* What the receipt prints for each tender. */
static const char * const method_tenders[METHOD_COUNT] =
{
    "Card - Visa 4242", "Cash", "Crypto - BTC",
};

static lv_obj_t * method_buttons[METHOD_COUNT];
static lv_obj_t * method_label;
static uint32_t method_selected;
static lv_obj_t * amount_value;
static lv_obj_t * charge_label;

/*******************************************************************************
* Function Name: page_payment_refresh
********************************************************************************
* Summary:
*  Focus callback. Shows the amount armed by whichever page sent the operator
*  here: the basket total, or the value keyed in on the ad hoc page. Safe to
*  call before the page exists.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void page_payment_refresh(void)
{
    const char * amount = ui_sale_amount_text();

    if (NULL != amount_value)
    {
        lv_label_set_text(amount_value, amount);
    }

    if (NULL != charge_label)
    {
        lv_label_set_text_fmt(charge_label, "Charge %s", amount);
    }
}


/*******************************************************************************
* Function Name: back_event_cb
********************************************************************************
* Summary:
*  Returns the operator to the page the amount was set on, which is the ad hoc
*  keypad rather than the basket when the sale was keyed in.
*
* Parameters:
*  e: Click event.
*
* Return:
*  None
*
*******************************************************************************/
static void back_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    (void)ui_nav_go_to(ui_sale_is_adhoc() ? UI_PAGE_ADHOC : UI_PAGE_BASKET,
                       UI_NAV_DIR_BACK);
}

/*******************************************************************************
* Function Name: charge_event_cb
********************************************************************************
* Summary:
*  Completes the sale and shows the receipt.
*
* Parameters:
*  e: Click event.
*
* Return:
*  None
*
*******************************************************************************/
static void charge_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    (void)ui_nav_go_to(UI_PAGE_RECEIPT, UI_NAV_DIR_FORWARD);
}

/*******************************************************************************
* Function Name: page_payment_tender_text
********************************************************************************
* Summary:
*  Returns the receipt description of the tender the customer is paying with.
*
* Parameters:
*  None
*
* Return:
*  const char *: Tender description. Never NULL.
*
*******************************************************************************/
const char * page_payment_tender_text(void)
{
    return method_tenders[method_selected];
}

/*******************************************************************************
* Function Name: method_event_cb
********************************************************************************
* Summary:
*  Makes the tender choice exclusive and updates the confirmation line.
*
* Parameters:
*  e: Click event.
*
* Return:
*  None
*
*******************************************************************************/
static void method_event_cb(lv_event_t * e)
{
    uint32_t selected = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    uint32_t i;

    for (i = 0U; i < METHOD_COUNT; i++)
    {
        if (i == selected)
        {
            lv_obj_add_state(method_buttons[i], LV_STATE_CHECKED);
        }
        else
        {
            lv_obj_remove_state(method_buttons[i], LV_STATE_CHECKED);
        }
    }

    method_selected = selected;
    lv_label_set_text_fmt(method_label, "Tender: %s", method_names[selected]);
}


/*******************************************************************************
* Function Name: add_method_button
********************************************************************************
* Summary:
*  Creates one tender option.
*
* Parameters:
*  parent: Row container.
*  index: Index into the method tables.
*
* Return:
*  None
*
*******************************************************************************/
static void add_method_button(lv_obj_t * parent, uint32_t index)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_t * label;

    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, ui_style_btn_secondary(), 0);
    lv_obj_add_style(btn, ui_style_selected(), LV_STATE_CHECKED);
    lv_obj_set_style_opa(btn, UI_BTN_PRESSED_OPA, LV_STATE_PRESSED);
    ui_page_add_press_style(btn);
    lv_obj_set_height(btn, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    label = lv_label_create(btn);
    lv_label_set_text(label, method_icons[index]);

    label = lv_label_create(btn);
    lv_label_set_text(label, method_names[index]);

    lv_obj_add_event_cb(btn, method_event_cb, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)index);

    method_buttons[index] = btn;
}


/*******************************************************************************
* Function Name: page_payment_create
********************************************************************************
* Summary:
*  Builds the payment method page.
*
* Parameters:
*  parent: Page container.
*
* Return:
*  None
*
*******************************************************************************/
void page_payment_create(lv_obj_t * parent)
{
    lv_obj_t * methods;
    lv_obj_t * footer;
    lv_obj_t * btn;
    lv_obj_t * row;
    uint32_t i;

    (void)ui_page_add_heading(parent, "Take payment",
                              "Choose how the customer is paying");

    amount_value = ui_page_add_amount_card(parent, "Amount due");

    methods = ui_page_add_card(parent, 1U);
    /* Centred in the space the card grows into; the buttons keep their size. */
    lv_obj_set_flex_align(methods, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    row = ui_page_add_button_row(methods);

    for (i = 0U; i < METHOD_COUNT; i++)
    {
        add_method_button(row, i);
    }

    method_label = lv_label_create(methods);
    lv_obj_add_style(method_label, ui_style_muted(), 0);
    lv_label_set_text_fmt(method_label, "Tender: %s",
                          method_names[method_selected]);
    lv_obj_add_state(method_buttons[method_selected], LV_STATE_CHECKED);

    footer = ui_page_add_nav_footer(parent, NULL, UI_PAGE_HOME,
                                    NULL, UI_PAGE_HOME);

    /* Both buttons are built here rather than by the footer helper: Back's
     * destination depends on the route the sale came in on, and Charge's
     * caption has to be rewritten with the amount on every visit. */
    (void)ui_page_add_button(footer, "Back", false, back_event_cb, NULL);
    btn = ui_page_add_button(footer, "Charge", true, charge_event_cb, NULL);
    charge_label = lv_obj_get_child(btn, 0);

    page_payment_refresh();
}

/* [] END OF FILE */

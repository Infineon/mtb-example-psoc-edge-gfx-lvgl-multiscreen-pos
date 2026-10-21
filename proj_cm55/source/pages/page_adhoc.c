/*******************************************************************************
* File Name        : page_adhoc.c
*
* Description      : Ad hoc sale page. Lets the operator key an amount in on a
*                    numeric keypad instead of working from the scanned basket,
*                    and hands that amount to the payment page.
*
*                    The keypad is a plain lv_buttonmatrix with a fixed map
*                    rather than lv_keyboard, so the demo pulls in no extra
*                    widget dependencies and the operator cannot switch the pad
*                    into a text mode that has no meaning for a currency field.
*
*                    Digits are shifted in from the right the way a till works:
*                    "1", "2", "5" reads as $1.25, so no decimal point key and
*                    no input validation are needed.
*
*                    This page is deliberately outside the SW2 carousel (see
*                    UI_PAGE_FLOW_COUNT in ui_nav.h); it is reachable from the
*                    landing page tile only.
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
#include <string.h>

#include "ui_pages.h"
#include "ui_nav.h"
#include "ui_theme.h"
#include "ui_layout.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* Largest amount that can be keyed in, in cents: $99,999.99. Chosen so the
 * formatted text always fits the field ui_page_amount_text() writes into. */
#define AMOUNT_MAX_CENTS                (9999999U)

#define DECIMAL_BASE                    (10U)

#define KEY_CLEAR                       "C"

/*******************************************************************************
* Global Variables
*******************************************************************************/
static const char * const keypad_map[] =
{
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    KEY_CLEAR, "0", LV_SYMBOL_BACKSPACE, "",
};

static lv_obj_t * amount_value;
static lv_obj_t * charge_btn;
static uint32_t entered_cents;

/*******************************************************************************
* Function Name: show_amount
********************************************************************************
* Summary:
*  Mirrors the working value into the big amount label, and keeps the payment
*  button out of reach until there is something to charge.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
static void show_amount(void)
{
    if (NULL == amount_value)
    {
        return;
    }

    lv_label_set_text(amount_value, ui_page_amount_text(entered_cents));

    if (0U == entered_cents)
    {
        lv_obj_add_state(charge_btn, LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_remove_state(charge_btn, LV_STATE_DISABLED);
    }
}


/*******************************************************************************
* Function Name: page_adhoc_reset
********************************************************************************
* Summary:
*  Clears the keyed-in amount so a new ad hoc sale starts from zero. Called by
*  the landing page when the operator picks the ad hoc tile. It is deliberately
*  not a focus callback: the payment page's Back button also lands here, and
*  that visit must keep the amount so it can be corrected. Safe to call before
*  the page exists.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void page_adhoc_reset(void)
{
    entered_cents = 0U;
    show_amount();
}


/*******************************************************************************
* Function Name: keypad_event_cb
********************************************************************************
* Summary:
*  Applies one key press to the working value.
*
* Parameters:
*  e: Value-changed event from the keypad.
*
* Return:
*  None
*
*******************************************************************************/
static void keypad_event_cb(lv_event_t * e)
{
    lv_obj_t * keypad = lv_event_get_target(e);
    uint32_t id = lv_buttonmatrix_get_selected_button(keypad);
    const char * key = lv_buttonmatrix_get_button_text(keypad, id);

    if (NULL == key)
    {
        return;
    }

    if (0 == strcmp(key, KEY_CLEAR))
    {
        entered_cents = 0U;
    }
    else if (0 == strcmp(key, LV_SYMBOL_BACKSPACE))
    {
        entered_cents /= DECIMAL_BASE;
    }
    else if (entered_cents <= (AMOUNT_MAX_CENTS / DECIMAL_BASE))
    {
        entered_cents = (entered_cents * DECIMAL_BASE) +
                        (uint32_t)(key[0] - '0');
    }
    else
    {
        /* At the input limit; ignore further digits. */
    }

    show_amount();
}


/*******************************************************************************
* Function Name: charge_event_cb
********************************************************************************
* Summary:
*  Arms the open sale with the keyed-in amount and continues to the payment
*  page. The direction is stated explicitly because the ad hoc page sits after
*  the payment page in the enum, so the generic rule used by the shared
*  navigation buttons would animate this the wrong way round.
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

    ui_sale_arm(entered_cents, true);
    (void)ui_nav_go_to(UI_PAGE_PAYMENT, UI_NAV_DIR_FORWARD);
}


/*******************************************************************************
* Function Name: page_adhoc_create
********************************************************************************
* Summary:
*  Builds the ad hoc amount entry page.
*
* Parameters:
*  parent: Page container.
*
* Return:
*  None
*
*******************************************************************************/
void page_adhoc_create(lv_obj_t * parent)
{
    lv_obj_t * pad_card;
    lv_obj_t * keypad;
    lv_obj_t * footer;

    (void)ui_page_add_heading(parent, "Enter an amount",
                              "Digits fill in from the cents up");

    amount_value = ui_page_add_amount_card(parent, "Amount due");

    pad_card = ui_page_add_card(parent, 1U);

    keypad = lv_buttonmatrix_create(pad_card);
    lv_buttonmatrix_set_map(keypad, keypad_map);
    /* The keypad is a command surface, not a selection: no key may stay lit. */
    lv_buttonmatrix_set_one_checked(keypad, false);
    /* Without this the keys keep the stock theme: a fixed 14 px font on every
     * panel, and a light-mode drop shadow this UI does not use. */
    lv_obj_add_style(keypad, ui_style_key(), LV_PART_ITEMS);
    lv_obj_set_style_opa(keypad, UI_BTN_PRESSED_OPA,
                         (lv_style_selector_t)LV_PART_ITEMS |
                         (lv_style_selector_t)LV_STATE_PRESSED);
    lv_obj_set_width(keypad, LV_PCT(100));
    lv_obj_set_flex_grow(keypad, 1);
    lv_obj_set_style_pad_all(keypad, 0, 0);
    lv_obj_set_style_border_width(keypad, 0, 0);
    lv_obj_set_style_bg_opa(keypad, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_gap(keypad, ui_layout_pad(), 0);
    lv_obj_add_event_cb(keypad, keypad_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    footer = ui_page_add_nav_footer(parent, "Back", UI_PAGE_HOME, NULL,
                                    UI_PAGE_HOME);

    charge_btn = ui_page_add_button(footer, "Take payment", true,
                                    charge_event_cb, NULL);
    lv_obj_add_style(charge_btn, ui_style_disabled(), LV_STATE_DISABLED);

    /* Last, not next to the label: show_amount() also sets the button state. */
    show_amount();
}

/* [] END OF FILE */

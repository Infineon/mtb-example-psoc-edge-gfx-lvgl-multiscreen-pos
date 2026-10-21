/*******************************************************************************
* File Name        : ui_pages.h
*
* Description      : Widget tree builders for the POS pages.
*
*                    Page content is deliberately decoupled from the navigation
*                    framework: a page only knows how to fill a container it is
*                    handed, and how to ask ui_nav to go somewhere else. That
*                    keeps the branding free to change without touching the
*                    reusable parts of this code example.
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

#ifndef UI_PAGES_H
#define UI_PAGES_H

#include "lvgl.h"

#include "ui_nav.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

void page_home_create(lv_obj_t * parent);

void page_home_refresh(void);

void page_basket_create(lv_obj_t * parent);

uint32_t page_basket_item_count(void);

uint32_t page_basket_total_cents(void);

void page_basket_refresh(void);

void page_adhoc_create(lv_obj_t * parent);

void page_adhoc_reset(void);

void page_payment_create(lv_obj_t * parent);

void page_payment_refresh(void);

const char * page_payment_tender_text(void);

void page_receipt_create(lv_obj_t * parent);

void page_receipt_refresh(void);

void page_settings_create(lv_obj_t * parent);

void page_settings_sync_theme_switch(void);

void page_settings_defocus(void);

/*******************************************************************************
* Open sale
********************************************************************************
* The amount being charged, and the route it came from, are the pieces of state
* more than one page needs, so they live here instead of in whichever page
* happens to display them. The basket page arms the sale with the basket total,
* the ad hoc page with the keyed-in value, and the payment and receipt pages
* only ever read it.
*******************************************************************************/

void ui_sale_arm(uint32_t cents, bool adhoc);

bool ui_sale_is_adhoc(void);

const char * ui_sale_amount_text(void);

/*******************************************************************************
* Shared page helpers
*******************************************************************************/

lv_obj_t * ui_page_add_heading(lv_obj_t * parent, const char * title,
                               const char * subtitle);

lv_obj_t * ui_page_add_card(lv_obj_t * parent, uint8_t grow);

lv_obj_t * ui_page_add_row(lv_obj_t * parent, const char * label,
                           const char * value, bool strong);

lv_obj_t * ui_page_add_amount_card(lv_obj_t * parent, const char * caption);

lv_obj_t * ui_page_add_nav_footer(lv_obj_t * parent,
                                  const char * back_text,
                                  ui_page_id_t back_target,
                                  const char * forward_text,
                                  ui_page_id_t forward_target);

lv_obj_t * ui_page_add_button(lv_obj_t * parent, const char * text,
                              bool primary, lv_event_cb_t cb,
                              void * user_data);

lv_obj_t * ui_page_add_button_row(lv_obj_t * parent);

void ui_page_add_press_style(lv_obj_t * obj);

const char * ui_page_amount_text(uint32_t cents);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UI_PAGES_H */

/* [] END OF FILE */

/*******************************************************************************
* File Name        : page_home.c
*
* Description      : Landing page of the POS flow. Offers the two ways of
*                    starting a sale - work from the scanned basket, or key an
*                    amount in - and summarises what is already scanned.
*
*                    The tile row re-flows on its own: every tile is
*                    flex-grown and nothing is positioned by hand, so the same
*                    code fills a 480x480 round-ish panel and a 1024x600 bar
*                    display without a single hard-coded coordinate.
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
* Data Structures
*******************************************************************************/
typedef struct
{
    const char * icon;
    const char * name;
    const char * hint;
    ui_page_id_t target;
} start_option_t;

/*******************************************************************************
* Global Variables
*******************************************************************************/
static const start_option_t start_options[] =
{
    { LV_SYMBOL_LIST, "Basket", "Scanned items", UI_PAGE_BASKET },
    { LV_SYMBOL_EDIT, "Ad hoc", "Key an amount", UI_PAGE_ADHOC  },
};

#define START_OPTION_COUNT  (sizeof(start_options) / sizeof(start_options[0]))

static lv_obj_t * items_value;

/*******************************************************************************
* Function Name: page_home_refresh
********************************************************************************
* Summary:
*  Focus callback. Restates the open basket so the landing page and the basket
*  page can never disagree about what is in the sale, and re-arms the sale with
*  the basket total in case the last visit went down the ad hoc route.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void page_home_refresh(void)
{
    ui_sale_arm(page_basket_total_cents(), false);

    if (NULL != items_value)
    {
        lv_label_set_text_fmt(items_value, "%u items",
                              (unsigned int)page_basket_item_count());
    }
}


/*******************************************************************************
* Function Name: tile_event_cb
********************************************************************************
* Summary:
*  Sends the operator down the route the pressed tile stands for.
*
* Parameters:
*  e: Click event carrying the destination page id.
*
* Return:
*  None
*
*******************************************************************************/
static void tile_event_cb(lv_event_t * e)
{
    ui_page_id_t target = (ui_page_id_t)(uintptr_t)lv_event_get_user_data(e);

    if (UI_PAGE_ADHOC == target)
    {
        /* Reset here rather than on focus: the payment page's Back button also
         * lands on the keypad, and that visit must keep the keyed-in amount. */
        page_adhoc_reset();
    }

    (void)ui_nav_go_to(target, UI_NAV_DIR_FORWARD);
}


/*******************************************************************************
* Function Name: add_tile
********************************************************************************
* Summary:
*  Creates one start-a-sale tile inside the tile row.
*
* Parameters:
*  row: Row container.
*  entry: Option to render.
*
* Return:
*  None
*
*******************************************************************************/
static void add_tile(lv_obj_t * row, const start_option_t * entry)
{
    lv_obj_t * tile = lv_button_create(row);
    lv_obj_t * label;

    lv_obj_remove_style_all(tile);
    lv_obj_add_style(tile, ui_style_card(), 0);
    lv_obj_set_style_opa(tile, UI_BTN_PRESSED_OPA, LV_STATE_PRESSED);
    lv_obj_add_style(tile, ui_style_pressed(), LV_STATE_PRESSED);
    lv_obj_set_height(tile, LV_PCT(100));
    lv_obj_set_flex_grow(tile, 1);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    label = lv_label_create(tile);
    lv_obj_add_style(label, ui_style_title(), 0);
    /* Shared style, not a local colour: a local one would freeze at the mode
     * that was active when the tile was built and ignore the theme toggle. */
    lv_obj_add_style(label, ui_style_accent_text(), 0);
    lv_label_set_text(label, entry->icon);

    label = lv_label_create(tile);
    lv_obj_add_style(label, ui_style_body(), 0);
    lv_label_set_text(label, entry->name);

    label = lv_label_create(tile);
    lv_obj_add_style(label, ui_style_muted(), 0);
    lv_label_set_text(label, entry->hint);

    lv_obj_add_event_cb(tile, tile_event_cb, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)entry->target);
}


/*******************************************************************************
* Function Name: page_home_create
********************************************************************************
* Summary:
*  Builds the "New sale" landing page.
*
* Parameters:
*  parent: Page container.
*
* Return:
*  None
*
*******************************************************************************/
void page_home_create(lv_obj_t * parent)
{
    lv_obj_t * grid;
    lv_obj_t * row;
    lv_obj_t * totals;
    lv_obj_t * label;
    uint32_t i;

    (void)ui_page_add_heading(parent, "Start a sale",
                              "Work from the basket or key in an amount");

    /* The grid is the scrolling part of the page. */
    grid = ui_page_add_card(parent, 1U);

    row = ui_page_add_button_row(grid);
    lv_obj_set_height(row, LV_PCT(100));
    lv_obj_set_flex_grow(row, 1);

    for (i = 0U; i < START_OPTION_COUNT; i++)
    {
        add_tile(row, &start_options[i]);
    }

    totals = ui_page_add_card(parent, 0U);
    /* Child 1 of a row is its value label; kept so the summary can be restated
     * on every visit. */
    items_value = lv_obj_get_child(ui_page_add_row(totals, "Scanned items",
                                                   "", true), 1);

    label = lv_label_create(totals);
    lv_obj_add_style(label, ui_style_muted(), 0);
    lv_label_set_text(label, "Open the basket to review the totals and bill");

    page_home_refresh();

    (void)ui_page_add_nav_footer(parent,
                                 "Settings", UI_PAGE_SETTINGS,
                                 NULL, UI_PAGE_HOME);
}

/* [] END OF FILE */

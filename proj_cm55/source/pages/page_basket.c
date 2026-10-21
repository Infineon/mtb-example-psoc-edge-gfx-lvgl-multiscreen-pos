/*******************************************************************************
* File Name        : page_basket.c
*
* Description      : Basket page. Lists the items in the open sale and the
*                    totals. The items stand in for goods scanned at the till,
*                    so there is deliberately no on-screen item entry.
*
*                    This page is the one that proves pages are kept alive
*                    rather than rebuilt: scroll the item list, navigate away,
*                    come back, and the scroll position is still where it was.
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
#include "ui_header.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* Long enough for "NNN items". */
#define CAPTION_TEXT_LEN    (16U)

/* Long enough for "-$99999.99" and "Tax (99.99%)". */
#define AMOUNT_TEXT_LEN     (20U)

/* Tax on the subtotal, in hundredths of a percent: 875 is 8.75%. */
#define TAX_BASIS_POINTS    (875U)
#define BASIS_POINTS_UNIT   (10000U)
#define PERCENT_SCALE       (100U)

/*******************************************************************************
* Data Structures
*******************************************************************************/
typedef struct
{
    const char * name;
    int32_t cents;      /* Line total; negative for a discount.           */
    uint32_t qty;       /* 0 for an adjustment line, which is not an item. */
} line_item_t;

/*******************************************************************************
* Global Variables
*******************************************************************************/
static const line_item_t line_items[] =
{
    { "Flat white  x2",       760, 2U },
    { "Almond croissant",     420, 1U },
    { "Sourdough loaf",       650, 1U },
    { "Orange juice  x2",     500, 2U },
    { "Oat milk upgrade",      80, 1U },
    { "Bananas  1 kg",        120, 1U },
    { "Free-range eggs  x12", 450, 1U },
    { "Whole milk  2 L",      260, 1U },
    { "Cheddar  250 g",       380, 1U },
    { "Baby spinach",         240, 1U },
    { "Tomatoes  x3",         210, 3U },
    { "Loyalty discount",    -130, 0U },
};

#define LINE_ITEM_COUNT     (sizeof(line_items) / sizeof(line_items[0]))

/*******************************************************************************
* Function Name: basket_subtotal_cents
********************************************************************************
* Summary:
*  Sums every line of the basket, adjustments included.
*
* Parameters:
*  None
*
* Return:
*  uint32_t: Subtotal in cents.
*
*******************************************************************************/
static uint32_t basket_subtotal_cents(void)
{
    int32_t sum = 0;
    uint32_t i;

    for (i = 0U; i < LINE_ITEM_COUNT; i++)
    {
        sum += line_items[i].cents;
    }

    return (uint32_t)sum;
}


/*******************************************************************************
* Function Name: basket_tax_cents
********************************************************************************
* Summary:
*  Tax on the subtotal, rounded half up to the cent.
*
* Parameters:
*  None
*
* Return:
*  uint32_t: Tax in cents.
*
*******************************************************************************/
static uint32_t basket_tax_cents(void)
{
    return ((basket_subtotal_cents() * TAX_BASIS_POINTS) +
            (BASIS_POINTS_UNIT / 2U)) / BASIS_POINTS_UNIT;
}


/*******************************************************************************
* Function Name: line_amount_text
********************************************************************************
* Summary:
*  Formats a line total, which unlike an amount due can be negative.
*
* Parameters:
*  cents: Line total in cents.
*
* Return:
*  const char *: "$1.23" or "-$1.23" text, valid until the next call.
*
*******************************************************************************/
static const char * line_amount_text(int32_t cents)
{
    static char text[AMOUNT_TEXT_LEN];

    if (cents >= 0)
    {
        return ui_page_amount_text((uint32_t)cents);
    }

    (void)snprintf(text, sizeof(text), "-%s",
                   ui_page_amount_text((uint32_t)(-cents)));

    return text;
}


/*******************************************************************************
* Function Name: page_basket_item_count
********************************************************************************
* Summary:
*  Returns the number of items in the open basket.
*
* Parameters:
*  None
*
* Return:
*  uint32_t: Total item quantity.
*
*******************************************************************************/
uint32_t page_basket_item_count(void)
{
    uint32_t count = 0U;
    uint32_t i;

    for (i = 0U; i < LINE_ITEM_COUNT; i++)
    {
        count += line_items[i].qty;
    }

    return count;
}


/*******************************************************************************
* Function Name: page_basket_total_cents
********************************************************************************
* Summary:
*  Returns the amount due for the open basket.
*
* Parameters:
*  None
*
* Return:
*  uint32_t: Amount in cents.
*
*******************************************************************************/
uint32_t page_basket_total_cents(void)
{
    return basket_subtotal_cents() + basket_tax_cents();
}


/*******************************************************************************
* Function Name: page_basket_refresh
********************************************************************************
* Summary:
*  Focus callback. Writes the basket size into the shared header caption and
*  arms the open sale with the basket total, so reaching the payment page
*  through the basket charges the basket rather than whatever an earlier ad hoc
*  sale left behind.
*
*  The registry caption is a fixed string, so counting the line items here is
*  what keeps the header, the basket and the receipt telling the same story.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void page_basket_refresh(void)
{
    static char caption[CAPTION_TEXT_LEN];

    ui_sale_arm(page_basket_total_cents(), false);

    (void)snprintf(caption, sizeof(caption), "%u items",
                   (unsigned int)page_basket_item_count());

    ui_header_set_caption(caption);
}

/*******************************************************************************
* Function Name: page_basket_create
********************************************************************************
* Summary:
*  Builds the basket page.
*
* Parameters:
*  parent: Page container.
*
* Return:
*  None
*
*******************************************************************************/
void page_basket_create(lv_obj_t * parent)
{
    char tax_label[AMOUNT_TEXT_LEN];
    lv_obj_t * items;
    lv_obj_t * totals;
    lv_obj_t * total_row;
    uint32_t i;

    (void)ui_page_add_heading(parent, "Review the sale",
                              "Check the items before taking payment");

    items = ui_page_add_card(parent, 1U);

    for (i = 0U; i < LINE_ITEM_COUNT; i++)
    {
        (void)ui_page_add_row(items, line_items[i].name,
                              line_amount_text(line_items[i].cents), true);
    }

    (void)snprintf(tax_label, sizeof(tax_label), "Tax (%u.%02u%%)",
                   (unsigned int)(TAX_BASIS_POINTS / PERCENT_SCALE),
                   (unsigned int)(TAX_BASIS_POINTS % PERCENT_SCALE));

    totals = ui_page_add_card(parent, 0U);
    (void)ui_page_add_row(totals, "Subtotal",
                          ui_page_amount_text(basket_subtotal_cents()), false);
    (void)ui_page_add_row(totals, tax_label,
                          ui_page_amount_text(basket_tax_cents()), false);

    total_row = ui_page_add_row(totals, "Amount due",
                                ui_page_amount_text(page_basket_total_cents()),
                                true);
    /* The amount is the one thing on this page that must be unmissable. */
    lv_obj_add_style(lv_obj_get_child(total_row, 1), ui_style_amount(), 0);

    (void)ui_page_add_nav_footer(parent,
                                 "Back", UI_PAGE_HOME,
                                 "Take payment", UI_PAGE_PAYMENT);
}

/* [] END OF FILE */

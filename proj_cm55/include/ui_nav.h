/*******************************************************************************
* File Name        : ui_nav.h
*
* Description      : Navigation API implemented by ui_nav_host.c: a single
*                    navigation host screen holding one container per page,
*                    with custom spring-eased transitions between them.
*
*                    Page content code only ever calls this API, so pages carry
*                    no knowledge of how a transition is performed.
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

#ifndef UI_NAV_H
#define UI_NAV_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* Data Structures
*******************************************************************************/

/* Pages of the POS flow, in navigation order.
 *
 *  Everything from UI_PAGE_ADHOC onwards is a side branch: reachable from its
 *  own on-screen button only, never from the SW2 carousel. Keep such pages at
 *  the end of the enum so UI_PAGE_FLOW_COUNT below stays a simple bound. */
typedef enum
{
    UI_PAGE_HOME = 0,
    UI_PAGE_BASKET,
    UI_PAGE_PAYMENT,
    UI_PAGE_RECEIPT,
    UI_PAGE_ADHOC,
    UI_PAGE_SETTINGS,
    UI_PAGE_COUNT,
} ui_page_id_t;

/* Direction the new page slides in from. */
typedef enum
{
    UI_NAV_DIR_FORWARD = 0, /* New page enters from the right. */
    UI_NAV_DIR_BACK,        /* New page enters from the left.  */
} ui_nav_dir_t;

/* Length of the forward "sale flow" carousel. Ad hoc and Settings are
 *  deliberately last in the enum so they can be excluded here: both are side
 *  branches reached from their own button, not stops that ui_nav_next() /
 *  ui_nav_prev() cycle through. */
#define UI_PAGE_FLOW_COUNT              (UI_PAGE_ADHOC)

/* Builds the widget tree of a page. 'parent' is the container the page must
 * populate; it is already styled and sized, so the callback only adds
 * children. */
typedef void (*ui_page_create_cb_t)(lv_obj_t * parent);

/* Resets transient state each time a page becomes visible. Pages are created
 * once and kept, so anything that must look fresh on every visit has to be
 * reset here rather than in the create callback. */
typedef void (*ui_page_focus_cb_t)(void);

/* Winds down transient state when a transition away from a page starts,
 * e.g. closes a popup or cancels a pending timer. */
typedef void (*ui_page_defocus_cb_t)(void);

/* Refreshes incoming content before the page becomes current or slides in.
 * Arrival-only effects remain in the focus callback. */
typedef void (*ui_page_prepare_cb_t)(void);

/* One entry of the page registry. */
typedef struct
{
    const char * name;               /* Trace name, e.g. "page_home".     */
    const char * title;              /* Text shown in the shared header.  */
    const char * caption;            /* Sub-title shown in the header.    */
    ui_page_create_cb_t create_cb;   /* Widget tree builder.              */
    ui_page_focus_cb_t focus_cb;     /* Per-visit reset, may be NULL.     */
    ui_page_defocus_cb_t defocus_cb; /* Wind-down, may be NULL.           */
    ui_page_prepare_cb_t prepare_cb; /* Pre-entry refresh, may be NULL.   */
} ui_page_desc_t;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

void ui_nav_init(void);

bool ui_nav_go_to(ui_page_id_t id, ui_nav_dir_t dir);

bool ui_nav_next(void);

bool ui_nav_prev(void);

ui_page_id_t ui_nav_current(void);

bool ui_nav_is_busy(void);

const ui_page_desc_t * ui_nav_page_desc(ui_page_id_t id);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UI_NAV_H */

/* [] END OF FILE */

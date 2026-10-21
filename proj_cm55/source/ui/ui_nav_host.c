/*******************************************************************************
* File Name        : ui_nav_host.c
*
* Description      : Navigation-host implementation of the ui_nav API.
*
*                    Why a host instead of one real screen per page: LVGL only
*                    ever refreshes the active screen, the outgoing screen it
*                    owns internally during lv_screen_load_anim(), and the top
*                    and system layers. The outgoing screen pointer is private,
*                    so a custom easing curve cannot be applied to it. Parking
*                    one real screen as a "host" and making every page a child
*                    container of a viewport inside it puts both the incoming
*                    and the outgoing page under application control, which is
*                    what allows the spring curve, the interruptible state
*                    machine and the lifecycle trace.
*
*                    Layout built here:
*
*                      host screen  (flex column, no scrolling)
*                        +-- shared header      (fixed height)
*                        +-- viewport           (flex grow 1, clips children)
*                        |     +-- page[0..N]   (absolute, LV_PCT(100))
*                        +-- header shadow      (strip over viewport top)
*
*                    Pages are created lazily on first visit and then kept, so
*                    a revisit is instant and page state survives.
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
#include "ui_config.h"

#include "cybsp.h"

#include "ui_nav.h"
#include "ui_pages.h"
#include "ui_header.h"
#include "ui_theme.h"
#include "ui_layout.h"
#include "ui_lifecycle.h"
#include "ui_spring_anim.h"

/*******************************************************************************
* Global Variables
*******************************************************************************/
static lv_obj_t * host;
static lv_obj_t * viewport;
static lv_obj_t * pages[UI_PAGE_COUNT];

static ui_page_id_t current_page = UI_PAGE_HOME;
static bool transition_busy;

/* Everything the completion callback needs. Kept in one place instead of
 * smuggled through lv_anim_t::user_data, because user_data already carries the
 * spring parameters. */
static struct
{
    ui_page_id_t from;
    ui_page_id_t to;
} transition;

/*******************************************************************************
* Function Name: transition_end
********************************************************************************
* Summary:
*  Reopens the navigation state machine. Shared by the normal completion path
*  and the abort path so that both can never diverge.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
static void transition_end(void)
{
    transition_busy = false;
}


/*******************************************************************************
* Function Name: page_delete_cb
********************************************************************************
* Summary:
*  Emits the destroy lifecycle event. Pages are not normally deleted in this
*  code example, but wiring the hook up keeps the trace honest if an
*  application built on top of it starts evicting pages to save memory.
*
* Parameters:
*  e: LV_EVENT_DELETE event.
*
* Return:
*  None
*
*******************************************************************************/
static void page_delete_cb(lv_event_t * e)
{
    ui_page_id_t id = (ui_page_id_t)(uintptr_t)lv_event_get_user_data(e);
    const ui_page_desc_t * desc = ui_nav_page_desc(id);

    if (NULL != desc)
    {
        ui_lifecycle_log(desc->name, UI_LC_DESTROY);
    }

    /* Deleting an object cancels its animations without running their
     * completion callback, so a page evicted mid-transition would otherwise
     * leave transition_busy set and block navigation for good. */
    if (transition_busy && ((id == transition.from) || (id == transition.to)))
    {
        transition_end();
    }

    pages[id] = NULL;
}


/*******************************************************************************
* Function Name: page_ensure
********************************************************************************
* Summary:
*  Returns the container of a page, creating and populating it on first use.
*
* Parameters:
*  id: Page id.
*
* Return:
*  lv_obj_t *: Page container, or NULL if the id is unknown.
*
*******************************************************************************/
static lv_obj_t * page_ensure(ui_page_id_t id)
{
    const ui_page_desc_t * desc = ui_nav_page_desc(id);
    lv_obj_t * page;

    if (NULL == desc)
    {
        return NULL;
    }

    if (NULL != pages[id])
    {
        return pages[id];
    }

    page = lv_obj_create(viewport);
    lv_obj_remove_style_all(page);
    lv_obj_add_style(page, ui_style_page(), 0);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(page, 0, 0);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    /* Vertical scrolling only: a horizontal drag would fight the transition. */
    lv_obj_set_scroll_dir(page, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_event_cb(page, page_delete_cb, LV_EVENT_DELETE,
                        (void *)(uintptr_t)id);

    pages[id] = page;

    desc->create_cb(page);
    ui_lifecycle_log(desc->name, UI_LC_CREATE);

    return page;
}


/*******************************************************************************
* Function Name: page_set_focused
********************************************************************************
* Summary:
*  Applies the header text for a page and emits the focus event.
*
* Parameters:
*  id: Page that has become visible.
*
* Return:
*  None
*
*******************************************************************************/
static void page_set_focused(ui_page_id_t id)
{
    const ui_page_desc_t * desc = ui_nav_page_desc(id);

    if (NULL == desc)
    {
        return;
    }

    ui_header_set_title(desc->title);
    ui_header_set_caption(desc->caption);

    if (NULL != desc->focus_cb)
    {
        desc->focus_cb();
    }

    ui_lifecycle_log(desc->name, UI_LC_FOCUS);
}


/*******************************************************************************
* Function Name: anim_x_cb
********************************************************************************
* Summary:
*  Animation execution callback. lv_obj_set_x() is not used directly because
*  its signature does not match lv_anim_exec_xcb_t.
*
* Parameters:
*  var: Object being animated.
*  v: New x coordinate.
*
* Return:
*  None
*
*******************************************************************************/
static void anim_x_cb(void * var, int32_t v)
{
    lv_obj_set_x((lv_obj_t *)var, v);
}


/*******************************************************************************
* Function Name: transition_done_cb
********************************************************************************
* Summary:
*  Runs when the incoming page has settled. Finishes the state machine: the
*  outgoing page is parked off-screen and hidden so it costs nothing to render,
*  and the lifecycle trace is completed.
*
* Parameters:
*  a: Animation that completed (unused).
*
* Return:
*  None
*
*******************************************************************************/
static void transition_done_cb(lv_anim_t * a)
{
    const ui_page_desc_t * from_desc = ui_nav_page_desc(transition.from);
    lv_obj_t * from_page = pages[transition.from];

    LV_UNUSED(a);

    transition_end();

    if ((NULL != from_page) && (transition.from != transition.to))
    {
        lv_obj_set_x(from_page, 0);
        lv_obj_add_flag(from_page, LV_OBJ_FLAG_HIDDEN);

        if (NULL != from_desc)
        {
            ui_lifecycle_log(from_desc->name, UI_LC_UNLOADED);
        }
    }

    /* Snap the incoming page to its resting position. The spring path can end
     * a pixel off if the animation is cut short. */
    if (NULL != pages[transition.to])
    {
        lv_obj_set_x(pages[transition.to], 0);
    }

    page_set_focused(transition.to);
}


/*******************************************************************************
* Function Name: start_slide
********************************************************************************
* Summary:
*  Starts one spring-eased horizontal slide.
*
* Parameters:
*  target: Object to move.
*  from_x: Start x.
*  to_x: End x.
*  with_completed_cb: true on the incoming page only.
*
* Return:
*  None
*
*******************************************************************************/
static void start_slide(lv_obj_t * target, int32_t from_x, int32_t to_x,
                        bool with_completed_cb)
{
    lv_anim_t a;

    lv_anim_init(&a);
    lv_anim_set_var(&a, target);
    lv_anim_set_exec_cb(&a, anim_x_cb);
    lv_anim_set_values(&a, from_x, to_x);
    lv_anim_set_duration(&a, UI_SPRING_DURATION_MS);
    lv_anim_set_path_cb(&a, ui_anim_path_spring);
    /* NULL user data makes the path callback fall back to the defaults from
     * ui_config.h; a per-transition ui_spring_params_t could be passed here to
     * give, say, "back" a stiffer feel than "forward". */
    lv_anim_set_user_data(&a, NULL);

    if (with_completed_cb)
    {
        lv_anim_set_completed_cb(&a, transition_done_cb);
    }

    lv_obj_set_x(target, from_x);
    (void)lv_anim_start(&a);
}


/*******************************************************************************
* Function Name: ui_nav_init
********************************************************************************
* Summary:
*  Builds the host screen, the shared header and the viewport, then shows the
*  first page without animating. Must be called from the LVGL task after
*  ui_theme_init().
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void ui_nav_init(void)
{
    lv_obj_t * first;

    host = lv_screen_active();
    lv_obj_remove_style_all(host);
    lv_obj_add_style(host, ui_style_page(), 0);
    lv_obj_set_style_pad_all(host, 0, 0);
    lv_obj_set_style_pad_row(host, 0, 0);
    lv_obj_remove_flag(host, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(host, LV_FLEX_FLOW_COLUMN);

    (void)ui_header_create(host);

    viewport = lv_obj_create(host);
    lv_obj_remove_style_all(viewport);
#if (UI_SPRING_HIDE_OVERSHOOT_GAP)
    /* An under-damped spring overshoots past the resting position, briefly
     * exposing a sliver of viewport at the trailing edge. Sharing the page
     * style paints that sliver in the page background colour, so it is
     * invisible - and because the style is the shared one, it keeps matching
     * after a theme toggle. */
    lv_obj_add_style(viewport, ui_style_page(), 0);
#else
    /* Tuning aid: paint the gap in the danger colour so the overshoot can be
     * seen and the spring constants judged. This one deliberately does not
     * follow the theme; it is not meant to ship. */
    lv_obj_set_style_bg_color(viewport, ui_theme_color(UI_COLOR_DANGER), 0);
    lv_obj_set_style_bg_opa(viewport, LV_OPA_COVER, 0);
#endif
    lv_obj_set_width(viewport, LV_PCT(100));
    lv_obj_set_flex_grow(viewport, 1);
    /* Local style, so it wins over the padding carried by the page style. */
    lv_obj_set_style_pad_all(viewport, 0, 0);
    lv_obj_remove_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);
    /* LV_OBJ_FLAG_OVERFLOW_VISIBLE is deliberately left clear: the viewport
     * clipping children is exactly what hides the pages parked off-screen. */

    ui_header_create_shadow(host);

    current_page = UI_PAGE_HOME;
    transition.from = UI_PAGE_HOME;
    transition.to = UI_PAGE_HOME;

    first = page_ensure(current_page);

    if (NULL != first)
    {
        lv_obj_set_x(first, 0);
        page_set_focused(current_page);
    }
}


/*******************************************************************************
* Function Name: ui_nav_go_to
********************************************************************************
* Summary:
*  Starts a transition to a page. Ignored while a transition is already running,
*  so a fast double press cannot leave two animations fighting over the same
*  containers.
*
* Parameters:
*  id: Destination page.
*  dir: Direction the destination enters from.
*
* Return:
*  bool: true if a transition was started.
*
*******************************************************************************/
bool ui_nav_go_to(ui_page_id_t id, ui_nav_dir_t dir)
{
    const ui_page_desc_t * from_desc;
    const ui_page_desc_t * to_desc;
    lv_obj_t * from_page;
    lv_obj_t * to_page;
    lv_obj_t * outgoing;
    int32_t width;
    int32_t enter_from;

    if ((id >= UI_PAGE_COUNT) || (id == current_page) || (NULL == viewport))
    {
        return false;
    }

    /* One transition at a time. Dropping the request is preferable to
     * restarting mid-flight, which would make the spring jump. */
    if (transition_busy)
    {
        return false;
    }

    from_page = pages[current_page];
    to_page = page_ensure(id);

    if ((NULL == from_page) || (NULL == to_page))
    {
        return false;
    }

    /* The viewport is laid out by flex, so its size is only valid after the
     * layout has been calculated. */
    lv_obj_update_layout(viewport);
    width = lv_obj_get_width(viewport);

    if (width <= 0)
    {
        return false;
    }

    enter_from = (UI_NAV_DIR_FORWARD == dir) ? width : -width;

    from_desc = ui_nav_page_desc(current_page);

    if (NULL != from_desc)
    {
        if (NULL != from_desc->defocus_cb)
        {
            from_desc->defocus_cb();
        }

        ui_lifecycle_log(from_desc->name, UI_LC_DEFOCUS);
    }

    to_desc = ui_nav_page_desc(id);

    if ((NULL != to_desc) && (NULL != to_desc->prepare_cb))
    {
        to_desc->prepare_cb();
    }

    transition.from = current_page;
    transition.to = id;
    transition_busy = true;
    current_page = id;

    lv_obj_remove_flag(to_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(to_page);

    outgoing = from_page;

    /* Both slides share the same duration and the same path, so the two pages
     * stay exactly one viewport width apart for the whole transition. */
    start_slide(outgoing, 0, -enter_from, false);
    start_slide(to_page, enter_from, 0, true);

    return true;
}


/*******************************************************************************
* Function Name: ui_nav_next
********************************************************************************
* Summary:
*  Moves to the next page of the sale flow, wrapping around. Only the first
*  UI_PAGE_FLOW_COUNT pages take part; called from Settings, it returns to the
*  start of the flow.
*
* Parameters:
*  None
*
* Return:
*  bool: true if a transition was started.
*
*******************************************************************************/
bool ui_nav_next(void)
{
    /* Settings is a side branch reached from its own buttons, not a stop on
     * the sale-flow carousel, so it is excluded from the wrap count here. */
    ui_page_id_t next = (current_page < UI_PAGE_FLOW_COUNT)
        ? (ui_page_id_t)((current_page + 1) % UI_PAGE_FLOW_COUNT)
        : UI_PAGE_HOME;

    /* Wrapping from the last page back to the first animates backwards so the
     * motion matches the mental model of a carousel rewinding. */
    return ui_nav_go_to(next, (UI_PAGE_HOME == next) ? UI_NAV_DIR_BACK
                                                     : UI_NAV_DIR_FORWARD);
}


/*******************************************************************************
* Function Name: ui_nav_prev
********************************************************************************
* Summary:
*  Moves to the previous page of the sale flow, wrapping around. The mirror
*  image of ui_nav_next(), so it excludes Settings from the carousel too.
*  Provided as the hook a swipe gesture or a hardware "back" key would use.
*
* Parameters:
*  None
*
* Return:
*  bool: true if a transition was started.
*
*******************************************************************************/
bool ui_nav_prev(void)
{
    bool wrapping = (UI_PAGE_HOME == current_page);
    ui_page_id_t prev;

    if (current_page >= UI_PAGE_FLOW_COUNT)
    {
        /* Leaving the Settings side branch always rejoins the flow at Home. */
        prev = UI_PAGE_HOME;
    }
    else if (wrapping)
    {
        prev = (ui_page_id_t)(UI_PAGE_FLOW_COUNT - 1);
    }
    else
    {
        prev = (ui_page_id_t)(current_page - 1);
    }

    return ui_nav_go_to(prev, wrapping ? UI_NAV_DIR_FORWARD : UI_NAV_DIR_BACK);
}


/*******************************************************************************
* Function Name: ui_nav_current
********************************************************************************
* Summary:
*  Returns the page currently focused.
*
* Parameters:
*  None
*
* Return:
*  ui_page_id_t: Current page id.
*
*******************************************************************************/
ui_page_id_t ui_nav_current(void)
{
    return current_page;
}


/*******************************************************************************
* Function Name: ui_nav_is_busy
********************************************************************************
* Summary:
*  Reports whether a transition animation is running. Navigation requests made
*  meanwhile are dropped, so check this before offering the user a control that
*  must not be ignored.
*
* Parameters:
*  None
*
* Return:
*  bool: true while a transition is in flight.
*
*******************************************************************************/
bool ui_nav_is_busy(void)
{
    return transition_busy;
}

/* [] END OF FILE */

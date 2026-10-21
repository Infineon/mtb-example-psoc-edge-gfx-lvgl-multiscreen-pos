/*******************************************************************************
* File Name        : ui_lifecycle.c
*
* Description      : Screen lifecycle logger. Owns the textual trace format
*                    printed for every page state change.
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

#include "lvgl.h"

#include "ui_lifecycle.h"
#include "ui_config.h"

/*******************************************************************************
* Global Variables
*******************************************************************************/
static const char * const evt_names[] =
{
    [UI_LC_CREATE]   = "CREATE",
    [UI_LC_FOCUS]    = "FOCUS",
    [UI_LC_DEFOCUS]  = "DEFOCUS",
    [UI_LC_UNLOADED] = "UNLOADED",
    [UI_LC_DESTROY]  = "DESTROY",
};

/*******************************************************************************
* Function Name: ui_lifecycle_evt_name
********************************************************************************
* Summary:
*  Maps a lifecycle event to its printable name, such as "FOCUS".
*
* Parameters:
*  evt: Lifecycle event.
*
* Return:
*  const char *: Event name, or "?" for an out-of-range value. Never NULL.
*
*******************************************************************************/
const char * ui_lifecycle_evt_name(ui_lifecycle_evt_t evt)
{
    if ((size_t)evt >= (sizeof(evt_names) / sizeof(evt_names[0])))
    {
        return "?";
    }

    return evt_names[evt];
}


/*******************************************************************************
* Function Name: ui_lifecycle_log
********************************************************************************
* Summary:
*  Prints one lifecycle trace line on the debug UART, for example
*  "[LC] page_home: FOCUS". Compiled out when UI_LOG_LIFECYCLE is 0. Called
*  from the LVGL task only, so plain printf() is safe here; retarget-io is not
*  re-entrant.
*
* Parameters:
*  page_name: Name of the page emitting the event.
*  evt: Lifecycle event.
*
* Return:
*  None
*
*******************************************************************************/
void ui_lifecycle_log(const char * page_name, ui_lifecycle_evt_t evt)
{
#if (UI_LOG_LIFECYCLE)
    printf("[LC] %s: %s\r\n",
           (NULL != page_name) ? page_name : "?",
           ui_lifecycle_evt_name(evt));
#else
    LV_UNUSED(page_name);
    LV_UNUSED(evt);
#endif
}

/* [] END OF FILE */

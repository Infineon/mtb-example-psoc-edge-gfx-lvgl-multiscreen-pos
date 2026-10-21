/*******************************************************************************
* File Name        : ui_lifecycle.h
*
* Description      : Screen lifecycle event definitions and logger.
*
*                    LVGL raises LV_EVENT_SCREEN_LOAD_START / SCREEN_LOADED /
*                    SCREEN_UNLOAD_START / SCREEN_UNLOADED only for real screen
*                    loads. The navigation-host pattern loads a single screen
*                    once and then swaps page containers inside it, so the host
*                    emits the equivalent events itself.
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

#ifndef UI_LIFECYCLE_H
#define UI_LIFECYCLE_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* Data Structures
*******************************************************************************/

/* Lifecycle phases a page passes through. */
typedef enum
{
    UI_LC_CREATE = 0,   /* Widget tree built (lazily, on first navigation). */
    UI_LC_FOCUS,        /* Enter transition finished; page is interactive.  */
    UI_LC_DEFOCUS,      /* Exit transition started; stop timers here.       */
    UI_LC_UNLOADED,     /* Exit transition finished; page is off-screen.    */
    UI_LC_DESTROY,      /* Page root deleted (driven by LV_EVENT_DELETE).   */
} ui_lifecycle_evt_t;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

const char * ui_lifecycle_evt_name(ui_lifecycle_evt_t evt);

void ui_lifecycle_log(const char * page_name, ui_lifecycle_evt_t evt);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UI_LIFECYCLE_H */

/* [] END OF FILE */

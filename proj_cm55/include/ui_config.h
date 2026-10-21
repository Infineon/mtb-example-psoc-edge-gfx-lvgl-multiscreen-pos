/*******************************************************************************
* File Name        : ui_config.h
*
* Description      : Single point of configuration for the multi-screen state
*                    machine code example. Every tunable of the navigation
*                    framework - spring parameters, transition duration, theme,
*                    layout breakpoints and button behaviour - lives here so it
*                    can be adapted without touching the framework sources.
*
* Related Document : See README.md and docs/design_and_implementation.md
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

#ifndef UI_CONFIG_H
#define UI_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* Spring easing
********************************************************************************
* The transition path is a damped harmonic oscillator solved analytically, so
* the parameters below are physical, not curve control points:
*
*   omega_n (natural frequency) = sqrt(STIFFNESS / MASS)      [rad/s]
*   zeta    (damping ratio)     = DAMPING / (2*sqrt(STIFFNESS*MASS))  [-]
*
*   zeta < 1  -> under-damped: overshoots the target and settles back (springy)
*   zeta == 1 -> critically damped: fastest approach without overshoot
*   zeta > 1  -> over-damped: slow, no overshoot
*
* UI_SPRING_DURATION_MS is the wall-clock window the animation is allowed to
* run for. The spring is evaluated in real seconds, so shortening the duration
* truncates the tail of the motion rather than speeding the spring up.
*
* Suggested starting points (STIFFNESS / DAMPING / MASS):
*   snappy    260 / 26 / 1.0  (zeta ~0.81)
*   default   180 / 22 / 1.0  (zeta ~0.82)
*   bouncy    180 / 12 / 1.0  (zeta ~0.45, visible overshoot)
*   no bounce 180 / 27 / 1.0  (zeta ~1.0, critically damped)
*******************************************************************************/
#define UI_SPRING_STIFFNESS             (180.0f)
#define UI_SPRING_DAMPING               (22.0f)
#define UI_SPRING_MASS                  (1.0f)
#define UI_SPRING_DURATION_MS           (450U)

/* Spring overshoot pushes the incoming page past its resting position, which
 * would briefly expose the viewport background at the trailing edge. Setting
 * this to 1 makes the viewport background match the page background so the gap
 * is invisible. Set to 0 to see the effect while tuning the parameters. */
#define UI_SPRING_HIDE_OVERSHOOT_GAP    (1U)

/*******************************************************************************
* Screen lifecycle logging
*******************************************************************************/
/* 1: print "[LC] page_home: FOCUS" style traces over the debug UART. */
#define UI_LOG_LIFECYCLE                (1U)

/*******************************************************************************
* Theme
*******************************************************************************/
/* Mode applied at boot. Toggled at runtime with SW4. */
#define UI_THEME_START_DARK             (0U)

/* Accent and success of the Default scheme's dark palette in ui_theme.c. Both
 * keep >4.5:1 contrast against its background and surface colours.
 * PRIMARY is the accent / call to action; SECONDARY is success /
 * confirmation. */
#define UI_COLOR_PRIMARY_HEX            (0x0A84FFU)
#define UI_COLOR_SECONDARY_HEX          (0x30D158U)

/*******************************************************************************
* Responsive layout breakpoints
********************************************************************************
* The kit supports four panels: 800x480, 1024x600 (two variants) and a square
* 480x480. Never hard-code pixel positions; use these classes instead.
*******************************************************************************/
/* Visible width below UI_BREAKPOINT_COMPACT_PX is compact, and at or above
 * UI_BREAKPOINT_WIDE_PX is wide. Anything in between is regular. */
#define UI_BREAKPOINT_COMPACT_PX        (600)
#define UI_BREAKPOINT_WIDE_PX           (900)

/*******************************************************************************
* Buttons
********************************************************************************
* KIT_PSE84_EVAL_EPC2 exposes SW2 and SW4 on the silkscreen; there is no SW1.
*   SW4 = CYBSP_USER_BTN2 (P8.7) -> toggle dark / light theme
*   SW2 = CYBSP_USER_BTN1 (P8.3) -> advance to the next page
*
* KIT_PSE84_AI has only SW2 (CYBSP_USER_BTN1), so on that target it advances
* the page and the theme is changed from the Settings page instead.
* app_buttons.c compiles the theme button out there.
*
* Falling-edge interrupts wake a FreeRTOS task, which confirms stable press and
* release levels before re-arming each pin. The ISR never calls LVGL; all UI
* work remains in task context where the LVGL mutex can be taken normally.
*******************************************************************************/
#define UI_BUTTON_DEBOUNCE_PERIOD_MS    (20U)
/* Number of consecutive identical samples required to accept a level change. */
#define UI_BUTTON_DEBOUNCE_SAMPLES      (3U)
/* Finite wait required because the ISR-signalled task runs with tickless
 * idle. */
#define UI_BUTTON_IDLE_WAIT_MS          (1000U)
/* In words. Sized for the theme toggle, which reaches into
 * lv_theme_default_init() - the same deep call chain the graphics task budgets
 * configMINIMAL_STACK_SIZE * 16 for. configMINIMAL_STACK_SIZE alone
 * overflows. */
#define UI_BUTTON_TASK_STACK_WORDS      (2048U)
#define UI_BUTTON_TASK_PRIORITY         (1U)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UI_CONFIG_H */

/* [] END OF FILE */

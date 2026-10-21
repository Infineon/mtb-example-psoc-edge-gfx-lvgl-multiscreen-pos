/*******************************************************************************
* File Name        : ui_spring_anim.c
*
* Description      : Analytical damped-spring easing curve for LVGL animations.
*                    Owns: the lv_anim_path_cb_t used by the navigation host.
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
#include <math.h>

#include "ui_spring_anim.h"
#include "ui_config.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* Below this distance from 1.0 the two roots of the over/under-damped
 * solutions become numerically unstable, so the critically damped closed form
 * is used instead. */
#define SPRING_ZETA_EPSILON             (1.0e-3f)

/* Guards against a division by zero if a caller passes a degenerate mass. */
#define SPRING_MIN_MASS                 (1.0e-6f)

/*******************************************************************************
* Global Variables
*******************************************************************************/
static const ui_spring_params_t default_params =
{
    .stiffness = UI_SPRING_STIFFNESS,
    .damping   = UI_SPRING_DAMPING,
    .mass      = UI_SPRING_MASS,
};

/*******************************************************************************
* Function Name: spring_progress
********************************************************************************
* Summary:
*  Evaluates the unit step response of a damped harmonic oscillator, i.e. the
*  normalised progress of a spring released at 0 and settling at 1.
*
*  Solving the ODE  M*x'' + D*x' + K*x = K  with x(0) = 0, x'(0) = 0 gives,
*  with omega_n = sqrt(K/M) and zeta = D / (2*sqrt(K*M)):
*
*    zeta < 1 : x(t) = 1 - e^(-z*wn*t) * (cos(wd*t) + (z*wn/wd)*sin(wd*t))
*    zeta = 1 : x(t) = 1 - e^(-wn*t)   * (1 + wn*t)
*    zeta > 1 : x(t) = 1 - (r1*e^(r2*t) - r2*e^(r1*t)) / (r1 - r2)
*
*  Only the under-damped branch can return a value greater than 1; that is the
*  overshoot which gives the transition its spring feel.
*
* Parameters:
*  p: Spring parameters.
*  t_sec: Elapsed time in seconds since the animation started.
*
* Return:
*  float: Progress, 0.0 at t = 0 and asymptotically 1.0. May exceed 1.0 while
*         the under-damped spring is overshooting.
*
*******************************************************************************/
static float spring_progress(const ui_spring_params_t * p, float t_sec)
{
    float mass  = (p->mass < SPRING_MIN_MASS) ? SPRING_MIN_MASS : p->mass;
    float omega_n = sqrtf(p->stiffness / mass);
    float zeta;
    float progress;

    if ((omega_n <= 0.0f) || (t_sec <= 0.0f))
    {
        return 0.0f;
    }

    zeta = p->damping / (2.0f * sqrtf(p->stiffness * mass));

    if (zeta < (1.0f - SPRING_ZETA_EPSILON))
    {
        /* Under-damped: oscillates towards the target. */
        float omega_d = omega_n * sqrtf(1.0f - (zeta * zeta));
        float envelope = expf(-zeta * omega_n * t_sec);

        progress = 1.0f - (envelope * (cosf(omega_d * t_sec) +
                   (((zeta * omega_n) / omega_d) * sinf(omega_d * t_sec))));
    }
    else if (zeta <= (1.0f + SPRING_ZETA_EPSILON))
    {
        /* Critically damped: fastest non-overshooting approach. */
        float envelope = expf(-omega_n * t_sec);

        progress = 1.0f - (envelope * (1.0f + (omega_n * t_sec)));
    }
    else
    {
        /* Over-damped: two real roots, no oscillation. */
        float root = sqrtf((zeta * zeta) - 1.0f);
        float r1 = -omega_n * (zeta - root);
        float r2 = -omega_n * (zeta + root);

        progress = 1.0f - (((r1 * expf(r2 * t_sec)) -
                            (r2 * expf(r1 * t_sec))) / (r1 - r2));
    }

    return progress;
}


/*******************************************************************************
* Function Name: ui_anim_path_spring
********************************************************************************
* Summary:
*  lv_anim_path_cb_t implementation of a damped spring, registered with
*  lv_anim_set_path_cb() and called by LVGL once per animated frame. The spring
*  parameters come from lv_anim_get_user_data(a) when it is a non-NULL
*  ui_spring_params_t pointer; otherwise the UI_SPRING_* defaults from
*  ui_config.h are used.
*
*  The result is deliberately not clamped to [start_value, end_value], so an
*  under-damped spring can overshoot. LVGL supports this: lv_anim_exec() clamps
*  only the elapsed time and passes the path result through unchanged (the stock
*  lv_anim_path_overshoot relies on the same behaviour).
*
* Parameters:
*  a: Animation being evaluated.
*
* Return:
*  int32_t: Current value for the animated property.
*
*******************************************************************************/
int32_t ui_anim_path_spring(const lv_anim_t * a)
{
    const ui_spring_params_t * params;
    int32_t span = a->end_value - a->start_value;
    float t_sec;
    float progress;

    /* Snap exactly onto the end value once the animation window has elapsed.
     * Without this the spring would stop wherever its tail happened to be. */
    if (a->act_time >= a->duration)
    {
        return a->end_value;
    }

    if (a->act_time <= 0)
    {
        return a->start_value;
    }

    params = (const ui_spring_params_t *)lv_anim_get_user_data(a);
    if (NULL == params)
    {
        params = &default_params;
    }

    t_sec = ((float)a->act_time) / 1000.0f;
    progress = spring_progress(params, t_sec);

    return a->start_value + (int32_t)lroundf(progress * (float)span);
}


/*******************************************************************************
* Function Name: ui_spring_default_params
********************************************************************************
* Summary:
*  Returns the compile-time default spring parameters from ui_config.h.
*
* Parameters:
*  None
*
* Return:
*  const ui_spring_params_t *: Static, read-only default parameter set.
*
*******************************************************************************/
const ui_spring_params_t * ui_spring_default_params(void)
{
    return &default_params;
}


/*******************************************************************************
* Function Name: ui_spring_damping_ratio
********************************************************************************
* Summary:
*  Computes the damping ratio zeta = D / (2 * sqrt(K * M)) for the given
*  parameter set. zeta < 1 overshoots, zeta == 1 is critically damped and
*  zeta > 1 never overshoots.
*
* Parameters:
*  p: Spring parameters. Must not be NULL.
*
* Return:
*  float: Damping ratio.
*
*******************************************************************************/
float ui_spring_damping_ratio(const ui_spring_params_t * p)
{
    float mass = (p->mass < SPRING_MIN_MASS) ? SPRING_MIN_MASS : p->mass;

    return p->damping / (2.0f * sqrtf(p->stiffness * mass));
}

/* [] END OF FILE */

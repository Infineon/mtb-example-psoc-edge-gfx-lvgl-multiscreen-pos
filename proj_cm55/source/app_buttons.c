/*******************************************************************************
* File Name        : app_buttons.c
*
* Description      : Interrupt-assisted debounce handling for the two user
*                    push buttons, running as its own FreeRTOS task.
*
*                    THREAD SAFETY
*                    This task is not the LVGL task, so it must not touch the
*                    LVGL object tree unprotected. LV_USE_OS is set to
*                    LV_OS_FREERTOS in lv_conf.h, which gives us lv_lock() and
*                    lv_unlock() around the same mutex lv_timer_handler() takes.
*                    Every call into the UI below is inside that pair.
*
*                    The pins are configured by the Device Configurator as
*                    pulled-up inputs with falling-edge interrupts. The shared
*                    Port 8 ISR only wakes the task; stable-level debounce and
*                    all UI work remain in task context.
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
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lvgl.h"

#include "app_buttons.h"
#include "ui_config.h"
#include "ui_nav.h"
#include "ui_pages.h"
#include "ui_theme.h"
#include "ui_header.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define BUTTON_TASK_NAME                ("Buttons")
#define BUTTON_INTERRUPT_PRIORITY       (5U)
#define BUTTON_NOTIFY(index)            (1UL << (index))

/* Pulled-up inputs: a pressed button reads 0. */
#define BUTTON_PRESSED_LEVEL            (0U)

#if !defined(USE_KIT_PSE84_AI)
#define BUTTON_COUNT                    (2U)
#define BUTTON_THEME                    (0U)
#define BUTTON_NEXT                     (1U)
#else
/* KIT_PSE84_AI exposes only a single user button (no CYBSP_USER_BTN2), so it
 * drives navigation and the theme is changed from the Settings page instead. */
#define BUTTON_COUNT                    (1U)
#define BUTTON_NEXT                     (0U)
#endif

/*******************************************************************************
* Data Structures
*******************************************************************************/
typedef struct
{
    GPIO_PRT_Type * port;
    uint32_t pin;
    uint32_t match_count;   /* Consecutive samples agreeing with candidate. */
    bool candidate;         /* Level the samples are converging on.         */
    bool stable;            /* Debounced level, true when pressed.          */
} button_state_t;

typedef enum
{
    BUTTON_EVENT_NONE,
    BUTTON_EVENT_PRESSED,
    BUTTON_EVENT_RELEASED
} button_event_t;

/*******************************************************************************
* Global Variables
*******************************************************************************/
static button_state_t buttons[BUTTON_COUNT] =
{
#if !defined(USE_KIT_PSE84_AI)
    /* SW4 */
    [BUTTON_THEME] = { .port = CYBSP_USER_BTN2_PORT,
                       .pin  = CYBSP_USER_BTN2_PIN },
#endif
    /* SW2 */
    [BUTTON_NEXT]  = { .port = CYBSP_USER_BTN1_PORT,
                       .pin  = CYBSP_USER_BTN1_PIN },
};

static TaskHandle_t button_task_handle;

/* PSE84 implements three NVIC priority bits, so logical priority 5 maps to
 * 0xA0. This is syscall-safe with configMAX_SYSCALL_INTERRUPT_PRIORITY=0x20.
 * Tickless idle is enabled, so the notified task uses a finite idle timeout.
 */
static const cy_stc_sysint_t button_irq_config =
{
    .intrSrc = CYBSP_USER_BTN1_IRQ,
    .intrPriority = BUTTON_INTERRUPT_PRIORITY
};

/*******************************************************************************
* Function Name: button_sample
********************************************************************************
* Summary:
*  Samples one button and reports a debounced level change.
*
*  Debouncing is done by counting consecutive identical samples rather than by
*  blocking for a settling delay, so the task never stalls and the two buttons
*  are debounced independently.
*
* Parameters:
*  btn: Button to sample.
*
* Return:
*  button_event_t: debounced press, release, or no level change.
*
*******************************************************************************/
static button_event_t button_sample(button_state_t * btn)
{
    bool level = (BUTTON_PRESSED_LEVEL == Cy_GPIO_Read(btn->port, btn->pin));
    button_event_t event = BUTTON_EVENT_NONE;

    if (level != btn->candidate)
    {
        btn->candidate = level;
        btn->match_count = 0U;
    }
    else if (btn->match_count < UI_BUTTON_DEBOUNCE_SAMPLES)
    {
        btn->match_count++;

        if ((UI_BUTTON_DEBOUNCE_SAMPLES == btn->match_count) &&
            (btn->stable != btn->candidate))
        {
            btn->stable = btn->candidate;
            event = btn->stable ? BUTTON_EVENT_PRESSED : BUTTON_EVENT_RELEASED;
        }
    }

    return event;
}


/*******************************************************************************
* Function Name: button_irq_handler
********************************************************************************
* Summary:
*  Handles falling edges from the shared Port 8 IRQ. An asserted button is
*  masked until its release is debounced by the task, preventing contact bounce
*  from repeatedly entering the ISR. SW2 and SW4 falling edges are the expected
*  trigger sources.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
static void button_irq_handler(void)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    uint32_t notification = 0U;

#if !defined(USE_KIT_PSE84_AI)
    if (0U != Cy_GPIO_GetInterruptStatusMasked(buttons[BUTTON_THEME].port,
                                                buttons[BUTTON_THEME].pin))
    {
        Cy_GPIO_SetInterruptMask(buttons[BUTTON_THEME].port,
                                 buttons[BUTTON_THEME].pin, 0U);
        Cy_GPIO_ClearInterrupt(buttons[BUTTON_THEME].port,
                               buttons[BUTTON_THEME].pin);
        notification |= BUTTON_NOTIFY(BUTTON_THEME);
    }
#endif

    if (0U != Cy_GPIO_GetInterruptStatusMasked(buttons[BUTTON_NEXT].port,
                                                buttons[BUTTON_NEXT].pin))
    {
        Cy_GPIO_SetInterruptMask(buttons[BUTTON_NEXT].port,
                                 buttons[BUTTON_NEXT].pin, 0U);
        Cy_GPIO_ClearInterrupt(buttons[BUTTON_NEXT].port,
                               buttons[BUTTON_NEXT].pin);
        notification |= BUTTON_NOTIFY(BUTTON_NEXT);
    }

    __DSB();
    NVIC_ClearPendingIRQ(button_irq_config.intrSrc);

    if ((0U != notification) && (NULL != button_task_handle))
    {
        (void)xTaskNotifyFromISR(button_task_handle, notification, eSetBits,
                                 &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}


/*******************************************************************************
* Function Name: button_rearm
********************************************************************************
* Summary:
*  Clears stale edge state and re-enables one button after a stable release.
*  The critical section protects the Port 8 interrupt-mask read-modify-write
*  from the shared IRQ handler.
*
* Parameters:
*  btn: Button to re-arm.
*
* Return:
*  None
*
*******************************************************************************/
static void button_rearm(button_state_t * btn)
{
    taskENTER_CRITICAL();
    Cy_GPIO_ClearInterrupt(btn->port, btn->pin);
    __DSB();
    Cy_GPIO_SetInterruptMask(btn->port, btn->pin, 1U);
    taskEXIT_CRITICAL();
}


/*******************************************************************************
* Function Name: button_task
********************************************************************************
* Summary:
*  Waits for button interrupt notifications, samples only while a button is
*  settling, and drives the UI after a stable press is confirmed.
*
* Parameters:
*  arg: Unused.
*
* Return:
*  None
*
*******************************************************************************/
static void button_task(void * arg)
{
    uint32_t active_buttons = 0U;
    uint32_t accepted_presses = 0U;

    (void)arg;

    for (;;)
    {
        uint32_t notification = 0U;
        TickType_t wait_ticks = (0U == active_buttons)
                                ? pdMS_TO_TICKS(UI_BUTTON_IDLE_WAIT_MS)
                                : pdMS_TO_TICKS(UI_BUTTON_DEBOUNCE_PERIOD_MS);

        (void)xTaskNotifyWait(0U, UINT32_MAX, &notification, wait_ticks);
        active_buttons |= notification;

        bool theme_pressed = false;
        bool next_pressed = false;

        for (uint32_t index = 0U; index < BUTTON_COUNT; index++)
        {
            uint32_t button_bit = BUTTON_NOTIFY(index);

            if (0U != (active_buttons & button_bit))
            {
                button_event_t event = button_sample(&buttons[index]);

                if (BUTTON_EVENT_PRESSED == event)
                {
                    accepted_presses |= button_bit;
#if !defined(USE_KIT_PSE84_AI)
                    theme_pressed = (BUTTON_THEME == index);
#endif
                    next_pressed = (BUTTON_NEXT == index);
                }
                else if ((BUTTON_EVENT_RELEASED == event) ||
                         ((0U == (accepted_presses & button_bit)) &&
                          !buttons[index].candidate &&
                          (UI_BUTTON_DEBOUNCE_SAMPLES ==
                           buttons[index].match_count)))
                {
                    button_rearm(&buttons[index]);
                    active_buttons &= ~button_bit;
                    accepted_presses &= ~button_bit;
                }
            }
        }

        if (theme_pressed || next_pressed)
        {
            lv_lock();

            if (theme_pressed)
            {
                ui_theme_toggle();
                ui_header_refresh_theme_icon();
                page_settings_sync_theme_switch();
            }

            if (next_pressed)
            {
                (void)ui_nav_next();
            }

            lv_unlock();
        }
    }
}


/*******************************************************************************
* Function Name: app_buttons_init
********************************************************************************
* Summary:
*  Starts interrupt-assisted button handling: registers the shared button
*  interrupt and creates its debounce task. The task runs outside the LVGL task,
*  so every LVGL call it makes is wrapped in lv_lock()/lv_unlock().
*
* Parameters:
*  None
*
* Return:
*  bool: true if the interrupt was registered and the task was created.
*
*******************************************************************************/
bool app_buttons_init(void)
{
    /* Generated falling-edge masks can leave Port 8 pending after BSP init. */
    Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN);
#if !defined(USE_KIT_PSE84_AI)
    Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN);
#endif
    __DSB();
    NVIC_ClearPendingIRQ(button_irq_config.intrSrc);

    cy_en_sysint_status_t irq_status = Cy_SysInt_Init(&button_irq_config,
                                                       button_irq_handler);

    if (CY_SYSINT_SUCCESS != irq_status)
    {
        return false;
    }

    BaseType_t task_status = xTaskCreate(button_task, BUTTON_TASK_NAME,
                                         UI_BUTTON_TASK_STACK_WORDS, NULL,
                                         UI_BUTTON_TASK_PRIORITY,
                                         &button_task_handle);

    if (pdPASS != task_status)
    {
        button_task_handle = NULL;
        return false;
    }

    uint32_t initially_pressed = 0U;

#if !defined(USE_KIT_PSE84_AI)
    if (BUTTON_PRESSED_LEVEL == Cy_GPIO_Read(buttons[BUTTON_THEME].port,
                                             buttons[BUTTON_THEME].pin))
    {
        Cy_GPIO_SetInterruptMask(buttons[BUTTON_THEME].port,
                                 buttons[BUTTON_THEME].pin, 0U);
        initially_pressed |= BUTTON_NOTIFY(BUTTON_THEME);
    }
#endif

    if (BUTTON_PRESSED_LEVEL == Cy_GPIO_Read(buttons[BUTTON_NEXT].port,
                                             buttons[BUTTON_NEXT].pin))
    {
        Cy_GPIO_SetInterruptMask(buttons[BUTTON_NEXT].port,
                                 buttons[BUTTON_NEXT].pin, 0U);
        initially_pressed |= BUTTON_NOTIFY(BUTTON_NEXT);
    }

    if (0U != initially_pressed)
    {
        (void)xTaskNotify(button_task_handle, initially_pressed, eSetBits);
    }

    NVIC_EnableIRQ(button_irq_config.intrSrc);

    return true;
}

/* [] END OF FILE */

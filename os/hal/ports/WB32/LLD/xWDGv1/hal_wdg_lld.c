/*
    Copyright (C) 2022 Westberry Technology (ChangZhou) Corp., Ltd

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    xWDGv1/hal_wdg_lld.c
 * @brief   WB32 WWDG Driver subsystem low level driver source.
 *
 * @addtogroup WDG
 * @{
 */

#include "hal.h"

#if (HAL_USE_WDG == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if WB32_WDG_USE_WWDG || defined(__DOXYGEN__)
WDGDriver WDGD1;
#endif

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if WB32_WDG_USE_WWDG || defined(__DOXYGEN__)
#if !defined(WB32_WWDG_SUPPRESS_ISR)
/**
 * @brief   WWDG early wakeup interrupt handler.
 * @note    This ISR is called when the counter reaches 0x40.
 *          The application can use this to perform last-minute actions
 *          before a potential reset, or to refresh the counter.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(WB32_WWDG_IRQ_VECTOR) {

  OSAL_IRQ_PROLOGUE();

  /* Clear the early wakeup interrupt flag.*/
  WWDG->SR = 0U;

  OSAL_IRQ_EPILOGUE();
}
#endif /* !defined(WB32_WWDG_SUPPRESS_ISR) */
#endif /* WB32_WDG_USE_WWDG */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level WDG driver initialization.
 *
 * @notapi
 */
void wdg_lld_init(void) {

#if WB32_WDG_USE_WWDG
  WDGD1.state = WDG_STOP;
  WDGD1.wwdg  = WWDG;
#endif
}

/**
 * @brief   Configures and activates the WWDG peripheral.
 *
 * @param[in] wdgp      pointer to the @p WDGDriver object
 *
 * @notapi
 */
void wdg_lld_start(WDGDriver *wdgp) {

#if WB32_WDG_USE_WWDG
  if (&WDGD1 == wdgp) {
    /* Enable WWDG clock.*/
    RCC->APB2ENR |= RCC_APB2ENR_WWDGEN;

    /* Configure the prescaler and window value.
       Note: CFR must be written before CR to set up the window properly.*/
    wdgp->wwdg->CFR = (wdgp->config->wdgtb & WB32_WWDG_CFR_WDGTB_MASK) |
                      WB32_WWDG_CFR_W(wdgp->config->win) |
                      (wdgp->config->ewi ? WB32_WWDG_CFR_EWI : 0U);

    /* Enable interrupt if early wakeup is configured.*/
    if (wdgp->config->ewi) {
      nvicEnableVector(WB32_WWDG_NUMBER, WB32_WDG_WWDG_IRQ_PRIORITY);
    }

    /* Enable WWDG and set initial counter value.
       Setting WDGA starts the watchdog immediately.*/
    wdgp->wwdg->CR = WB32_WWDG_CR_WDGA | WB32_WWDG_CR_T(wdgp->config->cnt);
  }
#endif
}

/**
 * @brief   Deactivates the WWDG peripheral.
 * @note    The WWDG cannot be stopped once started. This function will
 *          trigger an assertion if called while the watchdog is running.
 *          The only way to stop WWDG is through a system reset.
 *
 * @param[in] wdgp      pointer to the @p WDGDriver object
 *
 * @notapi
 */
void wdg_lld_stop(WDGDriver *wdgp) {

  osalDbgAssert(wdgp->state == WDG_STOP,
                "WWDG cannot be stopped once activated");
}

/**
 * @brief   Reloads the WWDG counter.
 * @note    This function must be called within the watchdog window:
 *          - After the counter has decremented to <= window value
 *          - Before the counter reaches 0x3F
 *          Calling outside this window will trigger a reset.
 *
 * @param[in] wdgp      pointer to the @p WDGDriver object
 *
 * @notapi
 */
void wdg_lld_reset(WDGDriver *wdgp) {

#if WB32_WDG_USE_WWDG
  /* Reload the counter with the configured value.
     The WDGA bit must remain set.*/
  wdgp->wwdg->CR = WB32_WWDG_CR_WDGA | WB32_WWDG_CR_T(wdgp->config->cnt);
#else
  (void)wdgp;
#endif
}

#endif /* HAL_USE_WDG == TRUE */

/** @} */

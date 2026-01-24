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
 * @file    xWDGv1/hal_wdg_lld.h
 * @brief   WB32 WWDG Driver subsystem low level driver header.
 * @note    This driver implements the Window Watchdog (WWDG) peripheral.
 *          The WWDG is a 7-bit downcounter that generates a reset when:
 *          - The counter reaches 0x3F (timeout)
 *          - The counter is refreshed when CNT > WIN (window violation)
 *
 * @addtogroup WDG
 * @{
 */

#ifndef HAL_WDG_LLD_H
#define HAL_WDG_LLD_H

#if (HAL_USE_WDG == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    WWDG_CR register definitions
 * @{
 */
#define WB32_WWDG_CR_T_MASK                 (0x7FU << 0)
#define WB32_WWDG_CR_T(n)                   ((n) << 0)
#define WB32_WWDG_CR_WDGA                   (1U << 7)
/** @} */

/**
 * @name    WWDG_CFR register definitions
 * @{
 */
#define WB32_WWDG_CFR_W_MASK                (0x7FU << 0)
#define WB32_WWDG_CFR_W(n)                  ((n) << 0)
#define WB32_WWDG_CFR_WDGTB_MASK            (3U << 7)
#define WB32_WWDG_CFR_WDGTB_DIV1            (0U << 7)
#define WB32_WWDG_CFR_WDGTB_DIV2            (1U << 7)
#define WB32_WWDG_CFR_WDGTB_DIV4            (2U << 7)
#define WB32_WWDG_CFR_WDGTB_DIV8            (3U << 7)
#define WB32_WWDG_CFR_EWI                   (1U << 9)
/** @} */

/**
 * @name    WWDG_SR register definitions
 * @{
 */
#define WB32_WWDG_SR_EWIF                   (1U << 0)
/** @} */

/**
 * @name    WWDG counter limits
 * @note    Counter must be > 0x3F to avoid immediate reset.
 *          Counter must be <= 0x7F (7-bit).
 * @{
 */
#define WB32_WWDG_CNT_MIN                   0x40U
#define WB32_WWDG_CNT_MAX                   0x7FU
#define WB32_WWDG_WIN_MAX                   0x7FU
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   WWDG driver enable switch.
 * @details If set to @p TRUE the support for WWDG is included.
 * @note    The default is @p FALSE.
 */
#if !defined(WB32_WDG_USE_WWDG) || defined(__DOXYGEN__)
#define WB32_WDG_USE_WWDG                   FALSE
#endif

/**
 * @brief   WWDG interrupt priority level setting.
 */
#if !defined(WB32_WDG_WWDG_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define WB32_WDG_WWDG_IRQ_PRIORITY          2
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if WB32_WDG_USE_WWDG && !WB32_HAS_WWDG
#error "WWDG not present in the selected device"
#endif

#if !WB32_WDG_USE_WWDG
#error "WDG driver activated but no WWDG peripheral assigned"
#endif

#if WB32_WDG_USE_WWDG &&                                                    \
    !OSAL_IRQ_IS_VALID_PRIORITY(WB32_WDG_WWDG_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to WWDG"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing a WDG driver.
 */
typedef struct WDGDriver WDGDriver;

/**
 * @brief   Driver configuration structure.
 * @note    The WWDG requires careful timing configuration:
 *          - Timeout = (4096 * 2^wdgtb * (cnt[5:0] + 1)) / PCLK2
 *          - Window value must be less than counter value
 */
typedef struct {
  /**
   * @brief   Counter initial value (0x40 to 0x7F).
   * @note    Higher values give longer timeout.
   *          The CNT[6] bit must be set (value >= 0x40).
   */
  uint8_t     cnt;
  /**
   * @brief   Window value (0x40 to 0x7F).
   * @note    Counter must be <= this value before refresh is allowed.
   *          Set to 0x7F to disable windowing (refresh allowed anytime).
   */
  uint8_t     win;
  /**
   * @brief   Prescaler value.
   * @note    One of WB32_WWDG_CFR_WDGTB_DIVx.
   *          DIV1 = fastest, DIV8 = slowest.
   */
  uint8_t     wdgtb;
  /**
   * @brief   Enable early wakeup interrupt.
   * @note    If TRUE, interrupt fires when counter reaches 0x40.
   */
  bool        ewi;
} WDGConfig;

/**
 * @brief   Structure representing a WDG driver.
 */
struct WDGDriver {
  /**
   * @brief   Driver state.
   */
  wdgstate_t                state;
  /**
   * @brief   Current configuration data.
   */
  const WDGConfig           *config;
  /* End of the mandatory fields.*/
  /**
   * @brief   Pointer to the WWDG registers block.
   */
  WWDG_TypeDef              *wwdg;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if WB32_WDG_USE_WWDG && !defined(__DOXYGEN__)
extern WDGDriver WDGD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void wdg_lld_init(void);
  void wdg_lld_start(WDGDriver *wdgp);
  void wdg_lld_stop(WDGDriver *wdgp);
  void wdg_lld_reset(WDGDriver *wdgp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_WDG == TRUE */

#endif /* HAL_WDG_LLD_H */

/** @} */

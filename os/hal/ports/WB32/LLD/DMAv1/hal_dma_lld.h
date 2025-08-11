/*
    Copyright (C) 2025 Westberry Technology Corp., Ltd

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
 * @file    DMAv1/hal_dma_lld.h
 * @brief   DMA helper driver header.
 *
 * @addtogroup WB32_DMA
 * @{
 */

#ifndef HAL_DMA_LLD_H
#define HAL_DMA_LLD_H

#include "wb32_dma.h"

/*===========================================================================*/
/* Driver constants.                                                           */
/*===========================================================================*/

#define WB32_DMA_MAX_BLOCK_SIZE      511U

/*===========================================================================*/
/* Driver pre-compile time settings.                                          */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                        */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                          */
/*===========================================================================*/

/**
 * @brief   DMA transfer configuration structure.
 */
typedef struct {
    uint32_t                source;       /**< Source address */
    uint32_t                destination;  /**< Destination address */
    uint32_t                size;         /**< Total transfer size in bytes */
    uint32_t                transfer_mode;/**< DMA transfer mode */
    uint32_t                priority;     /**< Channel priority */
    bool                    circular;     /**< Circular mode enable */
    uint32_t                flags;        /**< Channel interrupt flag */
} dam_config_t;

/**
 * @brief   DMA transfer state structure.
 */
typedef struct {
    const wb32_dma_stream_t *stream;     /**< DMA stream */
    dam_config_t            *config;     /**< Transfer configuration */
    uint32_t                transferred; /**< Bytes transferred so far */
    uint32_t                last;        /**< Bytes last transferred */
    void                    *callback;   /**< User callback */
    void                    *param;      /**< User parameter */
    uint8_t                 state;       /**< Current transfer state */
} dma_transfer_t;

/*===========================================================================*/
/* Driver macros.                                                             */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                      */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  bool dma_init(uint32_t id, 
                dma_transfer_t *transfer, 
                dam_config_t *config,
                wb32_dmaisr_t callback,
                void *param);
  bool dma_start(dma_transfer_t *transfer);
  void dma_stop(dma_transfer_t *transfer);
  uint32_t dma_get_transferred(dma_transfer_t *transfer);
  bool dma_is_busy(dma_transfer_t *transfer);
#ifdef __cplusplus
}
#endif

#endif /* HAL_DMA_LLD_H */

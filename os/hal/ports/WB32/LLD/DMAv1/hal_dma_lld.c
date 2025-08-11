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
 * @file    DMAv1/hal_dma_lld.c
 * @brief   DMA helper driver code.
 *
 * @addtogroup WB32_DMA
 * @details DMA sharing helper driver. In the WB32 the DMA streams are a
 *          shared resource, this driver allows to allocate and free DMA
 *          streams at runtime in order to allow all the other device
 *          drivers to coordinate the access to the resource.
 * @note    The DMA ISR handlers are all declared into this module because
 *          sharing, the various device drivers can associate a callback to
 *          ISRs when allocating streams.
 * @{
 */

#include "hal.h"

#if defined(WB32_DMA_REQUIRED) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                   */
/*===========================================================================*/

#define DMA_STATE_READY      0
#define DMA_STATE_BUSY       1
#define DMA_STATE_ERROR      2
#define DMA_STATE_COMPLETE   3

#define aa_min(a, b)  (((a) < (b)) ? (a) : (b))
#define aa_max(a, b)  (((a) > (b)) ? (a) : (b))

#define aa_min3(a, b, c)  (aa_min(a, aa_min(b, c)))

/*===========================================================================*/
/* Driver exported variables.                                                  */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                          */
/*===========================================================================*/

static void dma_transfer_callback(void *p, uint32_t flags) {
    dma_transfer_t *transfer = (dma_transfer_t *)p;

    if (flags & WB32_DMAC_IT_STATE_ERR) {
        transfer->state = DMA_STATE_ERROR;
        if (transfer->callback && (transfer->config->flags & WB32_DMA_CHCFG_TEIE)) {
            ((wb32_dmaisr_t)transfer->callback)(transfer->param, flags);
        }
        return;
    }

    if (flags & WB32_DMAC_IT_STATE_BLOCK) {
        uint32_t remain;

        transfer->transferred += transfer->last;

        remain = aa_max(transfer->config->size, transfer->transferred) - transfer->transferred;

        if ((remain == 0) && transfer->config->circular) {
            transfer->transferred = 0;
            remain                = transfer->config->size;
        }

        if (remain > 0) {
            uint32_t source      = transfer->config->source;
            uint32_t destination = transfer->config->destination;
            switch (transfer->config->transfer_mode & WB32_DMA_CHCFG_DIR_MASK) {
                case WB32_DMA_CHCFG_DIR_M2M: {
                    if (transfer->config->transfer_mode & WB32_DMA_CHCFG_PINC) {
                        source += transfer->transferred;
                    }
                    if (transfer->config->transfer_mode & WB32_DMA_CHCFG_MINC) {
                        destination += transfer->transferred;
                    }
                } break;
                case WB32_DMA_CHCFG_DIR_M2P: {
                    if (transfer->config->transfer_mode & WB32_DMA_CHCFG_MINC) {
                        source += transfer->transferred;
                    }
                    if (transfer->config->transfer_mode & WB32_DMA_CHCFG_PINC) {
                        destination += transfer->transferred;
                    }
                } break;
                case WB32_DMA_CHCFG_DIR_P2M: {
                    if (transfer->config->transfer_mode & WB32_DMA_CHCFG_PINC) {
                        source += transfer->transferred;
                    }
                    if (transfer->config->transfer_mode & WB32_DMA_CHCFG_MINC) {
                        destination += transfer->transferred;
                    }
                } break;
            }

            uint32_t nextSize = aa_min(remain, WB32_DMA_MAX_BLOCK_SIZE);

            transfer->last = nextSize;
            dmaStreamSetSource(transfer->stream, source);
            dmaStreamSetDestination(transfer->stream, destination);
            dmaStreamSetTransactionSize(transfer->stream, nextSize);
            dmaStreamEnable(transfer->stream);
        } else {
            transfer->state = DMA_STATE_COMPLETE;
            if (transfer->callback && (transfer->config->flags & WB32_DMA_CHCFG_TCIE)) {
                ((wb32_dmaisr_t)transfer->callback)(transfer->param, flags);
            }
        }
    }
}

/*===========================================================================*/
/* Driver local functions.                                                    */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                 */
/*===========================================================================*/

/**
 * @brief   Initializes the DMA transfer.
 *
 * @param[in] transfer      pointer to the @p dma_transfer_t structure
 * @param[in] config        pointer to the @p dam_config_t structure
 * @param[in] callback      callback function pointer
 * @param[in] param         parameter to be passed to the callback function
 *
 * @return                  The operation status.
 * @retval true            if the initialization succeeded.
 * @retval false           if the initialization failed.
 *
 * @api
 */
bool dma_init(uint32_t id, 
              dma_transfer_t *transfer, 
              dam_config_t *config,
              wb32_dmaisr_t callback,
              void *param) {
                    
    transfer->stream = dmaStreamAlloc(id, 
                                      config->priority,
                                      dma_transfer_callback, 
                                      transfer);
    
    if (transfer->stream == NULL) {
        return false;
    }

    transfer->config      = (dam_config_t *)config;
    transfer->callback    = callback;
    transfer->param       = param;
    transfer->transferred = 0;
    transfer->last        = 0;
    transfer->state       = DMA_STATE_READY;

    return true;
}

/**
 * @brief   Starts the DMA transfer.
 *
 * @param[in] transfer      pointer to the @p dma_transfer_t structure
 *
 * @return                  The operation status.
 * @retval true            if the transfer start succeeded.
 * @retval false           if the transfer start failed.
 *
 * @api
 */
bool dma_start(dma_transfer_t *transfer) {

    if ((transfer->state != DMA_STATE_READY) && (transfer->state != DMA_STATE_COMPLETE)) {
        return false;
    }

    uint32_t blockSize = aa_min(transfer->config->size, WB32_DMA_MAX_BLOCK_SIZE);

    transfer->config->flags = 0;
    if (transfer->config->transfer_mode & WB32_DMA_CHCFG_TCIE) {
        transfer->config->transfer_mode &= ~(WB32_DMA_CHCFG_TCIE);
        transfer->config->flags |= WB32_DMA_CHCFG_TCIE;
    }

    if (transfer->config->transfer_mode & WB32_DMA_CHCFG_HTIE) {
        transfer->config->transfer_mode &= ~(WB32_DMA_CHCFG_HTIE);
        transfer->config->flags |= WB32_DMA_CHCFG_HTIE;
    }

    if (transfer->config->transfer_mode & WB32_DMA_CHCFG_TEIE) {
        transfer->config->transfer_mode &= ~(WB32_DMA_CHCFG_TEIE);
        transfer->config->flags |= WB32_DMA_CHCFG_TEIE;
    }

    transfer->config->circular = false;
    if (transfer->config->transfer_mode & WB32_DMA_CHCFG_CIRC) {
        transfer->config->transfer_mode &= ~(WB32_DMA_CHCFG_CIRC);
        transfer->config->circular = true;
    }

    transfer->transferred = 0;
    transfer->last        = blockSize;
    dmaStreamSetMode(transfer->stream, transfer->config->transfer_mode | WB32_DMA_CHCFG_HTIE | WB32_DMA_CHCFG_TEIE);
    dmaStreamSetSource(transfer->stream, transfer->config->source);
    dmaStreamSetDestination(transfer->stream, transfer->config->destination);
    dmaStreamSetTransactionSize(transfer->stream, blockSize);

    transfer->state = DMA_STATE_BUSY;
    dmaStreamEnable(transfer->stream);

    return true;
}

/**
 * @brief   Stops the DMA transfer.
 *
 * @param[in] transfer      pointer to the @p dma_transfer_t structure
 *
 * @api
 */
void dma_stop(dma_transfer_t *transfer) {
    dmaStreamDisable(transfer->stream);
    transfer->state = DMA_STATE_READY;
}

/**
 * @brief   Returns the number of bytes transferred so far.
 *
 * @param[in] transfer      pointer to the @p dma_transfer_t structure
 *
 * @return                  The number of bytes transferred.
 *
 * @api
 */
uint32_t dma_get_transferred(dma_transfer_t *transfer) {
    return transfer->transferred;
}

/**
 * @brief   Returns whether the DMA transfer is ongoing.
 *
 * @param[in] transfer      pointer to the @p dma_transfer_t structure
 *
 * @return                  The busy status.
 * @retval true            if the transfer is ongoing.
 * @retval false           if the transfer is not ongoing.
 *
 * @api
 */
bool dma_is_busy(dma_transfer_t *transfer) {
    return transfer->state == DMA_STATE_BUSY;
}

#endif /* HAL_USE_DMA */

/** @} */

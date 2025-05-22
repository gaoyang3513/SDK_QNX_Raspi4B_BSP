
/*
 * $QNXLicenseC:
 * Copyright 2020, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#include <assert.h>
#include "proto.h"

static i2c_status_t bcm2711_check_status(bcm2711_dev_t *dev)
{
    dev->status = in32(dev->regbase + BCM2711_I2C_STATREG_OFF) & STATREG_MASK;

    if (dev->status & STATREG_ERR) {
        bcm2711_i2c_slogf(dev, _SLOG_ERROR, "%s:  NACK",__FUNCTION__);
        return I2C_STATUS_NACK;
    }

    if (dev->status & STATREG_DONE) {
        dev->txrx = BCM2711_I2C_IDLE;
        return I2C_STATUS_DONE;
    }

    return I2C_STATUS_ERROR;
}

static int bcm2711_intr(bcm2711_dev_t *dev)
{
    /* Get status */
    dev->status = in32(dev->regbase + BCM2711_I2C_STATREG_OFF) & STATREG_MASK;

    if (dev->status & (STATREG_ERR | STATREG_CLKT)) {
        disable_interrupt(dev);
        dev->txrx = BCM2711_I2C_IDLE;
        return EIO;
    }
    else if (dev->status & STATREG_DONE) {
        disable_interrupt(dev);

        /* Check if any data left */
        read_data(dev);

        dev->txrx = BCM2711_I2C_IDLE;
        return EOK;
    }
    else if ((dev->txrx == BCM2711_I2C_RX) && (dev->status & STATREG_RXR)) {
        if (dev->status & STATREG_RXD) {
            read_data(dev);
        }
    }
    else if ((dev->txrx == BCM2711_I2C_TX) && (dev->status & STATREG_TXW)) {
        if (dev->status & STATREG_TXD) {
            push_data(dev);
        }
    }

    return EINPROGRESS;
}

i2c_status_t bcm2711_wait_complete(bcm2711_dev_t *dev)
{
    uint64_t    ntime;
    int         interr = EINPROGRESS;

    /* Wait for interrupt to fire */
    ntime = (uint64_t) ( (uint64_t)(10000000)
            * (uint64_t)(10000000)/(uint64_t)dev->speed );

    do {
        TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_INTR, NULL, &ntime, NULL);
        interr = InterruptWait(0, NULL);
        if (interr == ETIMEDOUT) {
            bcm2711_i2c_slogf(dev, _SLOG_ERROR, "%s:  Timeout, total data %d sent data %d",
                                                    __FUNCTION__, dev->tot_len, dev->cur_len);
            return I2C_STATUS_BUSY;
        }

        interr = bcm2711_intr(dev);
        InterruptUnmask(dev->intr, dev->iid);
    } while (interr == EINPROGRESS);

    return bcm2711_check_status(dev);
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/i2c/bcm2711/wait.c $ $Rev: 960373 $")
#endif

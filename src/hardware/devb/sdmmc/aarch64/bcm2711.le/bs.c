/*
 * $QNXLicenseC:
 * Copyright 2020 QNX Software Systems.
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

/* Module Description:  board specific interface */

#include <internal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <hw/inout.h>
#include <sys/mman.h>
#include <drvr/hwinfo.h>
#include <sdhci.h>

static void bcm2711_get_sdmmc_dma_range(sdio_hc_t * const hc)
{
	sdio_hc_cfg_t * const cfg = &hc->cfg;
	unsigned const hwioff = hwi_find_device("bcm2711-sdmmc", 0);
	if (hwioff != HWI_NULL_OFF) {
		hwi_tag * const tag = hwi_tag_find(hwioff, HWI_TAG_NAME_location, NULL);
		if (tag != NULL) {
			cfg->bmstr_xlat = tag->location.base;
			sdio_slogf(_SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 0,
					   "%s: found SDMMC DMA range in syspage %lx", __func__, cfg->bmstr_xlat);
		} else {
			sdio_slogf(_SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 0,
					   "%s: did not find bcm2711-sdmmc location tag", __func__);
		}
	} else {
		sdio_slogf(_SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 0,
				   "%s: did not find bcm2711-sdmmc tag", __func__);
	}
}

static int bcm2711_bs_args(sdio_hc_t *hc, char *options)
{
	char	*value;
	int		opt;
	sdio_hc_cfg_t	*cfg = &hc->cfg;

	static char     *opts[] = {
#define BUSMASTER_BASE		0
	"bmstr_base",
	NULL};

	while (options && *options != '\0') {
		if ((opt = sdio_hc_getsubopt( &options, opts, &value)) == -1) {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 0, "%s: invalid BS options %s", __func__, options);
			return EINVAL;
		}

		switch (opt) {
			case BUSMASTER_BASE:
				if (value == NULL) {
					sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 0, "%s(%d): invalid BS options %s",
						__func__, __LINE__,  options);
					return EINVAL;
				}
				cfg->bmstr_xlat = strtoull(value, NULL, 0);
				break;
			default:
				break;
		}
	}

	return EOK;
}

static int bcm2711_bs_init(sdio_hc_t *hc)
{
	int    status = EOK;
	sdio_hc_cfg_t  *cfg = &hc->cfg;

	bcm2711_get_sdmmc_dma_range(hc);

	if (bcm2711_bs_args(hc, cfg->options)) {
		return EINVAL;
	}

	if(hc->caps & HC_CAP_SLOT_TYPE_EMBEDDED) {
		hc->caps |= HC_CAP_HS200;
	}

	status = sdhci_init(hc);
	if( status != EOK ) {
		return status;
	}

	if(!(hc->caps & HC_CAP_SLOT_TYPE_EMBEDDED)) {
		/* Overwrite some of the capabilities that are set by sdhci_init() */
		hc->caps &= ~HC_CAP_CD_INTR;
		hc->caps &= ~HC_CAP_SV_1_8V;
	}
	return status;
}

sdio_product_t  sdio_fs_products[] = {
	{ .did = SDIO_DEVICE_ID_WILDCARD, .class = 0, .aflags = 0, .name = "bcm2711", .init = bcm2711_bs_init },
};

sdio_vendor_t   sdio_vendors[] = {
	{ .vid = SDIO_VENDOR_ID_WILDCARD, .name = "Broadcom", .chipsets = sdio_fs_products },
    { .vid = 0, .name = NULL, .chipsets = NULL }
};

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/devb/sdmmc/aarch64/bcm2711.le/bs.c $ $Rev: 992620 $")
#endif

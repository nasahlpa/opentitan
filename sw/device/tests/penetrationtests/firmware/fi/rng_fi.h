// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef OPENTITAN_SW_DEVICE_TESTS_PENETRATIONTESTS_FIRMWARE_FI_RNG_FI_H_
#define OPENTITAN_SW_DEVICE_TESTS_PENETRATIONTESTS_FIRMWARE_FI_RNG_FI_H_

#include "sw/device/lib/base/status.h"
#include "sw/device/lib/ujson/ujson.h"

/**
 * rng_fi_edn_bias command handler.
 *
 * Induce a bias into the random data generated by the EDN.
 *
 * Faults are injected during the trigger_high & trigger_low.
 *
 * @param uj An initialized uJSON context.
 * @return OK or error.
 */
status_t handle_rng_fi_edn_bias(ujson_t *uj);

/**
 * rng_fi_edn_resp_ack command handler.
 *
 * Generate random data. Try to inject fault during bus ack to receive the same
 * random data multiple times.
 *
 * Faults are injected during the trigger_high & trigger_low.
 *
 * @param uj An initialized uJSON context.
 * @return OK or error.
 */
status_t handle_rng_fi_edn_resp_ack(ujson_t *uj);

/**
 * Initializes the trigger and configures the device for the EDN FI test.
 *
 * @param uj An initialized uJSON context.
 * @return OK or error.
 */
status_t handle_rng_fi_edn_init(ujson_t *uj);

/**
 * csrng_bias command handler.
 *
 * Generate known data using the CSRNG.
 *
 * Faults are injected during the trigger_high & trigger_low.
 *
 * @param uj An initialized uJSON context.
 * @return OK or error.
 */
status_t handle_rng_fi_csrng_bias(ujson_t *uj);

/**
 * Initializes the trigger and configures the device for the CSRNG FI test.
 *
 * @param uj An initialized uJSON context.
 * @return OK or error.
 */
status_t handle_rng_fi_csrng_init(ujson_t *uj);

/**
 * Rng FI command handler.
 *
 * Command handler for the Rng FI command.
 *
 * @param uj An initialized uJSON context.
 * @return OK or error.
 */
status_t handle_rng_fi(ujson_t *uj);

#endif  // OPENTITAN_SW_DEVICE_TESTS_PENETRATIONTESTS_FIRMWARE_FI_RNG_FI_H_
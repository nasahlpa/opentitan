// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef OPENTITAN_SW_DEVICE_TESTS_PENETRATIONTESTS_FIRMWARE_CRYPTOLIB_FI_H_
#define OPENTITAN_SW_DEVICE_TESTS_PENETRATIONTESTS_FIRMWARE_CRYPTOLIB_FI_H_

#include "sw/device/lib/base/status.h"
#include "sw/device/lib/ujson/ujson.h"

/**
 * The cryptolib fi aes handler.
 * 
 * This command encrypts or decrypts using an AES call accepting multiple padding schemes and modes of operation.
 * 
 * @param uj An initialized uJSON context.
 * @return OK or error.
 */
status_t handle_cryptolib_fi_aes(ujson_t *uj);

/**
 * The cryptolib fi cmac handler.
 * 
 * This command generates and verifies a tag using CMAC.
 * 
 * @param uj An initialized uJSON context.
 * @return OK or error.
 */
status_t handle_cryptolib_fi_cmac(ujson_t *uj);

/**
 * The cryptolib fi gcm handler.
 * 
 * This command generates a ciphertext and a tag which is verified using GCM.
 * 
 * @param uj An initialized uJSON context.
 * @return OK or error.
 */
status_t handle_cryptolib_fi_gcm(ujson_t *uj);

/**
 * Initialize CryptoLib command handler.
 *
 * This command is designed to setup the CryptoLib FI firmware.
 *
 * @param uj An initialized uJSON context.
 * @return OK or error.
 */
status_t handle_cryptolib_fi_init(ujson_t *uj);

/**
 * CryptoLib FI command handler.
 *
 * Command handler for the CryptoLib FI command.
 *
 * @param uj An initialized uJSON context.
 * @return OK or error.
 */
status_t handle_cryptolib_fi(ujson_t *uj);

#endif  // OPENTITAN_SW_DEVICE_TESTS_PENETRATIONTESTS_FIRMWARE_CRYPTOLIB_FI_H_

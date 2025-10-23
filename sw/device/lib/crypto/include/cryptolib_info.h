// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef OPENTITAN_SW_DEVICE_LIB_CRYPTO_INCLUDE_CRYPTOLIB_INFO_H_
#define OPENTITAN_SW_DEVICE_LIB_CRYPTO_INCLUDE_CRYPTOLIB_INFO_H_

#include <stdint.h>

#include "sw/device/lib/base/macros.h"

/**
 * A truncated commit hash from the open-source OpenTitan repo that can be
 * used to reproduce the ROM binary.
 */
typedef struct cryptolib_info_scm_revision {
  /**
   * Least significant word of the truncated commit hash.
   */
  uint32_t scm_revision_low;
  /**
   * Most significant word of the truncated commit hash.
   */
  uint32_t scm_revision_high;
} cryptolib_info_scm_revision_t;

typedef struct cryptolib_info {
  /**
   * Truncated commit hash.
   */
  cryptolib_info_scm_revision_t scm_revision;
  /**
   * Cryptolib info format version.
   */
  uint32_t version;
} cryptolib_info_t;

enum {
  /**
   * Cryptolib info format version 1 value.
   */
  kCryptoLibInfoVersion1 = 0x4efecea6,
};

/**
 * Extern declaration for the `kCryptoLibInfo` instance.
 *
 * The actual definition is in an auto-generated file.
 */
extern const cryptolib_info_t kCryptoLibInfo;

#endif  // OPENTITAN_SW_DEVICE_LIB_CRYPTO_INCLUDE_CRYPTOLIB_INFO_H_

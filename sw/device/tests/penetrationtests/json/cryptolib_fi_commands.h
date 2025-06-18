// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef OPENTITAN_SW_DEVICE_TESTS_PENETRATIONTESTS_JSON_CRYPTOLIB_FI_COMMANDS_H_
#define OPENTITAN_SW_DEVICE_TESTS_PENETRATIONTESTS_JSON_CRYPTOLIB_FI_COMMANDS_H_
#include "sw/device/lib/ujson/ujson_derive.h"
#ifdef __cplusplus
extern "C" {
#endif

#define AES_CMD_MAX_MSG_BYTES 64
#define AES_CMD_MAX_KEY_BYTES 32
#define AES_CMD_MAX_BLOCK_BYTES 16

// clang-format off

#define CRYPTOLIBFI_SUBCOMMAND(_, value) \
    value(_, Aes) \
    value(_, Cmac) \
    value(_, Gcm) \
    value(_, Init)
C_ONLY(UJSON_SERDE_ENUM(CryptoLibFiSubcommand, cryptolib_fi_subcommand_t, CRYPTOLIBFI_SUBCOMMAND));
RUST_ONLY(UJSON_SERDE_ENUM(CryptoLibFiSubcommand, cryptolib_fi_subcommand_t, CRYPTOLIBFI_SUBCOMMAND, RUST_DEFAULT_DERIVE, strum::EnumString));

#define CRYPTOLIBFI_AES_IN(field, string) \
    field(plaintext, uint8_t, AES_CMD_MAX_MSG_BYTES) \
    field(plaintext_len, uint32_t) \
    field(key, uint8_t, AES_CMD_MAX_KEY_BYTES) \
    field(key_len, uint32_t) \
    field(iv, uint8_t, AES_CMD_MAX_BLOCK_BYTES) \
    field(padding_mode, uint32_t) \
    field(mode_of_op, uint32_t) \
    field(encrypt, bool) \
    field(trigger, uint32_t)
UJSON_SERDE_STRUCT(CryptoLibFiAesIn, cryptolibfi_aes_in_t, CRYPTOLIBFI_AES_IN);

#define CRYPTOLIBFI_AES_OUT(field, string) \
    field(ciphertext, uint8_t, AES_CMD_MAX_MSG_BYTES) \
    field(ciphertext_len, uint32_t)
UJSON_SERDE_STRUCT(CryptoLibFiAesOut, cryptolibfi_aes_out_t, CRYPTOLIBFI_AES_OUT);

#define CRYPTOLIBFI_CMAC_IN(field, string) \
    field(plaintext, uint8_t, AES_CMD_MAX_MSG_BYTES) \
    field(plaintext_len, uint32_t) \
    field(key, uint8_t, AES_CMD_MAX_KEY_BYTES) \
    field(key_len, uint32_t) \
    field(iv, uint8_t, AES_CMD_MAX_BLOCK_BYTES) \
    field(trigger, uint32_t)
UJSON_SERDE_STRUCT(CryptoLibFiCmacIn, cryptolibfi_cmac_in_t, CRYPTOLIBFI_CMAC_IN);

#define CRYPTOLIBFI_CMAC_OUT(field, string) \
    field(mac, uint8_t, AES_CMD_MAX_MSG_BYTES) \
    field(mac_len, uint32_t)
UJSON_SERDE_STRUCT(CryptoLibFiCmacOut, cryptolibfi_cmac_out_t, CRYPTOLIBFI_CMAC_OUT);

#define CRYPTOLIBFI_GCM_IN(field, string) \
    field(plaintext, uint8_t, AES_CMD_MAX_MSG_BYTES) \
    field(plaintext_len, uint32_t) \
    field(aad, ) \
    field(aad_len, uint32_t) \
    field(key, uint8_t, AES_CMD_MAX_KEY_BYTES) \
    field(key_len, uint32_t) \
    field(iv, uint8_t, AES_CMD_MAX_BLOCK_BYTES) \
    field(trigger, uint32_t)
UJSON_SERDE_STRUCT(CryptoLibFiGcmIn, cryptolibfi_gcm_in_t, CRYPTOLIBFI_GCM_IN);

#define CRYPTOLIBFI_GCM_OUT(field, string) \
    field(ciphertext, uint8_t, AES_CMD_MAX_MSG_BYTES) \
    field(ciphertext_len, uint32_t) \
    field(tag, uint8_t, AES_CMD_MAX_MSG_BYTES) \
    field(tag_len, uint32_t)
UJSON_SERDE_STRUCT(CryptoLibFiGcmOut, cryptolibfi_gcm_out_t, CRYPTOLIBFI_GCM_OUT);

// clang-format on

#ifdef __cplusplus
}
#endif
#endif  // OPENTITAN_SW_DEVICE_TESTS_PENETRATIONTESTS_JSON_CRYPTOLIB_FI_COMMANDS_H_

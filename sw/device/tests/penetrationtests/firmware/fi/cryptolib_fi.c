// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/tests/penetrationtests/firmware/fi/cryptolib_fi.h"

#include "sw/device/lib/base/memory.h"
#include "sw/device/lib/base/status.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/ottf_test_config.h"
#include "sw/device/lib/testing/test_framework/ujson_ottf.h"
#include "sw/device/lib/ujson/ujson.h"
#include "sw/device/tests/penetrationtests/firmware/lib/pentest_lib.h"
#include "sw/device/tests/penetrationtests/json/cryptolib_fi_commands.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

status_t handle_cryptolib_fi_init(ujson_t *uj) {
  penetrationtest_cpuctrl_t uj_cpuctrl;
  TRY(ujson_deserialize_penetrationtest_cpuctrl_t(uj, &uj_cpuctrl));

  pentest_select_trigger_type(kPentestTriggerTypeSw);
  // As we are using the software defined trigger, the first argument of
  // pentest_init is not needed. kPentestTriggerSourceAes is selected as a
  // placeholder.
  pentest_init(kPentestTriggerSourceAes,
               kPentestPeripheralIoDiv4 | kPentestPeripheralEdn |
                   kPentestPeripheralCsrng | kPentestPeripheralEntropy |
                   kPentestPeripheralAes | kPentestPeripheralHmac |
                   kPentestPeripheralKmac | kPentestPeripheralOtbn);

  // Configure the CPU for the pentest.
  penetrationtest_device_info_t uj_output;
  TRY(pentest_configure_cpu(
      uj_cpuctrl.icache_disable, uj_cpuctrl.dummy_instr_disable,
      uj_cpuctrl.enable_jittery_clock, uj_cpuctrl.enable_sram_readback,
      &uj_output.clock_jitter_locked, &uj_output.clock_jitter_en,
      &uj_output.sram_main_readback_locked, &uj_output.sram_ret_readback_locked,
      &uj_output.sram_main_readback_en, &uj_output.sram_ret_readback_en));

  // Read device ID and return to host.
  TRY(pentest_read_device_id(uj_output.device_id));
  RESP_OK(ujson_serialize_penetrationtest_device_info_t, uj, &uj_output);

  return OK_STATUS();
}

status_t handle_cryptolib_fi(ujson_t *uj) {
  cryptolib_fi_subcommand_t cmd;
  TRY(ujson_deserialize_cryptolib_fi_subcommand_t(uj, &cmd));
  switch (cmd) {
    case kCryptoLibFiSubcommandInit:
      return handle_cryptolib_fi_init(uj);
    default:
      LOG_ERROR("Unrecognized CryptoLib FI subcommand: %d", cmd);
      return INVALID_ARGUMENT();
  }
  return OK_STATUS();
}

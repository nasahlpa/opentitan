// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/lib/dif/dif_pinmux.h"
#include "sw/device/lib/dif/dif_pwrmgr.h"
#include "sw/device/lib/dif/dif_rv_plic.h"
#include "sw/device/lib/dif/dif_sysrst_ctrl.h"
#include "sw/device/lib/runtime/ibex.h"
#include "sw/device/lib/runtime/irq.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/aon_timer_testutils.h"
#include "sw/device/lib/testing/pwrmgr_testutils.h"
#include "sw/device/lib/testing/rstmgr_testutils.h"
#include "sw/device/lib/testing/rv_plic_testutils.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/testing/test_framework/ottf_utils.h"
#include "sw/device/lib/testing/test_framework/status.h"

#include "pwrmgr_regs.h"

/*
 * RV_DM access after wakeup test.
 */

OTTF_DEFINE_TEST_CONFIG(.enable_uart_flow_control = true);

enum {
  kSoftwareBarrierTimeoutUsec = 1000000,
  kPlicTarget = 0,
};

// This variable will be updated by the DV environment.
OT_SECTION(".rodata")
static volatile const uint8_t kSoftwareBarrierDv = 0;

// This variable will be updated by the test host.
OT_SECTION(".data")
static volatile const uint8_t kSoftwareBarrierHost = 0;

static_assert(kDtPwrmgrCount == 1, "this test expects exactly one pwrmgr");
static const dt_pwrmgr_t kPwrmgrDt = 0;
static_assert(kDtRvPlicCount == 1, "this test expects exactly one rv_plic");
static const dt_rv_plic_t kRvPlicDt = 0;
static_assert(kDtSysrstCtrlCount >= 1,
              "this test expects at least one sysrst_ctrl");
static const dt_sysrst_ctrl_t kSysrstCtrlDt = 0;
static_assert(kDtPinmuxCount == 1, "this test expects exactly one pinmux");
static const dt_pinmux_t kPinmuxDt = 0;

// Handle to the plic
dif_rv_plic_t rv_plic;

/**
 * External interrupt handler.
 *
 * Simply claim the interrupt and does nothing else.
 */
void ottf_external_isr(uint32_t *exc_info) {
  dif_rv_plic_irq_id_t plic_irq_id;
  CHECK_DIF_OK(dif_rv_plic_irq_claim(&rv_plic, kPlicTarget, &plic_irq_id));
}

/**
 * Put the device to sleep.
 *
 * @param pwrmgr A handle to the power manager.
 * @param deep_sleep Whether or not to enter a deep sleep.
 */
static void put_to_sleep(dif_pwrmgr_t *pwrmgr, bool deep_sleep) {
  dif_pwrmgr_domain_config_t cfg;
  CHECK_DIF_OK(dif_pwrmgr_get_domain_config(pwrmgr, &cfg));
  if (deep_sleep) {
    cfg &= ~kDifPwrmgrDomainOptionMainPowerInLowPower;
  } else {
    cfg |= kDifPwrmgrDomainOptionMainPowerInLowPower;
  }

  dif_pwrmgr_request_sources_t wakeup_sources;
  CHECK_DIF_OK(dif_pwrmgr_find_request_source(
      pwrmgr, kDifPwrmgrReqTypeWakeup,
      dt_sysrst_ctrl_instance_id(kSysrstCtrlDt), kDtSysrstCtrlWakeupWkupReq,
      &wakeup_sources));
  CHECK_STATUS_OK(
      pwrmgr_testutils_enable_low_power(pwrmgr, wakeup_sources, cfg));
  LOG_INFO("%s",
           deep_sleep ? "Entering deep sleep." : "Entering normal sleep.");
  wait_for_interrupt();
}

bool test_main(void) {
  // Enable global and external IRQ at Ibex.
  irq_global_ctrl(true);
  irq_external_ctrl(true);

  dif_pinmux_t pinmux;
  dif_pwrmgr_t pwrmgr;
  dif_sysrst_ctrl_t sysrst_ctrl;

  CHECK_DIF_OK(dif_pinmux_init_from_dt(kPinmuxDt, &pinmux));
  CHECK_DIF_OK(dif_pwrmgr_init_from_dt(kPwrmgrDt, &pwrmgr));
  CHECK_DIF_OK(dif_rv_plic_init_from_dt(kRvPlicDt, &rv_plic));
  CHECK_DIF_OK(dif_sysrst_ctrl_init_from_dt(kSysrstCtrlDt, &sysrst_ctrl));

  const volatile uint8_t *const software_barrier = (kDeviceType == kDeviceSimDV)
                                                       ? &kSoftwareBarrierDv
                                                       : &kSoftwareBarrierHost;

  switch (rstmgr_testutils_reason_get()) {
    case kDifRstmgrResetInfoPor:  // The first power-up.
      LOG_INFO("Software Setup.");
      // Wait for sequence to run its checks.
      OTTF_WAIT_FOR(*software_barrier == 1, kSoftwareBarrierTimeoutUsec);

      // Enable all the AON interrupts used in this test.
      dif_rv_plic_irq_id_t plic_id =
          dt_pwrmgr_irq_to_plic_id(kPwrmgrDt, kDtPwrmgrIrqWakeup);
      rv_plic_testutils_irq_range_enable(&rv_plic, kPlicTarget, plic_id,
                                         plic_id);

      // Enable pwrmgr interrupt.
      CHECK_DIF_OK(dif_pwrmgr_irq_set_enabled(&pwrmgr, kDifPwrmgrIrqWakeup,
                                              kDifToggleEnabled));

      // Set up power button as wake up source.
      dif_sysrst_ctrl_input_change_config_t config = {
          .input_changes = kDifSysrstCtrlInputPowerButtonH2L,
          .debounce_time_threshold = 1,  // 5us
      };
      CHECK_DIF_OK(
          dif_sysrst_ctrl_input_change_detect_configure(&sysrst_ctrl, config));
      CHECK_DIF_OK(dif_pinmux_mio_select_input(
          &pinmux,
          dt_sysrst_ctrl_periph_io(kSysrstCtrlDt, kDtSysrstCtrlPeriphIoPwrbIn),
          kDtPadIor13));

      // Put the device in a normal sleep.
      put_to_sleep(&pwrmgr, /*deep_sleep=*/false);
      LOG_INFO("Waking up from normal sleep.");

      // Clean up wakeup source after sleep.
      CHECK_DIF_OK(dif_sysrst_ctrl_ulp_wakeup_clear_status(&sysrst_ctrl));

      // Wait for sequence to run its checks.
      OTTF_WAIT_FOR(*software_barrier == 2, kSoftwareBarrierTimeoutUsec);

      // Put the device in a deep sleep.
      put_to_sleep(&pwrmgr, /*deep_sleep=*/true);
      break;

    case kDifRstmgrResetInfoLowPowerExit:  // The power up after deep sleep.
      LOG_INFO("Waking up from deep sleep.");

      // Wait for sequence to finish before returning.
      OTTF_WAIT_FOR(*software_barrier == 3, kSoftwareBarrierTimeoutUsec);
      return true;

    default:
      LOG_ERROR("Device was reset by an unexpected source.");
      break;
  }
  return false;
}

// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Testbench module for ping timer in alert handler. Intended to use with
// a formal tool.

module ${module_instance_name}_ping_timer_tb import ${module_instance_name}_pkg::*; (
  input                          clk_i,
  input                          rst_ni,
  output logic                   edn_req_o,
  input                          edn_ack_i,
  input        [LfsrWidth-1:0]   edn_data_i,
  input                          en_i,
  input        [NAlerts-1:0]     alert_ping_en_i,
  input        [PING_CNT_DW-1:0] ping_timeout_cyc_i,
  input        [PING_CNT_DW-1:0] wait_cyc_mask_i,
  output logic [NAlerts-1:0]     alert_ping_req_o,
  output logic [N_ESC_SEV-1:0]   esc_ping_req_o,
  input        [NAlerts-1:0]     alert_ping_ok_i,
  input        [N_ESC_SEV-1:0]   esc_ping_ok_i,
  output logic                   alert_ping_fail_o,
  output logic                   esc_ping_fail_o
);

  ${module_instance_name}_ping_timer #(
    // disable max length check in FPV, otherwise this
    // will not converge within acceptable compute time
    .MaxLenSVA  ( 1'b0 )
  ) i_alert_handler_ping_timer (
    .clk_i             ,
    .rst_ni            ,
    .edn_req_o         ,
    .edn_ack_i         ,
    .edn_data_i        ,
    .en_i              ,
    .alert_ping_en_i   ,
    .ping_timeout_cyc_i,
    .wait_cyc_mask_i   ,
    .alert_ping_req_o  ,
    .esc_ping_req_o    ,
    .alert_ping_ok_i   ,
    .esc_ping_ok_i     ,
    .alert_ping_fail_o ,
    .esc_ping_fail_o
  );

endmodule : ${module_instance_name}_ping_timer_tb

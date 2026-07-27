# Copyright lowRISC contributors (OpenTitan project).
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Block-level MeridianCDC constraints for rram_ctrl.
#
# rram_ctrl has two asynchronous clock domains:
#   * main clock  : clk_i     / rst_ni
#   * OTP  clock  : clk_otp_i / rst_otp_ni   (the "OTP plug" crossing)
# Defining them as two independent create_clock domains (with no async
# clock-group grouping) makes MeridianCDC analyze the crossings between them.

set SETUP_CLOCK_UNCERTAINTY 0.5

#####################
# main clock        #
#####################
set MAIN_CLK_PIN clk_i
set MAIN_RST_PIN rst_ni
# overconstrain to 125 MHz
set MAIN_TCK 8.0
create_clock ${MAIN_CLK_PIN} -period ${MAIN_TCK}
set_ideal_network ${MAIN_CLK_PIN}
set_ideal_network ${MAIN_RST_PIN}
set_clock_uncertainty ${SETUP_CLOCK_UNCERTAINTY} ${MAIN_CLK_PIN}

#####################
# OTP clock         #
#####################
set OTP_CLK_PIN clk_otp_i
set OTP_RST_PIN rst_otp_ni
# overconstrain to 125 MHz
set OTP_TCK 8.0
create_clock ${OTP_CLK_PIN} -period ${OTP_TCK}
set_ideal_network ${OTP_CLK_PIN}
set_ideal_network ${OTP_RST_PIN}
set_clock_uncertainty ${SETUP_CLOCK_UNCERTAINTY} ${OTP_CLK_PIN}

#####################
# clock groups      #
#####################
# The main (clk_i) and OTP (clk_otp_i) clocks are generated from independent
# PLLs (sys vs. io_div4) and are mutually asynchronous. At block level there is
# no clock tree for the tool to infer this from, so declare it explicitly -
# otherwise MeridianCDC treats the two domains as synchronous, skips the
# clk_i <-> clk_otp_i crossing analysis, and reports the real OTP synchronizers
# as redundant (W_REDUNDANT_SYNC).
set_clock_groups -name grp_main_otp_async -async \
  -group [get_clocks ${MAIN_CLK_PIN}] \
  -group [get_clocks ${OTP_CLK_PIN}]

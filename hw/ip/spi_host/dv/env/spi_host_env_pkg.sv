// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

package spi_host_env_pkg;
  // dep packages
  import uvm_pkg::*;
  import top_pkg::*;
  import dv_utils_pkg::*;
  import csr_utils_pkg::*;
  import dv_lib_pkg::*;
  import tl_agent_pkg::*;
  import spi_agent_pkg::*;
  import cip_base_pkg::*;
  import dv_base_reg_pkg::*;
  import spi_host_reg_pkg::*;
  import spi_host_ral_pkg::*;
  import spi_host_env_cfg_pkg::*;

  // Re-export SPI_HOST_NUM_CS from spi_host_env_cfg_pkg to make it available in DV
  export spi_host_env_cfg_pkg::SPI_HOST_NUM_CS;

  // parameters
  parameter uint SPI_HOST_TX_DEPTH       = spi_host_reg_pkg::TxDepth;
  parameter uint SPI_HOST_RX_DEPTH       = spi_host_reg_pkg::RxDepth;
  parameter uint SPI_HOST_CMD_DEPTH      = spi_host_reg_pkg::CmdDepth;
  parameter bit  SPI_HOST_BYTEORDER      = spi_host_reg_pkg::ByteOrder;
  parameter uint SPI_HOST_BLOCK_AW       = spi_host_reg_pkg::BlockAw;
  parameter uint SPI_HOST_TX_FIFO_START  = spi_host_reg_pkg::SPI_HOST_TXDATA_OFFSET;
  parameter uint SPI_HOST_TX_FIFO_END    = (SPI_HOST_TX_FIFO_START - 1) +
                                           spi_host_reg_pkg::SPI_HOST_TXDATA_SIZE;

  parameter uint SPI_HOST_RX_FIFO_START  = spi_host_reg_pkg::SPI_HOST_RXDATA_OFFSET;
  parameter uint SPI_HOST_RX_FIFO_END    = (SPI_HOST_RX_FIFO_START - 1) +
                                           spi_host_reg_pkg::SPI_HOST_RXDATA_SIZE;

  parameter uint SPI_HOST_COMMAND_LEN_SIZE_BITS = 20;
  // macro includes
  `include "uvm_macros.svh"
  `include "dv_macros.svh"

  // types
  typedef enum int {
    SpiHostError     = 0,
    SpiHostEvent     = 1,
    NumSpiHostIntr   = 2
  } spi_host_intr_e;

  typedef enum int {
    TxFifo   = 0,
    RxFifo   = 1,
    AllFifos = 2
  } spi_host_fifo_e;

  typedef enum {
    Command,
    Dummy,
    Data
  } spi_segment_type_e;

  // spi config
  typedef struct {
    // configopts register fields
    rand bit        cpol;
    rand bit        cpha;
    rand bit        fullcyc;
    rand bit [3:0]  csnlead;
    rand bit [3:0]  csntrail;
    rand bit [3:0]  csnidle;
    rand bit [15:0] clkdiv;
  } spi_host_configopts_t;

  typedef struct {
    rand bit spien;
    rand bit output_en;
    rand bit sw_rst;
    // csid register
    rand bit [31:0] csid;
    // control register fields
    rand bit [8:0]  tx_watermark;
    rand bit [6:0]  rx_watermark;
  } spi_host_ctrl_t;

  // spi direction
  typedef enum bit [1:0] {
    None     = 2'b00,
    RxOnly   = 2'b01,
    TxOnly   = 2'b10,
    Bidir    = 2'b11
  } spi_dir_e;

  typedef struct {
    // command register fields
    rand spi_mode_e mode;
    rand spi_dir_e  direction;
    rand bit        csaat;
    rand bit [SPI_HOST_COMMAND_LEN_SIZE_BITS-1:0] len;
  } spi_host_command_t;

  typedef struct packed {
    bit          ready;
    bit          active;
    bit          txfull;
    bit          txempty;
    bit          txstall;
    bit          tx_wm;
    bit          rxfull;
    bit          rxempty;
    bit          rxstall;
    bit          byteorder;
    bit          rsv_0;
    bit          rx_wm;
    bit [19:16]  cmd_qd;
    bit [15:8]   rx_qd;
    bit [7:0]    tx_qd;
  } spi_host_status_t;

  typedef struct packed{
    bit csidinval;
    bit cmdinval;
    bit underflow;
    bit overflow;
    bit cmdbusy;
  } spi_host_error_enable_t;

  typedef struct packed{
    bit accessinval;
    bit csidinval;
    bit cmdinval;
    bit underflow;
    bit overflow;
    bit cmdbusy;
  } spi_host_error_status_t;

  typedef struct packed{
    bit idle;
    bit ready;
    bit txwm;
    bit rxwm;
    bit txempty;
    bit rxfull;
  } spi_host_event_enable_t;

  typedef struct packed{
    bit spi_event;
    bit error;
  } spi_host_intr_state_t;

  typedef struct packed{
    bit spi_event;
    bit error;
  } spi_host_intr_enable_t;

  typedef struct packed{
    bit spi_event;
    bit error;
  } spi_host_intr_test_t;

  // alerts
  parameter uint NUM_ALERTS = 1;
  parameter string LIST_OF_ALERTS[NUM_ALERTS] = {"fatal_fault"};

  // Object shared through UVM events using the UVM pool for finer timeout control.
  // It's used so an UVM event is triggered and depending whether the object has the
  // field `stop` set the timeout continues or stall.
  // This is used in SPI host in situations where there `spien` or `sw_rst` are set, so the timeout
  // shouldn't be counting until the module is re-enabled
  class csr_spinwait_ctrl_object extends uvm_object;
    `uvm_object_utils(csr_spinwait_ctrl_object)
    bit            stop;
    function new(string name = "");
      super.new(name);
    endfunction
  endclass

  // functions

  // Convenience function to detect mask validity for TXFIFO. Actual allowed masks are taken
  // directly from RTL: `spi_host.access_valid`
  function automatic bit is_valid_mask(bit [TL_DBW-1:0] mask, bit txfifo);

    if (txfifo && (mask inside {4'b1000, 4'b0100, 4'b0010, 4'b0001, 4'b1100,
                                4'b0110, 4'b0011,4'b1111})) begin
      return 1;
    end else if (!txfifo && mask!=0) begin
      return 1;
    end
    return 0;
  endfunction // is_valid_mask

  // package sources
  `include "spi_host_seq_cfg.sv"
  `include "spi_host_env_cfg.sv"
  `include "spi_host_env_cov.sv"
  `include "spi_segment_item.sv"
  `include "spi_transaction_item.sv"
  `include "spi_host_virtual_sequencer.sv"
  `include "spi_host_scoreboard.sv"
  `include "spi_host_env.sv"
  `include "spi_host_vseq_list.sv"

endpackage : spi_host_env_pkg

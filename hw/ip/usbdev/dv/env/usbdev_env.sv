// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

class usbdev_env extends cip_base_env #(
  .CFG_T              (usbdev_env_cfg),
  .COV_T              (usbdev_env_cov),
  .VIRTUAL_SEQUENCER_T(usbdev_virtual_sequencer),
  .SCOREBOARD_T       (usbdev_scoreboard)
);
  `uvm_component_utils(usbdev_env)

  usb20_agent m_usb20_agent;

  `uvm_component_new

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    // get clock/resets
    if (!uvm_config_db#(virtual clk_rst_if)::get(this, "", "aon_clk_rst_vif",
        cfg.aon_clk_rst_vif)) begin
      `uvm_fatal(get_full_name(), "failed to get aon_clk_rst_if from uvm_config_db")
    end
    if (!uvm_config_db#(virtual clk_rst_if)::get(this, "", "host_clk_rst_vif",
        cfg.host_clk_rst_vif)) begin
      `uvm_fatal(get_full_name(), "failed to get host_clk_rst_if from uvm_config_db")
    end
    // get usb20_if handle
    if (!uvm_config_db#(virtual usb20_if)::get(this, "", "vif", cfg.usb20_usbdpi_vif)) begin
      `uvm_fatal(`gfn, "failed to get usb20_if handle from uvm_config_db")
    end

    // Use the configured USB speed for the main clock
    cfg.clk_rst_vif.set_freq_khz(cfg.usb_clk_freq_khz);

    // Use a sensible speed for the AON clock
    cfg.aon_clk_rst_vif.set_freq_khz(cfg.aon_clk_freq_khz);

    // Use the selected host clock frequency; this may differ from that of the DUT.
    // The DUT is required to cope with a certain amount of disparity, and adjust itself
    // to match the frequency of the host over time.
    cfg.host_clk_rst_vif.set_freq_khz(cfg.host_clk_freq_khz);

    // Report the clock frequencies.
    `uvm_info(`gfn, $sformatf("usb_clk %dkHz, host_clk %dkHz",
                              cfg.usb_clk_freq_khz, cfg.host_clk_freq_khz), UVM_MEDIUM)

    // create components
    m_usb20_agent = usb20_agent::type_id::create("m_usb20_agent", this);
    uvm_config_db#(usb20_agent_cfg)::set(this, "m_usb20_agent*", "cfg", cfg.m_usb20_agent_cfg);
    cfg.m_usb20_agent_cfg.en_cov = cfg.en_cov;
  endfunction

  function void connect_phase(uvm_phase phase);
    super.connect_phase(phase);
    if (cfg.en_scb) begin
      m_usb20_agent.monitor.req_analysis_port.connect(scoreboard.req_usb20_fifo.analysis_export);
      m_usb20_agent.monitor.rsp_analysis_port.connect(scoreboard.rsp_usb20_fifo.analysis_export);
    end
    if (cfg.is_active && cfg.m_usb20_agent_cfg.is_active) begin
      virtual_sequencer.usb20_sequencer_h = m_usb20_agent.sequencer;
    end
  endfunction

endclass

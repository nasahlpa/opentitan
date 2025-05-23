// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
{
  // Name of the sim cfg - typically same as the name of the DUT.
  name: ${module_instance_name}

  // Top level dut name (sv module).
  dut: ${module_instance_name}

  // Top level testbench name (sv module).
  tb: tb

  // Simulator used to sign off this block
  tool: vcs

  // Fusesoc core file used for building the file list.
  fusesoc_core: ${instance_vlnv(f"lowrisc:dv:{module_instance_name}_sim:0.1")}

  // Testplan hjson file.
  testplan: "{self_dir}/../data/${module_instance_name}_testplan.hjson"

  // Import additional common sim cfg files.
  import_cfgs: [// Project wide common sim cfg file
                "{proj_root}/hw/dv/tools/dvsim/common_sim_cfg.hjson",
                // Common CIP test lists
                "{proj_root}/hw/dv/tools/dvsim/tests/csr_tests.hjson",
                "{proj_root}/hw/dv/tools/dvsim/tests/intr_test.hjson",
                "{proj_root}/hw/dv/tools/dvsim/tests/tl_access_tests.hjson",
                "{proj_root}/hw/dv/tools/dvsim/tests/shadow_reg_errors_tests.hjson",
                "{proj_root}/hw/dv/tools/dvsim/tests/sec_cm_tests.hjson",
                "{proj_root}/hw/dv/tools/dvsim/tests/stress_tests.hjson"]

  // Add additional tops for simulation.
  sim_tops: ["${module_instance_name}_bind",
             "${module_instance_name}_cov_bind",
             "sec_cm_prim_sparse_fsm_flop_bind",
             "sec_cm_prim_count_bind",
             "sec_cm_prim_double_lfsr_bind",
             "sec_cm_prim_onehot_check_bind"]

  // Default iterations for all tests - each test entry can override this.
  reseed: 50

  overrides: [
    {
      name: cover_reg_top_vcs_cov_cfg_file
      value: "-cm_hier {proj_root}/hw/top_earlgrey/ip_autogen/${module_instance_name}/dv/cov/${module_instance_name}_cover_reg_top.cfg+{dv_root}/tools/vcs/common_cov_excl.cfg"
    }
  ]

  // Add ALERT_HANDLER specific exclusion files.
  vcs_cov_excl_files: ["{self_dir}/cov/alert_handler_cov_excl.el",
                       "{self_dir}/cov/alert_handler_cov_unr.el"]

  // Default UVM test and seq class name.
  uvm_test: alert_handler_base_test
  uvm_test_seq: alert_handler_base_vseq

  // List of test specifications.
  tests: [
    {
      name: ${module_instance_name}_smoke
      uvm_test_seq: alert_handler_smoke_vseq
    }

    {
      name: ${module_instance_name}_random_alerts
      uvm_test_seq: alert_handler_random_alerts_vseq
    }

    {
      name: ${module_instance_name}_random_classes
      uvm_test_seq: alert_handler_random_classes_vseq
    }

    {
      name: ${module_instance_name}_esc_intr_timeout
      uvm_test_seq: alert_handler_esc_intr_timeout_vseq
    }

    {
      name: ${module_instance_name}_esc_alert_accum
      uvm_test_seq: alert_handler_esc_alert_accum_vseq
    }

    {
      name: ${module_instance_name}_sig_int_fail
      uvm_test_seq: alert_handler_sig_int_fail_vseq
    }

    {
      name: ${module_instance_name}_entropy
      uvm_test_seq: alert_handler_entropy_vseq
      run_opts: ["+test_timeout_ns=1_000_000_000"]
    }

    {
      name: ${module_instance_name}_ping_timeout
      uvm_test_seq: alert_handler_ping_timeout_vseq
      run_opts: ["+test_timeout_ns=1_000_000_000"]
    }

    {
      name: ${module_instance_name}_lpg
      uvm_test_seq: alert_handler_lpg_vseq
      run_opts: ["+test_timeout_ns=1_000_000_000"]
    }

    {
      name: ${module_instance_name}_lpg_stub_clk
      uvm_test_seq: alert_handler_lpg_stub_clk_vseq
      run_opts: ["+test_timeout_ns=1_000_000_000"]
    }

    {
      name: ${module_instance_name}_entropy_stress
      uvm_test_seq: alert_handler_entropy_stress_vseq
      // This sequence forces signal `wait_cyc_mask_i` to a much smaller value.
      // So all the timings are not accurate and we need to disable the scb.
      run_opts: ["+en_scb=0"]
      reseed: 20
    }

    {
      name: ${module_instance_name}_stress_all
      run_opts: ["+test_timeout_ns=15_000_000_000"]
    }

    {
      name: ${module_instance_name}_shadow_reg_errors_with_csr_rw
      run_opts: ["+test_timeout_ns=500_000_000"]
      run_timeout_mins: 120
    }

    {
      name: ${module_instance_name}_alert_accum_saturation
      uvm_test_seq: alert_handler_alert_accum_saturation_vseq
      // This is a direct sequence that forces the accum_cnt to a large number, so does not support
      // scb checkings.
      run_opts: ["+en_scb=0"]
      reseed: 20
    }
  ]

  // List of regressions.
  regressions: [
    {
      name: smoke
      tests: ["${module_instance_name}_smoke"]
    }
  ]
}

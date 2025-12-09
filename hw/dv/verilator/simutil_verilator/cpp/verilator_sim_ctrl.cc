// Copyright lowRISC contributors (OpenTitan project).
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "verilator_sim_ctrl.h"

#include <getopt.h>
#include <iostream>
#include <signal.h>
#include <sys/stat.h>
#include <verilated.h>

#include "Vchip_sim_tb___024root.h"

// This is defined by Verilator and passed through the command line
#ifndef VM_TRACE
#define VM_TRACE 0
#endif

/**
 * Get the current simulation time
 *
 * Called by $time in Verilog, converts to double, to match what SystemC does
 */
double sc_time_stamp() { return VerilatorSimCtrl::GetInstance().GetTime(); }

#ifdef VL_USER_STOP
/**
 * A simulation stop was requested, e.g. through $stop() or $error()
 *
 * This function overrides Verilator's default implementation to more gracefully
 * shut down the simulation.
 */
void vl_stop(const char *filename, int linenum, const char *hier) VL_MT_UNSAFE {
  VerilatorSimCtrl::GetInstance().RequestStop(false);
}
#endif

VerilatorSimCtrl &VerilatorSimCtrl::GetInstance() {
  static VerilatorSimCtrl instance;
  return instance;
}

void VerilatorSimCtrl::SetTop(VerilatedToplevel *top, CData *sig_clk,
                              CData *sig_rst, VerilatorSimCtrlFlags flags) {
  top_ = top;
  sig_clk_ = sig_clk;
  sig_rst_ = sig_rst;
  flags_ = flags;
}

std::pair<int, bool> VerilatorSimCtrl::Exec(int argc, char **argv) {
  bool exit_app = false;
  bool good_cmdline = ParseCommandArgs(argc, argv, exit_app);
  if (exit_app) {
    return std::make_pair(good_cmdline ? 0 : 1, false);
  }

  RunSimulation();

  int retcode = WasSimulationSuccessful() ? 0 : 1;
  return std::make_pair(retcode, true);
}

static bool read_ul_arg(unsigned long *arg_val, const char *arg_name,
                        const char *arg_text) {
  assert(arg_val && arg_name && arg_text);

  bool bad_fmt = false;
  bool out_of_range = false;

  // We have a stricter input format that strtoul: no leading space and no
  // leading plus or minus signs (strtoul has magic behaviour if the input
  // starts with a minus sign, but we don't want that). We're using auto base
  // detection, but a valid number will always start with 0-9 (since hex
  // constants start with "0x")
  if (!(('0' <= arg_text[0]) && (arg_text[0] <= '9'))) {
    bad_fmt = true;
  } else {
    char *txt_end;
    errno = 0;
    *arg_val = strtoul(arg_text, &txt_end, 0);

    // If txt_end doesn't point at a \0 then we didn't read the entire
    // argument.
    if (*txt_end) {
      bad_fmt = true;
    } else {
      // If the value was too big to fit in an unsigned long, strtoul sets
      // errno to ERANGE.
      if (errno != 0) {
        assert(errno == ERANGE);
        out_of_range = true;
      }
    }
  }

  if (bad_fmt) {
    std::cerr << "ERROR: Bad format for " << arg_name << " argument: `"
              << arg_text << "' is not an unsigned integer.\n";
    return false;
  }
  if (out_of_range) {
    std::cerr << "ERROR: Bad format for " << arg_name << " argument: `"
              << arg_text << "' is too big.\n";
    return false;
  }

  return true;
}

bool VerilatorSimCtrl::ParseCommandArgs(int argc, char **argv, bool &exit_app) {
  const struct option long_options[] = {
      {"term-after-cycles", required_argument, nullptr, 'c'},
      {"trace", optional_argument, nullptr, 't'},
      {"help", no_argument, nullptr, 'h'},
      {"fi-trigger-window", required_argument, nullptr, 'f'},
      {"faulty-machine", required_argument, nullptr, 'm'},
      {"fi-index", required_argument, nullptr, 'i'},
      {"fi-bit", required_argument, nullptr, 'b'},
      {"fi-dtype", required_argument, nullptr, 'd'},
      {"fi-id", required_argument, nullptr, 'k'},
      {nullptr, no_argument, nullptr, 0}};

  while (1) {
    int c = getopt_long(argc, argv, "-:c:th", long_options, nullptr);
    if (c == -1) {
      break;
    }

    // Disable error reporting by getopt
    opterr = 0;

    switch (c) {
      case 0:
      case 1:
        break;
      case 't':
        if (!tracing_possible_) {
          std::cerr << "ERROR: Tracing has not been enabled at compile time."
                    << std::endl;
          exit_app = true;
          return false;
        }
        if (optarg != nullptr) {
          trace_file_path_.assign(optarg);
        }
        TraceOn();
        break;
      case 'c':
        if (!read_ul_arg(&term_after_cycles_, "term-after-cycles", optarg)) {
          exit_app = true;
          return false;
        }
        break;
      case 'h':
        PrintHelp();
        exit_app = true;
        break;
      case 'f':
        if (!read_ul_arg(&fi_trigger_window_, "fi-trigger-window", optarg)) {
          exit_app = true;
          return false;
        }
        break;
      case 'm':
        if (!read_ul_arg(&faulty_machine_, "faulty-machine", optarg)) {
          exit_app = true;
          return false;
        }
        break;
      case 'i':
        if (!read_ul_arg(&fault_index_, "fi-index", optarg)) {
          exit_app = true;
          return false;
        }
        break;
      case 'b':
        if (!read_ul_arg(&fault_bit_, "fi-bit", optarg)) {
          exit_app = true;
          return false;
        }
        break;
      case 'd':
        if (!read_ul_arg(&fault_dtype_, "fi-dtype", optarg)) {
          exit_app = true;
          return false;
        }
        break;
      case 'k':
        if (!read_ul_arg(&fault_id_, "fi-id", optarg)) {
          exit_app = true;
          return false;
        }
        break;
      case ':':  // missing argument
        std::cerr << "ERROR: Missing argument." << std::endl << std::endl;
        exit_app = true;
        return false;
      case '?':
      default:;
        // Ignore unrecognized options since they might be consumed by
        // Verilator's built-in parsing below.
    }
  }

  // Pass args to verilator
  Verilated::commandArgs(argc, argv);

  // Parse arguments for all registered extensions
  for (auto it = extension_array_.begin(); it != extension_array_.end(); ++it) {
    if (!(*it)->ParseCLIArguments(argc, argv, exit_app)) {
      exit_app = true;
      return false;
      if (exit_app) {
        return true;
      }
    }
  }
  return true;
}

void VerilatorSimCtrl::RunSimulation() {
  RegisterSignalHandler();

  // Print helper message for tracing
  if (TracingPossible()) {
    std::cout << "Tracing can be toggled by sending SIGUSR1 to this process:"
              << std::endl
              << "$ kill -USR1 " << getpid() << std::endl;
  }
  // Call all extension pre-exec methods
  for (auto it = extension_array_.begin(); it != extension_array_.end(); ++it) {
    (*it)->PreExec();
  }
  // Run the simulation
  Run();
  // Call all extension post-exec methods
  for (auto it = extension_array_.begin(); it != extension_array_.end(); ++it) {
    (*it)->PostExec();
  }
  // Print simulation speed info
  PrintStatistics();
  // Print helper message for tracing
  if (TracingEverEnabled()) {
    std::cout << std::endl
              << "You can view the simulation traces by calling" << std::endl
              << "$ gtkwave " << GetTraceFileName() << std::endl;
  }
}

void VerilatorSimCtrl::SetInitialResetDelay(unsigned int cycles) {
  initial_reset_delay_cycles_ = cycles;
}

void VerilatorSimCtrl::SetResetDuration(unsigned int cycles) {
  reset_duration_cycles_ = cycles;
}

void VerilatorSimCtrl::SetTimeout(unsigned int cycles) {
  term_after_cycles_ = cycles;
}

void VerilatorSimCtrl::RequestStop(bool simulation_success) {
  request_stop_ = true;
  simulation_success_ &= simulation_success;
}

void VerilatorSimCtrl::RegisterExtension(SimCtrlExtension *ext) {
  extension_array_.push_back(ext);
}

VerilatorSimCtrl::VerilatorSimCtrl()
    : top_(nullptr),
      time_(0),
#ifdef VM_TRACE_FMT_FST
      trace_file_path_("sim.fst"),
#else
      trace_file_path_("sim.vcd"),
#endif
      tracing_enabled_(false),
      tracing_enabled_changed_(false),
      tracing_ever_enabled_(false),
      tracing_possible_(VM_TRACE),
      initial_reset_delay_cycles_(2),
      reset_duration_cycles_(2),
      request_stop_(false),
      simulation_success_(true),
      tracer_(VerilatedTracer()),
      term_after_cycles_(0),
      fi_trigger_window_(0),
      faulty_machine_(0),
      fault_index_(0),
      fault_bit_(0),
      fault_dtype_(0),
      fault_id_(0) {
}

void VerilatorSimCtrl::RegisterSignalHandler() {
  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = SignalHandler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);
  sigaction(SIGUSR1, &sigIntHandler, NULL);
}

void VerilatorSimCtrl::SignalHandler(int sig) {
  VerilatorSimCtrl &simctrl = VerilatorSimCtrl::GetInstance();

  switch (sig) {
    case SIGINT:
      simctrl.RequestStop(true);
      break;
    case SIGUSR1:
      if (simctrl.TracingEnabled()) {
        simctrl.TraceOff();
      } else {
        simctrl.TraceOn();
      }
      break;
  }
}

void VerilatorSimCtrl::PrintHelp() const {
  std::cout << "Execute a simulation model for " << GetName() << "\n\n";
  if (tracing_possible_) {
    std::cout << "-t|--trace\n"
                 "   --trace=FILE\n"
                 "  Write a trace file from the start\n\n";
  }
  std::cout << "-c|--term-after-cycles=N\n"
               "  Terminate simulation after N cycles. 0 means no timeout.\n\n"
               "-h|--help\n"
               "  Show help\n\n"
               "All arguments are passed to the design and can be used "
               "in the design, e.g. by DPI modules.\n\n";
}

bool VerilatorSimCtrl::TraceOn() {
  bool old_tracing_enabled = tracing_enabled_;

  tracing_enabled_ = tracing_possible_;
  tracing_ever_enabled_ = tracing_enabled_;

  if (old_tracing_enabled != tracing_enabled_) {
    tracing_enabled_changed_ = true;
  }
  return tracing_enabled_;
}

bool VerilatorSimCtrl::TraceOff() {
  if (tracing_enabled_) {
    tracing_enabled_changed_ = true;
  }
  tracing_enabled_ = false;
  return tracing_enabled_;
}

void VerilatorSimCtrl::PrintStatistics() const {
  double speed_hz = time_ / 2 / (GetExecutionTimeMs() / 1000.0);
  double speed_khz = speed_hz / 1000.0;

  std::cout << std::endl
            << "Simulation statistics" << std::endl
            << "=====================" << std::endl
            << "Executed cycles:  " << std::dec << time_ / 2 << std::endl
            << "Wallclock time:   " << GetExecutionTimeMs() / 1000.0 << " s"
            << std::endl
            << "Simulation speed: " << speed_hz << " cycles/s "
            << "(" << speed_khz << " kHz)" << std::endl;

  int trace_size_byte;
  if (tracing_enabled_ && FileSize(GetTraceFileName(), trace_size_byte)) {
    std::cout << "Trace file size:  " << trace_size_byte << " B" << std::endl;
  }
}

std::string VerilatorSimCtrl::GetTraceFileName() const {
  return trace_file_path_;
}

std::vector<CData *> VerilatorSimCtrl::FiTargetSignalsCData() {
  std::vector<CData *> fault_signals_CData = {
  };


  return fault_signals_CData;
}

std::vector<SData *> VerilatorSimCtrl::FiTargetSignalsSData() {
  std::vector<SData *> fault_signals_SData = {
  };


  return fault_signals_SData;
}

std::vector<IData *> VerilatorSimCtrl::FiTargetSignalsIData() {
  std::vector<IData *> fault_signals_IData = {
  };


  return fault_signals_IData;
}

std::vector<QData *> VerilatorSimCtrl::FiTargetSignalsQData() {
  std::vector<QData *> fault_signals_QData = {
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__1__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__2__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__3__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__4__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__5__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__6__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__7__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__8__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__9__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__10__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__11__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__12__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__13__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__14__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__15__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__16__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__17__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__18__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__19__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__20__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__21__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__22__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__23__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__24__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__25__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__31__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__26__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__27__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__28__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__29__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_rf_flops__BRA__30__KET____DOT__rf_reg_q,
    &top_->dut().rootp->chip_sim_tb__DOT__u_dut__DOT__top_earlgrey__DOT__u_rv_core_ibex__DOT__u_core__DOT__gen_regfile_ff__DOT__register_file_i__DOT__g_dummy_r0__DOT__rf_r0_q
  };


  return fault_signals_QData;
}

void VerilatorSimCtrl::FiGmFindTriggerWindow(bool *trigger_detected_, unsigned long cycle_) {
  if (faulty_machine_ == 0) {
    // If we are executing the golden model, find the trigger high and low.
    if (top_->dut().rootp->chip_sim_tb__DOT__cio_gpio_d2p == 1 && *trigger_detected_ == false) {
      *trigger_detected_ = true;
      // Print out the trigger high cycle - we are parsing it in the Python script.
      std::cout << "Trigger high at cycle: " << cycle_ << std::endl;
    } else if (top_->dut().rootp->chip_sim_tb__DOT__cio_gpio_d2p == 0 && *trigger_detected_ == true) {
      *trigger_detected_ = false;
      // Print out the trigger low cycle - we are parsing it in the Python script.
      std::cout << "Trigger low at cycle: " << cycle_ << std::endl;
    }
  }
}

void VerilatorSimCtrl::FiFmSaveState(bool *fm_saved_, unsigned long *trigger_high_cycle_, unsigned long cycle_) {
  if (faulty_machine_ == 1 && *fm_saved_ == false) {
    if (top_->dut().rootp->chip_sim_tb__DOT__cio_gpio_d2p == 1) {
      // Save the state at the trigger high.
      SaveState();
      *fm_saved_ = true;
      *trigger_high_cycle_ = cycle_;
    }
  }
}

bool VerilatorSimCtrl::FiFmRestoreState(bool *fm_restore_state_, unsigned long *cycle_, unsigned long *trigger_delay_, unsigned long *fi_timeout_cycle_) {
  if (faulty_machine_ == 1 && *fm_restore_state_ == true) {
    // Restore the state after the test has completed.
    *fm_restore_state_ = false;
    RestoreState();
    *cycle_ = time_ / 2;
    *fi_timeout_cycle_ = *fi_timeout_cycle_ + (*fi_timeout_cycle_ * 0.2) + *cycle_;
    *trigger_delay_ = *trigger_delay_ + 1;
    if (*trigger_delay_ > fi_trigger_window_) {
      return true;
    }
  }
  return false;
}

void VerilatorSimCtrl::FiFmDetectTestEnd(bool *fm_restore_state_, unsigned long cycle_, unsigned long trigger_high_cycle_, unsigned long *fi_timeout_cycle_, bool *fi_running) {
  if (faulty_machine_ == 1) {
    if (top_->dut().rootp->chip_sim_tb__DOT__cio_gpio_d2p == 2 && *fm_restore_state_ == false) {
      std::cout << "Test end detected." << std::endl;
      *fm_restore_state_ = true;
      *fi_running = false;
      *fi_timeout_cycle_ = cycle_ - trigger_high_cycle_;
    }
  }
}

bool VerilatorSimCtrl::FiFmCheckTimeout(unsigned long *cycle_, unsigned long fi_timeout_cycle_, unsigned long *trigger_delay_, bool *fi_running) {
  if (faulty_machine_ == 1) {
    if (*cycle_ > fi_timeout_cycle_ && *fi_running == true) {
      std::cout << "DUT timed out, restoring state." << std::endl;
      RestoreState();
      *fi_running = false;
      *cycle_ = time_ / 2;
      *trigger_delay_ = *trigger_delay_ + 1;
      if (*trigger_delay_ > fi_trigger_window_) {
        return true;
      }
    }
  }
  return false;
}

void VerilatorSimCtrl::FiInjectFault(unsigned long fi_trigger_delay, unsigned long trigger_high_cycle_, unsigned long cycle_, bool *fi_running, const std::vector<CData *> &fault_signals_cdata, const std::vector<SData *> &fault_signals_sdata, const std::vector<IData *> &fault_signals_idata, const std::vector<QData *> &fault_signals_qdata) {
  if (faulty_machine_ == 1 && time_ % 2 == 0) {
    // Check if the trigger is still high.
    if (top_->dut().rootp->chip_sim_tb__DOT__cio_gpio_d2p == 1) {
      // Check if we have reached the cycle we are aiming for.
      unsigned long target_cycle_ = fi_trigger_delay + trigger_high_cycle_;
      if (cycle_ == target_cycle_) {
        *fi_running = true;
        // Inject a fault at the given cycle and signal.
        std::cout << "Injecting fault into signal (" << fault_dtype_ << "," << fault_index_ << "," << fault_bit_ << ") at cycle: " << cycle_ << std::endl;
        uint64_t mask = 1 << fault_bit_;
        switch(fault_dtype_) {
          case 0:
            *fault_signals_cdata[fault_index_] ^= (uint8_t)mask;
            break;
          case 1:
            *fault_signals_sdata[fault_index_] ^= (uint16_t)mask;
            break;
          case 2:
            *fault_signals_idata[fault_index_] ^= (uint32_t)mask;
            break;
          case 3:
              *fault_signals_qdata[fault_index_] ^= (uint32_t)mask;
            break;
          default:
            std::cout << "Invalid fault signal!" << std::endl;
        }
      }
    }
  }
}

void VerilatorSimCtrl::Run() {
  assert(top_ && "Use SetTop() first.");

  // We always need to enable this as tracing can be enabled at runtime
  if (tracing_possible_) {
    Verilated::traceEverOn(true);
    top_->trace(tracer_, 99, 0);
  }

  // Evaluate all initial blocks, including the DPI setup routines
  top_->eval();

  std::cout << std::endl
            << "Simulation running, end by pressing CTRL-c." << std::endl;

  time_begin_ = std::chrono::steady_clock::now();
  UnsetReset();
  Trace();

  unsigned long start_reset_cycle_ = initial_reset_delay_cycles_;
  unsigned long end_reset_cycle_ = start_reset_cycle_ + reset_duration_cycles_;

  unsigned long trigger_high_cycle_ = 0;
  unsigned long fi_trigger_delay_ = 0;
  unsigned long fi_max_cycle_ = 0;
  unsigned long fi_timeout_cycle_ = 0;
  bool terminate_ = false;
  bool timeout_ = false;

  bool gm_trigger_detected_ = false;
  bool fm_saved_ = false;
  bool fm_restore_state_ = false;
  bool fm_fi_running = false;

  std::vector<CData *> fault_signals_cdata = FiTargetSignalsCData();
  std::vector<SData *> fault_signals_sdata = FiTargetSignalsSData();
  std::vector<IData *> fault_signals_idata = FiTargetSignalsIData();
  std::vector<QData *> fault_signals_qdata = FiTargetSignalsQData();

  bool state_saved = false;

  while (1) {
    unsigned long cycle_ = time_ / 2;

    if (cycle_ == start_reset_cycle_) {
      SetReset();
    } else if (cycle_ == end_reset_cycle_) {
      UnsetReset();
    }

    *sig_clk_ = !*sig_clk_;

    // Call all extension on-clock methods
    if (*sig_clk_) {
      for (auto it = extension_array_.begin(); it != extension_array_.end();
           ++it) {
        (*it)->OnClock(time_);
      }
    }

    // Find the trigger high and low trigger for the FI good machine.
    FiGmFindTriggerWindow(&gm_trigger_detected_, cycle_);

    // Check if the test is finished. If it is finished, restart it using the
    // saved state.
    FiFmDetectTestEnd(&fm_restore_state_, cycle_, trigger_high_cycle_, &fi_timeout_cycle_, &fm_fi_running);

    // Find the trigger high and restore the state for the FI faulty machine.
    terminate_ = FiFmRestoreState(&fm_restore_state_, &cycle_, &fi_trigger_delay_, &fi_timeout_cycle_);

    // Find the trigger high and save the state for the faulty machine when we
    // are executing for the first time.
    FiFmSaveState(&fm_saved_, &trigger_high_cycle_, cycle_);

    // Inject a fault at the given cycle.
    FiInjectFault(fi_trigger_delay_, trigger_high_cycle_, cycle_, &fm_fi_running, fault_signals_cdata, fault_signals_sdata, fault_signals_idata, fault_signals_qdata);

    // If the injected fault caused the CPU to halt, detect this and restore
    // the saved state.
    timeout_ = FiFmCheckTimeout(&cycle_, fi_timeout_cycle_, &fi_trigger_delay_, &fm_fi_running);

    top_->eval();
    time_++;

    Trace();

    if (request_stop_) {
      std::cout << "Received stop request, shutting down simulation."
                << std::endl;
      break;
    }
    if (Verilated::gotFinish()) {
      std::cout << "Received $finish() from Verilog, shutting down simulation."
                << std::endl;
      break;
    }
    if (term_after_cycles_ && (time_ / 2 >= term_after_cycles_)) {
      std::cout << "Simulation timeout of " << term_after_cycles_
                << " cycles reached, shutting down simulation." << std::endl;
      break;
    }

    if (terminate_ || timeout_) {
      std::cout << "Received stop request from FI, shutting down simulation."
                << std::endl;
      break;
    }
  }

  top_->final();
  time_end_ = std::chrono::steady_clock::now();

  if (TracingEverEnabled()) {
    tracer_.close();
  }
}

std::string VerilatorSimCtrl::GetName() const {
  if (top_) {
    return top_->name();
  }
  return "unknown";
}

unsigned int VerilatorSimCtrl::GetExecutionTimeMs() const {
  return std::chrono::duration_cast<std::chrono::milliseconds>(time_end_ -
                                                               time_begin_)
      .count();
}

void VerilatorSimCtrl::SetReset() {
  if (flags_ & ResetPolarityNegative) {
    *sig_rst_ = 0;
  } else {
    *sig_rst_ = 1;
  }
}

void VerilatorSimCtrl::UnsetReset() {
  if (flags_ & ResetPolarityNegative) {
    *sig_rst_ = 1;
  } else {
    *sig_rst_ = 0;
  }
}

bool VerilatorSimCtrl::FileSize(std::string filepath, int &size_byte) const {
  struct stat statbuf;
  if (stat(filepath.data(), &statbuf) != 0) {
    size_byte = 0;
    return false;
  }

  size_byte = statbuf.st_size;
  return true;
}

void VerilatorSimCtrl::Trace() {
  // We cannot output a message when calling TraceOn()/TraceOff() as these
  // functions can be called from a signal handler. Instead we print the message
  // here from the main loop.
  if (tracing_enabled_changed_) {
    if (TracingEnabled()) {
      std::cout << "Tracing enabled." << std::endl;
    } else {
      std::cout << "Tracing disabled." << std::endl;
    }
    tracing_enabled_changed_ = false;
  }

  if (!TracingEnabled()) {
    return;
  }

  if (!tracer_.isOpen()) {
    tracer_.open(GetTraceFileName().c_str());
    std::cout << "Writing simulation traces to " << GetTraceFileName()
              << std::endl;
  }

  tracer_.dump(GetTime());
}

void VerilatorSimCtrl::SaveState() {
    VerilatedSave os;
    std::string file = "fi_sim_state_" + std::to_string(fault_id_) + ".verilate";
    os.open(file);
    os << GetTime();
    os << top_->dut();
}

void VerilatorSimCtrl::RestoreState() {
    VerilatedRestore os;
    std::string file = "fi_sim_state_" + std::to_string(fault_id_) + ".verilate";
    os.open(file);
    os >> time_;
    os >> top_->dut();
}

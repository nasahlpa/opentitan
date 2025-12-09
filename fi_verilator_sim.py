#!/usr/bin/env python3
# Copyright lowRISC contributors (OpenTitan project).
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging as log
import multiprocessing
import json
import subprocess
import sys
import serial
import functools
import random
from dataclasses import dataclass
from pathlib import Path

import time

data_type_mapping = {"CData": 0, "SData": 1, "IData": 2, "QData": 3}

@dataclass
class FaultDescription:
    cycle: int
    fault_dtype: str
    fault_idx: int
    fault_bit: int


@dataclass
class FaultResult:
    no_response: bool
    fault: FaultDescription
    uart_response: dict
    fault_id: int


def find_trigger_window(process: subprocess) -> tuple[int, int]:
    """ Find the trigger window. """
    trigger_high = 0
    trigger_low = 0
    while (line := process.stdout.readline()) != "":
        line = line.rstrip()
        if "Trigger high at cycle" in line:
            # Verilator prints the following line:
            # Trigger high at cycle: %u
            # Where the first argument after: is the cycle
            trigger_high = int(line.split(":")[1])
        if "Trigger low at cycle" in line:
            # Verilator prints the following line:
            # Trigger low at cycle: %u
            # Where the first argument after: is the cycle
            trigger_low = int(line.split(":")[1])
            break
    return trigger_low - trigger_high


def find_uart(process: subprocess) -> str:
    """ Determine the UART interface. """
    uart_port = ""
    while (line := process.stdout.readline()) != "":
        line = line.rstrip()
        # Verilator prints the following line:
        # UART: Created %s for %s. Connect to it with any terminal program, e.g.
        # The first argument after Created is the UART port.
        if "for uart0" in line:
            uart_port = line.split(" ")[2]
            break
    com_interface = serial.Serial(uart_port)
    return com_interface


def dump_uart(interface: serial.Serial) -> None:
    connection_live = True
    while connection_live:
        try:
            print(interface.readline())
        except:
            connection_live = False


def find_test_start(interface: serial.Serial) -> None:
    """ Blocks until the test start has been found. """
    not_found = True
    while not_found:
        line = interface.readline()
        if "Running sw/device/tests/" in str(line):
            not_found = False


def create_fault_list(num_faults: int, trigger_window: int, fault_signal_list: dict) -> dict:
    fault_list = {}
    num_faulty_signals = int(num_faults / trigger_window)
    for fault_id in range(0, num_faulty_signals):
        # We have the following Verilator datatypes:
        # "CData", "SData", "IData", "QData"
        # select one randomly.
        dtypes_available = []
        for type in fault_signal_list:
            dtypes_available.append(type)
        dtype_selected = random.choice(dtypes_available)
        # There are len(fault_signal_list[dtype])
        # signals in the list. Select one randomly.
        idx = random.randrange(0, len(fault_signal_list[dtype_selected]))
        # The signal has n bits. Select one bit for the fault
        # injection.
        bit = random.randrange(0, fault_signal_list[dtype_selected][idx][1] - 1)
        # Assemble the fault description and append it to the list.
        fault = FaultDescription(trigger_window, dtype_selected, idx, bit)
        fault_list[fault_id] = fault

    return fault_list


def parse_fault_signal_list(fault_signal_list_file: Path) -> dict:
    fault_signal_list = {}
    with open(fault_signal_list_file, "r") as file:
        for line in file:
            fault_description = line.replace("\n", "").split(";")
            if fault_description[0] not in fault_signal_list:
                fault_signal_list[fault_description[0]] = []
            fault_signal_list[fault_description[0]].append((fault_description[1], int(fault_description[2])))
    return fault_signal_list


def run_golden_model(cmd: list) -> tuple[int, int]:
    print("Starting golden model...")
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, universal_newlines=True)
    com_interface = find_uart(p)
    find_test_start(com_interface)

    com_interface.write(json.dumps("IbexFi").encode("ascii"))
    com_interface.write(json.dumps("Init").encode("ascii"))

    connection_live = True
    while connection_live:
        line = com_interface.readline()
        if "RESP_OK" in str(line):
            connection_live = False

    com_interface.write(json.dumps("IbexFi").encode("ascii"))
    com_interface.write(json.dumps("CharRegOpLoop").encode("ascii"))
    trigger_window = find_trigger_window(p)
    connection_live = True
    while connection_live:
        line = com_interface.readline()
        if "RESP_OK" in str(line):
            connection_live = False

    print(f"Trigger window length for the FI experiment is: {trigger_window}")

    return trigger_window


def monitor_stdout(process: subprocess) -> list:
    """ Read STDOUT of the faulty model process. """
    fault_info = {}
    test_result = {}
    test_counter = 0
    while (line := process.stdout.readline()) != "":
        # Read STDOUT.
        if "Injecting fault into signal" in line:
            fault_info[test_counter] = line
            print("Fault info detected")
        elif "DUT timed out" in line:
            test_result[test_counter] = 0
            test_counter += 1
            print("Timeout detected")
        elif "Test end detected" in line:
            test_result[test_counter] = 1
            test_counter += 1
            print("test end detected")

    return fault_info, test_result


def monitor_uart(interface: serial.Serial) -> list:
    """ Read the UART response of the faulty model. """
    responses = []
    connection_live = True
    while connection_live:
        try:
            # Read the UART response.
            uart_line = interface.readline()
            if "RESP_OK" in str(uart_line):
                read_line = str(uart_line.decode().strip())
                print(read_line)
                responses.append(read_line.split("RESP_OK:")[1].split(" CRC:")[0])
        except:
            connection_live = False

    return responses


def parse_responses(fault_info: dict, test_result: dict, uart_responses: list, fault_id: int) -> list:
    assert ((len(fault_info) + 1) == len(test_result))
    
    response = []
    # Start at 1 to skip the first response as this is the output without faults.
    uart_responses_it = 1
    for it in range(1, len(test_result)):
        # Parse the properties of the injected fault.
        fault_info_str = fault_info[it].split("(")[1].split(",")
        fault_dtype = fault_info_str[0]
        fault_idx = int(fault_info_str[1])
        fault_bit = int(fault_info_str[2].split(')')[0])
        cycle = int(fault_info_str[2].split(": ")[1])
        fault_description = FaultDescription(cycle, fault_dtype, fault_idx, fault_bit)

        # Default values if a timeout was detected.
        uart_response = ""
        no_response = True

        # No timeout was detected, we should have received an UART response.
        if test_result[it] == 1:
            uart_response = uart_responses[uart_responses_it]
            no_response = False
            uart_responses_it += 1

        fault_result = FaultResult(fault_id = fault_id, fault = fault_description, no_response = no_response, uart_response = uart_response)
        response.append(fault_result)

    assert(len(uart_responses) == uart_responses_it)

    return response


def run_faulty_model(fault_id: int, cmd: list, faults: dict) -> None:
    # Assemble the command line arguments.
    trigger_window = str(faults[fault_id].cycle)
    trigger_window_str = "--fi-trigger-window=" + trigger_window
    cmd.append(trigger_window_str)
    fault_bit = str(faults[fault_id].fault_bit)
    fault_bit_str = "--fi-bit=" + fault_bit
    cmd.append(fault_bit_str)
    fault_idx = str(faults[fault_id].fault_idx)
    fault_idx_str = "--fi-index=" + fault_idx
    cmd.append(fault_idx_str)
    fault_dtype = str(data_type_mapping[faults[fault_id].fault_dtype])
    fault_dtype_str = "--fi-dtype=" + fault_dtype
    cmd.append(fault_dtype_str)
    fault_id_str = "--fi-id=" + str(fault_id)
    cmd.append(fault_id_str)

    # Start the Verilator model.
    p_verilate = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, universal_newlines=True)
    com_interface = find_uart(p_verilate)

    # Wait until we have received the test start output.
    find_test_start(com_interface)

    # Start the test.
    com_interface.write(json.dumps("IbexFi").encode("ascii"))
    com_interface.write(json.dumps("Init").encode("ascii"))

    while True:
        line = com_interface.readline()
        if "RESP_OK" in str(line):
            break

    com_interface.write(json.dumps("IbexFi").encode("ascii"))
    com_interface.write(json.dumps("CharRegOpLoop").encode("ascii"))

    uart_responses = monitor_uart(com_interface)
    fault_info, test_result = monitor_stdout(p_verilate)

    # Parse the responses.
    return_data = {}
    return_data[fault_id] = parse_responses(fault_info, test_result, uart_responses, fault_id)

    return return_data


def execute_verilator():
    subprocess.call()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--multi-processes',
                        '--mp', 
                        required=True,
                        type=int,
                        help='Number of processes used for the FI sim')
    parser.add_argument('--number-faults',
                        '--nf', 
                        required=True,
                        type=int,
                        help='Number of faults to inject')
    parser.add_argument('--fault-list',
                        '--fl', 
                        required=True,
                        type=Path,
                        help='Fault list')

    args = parser.parse_args()
    
    # Create the command to invoke Verilator.
    binary = "tmp_verilator_build/hw/build.verilator_real/lowrisc_dv_top_earlgrey_chip_verilator_sim_0.1/sim-verilator/Vchip_sim_tb"
    cmd_args = ["--meminit=rom0,sw/device/lib/testing/test_rom/test_rom_sim_verilator.39.scr.vmem",
                "--meminit=flash0,sw/device/tests/penetrationtests/fi_ibex_sim_verilator.64.vmem",
                "--meminit=otp,hw/top_earlgrey/data/otp/img_rma.24.vmem"]
    cmd = []
    cmd.append(binary)
    cmd.extend(cmd_args)
    
    # Run the golden model to determine the trigger window.
    #trigger_window = run_golden_model(cmd)
    trigger_window = 4
    
    # Parse the fault signal list.
    fault_signals = parse_fault_signal_list(args.fault_list)

    # Create the fault list.
    fault_list = create_fault_list(args.number_faults, trigger_window, fault_signals)
    print(f"Created {len(fault_signals) * trigger_window} faults in total.")

    # Configure that the next Verilator executions are the faulty machine.
    cmd.append("--faulty-machine=1")

    # Spawn multiple processes for the faulty models.
    p = multiprocessing.Pool(args.multi_processes)
    test_results = p.map(functools.partial(run_faulty_model, cmd=cmd, faults=fault_list), fault_list)
    print(test_results)


if __name__ == '__main__':
    main()

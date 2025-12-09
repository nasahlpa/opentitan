#!/usr/bin/env python3
# Copyright lowRISC contributors (OpenTitan project).
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

import argparse

from pathlib import Path

def parse_header(header: Path, signal_id: str, module_id: str) -> dict:
    signals = {}
    signals["CData"] = []
    signals["SData"] = []
    signals["IData"] = []
    signals["QData"] = []

    with open(header, "r") as f:
        for line in f:
            if module_id in line and signal_id in line:
                line = line.split("/*")
                dtype = line[0].replace(" ", "")
                line = line[1].split(":")
                bits =line[0]
                line = line[1].split(" ")
                name = line[1].replace(";", "").replace("\n", "")
                if dtype in signals:
                    signals[dtype].append((name,bits))
    return signals


def write_fault_list(fault_list: Path, signals: dict) -> None:
    with open(fault_list, "w") as f:
        for key, signals in signals.items():
            if len(signals) > 0:
                for signal in signals:
                    write_str = key + ";" + signal[0] + ";" + signal[1] + "\n"
                    f.write(write_str)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--verilate-header',
                        '-vh', 
                        required=True,
                        type=Path,
                        help='Header file of the Verilate model')
    parser.add_argument('--verilate-cc',
                        '-vc', 
                        required=True,
                        type=Path,
                        help='CC file of the Verilate model')
    parser.add_argument('--output-fault-list',
                        '-ofl', 
                        required=True,
                        type=Path,
                        help='Output fault list file')
    parser.add_argument('--signal-identifier',
                        '-sid', 
                        required=True,
                        type=str,
                        help='Identifier of the signal')
    parser.add_argument('--module-identifier',
                        '-mid', 
                        required=True,
                        type=str,
                        help='Identifier of the module')
    args = parser.parse_args()

    signals = parse_header(args.verilate_header, args.signal_identifier,
                           args.module_identifier)
    write_fault_list(args.output_fault_list, signals)

if __name__ == '__main__':
    main()

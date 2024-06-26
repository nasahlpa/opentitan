#!/usr/bin/env python3
# Copyright lowRISC contributors (OpenTitan project).
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
r"""Parses lint report and dump filtered messages in hjson format.
"""
import argparse
import logging as log
import sys

from pathlib import Path
from LintParser import LintParser


# TODO(#9079): this script will be removed long term once the
# parser has been merged with the Dvsim core code.
def main():
    parser = argparse.ArgumentParser(
        description="""This script parses Verible Lint log and report files from
        a lint run, filters the messages and creates an aggregated result
        .hjson file with lint messages and their severities.

        The script returns nonzero status if any warnings or errors are
        present.
        """)
    parser.add_argument('--repfile',
                        type=lambda p: Path(p).resolve(),
                        default="./verilator.log'",
                        help="""The script searches the log file provided.
                        Defaults to './verilator.log'""")

    parser.add_argument('--outfile',
                        type=lambda p: Path(p).resolve(),
                        default="./results.hjson",
                        help="""Path to the results Hjson file.
                        Defaults to './results.hjson'""")

    args = parser.parse_args()

    # Patterns for lint.log
    parser_args = {
        args.repfile: [
            # If lint warnings have been found, the lint tool will exit
            # with a nonzero status code and fusesoc will always spit out
            # an error like
            #
            #    ERROR: Failed to run ip:core:name:0.1 : Lint failed
            #
            # If we found any other warnings or errors, there's no point in
            # listing this too. BUT we want to make sure we *do* see this
            # error if there are no other errors or warnings, since that
            # shows something has come unstuck. (Probably the lint tool
            # spat out a warning that we don't understand)
            ("fusesoc-error", r"^ERROR: Failed to run .*: Lint failed.*"),
            ("flow_error", r"^(?!ERROR: Failed to run .* Lint failed)ERROR: .*"),
            ("flow_error", r"^.*Error: .*"),
            ("flow_error", r"^E .*"),
            ("flow_error", r"^F .*"),
            ("flow_error", r".*: syntax error, rejected.*"),
            # TODO(https://github.com/olofk/edalize/issues/90):
            # this is a workaround until we actually have native Edalize
            # support for JasperGold and "formal" targets
            # TODO(#10071) remove countermeasure waiver.
            ("flow_warning",
             r"^(?!WARNING: Unknown item formal in section Target)"
             r"(?!WARNING: Countermeasure.*)"
             r"WARNING: .*"),
            ("flow_warning", r"^.*Warning: .* "),
            ("flow_warning", r"^W .*"),
            ("lint_warning", r"^.*\[Style:.*")
        ]
    }

    # Parse logs
    parser = LintParser()
    num_messages = parser.get_results(parser_args)

    # Write out results file
    parser.write_results_as_hjson(args.outfile)

    # return nonzero status if any warnings or errors are present
    # lint infos do not count as failures
    if num_messages['error'] > 0 or num_messages['warning'] > 0:
        log.info("Found %d lint errors and %d lint warnings",
                 num_messages['error'],
                 num_messages['warning'])
        sys.exit(1)

    log.info("Lint logfile parsed succesfully")
    sys.exit(0)


if __name__ == "__main__":
    main()

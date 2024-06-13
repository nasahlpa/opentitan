# Copyright lowRISC contributors (OpenTitan project).
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

package(default_visibility = ["//visibility:public"])

load(
    "//rules:autogen.bzl",
    "autogen_hjson_c_header",
    "autogen_hjson_rust_header",
)

autogen_hjson_c_header(
    name = "flash_ctrl_c_regs",
    srcs = [
        "data/flash_ctrl.hjson",
    ],
    node = "core",
)

autogen_hjson_rust_header(
    name = "flash_ctrl_rust_regs",
    srcs = [
        "data/flash_ctrl.hjson",
    ],
    node = "core",
)

filegroup(
    name = "all_files",
    srcs = glob(["**"]),
)
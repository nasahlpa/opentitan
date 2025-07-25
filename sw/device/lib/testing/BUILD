# Copyright lowRISC contributors (OpenTitan project).
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

load("//rules/opentitan:defs.bzl", "OPENTITAN_CPU", "OPENTITAN_PLATFORM")
load("//hw/top:defs.bzl", "ALL_IP_NAMES", "ALL_TOP_NAMES", "opentitan_if_ip", "opentitan_select_top")
load("//rules/opentitan:util.bzl", "flatten")
load("//rules:doxygen.bzl", "doxygen_gather_cc")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "doc_files",
    srcs = glob(["**/*.md"]) + [
        "//sw/device/lib/testing/test_framework:doc_files",
    ],
)

alias(
    name = "isr_testutils",
    actual = "//sw/device/lib/testing/autogen:isr_testutils",
)

cc_library(
    name = "aes_testutils",
    srcs = ["aes_testutils.c"],
    hdrs = ["aes_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//hw/ip/aes:model",
        "//hw/top:dt",
        "//sw/device/lib/dif:aes",
        "//sw/device/lib/dif:csrng_shared",
        "//sw/device/lib/dif:edn",
        "//sw/device/lib/runtime:ibex",
        "//sw/device/lib/testing:csrng_testutils",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "alert_handler_testutils",
    srcs = ["alert_handler_testutils.c"],
    hdrs = ["alert_handler_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//hw/top:dt",
        "//sw/device/lib/dif:alert_handler",
        "//sw/device/lib/dif:base",
        "//sw/device/lib/dif:edn",
        "//sw/device/lib/dif:rstmgr",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "aon_timer_testutils",
    srcs = ["aon_timer_testutils.c"],
    hdrs = ["aon_timer_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//hw/top:dt",
        "//sw/device/lib/arch:device",
        "//sw/device/lib/base:math",
        "//sw/device/lib/dif:aon_timer",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "binary_blob",
    hdrs = ["binary_blob.h"],
    deps = [
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "clkmgr_testutils",
    srcs = ["clkmgr_testutils.c"],
    hdrs = ["clkmgr_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//sw/device/lib/dif:clkmgr",
        "//sw/device/lib/runtime:ibex",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "csrng_testutils",
    srcs = ["csrng_testutils.c"],
    hdrs = ["csrng_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//sw/device/lib/dif:csrng",
        "//sw/device/lib/testing:rand_testutils",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "edn_testutils",
    srcs = ["edn_testutils.c"],
    hdrs = ["edn_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//sw/device/lib/dif:csrng_shared",
        "//sw/device/lib/dif:edn",
        "//sw/device/lib/testing:rand_testutils",
    ],
)

cc_library(
    name = "entropy_src_testutils",
    srcs = ["entropy_src_testutils.c"],
    hdrs = ["entropy_src_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//sw/device/lib/dif:entropy_src",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "entropy_testutils",
    srcs = ["entropy_testutils.c"],
    hdrs = ["entropy_testutils.h"],
    local_defines = opentitan_if_ip(
        "entropy_src",
        ["HAS_ENTROPY_SRC"],
        [],
    ),
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//hw/top:edn_c_regs",
        "//sw/device/lib/dif:csrng",
        "//sw/device/lib/dif:csrng_shared",
        "//sw/device/lib/dif:edn",
        "//sw/device/lib/testing/test_framework:check",
    ] + opentitan_if_ip(
        "entropy_src",
        ["//sw/device/lib/dif:entropy_src"],
        [],
    ),
)

cc_library(
    name = "flash_ctrl_testutils",
    srcs = ["flash_ctrl_testutils.c"],
    hdrs = ["flash_ctrl_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//hw/top:dt",
        "//hw/top:flash_ctrl_c_regs",
        "//sw/device/lib/base:abs_mmio",
        "//sw/device/lib/base:mmio",
        "//sw/device/lib/dif:flash_ctrl",
        "//sw/device/lib/runtime:hart",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "nv_counter_testutils",
    srcs = ["nv_counter_testutils.c"],
    hdrs = ["nv_counter_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        ":flash_ctrl_testutils",
        "//hw/top:flash_ctrl_c_regs",
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",
        "//sw/device/lib/dif:flash_ctrl",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "hexstr",
    srcs = ["hexstr.c"],
    hdrs = ["hexstr.h"],
    deps = [
        "//sw/device/lib/base:status",
    ],
)

cc_test(
    name = "hexstr_unittest",
    srcs = ["hexstr_unittest.cc"],
    deps = [
        ":hexstr",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "hmac_testutils",
    srcs = ["hmac_testutils.c"],
    hdrs = ["hmac_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//hw/top:hmac_c_regs",
        "//sw/device/lib/base:mmio",
        "//sw/device/lib/dif:hmac",
        "//sw/device/lib/runtime:ibex",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "i2c_testutils",
    srcs = ["i2c_testutils.c"],
    hdrs = ["i2c_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//hw/top:i2c_c_regs",
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",
        "//sw/device/lib/dif:i2c",
        "//sw/device/lib/dif:pinmux",
        "//sw/device/lib/runtime:ibex",
        "//sw/device/lib/testing:pinmux_testutils",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "keymgr_testutils",
    srcs = ["keymgr_testutils.c"],
    hdrs = ["keymgr_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        ":entropy_testutils",
        ":flash_ctrl_testutils",
        ":kmac_testutils",
        ":otp_ctrl_testutils",
        ":rstmgr_testutils",
        "//hw/top:keymgr_c_regs",
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",
        "//sw/device/lib/arch:boot_stage",
        "//sw/device/lib/base:mmio",
        "//sw/device/lib/dif:flash_ctrl",
        "//sw/device/lib/dif:keymgr",
        "//sw/device/lib/dif:otp_ctrl",
        "//sw/device/lib/dif:rstmgr",
        "//sw/device/lib/runtime:ibex",
        "//sw/device/lib/testing/test_framework:check",
        "//sw/device/silicon_creator/lib/base:chip",
        "//sw/device/silicon_creator/lib/drivers:retention_sram",
    ],
)

cc_library(
    name = "kmac_testutils",
    srcs = ["kmac_testutils.c"],
    hdrs = ["kmac_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//sw/device/lib/dif:kmac",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "lc_ctrl_testutils",
    srcs = ["lc_ctrl_testutils.c"],
    hdrs = ["lc_ctrl_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//sw/device/lib/dif:lc_ctrl",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "otbn_testutils",
    srcs = ["otbn_testutils.c"],
    hdrs = ["otbn_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//sw/device/lib/dif:otbn",
        "//sw/device/lib/runtime:ibex",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "otbn_testutils_rsa",
    srcs = ["otbn_testutils_rsa.c"],
    hdrs = ["otbn_testutils_rsa.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        ":otbn_testutils",
        "//sw/device/lib/base:status",
        "//sw/device/lib/dif:otbn",
        "//sw/otbn/crypto:rsa",
    ],
)

cc_library(
    name = "otp_ctrl_testutils",
    srcs = ["otp_ctrl_testutils.c"],
    hdrs = ["otp_ctrl_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//sw/device/lib/dif:otp_ctrl",
        "//sw/device/lib/runtime:ibex",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "pinmux_testutils",
    srcs = ["pinmux_testutils.c"],
    hdrs = ["pinmux_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//hw/top:dt",
        "//sw/device/lib/arch:device",
        "//sw/device/lib/dif:base",
        "//sw/device/lib/dif:gpio",
        "//sw/device/lib/dif:pinmux",
        "//sw/device/lib/runtime:hart",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "profile",
    srcs = ["profile.c"],
    hdrs = ["profile.h"],
    deps = [
        "//sw/device/lib/runtime:ibex",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "pwrmgr_testutils",
    srcs = ["pwrmgr_testutils.c"],
    hdrs = ["pwrmgr_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//sw/device/lib/dif:pwrmgr",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "rand_testutils",
    srcs = ["rand_testutils.c"],
    hdrs = ["rand_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        ":rv_core_ibex_testutils",
        "//hw/top:top_lib",
        "//sw/device/lib/arch:device",
        "//sw/device/lib/base:memory",
        "//sw/device/lib/dif:rv_core_ibex",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "randomness_quality",
    srcs = ["randomness_quality.c"],
    hdrs = ["randomness_quality.h"],
    deps = [
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "rstmgr_testutils",
    srcs = ["rstmgr_testutils.c"],
    hdrs = ["rstmgr_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//sw/device/lib/dif:rstmgr",
        "//sw/device/lib/testing/test_framework:check",
        "//sw/device/silicon_creator/lib/drivers:retention_sram",
    ],
)

cc_library(
    name = "rv_core_ibex_testutils",
    srcs = ["rv_core_ibex_testutils.c"],
    hdrs = ["rv_core_ibex_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//sw/device/lib/dif:rv_core_ibex",
        "//sw/device/lib/runtime:ibex",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "rv_plic_testutils",
    srcs = ["rv_plic_testutils.c"],
    hdrs = ["rv_plic_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        ":rand_testutils",
        "//sw/device/lib/dif:base",
        "//sw/device/lib/dif:rv_plic",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "sensor_ctrl_testutils",
    srcs = ["sensor_ctrl_testutils.c"],
    hdrs = ["sensor_ctrl_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//sw/device/lib/dif:base",
        "//sw/device/lib/dif:sensor_ctrl",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "sram_ctrl_testutils",
    srcs = ["sram_ctrl_testutils.c"],
    hdrs = ["sram_ctrl_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//sw/device/lib/base:mmio",
        "//sw/device/lib/dif:sram_ctrl",
        "//sw/device/lib/runtime:ibex",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "ret_sram_testutils",
    srcs = ["ret_sram_testutils.c"],
    hdrs = ["ret_sram_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//hw/top:top_lib",
        "//sw/device/lib/base:status",
        "//sw/device/lib/testing/test_framework:check",
        "//sw/device/silicon_creator/lib/base:chip",
        "//sw/device/silicon_creator/lib/drivers:retention_sram",
    ],
)

cc_library(
    name = "spi_device_testutils",
    srcs = ["spi_device_testutils.c"],
    hdrs = ["spi_device_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//hw/top:dt",
        "//sw/device/lib/base:status",
        "//sw/device/lib/dif:pinmux",
        "//sw/device/lib/dif:spi_device",
        "//sw/device/lib/testing/json:spi_passthru",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "spi_host_testutils",
    srcs = ["spi_host_testutils.c"],
    hdrs = ["spi_host_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        ":pinmux_testutils",
        "//hw/top:dt",
        "//sw/device/lib/base:status",
        "//sw/device/lib/dif:pinmux",
        "//sw/device/lib/dif:spi_host",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "spi_flash_testutils",
    srcs = [
        "spi_flash_emulator.c",
        "spi_flash_testutils.c",
    ],
    hdrs = [
        "spi_flash_emulator.h",
        "spi_flash_testutils.h",
    ],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        ":spi_device_testutils",
        "//sw/device/lib/base:bitfield",
        "//sw/device/lib/base:macros",
        "//sw/device/lib/base:status",
        "//sw/device/lib/dif:spi_host",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "sysrst_ctrl_testutils",
    srcs = ["sysrst_ctrl_testutils.c"],
    hdrs = ["sysrst_ctrl_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    visibility = ["//sw/device/tests:__pkg__"],
    deps = [
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",
        "//sw/device/lib/dif:pinmux",
        "//sw/device/lib/dif:sysrst_ctrl",
        "//sw/device/lib/runtime:log",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "uart_testutils",
    srcs = ["uart_testutils.c"],
    hdrs = ["uart_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//hw/top:uart_c_regs",
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",
        "//sw/device/lib/dif:pinmux",
        "//sw/device/lib/dif:uart",
        "//sw/device/lib/runtime:ibex",
        "//sw/device/lib/testing:pinmux_testutils",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "usb_testutils",
    srcs = [
        "usb_testutils.c",
        "usb_testutils_controlep.c",
    ],
    hdrs = [
        "usb_testutils.h",
        "usb_testutils_controlep.h",
        "usb_testutils_diags.h",
    ],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//hw/top_earlgrey/sw/autogen:top_earlgrey",
        "//sw/device/lib/dif:usbdev",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "usb_testutils_simpleserial",
    srcs = ["usb_testutils_simpleserial.c"],
    hdrs = ["usb_testutils_simpleserial.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        ":usb_testutils",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "usb_testutils_streams",
    srcs = ["usb_testutils_streams.c"],
    hdrs = ["usb_testutils_streams.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        ":usb_testutils",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "usb_logging",
    srcs = ["usb_logging.c"],
    hdrs = ["usb_logging.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        ":pinmux_testutils",
        ":usb_testutils",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "dma_testutils",
    srcs = ["dma_testutils.c"],
    hdrs = ["dma_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        "//hw/top_darjeeling/sw/autogen:top_darjeeling",
        "//sw/device/lib/dif:dma",
        "//sw/device/lib/dif:pinmux",
        "//sw/device/lib/dif:spi_host",
        "//sw/device/lib/runtime:ibex",
        "//sw/device/lib/testing/test_framework:check",
    ],
)

cc_library(
    name = "keymgr_dpe_testutils",
    srcs = ["keymgr_dpe_testutils.c"],
    hdrs = ["keymgr_dpe_testutils.h"],
    target_compatible_with = [OPENTITAN_CPU],
    deps = [
        ":entropy_testutils",
        ":kmac_testutils",
        ":otp_ctrl_testutils",
        ":rstmgr_testutils",
        "//hw/top:dt",
        "//hw/top:keymgr_dpe_c_regs",
        "//sw/device/lib/arch:boot_stage",
        "//sw/device/lib/base:mmio",
        "//sw/device/lib/dif:keymgr_dpe",
        "//sw/device/lib/dif:otp_ctrl",
        "//sw/device/lib/dif:rstmgr",
        "//sw/device/lib/runtime:ibex",
        "//sw/device/lib/testing/test_framework:check",
        "//sw/device/silicon_creator/lib/base:chip",
    ],
)

# Some testutils are not multitop yet.
TESTUTILS_EXCEPTIONS = [
    "aes",
    "csrng",
    "i2c",
    "uart",
    "rstmgr",
]

# We need to transition to the opentitan platform since
# testutils are not compatible with the host platform.
doxygen_gather_cc(
    name = "doxy_target",
    platform = OPENTITAN_PLATFORM,
    deps = [
        "isr_testutils",
    ] + flatten([
        # Add all compatible register headers
        opentitan_if_ip(
            ip,
            [":{}_testutils".format(ip)],
            [],
        )
        for ip in ALL_IP_NAMES
        if existing_rule("{}_testutils".format(ip)) != None and
           ip not in TESTUTILS_EXCEPTIONS
    ]) + opentitan_select_top(
        {
            "earlgrey": ["{}_testutils".format(ip) for ip in TESTUTILS_EXCEPTIONS],
        },
        [],
    ),
)

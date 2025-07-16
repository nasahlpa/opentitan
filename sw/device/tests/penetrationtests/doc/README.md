# Penetration Testing Framework

The purpose of this framework is to perform side-channel analysis (SCA) and fault injection (FI) attacks on the FPGA as well as on the chip.

![Full hardware setup](pentest_setup.png)

As shown in the block diagram, the pentest framework runs on the target and receives configuration commands by the SCA and FI [ot-sca](https://github.com/lowRISC/ot-sca) framework.

## Usage

To run the penetration tests either on the FPGA or the chip, please follow the instructions in the [ot-sca](https://github.com/lowRISC/ot-sca) repository.

### Building Images

Due to code size memory limitations, the firmware for the chip and the FPGA is split into a SCA, general FI, IBEX FI, and OTBN FI binary. To build these binaries for the chip, run:
```console
cd $REPO_TOP
./bazelisk.sh build \
  --//signing:token=//signing/tokens:cloud_kms_sival \
  //sw/device/tests/penetrationtests/firmware:pen_test_sca_silicon_owner_sival_rom_ext

./bazelisk.sh build \
  --//signing:token=//signing/tokens:cloud_kms_sival \
  //sw/device/tests/penetrationtests/firmware:pen_test_fi_silicon_owner_sival_rom_ext

./bazelisk.sh build \
  --//signing:token=//signing/tokens:cloud_kms_sival \
  //sw/device/tests/penetrationtests/firmware:pen_test_fi_ibex_silicon_owner_sival_rom_ext

./bazelisk.sh build \
  --//signing:token=//signing/tokens:cloud_kms_sival \
  //sw/device/tests/penetrationtests/firmware:pen_test_fi_otbn_silicon_owner_sival_rom_ext

./bazelisk.sh build \
  --//signing:token=//signing/tokens:cloud_kms_sival \
  //sw/device/tests/penetrationtests/firmware:pen_test_cryptolib_sym_fi_silicon_owner_sival_rom_ext

./bazelisk.sh build \
  --//signing:token=//signing/tokens:cloud_kms_sival \
  //sw/device/tests/penetrationtests/firmware:pen_test_cryptolib_asym_fi_silicon_owner_sival_rom_ext
  
./bazelisk.sh build \
  --//signing:token=//signing/tokens:cloud_kms_sival \
  //sw/device/tests/penetrationtests/firmware:pen_test_cryptolib_sym_sca_silicon_owner_sival_rom_ext

./bazelisk.sh build \
  --//signing:token=//signing/tokens:cloud_kms_sival \
  //sw/device/tests/penetrationtests/firmware:pen_test_cryptolib_asym_sca_silicon_owner_sival_rom_ext
```

The binaries are located in `bazel-bin/sw/device/tests/penetrationtests/firmware/`.

## Automated Testing

### Inner tests
To automatically test whether the pentest framework works, an automatic testing framework is provided.
This framework compares the responses of the pentest frameworks with reference testvectors.

Use the following command to automatically test the Ibex FI tests on the CW310 FPGA board:

```console
cd $REPO_TOP
./bazelisk.sh run //sw/device/tests/penetrationtests:fi_ibex_fpga_cw310_sival_rom_ext
```

### Testing the ot-sca host scripts
The [ot-sca](https://github.com/lowRISC/ot-sca) repo provides scripts to interact with the penetration test images on the chip or fpga.  
In order to test these scripts and to make use of the Bazel targets in ot-sca, add the root of the ot-sca repo by running 
```console
export OTSCA_EXTS_DIR=$(path/to/ot-sca/)
```
and placing the above in your ~/.bashrc file or an equivalent.

A test can be run via the following command:

```console
cd $REPO_TOP
./bazelisk.sh run @otsca_exts//test/penetrationtests/fi:fi_ibex_test --define=FW_ENV=fpga_cw310_sival_rom_ext
```

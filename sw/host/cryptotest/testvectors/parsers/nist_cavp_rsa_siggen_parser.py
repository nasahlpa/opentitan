#!/usr/bin/env python3
# Copyright lowRISC contributors (OpenTitan project).
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

"""Parser for NIST CAVP 186-3 RSA SigGen test vectors.

Reads SigGen15_186-3.txt (PKCS#1 v1.5) and SigGenPSS_186-3.txt (PSS) from
the nist_cavp_rsa_fips_186_3 Bazel external dependency and writes two ACVP-
format output files understood by acvp/src/rsa.rs:

  --vectors-dst  Test inputs. Groups carry n, d, sig_type. Test cases carry
                 msg and, for PKCS#1 v1.5, the NIST reference signature (sig).
  --expected-dst Expected outcomes. Every test case has testPassed=true.

PKCS#1 v1.5 (SigGen15_186-3.txt):
  The .txt files include n and d for the key that was used to produce each
  reference signature S.  We reuse n and d directly so that the firmware's
  signing output can be compared byte-for-byte against the NIST S values.
  No key generation is required; the output is fully deterministic.

PSS (SigGenPSS_186-3.txt):
  PSS signatures are non-deterministic (random salt), so comparing against
  NIST S values is not possible.  Instead the runner signs with a generated
  key and verifies the produced signature (sign-then-verify).  The key is
  generated with a deterministic PRNG seeded from the group parameters so
  the output remains reproducible across Bazel builds.  The key exponent is
  e=65537, which is required by the OpenTitan firmware.

SHA-224 groups are skipped because the firmware does not support that hash.

Output format:
  Both output files are JSON arrays containing a single vector-set object,
  matching the format already used by rsa_vectors.json / rsa_expected.json.
  The AcvpVectors untagged enum in acvp/src/main.rs requires this layout.

Usage (Bazel):
  The run_binary rule in testvectors/data/BUILD invokes this parser
  automatically.  To regenerate the vectors manually:

    python3 nist_cavp_rsa_siggen_parser.py \\
      --pkcs-src path/to/SigGen15_186-3.txt \\
      --pss-src  path/to/SigGenPSS_186-3.txt \\
      --vectors-dst  path/to/rsa_siggen_vectors.json \\
      --expected-dst path/to/rsa_siggen_expected.json
"""

import argparse
import hashlib
import json
import logging
import re
import sys

from Crypto.Hash import SHA256, SHA384, SHA512
from Crypto.PublicKey import RSA
from Crypto.Signature import pkcs1_15

# Maps NIST CAVP SHAAlg token -> ACVP hashAlg field value.
# SHA224 is intentionally absent; those groups are skipped.
_SHA_TO_ACVP = {
    "SHA256": "SHA2-256",
    "SHA384": "SHA2-384",
    "SHA512": "SHA2-512",
}

_HASH_MOD = {
    "SHA2-256": SHA256,
    "SHA2-384": SHA384,
    "SHA2-512": SHA512,
}


def _seeded_randfunc(seed: str):
    """Deterministic byte-generator seeded from *seed*.

    Used for PSS key generation so the output is reproducible across Bazel
    builds (hermetic run_binary rule).
    """
    state = [hashlib.sha256(seed.encode()).digest()]

    def randfunc(n: int) -> bytes:
        result = b""
        while len(result) < n:
            state[0] = hashlib.sha256(state[0]).digest()
            result += state[0]
        return result[:n]

    return randfunc


def _int_to_padded_hex(value: int) -> str:
    """Return uppercase hex with a leading 00 byte (signed-hex convention).

    The runner strips the leading zero from n before sending to firmware.
    """
    length = (value.bit_length() + 7) // 8
    return "00" + value.to_bytes(length, byteorder="big").hex().upper()


def _hex_to_padded_hex(hex_str: str) -> str:
    """Add a leading 00 byte to an existing big-endian hex string."""
    return "00" + hex_str.upper()


def _pkcs1v15_sign(key, message: bytes, hash_alg_acvp: str) -> str:
    h = _HASH_MOD[hash_alg_acvp].new(message)
    return pkcs1_15.new(key).sign(h).hex().upper()


def _parse_pkcs_txt(path: str) -> list:
    """Parse SigGen15_186-3.txt into a list of test-case dicts.

    Each dict has: modulo (int), sha_alg (str), msg (hex str), sig (hex str),
    n (hex str), d (hex str).  n and d are shared within each [mod=X] section.
    """
    cases = []
    modulo = None
    n_hex = None
    d_hex = None
    sha_alg = None
    msg_hex = None

    with open(path) as f:
        for line in f:
            line = line.strip()
            m = re.match(r"\[mod\s*=\s*(\d+)\]", line)
            if m:
                modulo = int(m.group(1))
                n_hex = d_hex = sha_alg = msg_hex = None
                continue
            m = re.match(r"n\s*=\s*([0-9A-Fa-f]+)", line)
            if m:
                n_hex = m.group(1)
                continue
            m = re.match(r"d\s*=\s*([0-9A-Fa-f]+)", line)
            if m:
                d_hex = m.group(1)
                continue
            m = re.match(r"SHAAlg\s*=\s*(\S+)", line)
            if m:
                sha_alg = m.group(1)
                msg_hex = None
                continue
            m = re.match(r"Msg\s*=\s*([0-9A-Fa-f]+)", line)
            if m:
                msg_hex = m.group(1).upper()
                continue
            m = re.match(r"S\s*=\s*([0-9A-Fa-f]+)", line)
            if m and modulo and n_hex and d_hex and sha_alg and msg_hex:
                cases.append({
                    "modulo": modulo,
                    "sha_alg": sha_alg,
                    "msg": msg_hex,
                    "sig": m.group(1).upper(),
                    "n": n_hex,
                    "d": d_hex,
                })
                msg_hex = None

    return cases


def _parse_pss_txt(path: str) -> list:
    """Parse SigGenPSS_186-3.txt into a list of message-only test-case dicts.

    Returns dicts with: modulo (int), sha_alg (str), msg (hex str).
    n, d, and S from the file are ignored because the NIST PSS key has
    e != 65537 and PSS signatures are non-deterministic (random salt).
    """
    cases = []
    modulo = None
    sha_alg = None

    with open(path) as f:
        for line in f:
            line = line.strip()
            m = re.match(r"\[mod\s*=\s*(\d+)\]", line)
            if m:
                modulo = int(m.group(1))
                sha_alg = None
                continue
            m = re.match(r"SHAAlg\s*=\s*(\S+)", line)
            if m:
                sha_alg = m.group(1)
                continue
            m = re.match(r"Msg\s*=\s*([0-9A-Fa-f]+)", line)
            if m and modulo is not None and sha_alg is not None:
                cases.append({
                    "modulo": modulo,
                    "sha_alg": sha_alg,
                    "msg": m.group(1).upper(),
                })

    return cases


def _build_pkcs_groups(cases: list, tg_id_start: int, tc_id_start: int):
    """Build ACVP groups for PKCS#1 v1.5 using NIST n/d/S directly.

    Groups cases by (modulo, sha_alg).  Within each group all cases share
    the same n and d (one key per modulus in the NIST CAVP format).

    Returns (out_groups, out_expected_groups, next_tg_id, next_tc_id).
    """
    # Group by (modulo, sha_alg), collecting the first n/d seen (constant per
    # modulus section) and all (msg, sig) pairs.
    groups: dict = {}
    for tc in cases:
        acvp_hash = _SHA_TO_ACVP.get(tc["sha_alg"])
        if acvp_hash is None:
            logging.debug("Skipping unsupported SHAAlg: %s", tc["sha_alg"])
            continue
        key = (tc["modulo"], acvp_hash)
        if key not in groups:
            groups[key] = {"n": tc["n"], "d": tc["d"], "tests": []}
        groups[key]["tests"].append({"msg": tc["msg"], "sig": tc["sig"]})

    out_groups = []
    out_expected_groups = []
    tg_id = tg_id_start
    tc_id = tc_id_start

    for (modulo, acvp_hash), gdata in groups.items():
        logging.info(
            "PKCS#1 v1.5 tgId=%d mod=%d %s (using NIST key)",
            tg_id, modulo, acvp_hash,
        )
        out_tests = []
        exp_tests = []
        for t in gdata["tests"]:
            out_tests.append({"tcId": tc_id, "msg": t["msg"], "sig": t["sig"]})
            exp_tests.append({"tcId": tc_id, "testPassed": True})
            tc_id += 1

        out_groups.append({
            "tgId": tg_id,
            "testType": "GDT",
            "sigType": "pkcs1v1.5",
            "modulo": modulo,
            "hashAlg": acvp_hash,
            "n": _hex_to_padded_hex(gdata["n"]),
            "d": _hex_to_padded_hex(gdata["d"]),
            "tests": out_tests,
        })
        out_expected_groups.append({"tgId": tg_id, "tests": exp_tests})
        tg_id += 1

    return out_groups, out_expected_groups, tg_id, tc_id


def _build_pss_groups(cases: list, tg_id_start: int, tc_id_start: int):
    """Build ACVP groups for PSS using a seeded-PRNG e=65537 key per group.

    Groups cases by (modulo, sha_alg).  Key generation uses a deterministic
    PRNG so the output is reproducible across Bazel builds.

    Returns (out_groups, out_expected_groups, next_tg_id, next_tc_id).
    """
    groups: dict = {}
    for tc in cases:
        acvp_hash = _SHA_TO_ACVP.get(tc["sha_alg"])
        if acvp_hash is None:
            logging.debug("Skipping unsupported SHAAlg: %s", tc["sha_alg"])
            continue
        groups.setdefault((tc["modulo"], acvp_hash), []).append(tc["msg"])

    out_groups = []
    out_expected_groups = []
    tg_id = tg_id_start
    tc_id = tc_id_start

    for (modulo, acvp_hash), messages in groups.items():
        seed = f"opentitan-rsa-siggen-pss-{modulo}-{acvp_hash}"
        logging.info(
            "PSS tgId=%d mod=%d %s (generating e=65537 key for sign-then-verify)",
            tg_id, modulo, acvp_hash,
        )
        rsa_key = RSA.generate(modulo, randfunc=_seeded_randfunc(seed), e=65537)

        out_tests = []
        exp_tests = []
        for msg_hex in messages:
            out_tests.append({"tcId": tc_id, "msg": msg_hex, "sig": ""})
            exp_tests.append({"tcId": tc_id, "testPassed": True})
            tc_id += 1

        out_groups.append({
            "tgId": tg_id,
            "testType": "GDT",
            "sigType": "pss",
            "modulo": modulo,
            "hashAlg": acvp_hash,
            "n": _int_to_padded_hex(rsa_key.n),
            "d": _int_to_padded_hex(rsa_key.d),
            "tests": out_tests,
        })
        out_expected_groups.append({"tgId": tg_id, "tests": exp_tests})
        tg_id += 1

    return out_groups, out_expected_groups, tg_id, tc_id


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Convert NIST CAVP 186-3 RSA SigGen .txt files to ACVP format."
    )
    parser.add_argument(
        "--pkcs-src",
        metavar="FILE",
        required=True,
        help="Path to SigGen15_186-3.txt (PKCS#1 v1.5).",
    )
    parser.add_argument(
        "--pss-src",
        metavar="FILE",
        required=True,
        help="Path to SigGenPSS_186-3.txt (PSS).",
    )
    parser.add_argument(
        "--vectors-dst",
        metavar="FILE",
        required=True,
        help="Output ACVP test-vector JSON.",
    )
    parser.add_argument(
        "--expected-dst",
        metavar="FILE",
        required=True,
        help="Output ACVP expected-results JSON (testPassed=true for all cases).",
    )
    args = parser.parse_args()

    pkcs_cases = _parse_pkcs_txt(args.pkcs_src)
    pss_cases = _parse_pss_txt(args.pss_src)

    pkcs_groups, pkcs_exp, next_tg, next_tc = _build_pkcs_groups(
        pkcs_cases, tg_id_start=1, tc_id_start=1
    )
    pss_groups, pss_exp, _, _ = _build_pss_groups(
        pss_cases, tg_id_start=next_tg, tc_id_start=next_tc
    )

    all_groups = pkcs_groups + pss_groups
    all_exp_groups = pkcs_exp + pss_exp

    vs_base = {
        "vsId": 1,
        "algorithm": "RSA",
        "mode": "sigGen",
        "revision": "1.0",
        "isSample": False,
    }

    with open(args.vectors_dst, "w") as f:
        json.dump([{**vs_base, "testGroups": all_groups}], f, indent=2)
    with open(args.expected_dst, "w") as f:
        json.dump([{**vs_base, "testGroups": all_exp_groups}], f, indent=2)

    n_tests = sum(len(g["tests"]) for g in all_groups)
    logging.info(
        "Wrote %d test vectors across %d groups (%d PKCS#1 v1.5, %d PSS).",
        n_tests, len(all_groups), len(pkcs_groups), len(pss_groups),
    )
    return 0


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    sys.exit(main())

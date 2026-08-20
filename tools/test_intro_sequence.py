"""Pixel and timing regressions for license, legal, and EA intro states."""

import argparse
import hashlib
import subprocess
import tempfile
from pathlib import Path

from PIL import Image


EXPECTED_RGB_SHA256 = {
    1: "1b5e54972edaa98766b74d1afc1e4d68c3959cfe2645c8fa0c3abf02bcc30f1a",
    134: "45842a44745313e8b236f134ecb06757527698f6b399d71da9a1a76de539b955",
    136: "865ff9f3d15e114ce595962ce95cae5dabad71b1be1efc71ec9533e533f3ff49",
    150: "ed859c95df18178f88849e68d2421a81ce67eb106ada8bfbf8f10268a001650f",
    344: "865ff9f3d15e114ce595962ce95cae5dabad71b1be1efc71ec9533e533f3ff49",
    346: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    # Completed-stage hashes are the RGB hashes of the address-synchronized
    # Mesen captures at $82:F2FE, $82:F37E, and $82:F43A.
    376: "7ee22a4cf926958bc27b86efca80f1be27d64d4f4f0043fc3e28290a59583d70",
    377: "9caa2274655a0717d4de3f05b3e5ea1e59f4be44f469232d28f68ea469f24e87",
    # Intermediate A zoom frames guard the independent $82:F512 Mode 7 layer;
    # same-color E/A overlap must remain filled rather than turning into holes.
    386: "65fde20b33b508ec54e583601c62c54099d730e357fe62f2cba05524b112bf1c",
    394: "b8d89a1e709edcc867fae72b72cfd948f5b4577a5cdedbb5b61348d64ebe2250",
    402: "1c458a82ecfe1756e3c076eb72988e54bee5a8c267eb5620c2d450149e57dd8f",
    407: "36359d7a2c5661d1af9cbeedb43ede90f5c5c06e60eac8ab457df1f60fde918e",
    408: "36359d7a2c5661d1af9cbeedb43ede90f5c5c06e60eac8ab457df1f60fde918e",
    467: "f99613ef471cf5e9e6a259166e85e422b24c1f60062397cf2a3f881e38aaf677",
    468: "556c14918c064d74b3d1da4b10a6c2e6aa7b11ddcb988d123ff318e47f064d99",
    647: "556c14918c064d74b3d1da4b10a6c2e6aa7b11ddcb988d123ff318e47f064d99",
    648: "8f39e89c89712020363c9dd1a1c8b96762f8ea21b1c54c793b91984f10855ac9",
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--rom", required=True, type=Path)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="nba95-intro-test-") as temp:
        directory = Path(temp)
        for frame, expected in EXPECTED_RGB_SHA256.items():
            output = directory / f"intro_{frame}.bmp"
            subprocess.run(
                [str(args.exe), "--headless", "--rom", str(args.rom),
                 "--assets", str(args.pack), "--frames", str(frame),
                 "--dump-frame", str(output)],
                text=True, capture_output=True, check=True,
            )
            actual = hashlib.sha256(
                Image.open(output).convert("RGB").tobytes()
            ).hexdigest()
            if actual != expected:
                raise AssertionError(
                    f"intro frame {frame} changed: {actual} != {expected}"
                )
    print("[TEST] PASS: license/legal fades and four-stage EA intro timing")


if __name__ == "__main__":
    main()

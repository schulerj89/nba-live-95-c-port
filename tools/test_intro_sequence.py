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
    377: "b4103964ea710b377da1a3412b2a8076fb07e545ef3668e01776c3662cbdc945",
    408: "d7a0f0ba9eb6f0a7848f588f9ec41300ff27ee026c3ccb6da6fd77488cc29a33",
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

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
    136: "e06c79b6e1ca522950abd90d119448875d2093089db044a440e1733d01b7f3f5",
    150: "b4c3c996462cf3808f9936e1234170b8388de552b33570b8bb943fa018b7911c",
    344: "e06c79b6e1ca522950abd90d119448875d2093089db044a440e1733d01b7f3f5",
    346: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    377: "b4103964ea710b377da1a3412b2a8076fb07e545ef3668e01776c3662cbdc945",
    408: "8b14d02308ff27dc0959e4595bbb05228739ab6e9af74e28343c9f0bd47b7909",
    468: "844779e6881a2d4081e66d061cff711980f46ae36030c5eb6dbfe24100230b23",
    647: "844779e6881a2d4081e66d061cff711980f46ae36030c5eb6dbfe24100230b23",
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

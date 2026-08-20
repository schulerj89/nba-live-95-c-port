"""Pixel and timing regressions for license, legal, and EA intro states."""

import argparse
import hashlib
import subprocess
import tempfile
from pathlib import Path

import numpy as np
from PIL import Image


EXPECTED_RGB_SHA256 = {
    1: "1b5e54972edaa98766b74d1afc1e4d68c3959cfe2645c8fa0c3abf02bcc30f1a",
    134: "45842a44745313e8b236f134ecb06757527698f6b399d71da9a1a76de539b955",
    136: "865ff9f3d15e114ce595962ce95cae5dabad71b1be1efc71ec9533e533f3ff49",
    150: "ed859c95df18178f88849e68d2421a81ce67eb106ada8bfbf8f10268a001650f",
    344: "865ff9f3d15e114ce595962ce95cae5dabad71b1be1efc71ec9533e533f3ff49",
    346: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    # Intermediate E frames use the indexed $82:F4F6 Mode 7 source rather
    # than enlarging the settled screenshot; this guards filled letter cells.
    352: "09f6f3f11a2b4077c1f601c30bbdcfc6ac5702f2674087833e60c2940e0c5a47",
    360: "9a0db08d8c7a8f3e69f3c855b81f9b4b947c997fab9673c6def91e42341fb6bd",
    # Completed-stage hashes are the RGB hashes of the address-synchronized
    # Mesen captures at $82:F2FE, $82:F37E, and $82:F43A.
    376: "7ee22a4cf926958bc27b86efca80f1be27d64d4f4f0043fc3e28290a59583d70",
    377: "7ee22a4cf926958bc27b86efca80f1be27d64d4f4f0043fc3e28290a59583d70",
    # Intermediate A zoom frames guard the independent $82:F512 Mode 7 layer;
    # same-color E/A overlap must remain filled rather than turning into holes.
    384: "ee453b3f0a53627c55fe84d728e07bf0bcface26faf8423df5b688e8d4730543",
    392: "1701d0eda01de05da670024186695ef3dcb126159fc12cc319b93dfe8ba85f37",
    400: "34513a647ccf5e19c4a76f3caec416c9ef49d057ec10390f8857c51c331e9f74",
    409: "36359d7a2c5661d1af9cbeedb43ede90f5c5c06e60eac8ab457df1f60fde918e",
    435: "0ef18801aa3b39697c8d8dd5dc6d8f47fe6d2d5cff6d9540083c77e96f673bb6",
    443: "f99613ef471cf5e9e6a259166e85e422b24c1f60062397cf2a3f881e38aaf677",
    445: "556c14918c064d74b3d1da4b10a6c2e6aa7b11ddcb988d123ff318e47f064d99",
    647: "556c14918c064d74b3d1da4b10a6c2e6aa7b11ddcb988d123ff318e47f064d99",
    648: "8f39e89c89712020363c9dd1a1c8b96762f8ea21b1c54c793b91984f10855ac9",
}

# Address-synchronized Mesen motion frames captured by mesen_intro_capture.lua.
# These comparisons remain phase-tolerant at the pixel level because the C
# renderer cannot reproduce the SNES analog RGB conversion byte-for-byte.
MESEN_MOTION_ORACLE = {352: 8, 360: 16, 384: 40, 392: 48, 400: 56}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--rom", required=True, type=Path)
    args = parser.parse_args()
    mesen_dir = Path(__file__).resolve().parent.parent / ".analysis" / "intro_capture"

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
            motion_frame = MESEN_MOTION_ORACLE.get(frame)
            oracle_path = mesen_dir / f"ea_motion_{motion_frame:03d}.png" if \
                motion_frame is not None else None
            if oracle_path and oracle_path.exists():
                port_rgb = np.asarray(Image.open(output).convert("RGB"), dtype=np.int16)
                mesen_rgb = np.asarray(Image.open(oracle_path).convert("RGB"), dtype=np.int16)
                mean_error = float(np.mean(np.abs(port_rgb - mesen_rgb)))
                limit = 1.0 if frame < 377 else 2.7
                if mean_error > limit:
                    raise AssertionError(
                        f"intro frame {frame} drifted from Mesen motion "
                        f"{motion_frame}: MAE {mean_error:.3f} > {limit:.1f}"
                    )
    print("[TEST] PASS: license/legal fades and four-stage EA intro timing")


if __name__ == "__main__":
    main()

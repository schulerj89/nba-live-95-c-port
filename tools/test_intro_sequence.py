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
    384: "de0cc705f6aab642830c44bab3b48d58e11d3e3b3d494e704d1b7be6ec0e5b2b",
    392: "7b0318b64c526f388ebac8bb4ed2226c9b06ce4d4e2d391f76f164b43a9058ff",
    400: "8bd36fba8d422a07adf5a3cff560a9eab4b2f87093b1a16dbc6c7fab8d7a33d8",
    # $82:F4C4 changes ownership from Mode 7 to fixed OAM. These eleven
    # frames are exact Mesen identity/flash/settle oracles (motion 56-66).
    401: "98e95c84f8fec5fcb9625aa2d3e1364da7b1cfe9ccca811d41c0b3ce43371999",
    402: "8b14d02308ff27dc0959e4595bbb05228739ab6e9af74e28343c9f0bd47b7909",
    403: "c7aae51ec5fffb153664ad4a19b68175ec3704204b3ff1e4ac613f760a554ad3",
    404: "2034bd915950fefcdb2d0d25f1c5e65e19b0844c4544bf0093982d1a9ea5375a",
    405: "c0dc50c061e10ce41086878befea1d86f6987a330296f5571a0c60f424567924",
    406: "b45f3fc0bdd311ae8daa2fa91c4881ed78c38ffa5ea24688d354ec5943922b90",
    407: "a6e1dd9672ea582a97dfdf5d106192dd69c5fc5da430cf917d166fe9468a010a",
    408: "f809ffa53352615275c136474d1fbba674fa2b2faf476df2175f9435f9671219",
    409: "a0b3098a01995675d002a95243b9048b11aadb4fd75666e33206714e859aeedb",
    410: "1b92d097da23715caa73def90ce7e2b62d723a172dfac9bd43b8b27f7aca54bd",
    411: "8b14d02308ff27dc0959e4595bbb05228739ab6e9af74e28343c9f0bd47b7909",
    435: "f99613ef471cf5e9e6a259166e85e422b24c1f60062397cf2a3f881e38aaf677",
    443: "f99613ef471cf5e9e6a259166e85e422b24c1f60062397cf2a3f881e38aaf677",
    445: "556c14918c064d74b3d1da4b10a6c2e6aa7b11ddcb988d123ff318e47f064d99",
    # $82:F469 exposes the completed logo first; the independent
    # ELECTRONIC ARTS line appears 31 frames later at captured motion 131.
    475: "556c14918c064d74b3d1da4b10a6c2e6aa7b11ddcb988d123ff318e47f064d99",
    476: "844779e6881a2d4081e66d061cff711980f46ae36030c5eb6dbfe24100230b23",
    647: "844779e6881a2d4081e66d061cff711980f46ae36030c5eb6dbfe24100230b23",
    648: "8f39e89c89712020363c9dd1a1c8b96762f8ea21b1c54c793b91984f10855ac9",
}

# Address-synchronized Mesen motion frames captured by mesen_intro_capture.lua.
# These comparisons remain phase-tolerant at the pixel level because the C
# renderer cannot reproduce the SNES analog RGB conversion byte-for-byte.
MESEN_MOTION_ORACLE = {
    352: 8, 360: 16, 384: 40, 392: 48, 400: 55,
    **{port: motion for port, motion in zip(range(401, 412), range(56, 67))},
    476: 131,
}


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
                limit = 0.0 if 401 <= frame <= 411 or frame == 476 else (
                    1.0 if frame < 377 else 2.7)
                if mean_error > limit:
                    raise AssertionError(
                        f"intro frame {frame} drifted from Mesen motion "
                        f"{motion_frame}: MAE {mean_error:.3f} > {limit:.1f}"
                    )
    print("[TEST] PASS: license/legal fades and four-stage EA intro timing")


if __name__ == "__main__":
    main()

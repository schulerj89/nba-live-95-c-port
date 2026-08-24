"""Pixel and timing regressions for license, legal, and EA intro states."""

import argparse
import hashlib
import struct
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
    352: "dcd37988a2880e12bdf84b001ebd737b9079897252643612e6a531412876b6e1",
    360: "6f59216b3572dead53e410a60e460fe0acd747f08f7a2740d2d2e3161e4c59b3",
    # Lock E's identity position, both final highlight frames, and the exact
    # A-stage handoff. The stale completed-stage bitmap used to move E from
    # (57,48) to (53,45) at frame 376 and briefly fill frame 378 with A.
    368: "bce76435bed6593ffc86ee5c2704f3837a721cf588e7bd63409bead9cd159296",
    369: "121dc0f1b7a208d53b5f72a92a1a1d72ce4eb6c8f57a1630220be2ead3e5e749",
    374: "3daf81a8307764b181f43ffb1d9f6d266e9550160c78d297582b3bf13b0522f7",
    375: "121dc0f1b7a208d53b5f72a92a1a1d72ce4eb6c8f57a1630220be2ead3e5e749",
    376: "35599f4966acb91e0d4b4567c8db83f1681fea10e5dc3a0da4e06e0a992b46a0",
    377: "87795095f10787e594b03f1a40d82647b007571eae2fa8eefe7e366fd37a2c0d",
    378: "121dc0f1b7a208d53b5f72a92a1a1d72ce4eb6c8f57a1630220be2ead3e5e749",
    379: "4c4aef613c1be7f0666e9f891454aec293be774e335682e45682d7a38f2bc19b",
    380: "4c4aef613c1be7f0666e9f891454aec293be774e335682e45682d7a38f2bc19b",
    # Intermediate A zoom frames guard the independent $82:F512 Mode 7 layer;
    # same-color E/A overlap must remain filled rather than turning into holes.
    384: "e44d25f3a0c37887244ed5c507439f3cca6a20fbf737a3be6063a39e6363f5f5",
    392: "10b5e7ea278bb05af15f15b555650aa4c97e47036b3e6f665ecc0d035bed8b93",
    400: "ccb158228aa68375370f2d2cb9caf36d20c4492653aa11b55e710ff9062134bc",
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
    # $82:F52E SPORTS is a ROM-indexed Mode 7 layer, never a difference of
    # flattened stage screenshots. Frames 412/418/419 lock the preparation,
    # final-offscreen, and first-visible boundary so a one-frame presentation
    # lead cannot make SPORTS appear disconnected from EA again.
    412: "8b14d02308ff27dc0959e4595bbb05228739ab6e9af74e28343c9f0bd47b7909",
    418: "8b14d02308ff27dc0959e4595bbb05228739ab6e9af74e28343c9f0bd47b7909",
    419: "a6ba0f792542a6f61bb2ad0ce74dc962b280523333d2e0101d823fee42e2359f",
    420: "53b85637c7a3ff8939f89fdafc290756add54db9f5b49da2a03d0b97977fb384",
    421: "e96e6c0020c2f9cb269ba1abf22afdfe24037b76f643f44bcfb4ea60cbec7d42",
    428: "0dd7ea3e85015be473bc2544c3a7d377444a582c533e326cf171640b4a7c08b4",
    # The same presentation delay applies to the two-sweep $82:F4C4 palette
    # highlight: motion 90 is pre-flash, then motion 91 begins at base color.
    435: "67f36aedfd033f9f06f2caad7224f30ddb777c91034bdb9e65c234f3a5bb79e1",
    436: "bf84a2960b6eb542ee5c78f8a81f6548df34b1f07032701ec16f9e12123ff810",
    437: "cc359c4d68eae7c0993bf3f6a9d8f554e889019adef8ba2ed3474c256f4f7783",
    438: "edeeb7546abc72804f23ee13e5cd51327395b5403a132f2076ba9edf8c3abc4d",
    439: "08fe984ae0118b29f08df791247bc568f451056744ff60275a726ed98086b958",
    440: "4c3960eb070d2c3d4b9c01118adabcacbe5eb9676de0c516624b916f9ee10f6d",
    441: "44864921350edf0fa745ccf21bb6493f010e3b8dce271e27450844e09bfb6bd4",
    442: "556c14918c064d74b3d1da4b10a6c2e6aa7b11ddcb988d123ff318e47f064d99",
    443: "cc359c4d68eae7c0993bf3f6a9d8f554e889019adef8ba2ed3474c256f4f7783",
    444: "edeeb7546abc72804f23ee13e5cd51327395b5403a132f2076ba9edf8c3abc4d",
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
    352: 7, 360: 15,
    **{port: port - 345 for port in
       (368, 369, 374, 375, 376, 377, 378, 379, 380, 384, 392, 400)},
    **{port: motion for port, motion in zip(range(401, 412), range(56, 67))},
    412: 67, 418: 73, 419: 74, 420: 75, 421: 76, 428: 83,
    **{port: motion for port, motion in zip(range(435, 445), range(90, 100))},
    445: 100,
    476: 131,
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--rom", required=True, type=Path)
    args = parser.parse_args()
    mesen_dir = Path(__file__).resolve().parent.parent / ".analysis" / "intro_capture"

    raw_pack = args.pack.read_bytes()
    version, count = struct.unpack_from("<II", raw_pack, 8)
    if raw_pack[:8] != b"NBA95PAK" or version != 24:
        raise AssertionError("EA intro requires asset-pack version 18")
    entries = {
        struct.unpack_from("<I", raw_pack, 16 + index * 24)[0]:
        struct.unpack_from("<6I", raw_pack, 16 + index * 24)
        for index in range(count)
    }
    if 74 not in entries:
        raise AssertionError("missing $82:F52E indexed SPORTS layer")
    _, _, size, width, height, _ = entries[74]
    if size != width * height * 4:
        raise AssertionError("invalid SPORTS Mode 7 layer dimensions")

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
                    1.2 if frame < 378 else 2.0 if 420 <= frame <= 443 else 2.7)
                if mean_error > limit:
                    raise AssertionError(
                        f"intro frame {frame} drifted from Mesen motion "
                        f"{motion_frame}: MAE {mean_error:.3f} > {limit:.1f}"
                    )
    implementation = (Path(__file__).resolve().parent.parent /
                      "src" / "nba_ea_intro.c").read_text()
    for marker in ("$82:F6D8", "nba_ea_intro_mode7_source",
                   "NBA_ASSET_EA_SPORTS_LAYER"):
        if marker not in implementation:
            raise AssertionError(f"missing exact SPORTS implementation marker: {marker}")
    if "col3 != col2" in implementation:
        raise AssertionError("flattened screenshot-difference SPORTS path returned")
    print("[TEST] PASS: license/legal fades and four-stage EA intro timing")


if __name__ == "__main__":
    main()

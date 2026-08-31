"""Deterministic C regressions for Setup; bounded native Rules witness separate.

Most historical hashes below are C output baselines, not independent ROM
equivalence. Only the explicitly sourced Rules reveal frames and independent
asset/audio witnesses carry their documented native evidence.
"""

import argparse
import csv
import hashlib
import re
import struct
import subprocess
import tempfile
import wave
from pathlib import Path

import numpy as np
from PIL import Image
from audio_fingerprint import assert_wav_fingerprint, wav_fingerprint
from test_setup_rules_reveal import strict_json, validate_witness
from test_setup_rules_settled import validate as validate_settled_witness
from test_setup_rules_return import validate as validate_return_witness


EXPECTED_RGB_SHA256 = {
    # Frames passed to --setup-only. 104 is the final forced-blank frame;
    # 105 releases $80:A2BF. 125/128 guard the formerly corrupt wrapped
    # construction cells; 130 is pixel-exact with Mesen transition frame 132.
    104: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    # Bounded brightness-only updates independently predicted from native
    # RGB555 conversion and audited against all57,344 pixels. This does not
    # promote the whole initial Setup entrance to native parity.
    105: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    118: "9047943a549bdc15fc77e61b8d34e93c14728d367185c3ab9d0733be907f50b0",
    125: "17b0b585205691998a2e9c5238ed919f874a90094afd8e51cd8025a4309377f5",
    128: "160ca9f0c0e602e43fa77116e9693179dbc79ca7121ff7383199c1970948c17e",
    130: "95e6190d88f4cbe4a6edb85ca1d4e8bc24870ff4cd09b3b9b2affd73a5666489",
    146: "047185a6c2ffb0c4f079f0984ebb6f04afeaa3220c651de7812d1e710df310a2",
    # Independently attributed fresh Arcade/12-minute defaults; old hashes
    # remain in build/setup-default-attribution-v5/report.json. Each delta is
    # 1116 pixels in the changed value cells, not a new native timing claim.
    162: "efc1d965830da3350414ce1ee826956fc918ac4c2ff5308200851a7e8b70882c",
    166: "8b5c6f85f54ecfa5ee30b67bf3af2eadc99d95bda719cc9c2981fdd53b6c7aff",
}
EXPECTED_AUDIO_RMS_EIGHTHS = [
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6123, 4989, 3945,
    3471, 1069, 1139, 888, 1505, 811, 404, 4485, 2990, 4451, 2948, 4595,
    3179, 5502, 3725, 1098, 4764, 4607, 3960, 4541, 4196, 2192, 2887,
    3940, 3612, 3854, 4021, 3740, 5273, 5603, 4133, 3521, 1826, 1086,
    903, 1483, 755, 404, 4160, 3388, 4422, 3201, 4388, 3193, 5181, 4132,
    1219, 4300, 4395, 4198, 4282, 4748, 2550, 2683, 4479, 3927, 4439,
    3793, 3971, 3214, 5939, 4129,
]
EXPECTED_AUDIO_BAND_PPM = [889127, 48767, 32034, 14587, 14255, 1068, 162]
EXPECTED_AUDIO_CHANNEL_RMS = [3363, 3363]
# Asset280 adds one F12 count entry. test_shot_assets independently proves
# only the index/count row changes, retaining the historical art hashes.
# Asset281 changes only F12's directory count row; test_shot_assets proves
# the pixel delta remains confined to that row before these hashes are used.
# Pack v31 adds the indexed gameplay PPU entry, changing only F12's displayed
# entry count; the decoded VRAM and OAM artwork remains locked below.
# Indexed intro75/76 replaces eleven entries: A1/new complete-image comparison
# verified only F12's metadata row changed. These remain C-only debugger hashes.
# See docs/intro-indexed-resources.md; every common asset is byte-identical.
EXPECTED_ASSET_DEBUGGER_SHA256 = "9c9a026b488b28c0317d9dacc47bbef9372db110d142634c8e692cf0a4c133fa"
EXPECTED_OAM_DEBUGGER_SHA256 = "919da3c071ad245b83ad027651fa2beb557038c19202f492aa6732ce124d85d9"
EXPECTED_RENDERED_MENU_SFX_SHA256 = {
    0x1A: "447a1ea48a94e2036ff0bdf1f4c5248d6284daec0b723b9a966f841976e703c4",
    0x1B: "96de89e954e4e8f75e555625abba5bf4380b8868b3263776a4cc27a6285de664",
    0x1C: "6c2879a2f1b4beda1318c14ae70e352953669dbc0eb60a49a8eec773908e29ee",
}
EXPECTED_MENU_SFX_PARAMETERS = {
    0x1A: ("$05A8", 9823),
    0x1B: ("$050A", 10964),
    0x1C: ("$03C6", 10662),
}

# Compact oracle derived from the independently recorded Mesen WAV beginning
# at its last-title-fade boundary (198.32 s). Raw emulator PCM is not shipped.
MESEN_SETUP_RMS_EIGHTHS = [
    155, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5213, 5602, 4033,
    3584, 1545, 1101, 910, 1468, 822, 397, 4229, 3257, 4183, 3206, 4353,
    3334, 5263, 3918, 986, 4301, 4664, 3808, 4546, 4394, 2432, 2202,
    4156, 3310, 4006, 3799, 3903, 4675, 5940, 4077, 3620, 2127, 1063,
    900, 1451, 793, 398, 3952, 3435, 4264, 3294, 4284, 3375, 5019, 4074,
    1616, 4221, 4304, 4166, 4171, 4577, 2802, 1190, 4904, 3803, 4399,
    3805, 3962, 2949, 5902, 4307,
]
EXPECTED_CURSOR_SHA256 = {
    0: "e3ec329dc39626391b9315e7ff60bf4c60b9a31b23d903bdcdca22ac5738fc65",
    1: "efc1e6e70a979c44ce821752be4a2eab6afff558ced34ad2fcb1aae0ac5c787b",
    2: "9fcfb2c5ec4c2fe5245c8f0ba166aa771f18ea27eb433f618a81c49b2b600ad8",
    3: "05567ed02467a3b7248729e6ca591c26f417274d1a38bbd3c14b5bcc2cc970ce",
    4: "ca995057d4f8848287c0422f307dd31330297eccb82019bbce7b0e7baa7c2354",
    5: "80d54ec7fc3fde9e6ad83a976b3b342c90ad59f06ddc24f59cf7f669b2b823c8",
    6: "e3ec329dc39626391b9315e7ff60bf4c60b9a31b23d903bdcdca22ac5738fc65",
}
EXPECTED_MENU_RGB_SHA256 = {
    # Native hold753/C450 after exact reveal + continued IRQ viewport and
    # idle arrow palette. The companion gate checks every617..753 RGB frame.
    "rules": "5282ebb046c621a3225df8c96b2b40b5908021bba0b2f0650e9be76d59bbbb9b",
    "options": "8f3e34fc6934d7313e486f6cb6e5c3793bd2e0aa6b28e2e5e0e91d3b8f30e246",
}
EXPECTED_MENU_ACTION_SHA256 = {
    ("rules", 2, 1): "0c3fab94548710f14ab5b1cd08f5ef1f086ef1d92add5e29ac239120512fc3e2",
    ("options", 0, 1): "e212fcc26cc78826fafc4ba6af327f6b0f0635bffe30fc4ec8db8bffe6dac691",
    ("rules", 7, 0): "0f2fdb67980025bca59b0d355d19c42fd9bcb85fcfeac2792afad5788d644a4a",
    ("rules", 9, 0): "f7e0ef0464f268d418e134d03e67a698123b94b5493327eaadd7a31728c77835",
    ("rules", 12, 0): "363dd4054d161d52fae9b2200d44e311f750eff0f81b9f0bbbdfd752851be566",
    ("options", 2, 1): "5ed31b3266cf48ef8412392ea55515238b2a4aedd134ae24b63a784fc0909302",
    ("options", 2, 2): "9c4be2893491cab8adcbab216f29ea6c0fb503b2192ca1373524168d6f7cce60",
    ("options", 3, 1): "bac62d29c447641c9d0141390532dc8a1de7662cf4a02bf7c603eb62cf7183ff",
    ("options", 4, 1): "3b4404450f9bd56de31ec5c7f8933847ffbe49ec64407952e519cc897d054ebf",
    ("options", 5, 1): "c2fbb4088093941e70e592d107993334c7b076395a5611f0c87f3552432996d2",
    ("options", 6, 1): "9471a8171fa543c5b570b0d5ffc85430038579908ae16f7c670d46631c9c2f26",
}
# These selected Rules snapshots now come from independent natural native UI
# journeys at753/C450. Input timings differ; this is settled-state/pixel proof,
# not equivalence of the navigation/scroll animation between button presses.
for _menu_row, _rights in ((2, 1), (7, 0), (9, 0), (12, 0)):
    _ui_witness = validate_settled_witness(strict_json((Path(__file__).resolve().parents[1] /
        f"tests/fixtures/setup-rules-row{_menu_row}-native.json").read_text()))
    EXPECTED_MENU_ACTION_SHA256[("rules", _menu_row, _rights)] = _ui_witness["rows"][0]["rgb_sha256"]
EXPECTED_SUBMENU_TRANSITION_SHA256 = {
    ("open", 198): "6ef0efca4c6ffc9f65ef9d9b663719ab7a77614a587a9613e8204d671e3cb889",
    ("open", 219): "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    # $2100 forced blank is not part of Mesen's screenBrightness property.
    # These checkpoints sit inside $80:A2BF's formerly exposed VRAM build.
    ("open", 234): "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    ("open", 242): "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    # Legacy values within native546..616 are replaced below by the strict
    # synchronous native witness. The old async screenshot proof was invalid.
    ("open", 246): "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    # Historical C baselines retained here for readability of the migration.
    ("open", 258): "173954d6e9c0b1deb1336065c077478e5df1fdfa7dd6b58c438d55ec1012200e",
    ("open", 259): "5fe120f6cd620bcc6149486dac74a39501450a0ffacc5d2c517ce95fef74b56e",
    ("open", 262): "923709f429d1eee5dba16739b9a9353969a8f3ef7cbd80f8ac5bade00f506b61",
    ("open", 266): "3f96e4fed79121d9caae851d45bc7c7af7b084c14f260b0660a7fcf41c829589",
    ("open", 270): "f87c5552d39fdb603d27d6554e8ba6d01a6a0b5dd93740bc76bdf27bd4e775fd",
    ("open", 274): "2e491d93222eb41828f1c3724390b45bcfcc5c58430d7996eca6ee51943c6b61",
    ("open", 278): "5ee642673003ccfb95b38ebad0ad11f06834a4d9a42106e7c806354d24e65b4d",
    ("open", 286): "a09b7c8333bf2f6f4a93e5a79ad0a276b840d2cdf44b911520ba66d5bd351c1a",
    ("open", 299): "d44b1b0d7f8661abc80ca824d7ee8844541778829adad255921e011979859a5c",
    ("open", 307): "50d8ff6af090c3da9634c71474242799c546164844cff3e9417f4a5029aaf8e5",
    ("open", 313): "dc47df3afd364178ea855980c1f9218f9e68530c2f4b3315678ab8e865b9f79b",
    # Native617 verifies the first frame after the packed trace is released.
    ("open", 314): "a975fcd258efba3f923f35e76ea80c479f01dccdd72864badc6c4cfa7f740cd5",
    # These early-return checkpoints straddle the map/CHR construction DMA.
    # They must remain clean outgoing-page scanout, never raw tile memory.
    ("close", 319): "65f8b285fa51bfcdab3c3a0ddee180d3668535eea7554a4eba39f58238d18bda",
    ("close", 320): "65f8b285fa51bfcdab3c3a0ddee180d3668535eea7554a4eba39f58238d18bda",
    ("close", 329): "3891ebee1e014af5eda09e56d2d20bc88f42e7b5caf3b5fcde6af1c2ce6fa172",
    ("close", 345): "68e73fd8f183420d0bf526963f300161316c78866a75e6576a9f18e806cec175",
    # Rules' return edge asserts forced blank earlier than Options.
    ("close", 350): "5e1fe6ffc477f6f973c9bf2b363fd0f60119227e452f977a5563844b8eacb8d5",
    ("close", 351): "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    ("close", 352): "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    ("close", 353): "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    ("close", 382): "28c274a8c4d2395fcd11de185969e87c3d723aa9bce9737d572d5cfa5c542034",
    ("close", 424): "85c0f2bf5b2e75e5ad1ec233e654f70ae13841b76a769e3e4c27e42d27d7affb",
    ("close", 450): "c238e0e8750eaffac37606ab71bb7201ce4fc81cf1447d28940f260b98446ce6",
}
_RULES_WITNESS = validate_witness(strict_json((Path(__file__).resolve().parents[1] /
    "tests/fixtures/setup-rules-reveal-native.json").read_text()))
for _row in _RULES_WITNESS["rows"]:
    _key = ("open", _row["port_step"])
    if _key in EXPECTED_SUBMENU_TRANSITION_SHA256:
        EXPECTED_SUBMENU_TRANSITION_SHA256[_key] = _row["rgb_sha256"]
_RULES_OPEN_WITNESS = validate_witness(strict_json((Path(__file__).resolve().parents[1] /
    "tests/fixtures/setup-rules-open-native.json").read_text()))
for _row in _RULES_OPEN_WITNESS["rows"]:
    _key = ("open", _row["port_step"] - 717)
    if _key in EXPECTED_SUBMENU_TRANSITION_SHA256:
        EXPECTED_SUBMENU_TRANSITION_SHA256[_key] = _row["rgb_sha256"]
_RULES_RETURN_WITNESS = validate_return_witness(strict_json((Path(__file__).resolve().parents[1] /
    "tests/fixtures/setup-rules-return-hold-native.json").read_text()))
for _row in _RULES_RETURN_WITNESS["rows"]:
    # Native Start830/C527 uses212 extra input-idle frames versus the legacy
    # C script's Start315. Preserve logical keys; expected pixels come only
    # from the native return witness, never from the modified implementation.
    _key = ("close", _row["port_step"] - 212)
    if _key in EXPECTED_SUBMENU_TRANSITION_SHA256:
        EXPECTED_SUBMENU_TRANSITION_SHA256[_key] = _row["rgb_sha256"]
EXPECTED_OPTIONS_OPEN_TRANSITION_SHA256 = {
    # Independent controlled-native brightness table predicts every changed
    # pixel from the preserved pre-change image (options-open audit).
    198: "8205aad58eab156a3e3729fab183c4c5348e86cbc77176dc09ed2d216d343be7",
    219: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    259: "923709f429d1eee5dba16739b9a9353969a8f3ef7cbd80f8ac5bade00f506b61",
    263: "3f96e4fed79121d9caae851d45bc7c7af7b084c14f260b0660a7fcf41c829589",
    267: "f87c5552d39fdb603d27d6554e8ba6d01a6a0b5dd93740bc76bdf27bd4e775fd",
    271: "2e491d93222eb41828f1c3724390b45bcfcc5c58430d7996eca6ee51943c6b61",
    275: "5ee642673003ccfb95b38ebad0ad11f06834a4d9a42106e7c806354d24e65b4d",
    279: "a12c633f18c9c5cb8379d26d470b7f6d8890189ec19ea1ae7a3bcdae18d39584",
    283: "a09b7c8333bf2f6f4a93e5a79ad0a276b840d2cdf44b911520ba66d5bd351c1a",
    287: "e0dc1a529cbf1cd9a6bc5b7dbc8bd4eef17f7bc3b10f8fc098669057faa02169",
    299: "65f046c92a25bfea2ad233b37c164adf2e73a4e0661e05c9a66f03e817782880",
    300: "ec8a9690ee755fa6da43cb97574bd02419f35c2d47c97c027bbe0834a37bcfbb",
    301: "651b63bc47901714f6082cc242768160cf26ce6202ef9ab2bf1f1c67c92ac625",
}
EXPECTED_OPTIONS_RETURN_TRANSITION_SHA256 = {
    315: "9395524d7f94eb5b605468d0c9fe48a7817de0460bf24873b5c3ba7b6bee1fb8",
    321: "c29ac2a0ecf74a04e17d3b75872ee76519c11053a474daea8898bac85f4e76d3",
    322: "c29ac2a0ecf74a04e17d3b75872ee76519c11053a474daea8898bac85f4e76d3",
    323: "c29ac2a0ecf74a04e17d3b75872ee76519c11053a474daea8898bac85f4e76d3",
    324: "cc267eeed6208dd3ef155933f0b6e313fed9c204b1a2e20c075a83c4f7d203cc",
    # Same independently verified brightness-only derivation; this does not
    # certify the legacy Options return scheduler/trace as ROM-equivalent.
    343: "40db00f19ac4a86382405788ba6732a3423ac8947a88c7c76c9a91a15aa6aeff",
    355: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    367: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    423: "6a6d9555e98d595b096bde43038040808a49a474bf987c6f5cbcc9caea494976",
    431: "9086981ffe8109b54c64f012bfe7666ecee9f2f310f46767953453161d7d7222",
}
MESEN_MENU_PRIMITIVE_SHA256 = {
    "rules_bar": "6a08a0e635e0d0ae5c24cafbaf3507fd16ce61c0f3adc0c282a8b620b36f48c8",
    "options_bar": "bda182a61baf1f225317538bd3e4d4ddae969498ae47fd40794504342a8708cb",
    # Native idle palette3; the old palette2 witness was a recent press.
    "down_arrow": "92409e5622a90c322305da2be37e4e34cd4fd895ad008131dbf24e49de94631b",
}
EXPECTED_MENU_SFX_SHA256 = {
    120: "da27ef1aacd1e96b71b40e6b0baacd0a8fa4f02ddae8754d3fc620602670eaae",
    121: "da27ef1aacd1e96b71b40e6b0baacd0a8fa4f02ddae8754d3fc620602670eaae",
    122: "303c2a89000b675eb48a42160c8d233d2a28ab728d8b7645628886bd224cb890",
}
EXPECTED_MENU_ASSET_SHA256 = {
    124: "acc87f5139c463275742a378f966c64cc030b40f9712dc0e7329ddc57e622b31",
    125: "d3fa496f57f7bfaca0780582175b06e1c897efdb4583210f8dbb7c81d05cba65",
    126: "0d25909881fe03449acf046c2d3a8cfaa64172f596864c3523c08806b581f89d",
    127: "c8135644bda6768020d67743a823e56eab734193a967ccddd7b570ba63d715a2",
    128: "824b693aab168ba4e993bfbbf5f68b1543245dca1910a9d7467c7eebfc399176",
    129: "90bc3aacc02d39f34ecd99204e85ad31f1ffdcfa4f46a7316b0932274d16c228",
    130: "42cd32dc8df82febef1936d3f1c2600e070e32797cee09690b0ea802ff529bf1",
    131: "e98b9ea84551ff95e47d3eb479d7705ba47232d994b40889eedb9762dc0cba06",
    132: "afdccdbbc0bfdf06228541d01bb8e5f9e44767eeb7a472c19f6af040b6159d02",
    133: "e4a55372bda54cce7014e114beb38fe30662f6e15437aaedad9e49d27fe54fbd",
    134: "d1446eb4b7c6f6d275a980f3b39e98129817fc84071f8151ae90b82ff79d8c60",
    135: "69dd72dc97b50cae9817effd52c29f5a9ff1bba5276221a4f5cdba1d7dbf960d",
    136: "b231ec61fa3e439ef7e9600d81d9840dfbbc2595114d4e634e62951320c12c14",
    137: "56e0c8bfb349bc77de2234141861f8fe0736723141bbd71398dc19e56db2f44e",
    138: "a037d5934c9a6b839b167828336af0b9d1bcce4bd98faa13ac32e2578fc5694f",
    139: "6d199ebab13adc93aef1ed3abcab3a77219e8d1e7d7811e89bfab3074d78256d",
    140: "abb16640b4954cc1d968305a345c3703a0447eb47fcbc5c98cc3af7d53e45887",
    141: "ed829c237970e861f130a6b7966ee0ef911623cd39d56c142238ec95432d2102",
    142: "e01ff3a82d09d77d018ed79f2b54fd7620f975b523c680690d15efeea8b6adfa",
    143: "1e5c485b7ed12fe444588a5beebd984fa6489598fcb43064187e1617ab0238ce",
    # Regenerated solely from canonical Simulation/3min native raw PPU inputs;
    # no rendered emulator image is included in these production assets.
    144: "34d010c965f58a851c9622553466102c49b1faabde83ac674957d90ee2d51553",
    # Native DMA publications (NBSPPU3), including baseline-zero writes;
    # independently audited in rules-publications-independent-audit.md and
    # reproduced by the full extractor in build/full-extraction-v1.
    145: "954bc211393c084654eadc4b58e3a5984db35cc11ead9cb7a964bd00b692f561",
    146: "1e5c485b7ed12fe444588a5beebd984fa6489598fcb43064187e1617ab0238ce",
    147: "8a95be0e8f109eec61af8385fc022072cffd57c13b13bf94b441a0d273ea8e65",
    148: "b2485adab570e6b133bf54108160ec9423070f041eaa9663cb566fc0e5e30b4e",
    149: "906335dc6637bc01d81cad135c00a8f16580dd83e2471850f70a5b82a641a392",
    150: "c8135644bda6768020d67743a823e56eab734193a967ccddd7b570ba63d715a2",
    151: "149925f90253de9d279723359ce56a4cb8c601d176b642c8366fc465409b882e",
    152: "eb893005c6d675b73d997f32fe2b479a9d1b8b71425f05e606519da09279ff68",
    153: "acc87f5139c463275742a378f966c64cc030b40f9712dc0e7329ddc57e622b31",
    154: "c0f020106386715daa7583eaca351851bf7a4863c2118079ffa7d56bbb2693ec",
    155: "8001427cf8ca80638662bf7cb49851bb30e2c6ce93c883c5e0914093e99637c0",
    156: "12d8c001afc6e355654d1965a1fbe7e7c405e028774449f70c3eece17919f43f",
    157: "a93724eda8d5190b1a1fd253812534aa942a68d0654c4608bf029342a111a01f",
}
# The seven newly restored native shadow strips alter only y=top+16,
# by30/33/33/24/19/21/26 pixels. Full47 native canvases and the independent
# span audit cover the19-line source contract. Controlled old/new attribution
# is retained in build/main-value-image-attribution-v1/report.json; these are
# C-only full-image baselines. Native glyph hashes below remain unchanged.
EXPECTED_MAIN_VALUE_RGB_SHA256 = {
    (0, 1): "ea1f305909dba2d3d83d3ae54edb81a934d9873c2480f971ef577713ad71b581",
    (0, 2): "ac10400e1b0f21a493912f8c64f03fcabbdc2fc2a48d131450d8a5fc1f862683",
    (0, 3): "282b14b34050edb05fea121848736646fbd7f12ef4dfd96aca4fbb8d595b4830",
    # Independent prediction: old C baseline plus native Custom980's y104
    # shadow strip at the same BG2v30. Exactly26 pixels change; this remains
    # a static C regression, not a full native main-value journey witness.
    (1, 1): "e8f91cea8043243dd2300f9b3e4e4c90a487e3c9c86f0459650eadd3c47c1771",
    (1, 2): "e39b9287ae34056e58db8ead3567984f8d612297a27b77aa01661d05d6f7839b",
    (2, 1): "5c0e82ffa2bedab68e8dd9014615df959d3713c2dd36b4f5aa52a5dcff670b52",
    (2, 2): "d60279d19d68d9fb39783f7acf7c3d27e332b26dfaabe587a78014b42e42d592",
    (3, 1): "e4ae718f8545e18560eebcfebf5e344773bcbda67e04f50b8e194d6671813a91",
    (3, 2): "908bc9d71553bc3809404d8175c59760dfcaad0087eb916a1e0e767ab32ad3a7",
    (3, 3): "9baf90891ffbe07f02e1c374d50fdbdd8ac29ab8b8b23248a276bc225ac94a93",
}
MESEN_MAIN_VALUE_GLYPH_SHA256 = {
    (0, 1): "dc35ee3889bed4f3c86a90e3a0acbee1a63d9fdf94d0de173f5437dde770268d",
    (0, 2): "cecbb02cd80d6a4d416881873a326623a3bd7f544d3fba3a86a91110027e2125",
    (0, 3): "b4cd582b37282e5ae24fc9561beb10532290512aa04d2c5e9353744f05ab2f7d",
    (1, 1): "ef4d6d61b67848d8e3d5550403e8a3d95085aa46fa7c00a0e16798034b15dbfd",
    (1, 2): "c99ceee59e9b4ecd50cae573ab869e91780d0e2e4436d85c72df9a6ad62e6a85",
    (2, 1): "f31de1b931dd30ced18dccf02abea99ff8aeaa9ee67027395f91dc99108ed6d4",
    (2, 2): "a5c4bf9f2197084dc28d5001cf6c30cd6c4fc8e4e1d7b064d437770fead998c5",
    (3, 1): "74afa56375a738feda3cf44e8890466735e48d4bb2b82ae40b6f3af6b7ea16a3",
    (3, 2): "35a31c28e8684ccf2240f6e52a61d28bd3a000d929aa5b275d65c52a23fae731",
    (3, 3): "b9f40f561ec4e6e772a6d5aced4207e96412bfd49f1287cae00bf6a9e24d1d80",
}
SETUP_LOOP_START = 2053956
SETUP_LOOP_END = 4048365


def load_pack(path):
    data = Path(path).read_bytes()
    if len(data) < 16 or data[:8] != b"NBA95PAK":
        raise AssertionError("invalid asset pack")
    version, count = struct.unpack_from("<II", data, 8)
    if version != 31 or 16 + count * 24 > len(data):
        raise AssertionError("invalid asset directory")
    assets = {}
    for index in range(count):
        asset_id, offset, size, _, _, _ = struct.unpack_from(
            "<6I", data, 16 + index * 24
        )
        if asset_id in assets:
            raise AssertionError(f"duplicate asset ID {asset_id}")
        if offset + size > len(data):
            raise AssertionError(f"asset {asset_id} extends beyond pack")
        assets[asset_id] = data[offset : offset + size]
    return data, assets


def check_pack(pack_path):
    raw, assets = load_pack(pack_path)
    required = {16, 17, 88, 89, 90, 91, 92, 93, 120, 121, 122,
                *range(124, 156), 156, 157}
    if not required.issubset(assets):
        raise AssertionError(f"missing Setup assets: {sorted(required - assets.keys())}")
    if len(assets[16]) != 0x10000 or len(assets[17]) != 0x200:
        raise AssertionError("invalid Setup VRAM/CGRAM size")
    for asset_id, expected_hash in EXPECTED_MENU_ASSET_SHA256.items():
        actual_hash = hashlib.sha256(assets[asset_id]).hexdigest()
        if actual_hash != expected_hash:
            raise AssertionError(f"Set Rules/Options asset {asset_id} changed")
    # Ghidra $81:9756/$81:9FD4 redraws one row in the mutable BG3 canvas.
    # Mesen must therefore show independent byte sets for Crowd OFF,
    # Slow-Motion ON, Shot CPU, and CPU-Assistance ON.  This guards the exact
    # cross-row contamination that previously left another value's shadow.
    base = assets[126]
    row_deltas = {
        asset_id: {index for index, (a, b) in enumerate(zip(base, assets[asset_id]))
                   if a != b}
        for asset_id in (152, 156, 132, 157)
    }
    if any(not delta for delta in row_deltas.values()):
        raise AssertionError("an Options glyph variant has no ROM BG3 delta")
    ids = list(row_deltas)
    for left_index, left_id in enumerate(ids):
        for right_id in ids[left_index + 1:]:
            if row_deltas[left_id] & row_deltas[right_id]:
                raise AssertionError(
                    f"Options row assets {left_id}/{right_id} overlap in BG3")
    for asset_id in (120, 121, 122):
        if assets[asset_id][:4] != b"RIFF":
            raise AssertionError(f"menu SFX asset {asset_id} is not an F11 WAV")
        if hashlib.sha256(assets[asset_id]).hexdigest() != EXPECTED_MENU_SFX_SHA256[asset_id]:
            raise AssertionError(f"menu SFX asset {asset_id} content changed")
    if len(assets[88]) != 0x10000 or len(assets[89]) != 0x80:
        raise AssertionError("invalid Setup SPC RAM/DSP size")
    if assets[90][:8] != b"NBTSSPC1" or assets[91][:8] != b"NBTSAPU1":
        raise AssertionError("invalid Setup SPC asset format")
    if assets[92][:8] != b"NBSPPU1\0":
        raise AssertionError("invalid Setup entrance PPU trace")
    if assets[93][:8] != b"NBTSDSP1":
        raise AssertionError("invalid Setup S-DSP trace")
    if any(blob[:4] == b"RIFF" for blob in
           (assets[88], assets[89], assets[90], assets[91], assets[93])):
        raise AssertionError("recorded Setup WAV returned")

    version, frames, writes = struct.unpack_from("<III", assets[91], 8)
    if version != 1 or frames != 9000 or writes != 289435:
        raise AssertionError(f"unexpected Setup APU dimensions: {frames}, {writes}")
    if len(assets[91]) != 20 + writes * 6:
        raise AssertionError("truncated Setup APU trace")
    previous = -1
    for index in range(writes):
        cycle, port, _ = struct.unpack_from("<IBB", assets[91], 20 + index * 6)
        if cycle < previous or port > 3:
            raise AssertionError(f"invalid cycle event {index}")
        previous = cycle
    if previous > frames * 1024000 // 60:
        raise AssertionError("Setup APU trace exceeds its declared duration")

    version, dsp_frames, dsp_writes = struct.unpack_from("<III", assets[93], 8)
    if version != 1 or dsp_frames != 9000 or dsp_writes != 114059:
        raise AssertionError(
            f"unexpected Setup S-DSP dimensions: {dsp_frames}, {dsp_writes}"
        )
    if len(assets[93]) != 20 + dsp_writes * 6:
        raise AssertionError("truncated Setup S-DSP trace")
    previous = -1
    for index in range(dsp_writes):
        cycle, register, _ = struct.unpack_from("<IBB", assets[93], 20 + index * 6)
        if cycle < previous or register >= 0x80:
            raise AssertionError(f"invalid S-DSP cycle event {index}")
        previous = cycle

    version, ppu_frames = struct.unpack_from("<II", assets[92], 8)
    if version != 1 or ppu_frames != 61:
        raise AssertionError("unexpected Setup entrance PPU dimensions")
    offset = 16
    ppu_writes = 0
    for _ in range(ppu_frames):
        if offset + 4 > len(assets[92]):
            raise AssertionError("truncated Setup entrance PPU record")
        vram_count, cgram_count = struct.unpack_from("<HH", assets[92], offset)
        offset += 4 + (vram_count + cgram_count) * 3
        ppu_writes += vram_count + cgram_count
    if offset != len(assets[92]) or ppu_writes != 3094:
        raise AssertionError("invalid Setup entrance PPU write stream")
    for asset_id, expected_frames, expected_writes in (
        # NBSPPU3 preserves actual native publication destinations, including
        # zero writes absent from the old changed-value-only stream.
        (145, 146, 132148), (148, 132, 23229),
        (151, 132, 23070), (155, 132, 103812)
    ):
        trace = assets[asset_id]
        publications = asset_id in (145, 155)
        if trace[:8] != (b"NBSPPU3\0" if publications else b"NBSPPU2\0"):
            raise AssertionError(f"invalid submenu PPU trace {asset_id}")
        version, frames = struct.unpack_from("<II", trace, 8)
        if version != (3 if publications else 2) or frames != expected_frames:
            raise AssertionError(f"unexpected submenu PPU dimensions {asset_id}")
        offset = 16
        writes = 0
        for _ in range(frames):
            if offset + 38 > len(trace):
                raise AssertionError(f"truncated submenu PPU trace {asset_id}")
            brightness, main, sub, reserved = struct.unpack_from("<BBBB", trace, offset)
            # Rules-open bit6 marks observed INIDISP; bit7 is forced blank.
            valid_flags = (0x40, 0xC0) if asset_id in (145, 155) else (0,)
            if brightness > 15 or main > 31 or sub > 31 or reserved not in valid_flags:
                raise AssertionError(f"invalid submenu PPU screen state {asset_id}")
            for layer in range(3):
                hscroll, vscroll, tilemap, char_base, wide, tall = \
                    struct.unpack_from("<HHHHBB", trace, offset + 4 + layer * 10)
                if hscroll > 1023 or vscroll > 1023 or tilemap & 1 or \
                        char_base & 1 or wide > 1 or tall > 1:
                    raise AssertionError(f"invalid submenu PPU layer state {asset_id}")
            vram_count, cgram_count = struct.unpack_from("<HH", trace, offset + 34)
            offset += 38 + vram_count * (4 if publications else 3) + cgram_count * 3
            writes += vram_count + cgram_count
        if offset != len(trace) or writes != expected_writes:
            raise AssertionError(f"invalid submenu PPU write stream {asset_id}")
    if b"post_ea_game_setup.wav" in raw:
        raise AssertionError("recorded Setup WAV name returned")

    # F11 must expose the exact streamed BRR directory used by Setup. Asset
    # metadata is width=SRCN, height=start, flags=loop; payload is audition WAV.
    directory = assets[88][0x200:0x278]
    entries = {}
    _, count = struct.unpack_from("<II", raw, 8)
    for index in range(count):
        entry = struct.unpack_from("<6I", raw, 16 + index * 24)
        entries[entry[0]] = entry
    for srcn in range(30):
        asset_id = 94 + srcn
        if asset_id not in assets or assets[asset_id][:4] != b"RIFF":
            raise AssertionError(f"F11 is missing Setup SRCN ${srcn:02X}")
        start, loop = struct.unpack_from("<HH", directory, srcn * 4)
        _, _, _, packed_srcn, packed_start, packed_loop = entries[asset_id]
        if (packed_srcn, packed_start, packed_loop) != (srcn, start, loop):
            raise AssertionError(f"Setup SRCN ${srcn:02X} pointer metadata changed")


def legacy_options_script(directory, confirm=False):
    """Real C button route preserving legacy Options A168/Start302 phase.

    Configure during the C entrance's accepted-input interval, then release
    between every navigation press. No configuration or PPU state is seeded.
    This preserves an existing C regression phase, not native entry timing.
    """
    script = Path(directory) / ("options_return.input" if confirm else "options_open.input")
    words = [0] * 431
    for i, word in enumerate((0x400, 0x100, 0x400, 0x400, 0x100, 0x800, 0x800, 0x800)):
        words[138 + 2 * i] = word
    for i in range(5):
        words[157 + 2 * i] = 0x400
    words[167] = 0x80
    if confirm:
        words[301] = 0x1000
    script.write_text("".join(f"1 {word:04x}\n" for word in words))
    return script


def check_frames(exe, rom, pack):
    with tempfile.TemporaryDirectory(prefix="nba95-setup-test-") as directory:
        # Rules/Options must never fall back to host-rendered text when a
        # mandatory game-authored variant is absent. Keep the pack structurally
        # valid but rename OFF's entry to an unused ID, then prove page entry is
        # refused.
        incomplete_raw = bytearray(pack.read_bytes())
        _, incomplete_count = struct.unpack_from("<II", incomplete_raw, 8)
        for index in range(incomplete_count):
            entry_offset = 16 + index * 24
            if struct.unpack_from("<I", incomplete_raw, entry_offset)[0] == 130:
                struct.pack_into("<I", incomplete_raw, entry_offset, 159)
                break
        else:
            raise AssertionError("canonical pack is missing OFF variant asset 130")
        incomplete_pack = Path(directory) / "missing_off_variant.pak"
        incomplete_pack.write_bytes(incomplete_raw)
        incomplete = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-menu", "options",
             "--frames", "450", "--rom", str(rom), "--assets", str(incomplete_pack)],
            text=True, capture_output=True, check=True,
        )
        if "[SETUP TEST] page=0" not in incomplete.stdout:
            raise AssertionError("incomplete pack opened Options with fallback graphics")

        missing_main_raw = bytearray(pack.read_bytes())
        _, main_count = struct.unpack_from("<II", missing_main_raw, 8)
        for index in range(main_count):
            entry_offset = 16 + index * 24
            if struct.unpack_from("<I", missing_main_raw, entry_offset)[0] == 133:
                struct.pack_into("<I", missing_main_raw, entry_offset, 158)
                break
        missing_main_pack = Path(directory) / "missing_season_variant.pak"
        missing_main_pack.write_bytes(missing_main_raw)
        missing_main = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-main-row", "0",
             "--setup-main-right", "1", "--frames", "200", "--rom", str(rom),
             "--assets", str(missing_main_pack)],
            text=True, capture_output=True, check=True,
        )
        if "mode=0 style=0 level=0 quarter=3" not in missing_main.stdout or \
                "DSP menu SFX SRCN $1A" in missing_main.stdout:
            raise AssertionError("missing main value asset used fallback graphics/state")

        # The main-page copier must clear the full old value cell but sample
        # only the new word's measured glyph+shadow span. Inject an obvious
        # stale tile near the right edge of Season's captured cell: the old
        # 110-pixel overlay exposed it, while $81:9756-style span copying must
        # leave the clean rendered frame unchanged.
        stale_tail_raw = bytearray(pack.read_bytes())
        _, stale_tail_count = struct.unpack_from("<II", stale_tail_raw, 8)
        for index in range(stale_tail_count):
            entry_offset = 16 + index * 24
            asset_id, asset_offset = struct.unpack_from("<II", stale_tail_raw,
                                                         entry_offset)
            if asset_id == 133:
                # The first sampled row (71) belongs to an empty tile row.
                # Use the next tile row, which actually contains Season ink.
                tile_row = (70 + 5) // 8
                source_entry = (tile_row * 32 + 144 // 8) * 2
                stale_entry = (tile_row * 32 + 216 // 8) * 2
                stale_tail_raw[asset_offset + stale_entry:
                               asset_offset + stale_entry + 2] = \
                    stale_tail_raw[asset_offset + source_entry:
                                   asset_offset + source_entry + 2]
                break
        else:
            raise AssertionError("canonical pack is missing Season variant asset 133")
        stale_tail_pack = Path(directory) / "season_stale_shadow_tail.pak"
        stale_tail_pack.write_bytes(stale_tail_raw)
        stale_tail_frame = Path(directory) / "season_stale_shadow_tail.bmp"
        subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-main-row", "0",
             "--setup-main-right", "1", "--frames", "200", "--rom", str(rom),
             "--assets", str(stale_tail_pack), "--dump-frame", str(stale_tail_frame)],
            text=True, capture_output=True, check=True,
        )
        # This is a controlled corruption guard, not an old-configuration
        # screenshot oracle. Compare the identical current input journey
        # against the pristine pack; native canvas parity is checked separately.
        clean_tail_frame = Path(directory) / "season_clean_shadow_tail.bmp"
        subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-main-row", "0",
             "--setup-main-right", "1", "--frames", "200", "--rom", str(rom),
             "--assets", str(pack), "--dump-frame", str(clean_tail_frame)],
            text=True, capture_output=True, check=True,
        )
        if Image.open(stale_tail_frame).convert("RGB").tobytes() != \
                Image.open(clean_tail_frame).convert("RGB").tobytes():
            raise AssertionError("main Setup copied stale pixels beyond Season's shadow span")

        asset_debug = Path(directory) / "asset_debug_options_vram.bmp"
        subprocess.run(
            [str(exe), "--headless", "--asset-debug", "126", "--frames", "1",
             "--rom", str(rom), "--assets", str(pack), "--dump-frame", str(asset_debug)],
            text=True, capture_output=True, check=True,
        )
        asset_debug_hash = hashlib.sha256(
            Image.open(asset_debug).convert("RGB").tobytes()
        ).hexdigest()
        if asset_debug_hash != EXPECTED_ASSET_DEBUGGER_SHA256:
            raise AssertionError("F12 ROM asset debugger rendering changed")

        oam_debug = Path(directory) / "asset_debug_rules_oam.bmp"
        subprocess.run(
            [str(exe), "--headless", "--asset-debug", "128", "--frames", "1",
             "--rom", str(rom), "--assets", str(pack), "--dump-frame", str(oam_debug)],
            text=True, capture_output=True, check=True,
        )
        if hashlib.sha256(Image.open(oam_debug).convert("RGB").tobytes()).hexdigest() != \
                EXPECTED_OAM_DEBUGGER_SHA256:
            raise AssertionError("F12 OAM/OBJ asset reconstruction changed")

        for srcn, expected_hash in EXPECTED_RENDERED_MENU_SFX_SHA256.items():
            output = Path(directory) / f"menu_sfx_{srcn:02x}.wav"
            result = subprocess.run(
                [str(exe), "--headless", "--rom", str(rom), "--assets", str(pack),
                 "--frames", "0", "--menu-sfx-srcn", hex(srcn),
                 "--dump-menu-sfx", str(output)],
                text=True, capture_output=True, check=True,
            )
            pitch, peak = EXPECTED_MENU_SFX_PARAMETERS[srcn]
            if (f"SRCN ${srcn:02X} pitch={pitch} ADSR1/2=$8E/$E0, "
                    f"volume=30/45 DSPVOL=$40 peak={peak}.") not in result.stdout:
                raise AssertionError(f"menu SRCN ${srcn:02X} DSP parameters changed")
            if hashlib.sha256(output.read_bytes()).hexdigest() != expected_hash:
                raise AssertionError(f"menu SRCN ${srcn:02X} PCM changed")
            with wave.open(str(output), "rb") as wav:
                if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(),
                        wav.getnframes()) != (2, 2, 32000, 24000):
                    raise AssertionError(f"menu SRCN ${srcn:02X} WAV shape changed")

        for frame, expected_hash in EXPECTED_RGB_SHA256.items():
            output = Path(directory) / f"setup_{frame}.bmp"
            audio_output = Path(directory) / "setup_runtime.wav"
            command = [str(exe), "--headless", "--setup-only", "--rom", str(rom),
                       "--assets", str(pack), "--frames", str(frame),
                       "--dump-frame", str(output)]
            if frame == min(EXPECTED_RGB_SHA256):
                command.extend(["--dump-audio", str(audio_output)])
            result = subprocess.run(
                command,
                text=True, capture_output=True, check=True,
            )
            match = re.search(
                r"Synthesized Game Setup through ROM BRR/S-DSP: "
                r"9000 frames, 114059 cycle-timed DSP writes, peak=(\d+); "
                rf"seamless host loop {SETUP_LOOP_START}\.\.{SETUP_LOOP_END} enabled",
                result.stdout,
            )
            if not match or int(match.group(1)) == 0:
                raise AssertionError("Game Setup audio was not synthesized from the SPC assets")
            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != expected_hash:
                raise AssertionError(
                    f"Setup transition frame {frame} changed: {actual} != {expected_hash}"
                )
            if frame == min(EXPECTED_RGB_SHA256):
                assert_wav_fingerprint(
                    audio_output, 4800000, EXPECTED_AUDIO_RMS_EIGHTHS,
                    EXPECTED_AUDIO_BAND_PPM, EXPECTED_AUDIO_CHANNEL_RMS,
                    0.9966, 25000, 26000
                )
                _, features, _ = wav_fingerprint(audio_output)
                actual = np.asarray(features["rms_eighths"], dtype=float)
                oracle = np.asarray(MESEN_SETUP_RMS_EIGHTHS, dtype=float)
                active = (actual + oracle) > 0
                correlation = np.corrcoef(actual[active], oracle[active])[0, 1]
                normalized_error = np.mean(np.abs(
                    actual[active] / actual[active].mean() -
                    oracle[active] / oracle[active].mean()
                ))
                if correlation < 0.97 or normalized_error > 0.10:
                    raise AssertionError(
                        "Setup PCM no longer follows the Mesen onset oracle: "
                        f"correlation={correlation:.3f}, error={normalized_error:.3f}"
                    )
                with wave.open(str(audio_output), "rb") as wav:
                    wav.setpos(SETUP_LOOP_START - 1)
                    start = np.frombuffer(wav.readframes(2), dtype="<i2").reshape(2, 2)
                    wav.setpos(SETUP_LOOP_END - 1)
                    end = np.frombuffer(wav.readframes(2), dtype="<i2").reshape(2, 2)
                if not (np.array_equal(start[1], end[1]) and
                        np.array_equal(start[1] - start[0], end[1] - end[0])):
                    raise AssertionError("Setup musical loop seam is no longer sample-continuous")

        # Exercise the real title-dismiss path as well as the direct fixture.
        # Start on title frame 0, take $80:E5C7's snap/hold/fade, preserve the
        # forced-blank loader, and land on the same first visible Setup frame.
        integrated = Path(directory) / "title_to_setup.bmp"
        result = subprocess.run(
            [str(exe), "--headless", "--title-only", "--enter-setup",
             "--rom", str(rom), "--assets", str(pack), "--frames", "243",
             "--dump-frame", str(integrated)],
            text=True, capture_output=True, check=True,
        )
        if "Synthesized Game Setup through ROM BRR/S-DSP" not in result.stdout:
            raise AssertionError("title handoff did not start the Setup SPC path")
        integrated_hash = hashlib.sha256(
            Image.open(integrated).convert("RGB").tobytes()
        ).hexdigest()
        if integrated_hash != EXPECTED_RGB_SHA256[105]:
            raise AssertionError("title-to-Setup integration timing changed")

        # Both $80:E5C7 paths must reach the same first visible Setup frame:
        # 120-frame snap hold while building, and 40-frame hold once complete.
        for press_frame, total_frames in ((100, 343), (1000, 1163)):
            output = Path(directory) / f"title_exit_{press_frame}.bmp"
            result = subprocess.run(
                [str(exe), "--headless", "--title-only", "--title-press",
                 str(press_frame), "--rom", str(rom), "--assets", str(pack),
                 "--frames", str(total_frames), "--dump-frame", str(output)],
                text=True, capture_output=True, check=True,
            )
            if "Synthesized Game Setup through ROM BRR/S-DSP" not in result.stdout:
                raise AssertionError(f"title exit at {press_frame} did not reach Setup")
            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != EXPECTED_RGB_SHA256[105]:
                raise AssertionError(f"title exit timing changed for press {press_frame}")

        # All six HDMA highlight rows plus one wrap back to row zero.
        for down_count, expected_hash in EXPECTED_CURSOR_SHA256.items():
            output = Path(directory) / f"setup_cursor_{down_count}.bmp"
            # Real held/released native words configure the historical
            # Simulation/3-minute state before visiting each highlight row.
            # This is an explicit C input phase, not injected configuration.
            script = Path(directory) / f"cursor_{down_count}.input"
            words = [0] * 200
            for i, word in enumerate((0x400, 0x100, 0x400, 0x400, 0x100, 0x800, 0x800, 0x800)):
                words[162 + 2 * i] = word
            for i in range(down_count):
                words[178 + 2 * i] = 0x400
            script.write_text("".join(f"1 {word:04x}\n" for word in words))
            subprocess.run(
                [str(exe), "--headless", "--setup-only", "--input-script",
                 str(script), "--rom", str(rom), "--assets", str(pack),
                 "--frames", "200", "--dump-frame", str(output)],
                text=True, capture_output=True, check=True,
            )
            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != expected_hash:
                raise AssertionError(f"Setup cursor row {down_count} changed")

        # Main Setup writes four 16-bit values at $7E:16FB + row*2. Verify
        # every non-default state, the shared $49 adjustment sound, and a
        # compact pixel oracle derived from the corresponding Mesen capture.
        expected_states = {
            (0, 1): (1, 1, 0, 0), (0, 2): (2, 1, 0, 0),
            (0, 3): (3, 1, 0, 0), (1, 1): (0, 2, 0, 0),
            (1, 2): (0, 0, 0, 0), (2, 1): (0, 1, 1, 0),
            (2, 2): (0, 1, 2, 0), (3, 1): (0, 1, 0, 1),
            (3, 2): (0, 1, 0, 2), (3, 3): (0, 1, 0, 3),
        }
        for (row, rights), expected_hash in EXPECTED_MAIN_VALUE_RGB_SHA256.items():
            output = Path(directory) / f"setup_main_{row}_{rights}.bmp"
            result = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-main-row", str(row),
                 "--setup-main-right", str(rights), "--rom", str(rom),
                 "--assets", str(pack), "--frames", "200", "--dump-frame", str(output)],
                text=True, capture_output=True, check=True,
            )
            if "DSP menu SFX SRCN $1A" not in result.stdout:
                raise AssertionError(f"main Setup row {row} did not dispatch adjust SFX")
            match = re.search(
                r"\[SETUP MAIN TEST\] row=\d+ mode=(\d+) style=(\d+) "
                r"level=(\d+) quarter=(\d+)", result.stdout,
            )
            if not match or tuple(map(int, match.groups())) != expected_states[(row, rights)]:
                raise AssertionError(f"main Setup row {row} state changed: {result.stdout}")
            image = Image.open(output).convert("RGB")
            if hashlib.sha256(image.tobytes()).hexdigest() != expected_hash:
                raise AssertionError(f"main Setup row {row} value {rights} frame changed")
            top = 70 + row * 18
            glyph = bytearray()
            for y in range(top, top + 16):
                for x in range(138, 248):
                    color = image.getpixel((x, y))
                    if color[0] >= 100 and color[1] >= 50:
                        glyph.extend((x - 138, y - top, *color))
            if hashlib.sha256(glyph).hexdigest() != \
                    MESEN_MAIN_VALUE_GLYPH_SHA256[(row, rights)]:
                raise AssertionError(f"main Setup row {row} glyph differs from Mesen")

        for row, maximum, defaults in ((0, 3, (0, 1, 0, 0)),
                                       (1, 2, (0, 1, 0, 0)),
                                       (2, 2, (0, 1, 0, 0)),
                                       (3, 3, (0, 1, 0, 0))):
            wrap = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-main-row", str(row),
                 "--setup-main-right", str(maximum + 1), "--rom", str(rom),
                 "--assets", str(pack), "--frames", "200"],
                text=True, capture_output=True, check=True,
            )
            match = re.search(r"mode=(\d+) style=(\d+) level=(\d+) quarter=(\d+)",
                              wrap.stdout)
            if not match or tuple(map(int, match.groups())) != defaults:
                raise AssertionError(f"main Setup row {row} no longer wraps")

        left_states = {
            0: (3, 1, 0, 0), 1: (0, 0, 0, 0),
            2: (0, 1, 2, 0), 3: (0, 1, 0, 3),
        }
        for row, expected in left_states.items():
            left = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-main-row", str(row),
                 "--setup-main-left", "1", "--rom", str(rom), "--assets", str(pack),
                 "--frames", "200"],
                text=True, capture_output=True, check=True,
            )
            match = re.search(r"mode=(\d+) style=(\d+) level=(\d+) quarter=(\d+)",
                              left.stdout)
            if not match or tuple(map(int, match.groups())) != expected or \
                    "DSP menu SFX SRCN $1A" not in left.stdout:
                raise AssertionError(f"main Setup row {row} left cycle changed")

        persisted = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-main-row", "3",
             "--setup-main-right", "1", "--setup-menu", "rules",
             "--setup-menu-confirm", "--rom", str(rom), "--assets", str(pack),
             "--frames", "650"],
            text=True, capture_output=True, check=True,
        )
        if "mode=0 style=1 level=0 quarter=1" not in persisted.stdout:
            raise AssertionError("main Setup values did not survive a Rules round trip")

        reentered = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-main-row", "3",
             "--setup-main-right", "1", "--setup-main-confirm", "--setup-reenter", "--rom", str(rom),
             "--assets", str(pack), "--frames", "200"],
            text=True, capture_output=True, check=True,
        )
        if "[SETUP REENTER] mode=0 style=1 level=0 quarter=1" not in reentered.stdout:
            raise AssertionError("session-owned Setup values were reset on scene re-entry")

        # Main edits are working values until confirmation, just like submenu
        # edits. A controlled scene re-entry must not commit abandoned input.
        abandoned = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute",
             "--setup-main-row", "3", "--setup-main-right", "1", "--setup-reenter",
             "--rom", str(rom), "--assets", str(pack), "--frames", "200"],
            text=True, capture_output=True, check=True,
        )
        if "[SETUP REENTER] mode=0 style=0 level=0 quarter=3" not in abandoned.stdout:
            raise AssertionError("uncommitted Main edits leaked across controlled scene re-entry")

        mode_routes = ("TEAM_SELECTION", "SEASON", "PLAYOFFS", "LOAD_SERIES")
        for mode, route in enumerate(mode_routes):
            navigation = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-main-row", "0",
                 "--setup-main-right", str(mode), "--setup-main-confirm",
                 "--rom", str(rom), "--assets", str(pack), "--frames", "250"],
                text=True, capture_output=True, check=True,
            )
            marker = f"Mode confirmed: mode={mode} route={route}"
            if "action=4" not in navigation.stdout or marker not in navigation.stdout or \
                    "DSP menu SFX SRCN $1C" not in navigation.stdout:
                raise AssertionError(f"main mode {mode} emitted the wrong route/action")

        # The settled Rules/Options pages are rendered from the captured ROM
        # VRAM/CGRAM, not recreated screenshots or host fonts.
        for menu, expected_hash in EXPECTED_MENU_RGB_SHA256.items():
            output = Path(directory) / f"setup_{menu}.bmp"
            result = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-menu", menu,
                 "--rom", str(rom), "--assets", str(pack), "--frames", "470",
                 "--dump-frame", str(output)],
                text=True, capture_output=True, check=True,
            )
            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != expected_hash:
                raise AssertionError(f"Set {menu.title()} settled frame changed")
            image = Image.open(output).convert("RGB")
            bar_boxes = ((144, 82, 192, 90), (144, 100, 192, 108)) if menu == "rules" \
                else ((160, 74, 208, 82), (160, 92, 208, 100))
            bar_hash = MESEN_MENU_PRIMITIVE_SHA256[f"{menu}_bar"]
            for box in bar_boxes:
                if hashlib.sha256(image.crop(box).tobytes()).hexdigest() != bar_hash:
                    raise AssertionError(f"Set {menu.title()} bar differs from Mesen")
            if menu == "rules":
                arrow = hashlib.sha256(image.crop((19, 185, 29, 197)).tobytes()).hexdigest()
                if arrow != MESEN_MENU_PRIMITIVE_SHA256["down_arrow"]:
                    raise AssertionError("Set Rules viewport arrow differs from Mesen")

        # Rules opening and return checkpoints use independent synchronous
        # native witnesses with fixed input-idle phase alignment.
        for (direction, frame), expected_hash in EXPECTED_SUBMENU_TRANSITION_SHA256.items():
            output = Path(directory) / f"submenu_{direction}_{frame}.bmp"
            command = [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-menu", "rules"]
            if direction == "close":
                command.extend(["--setup-menu-confirm", "--setup-menu-confirm-delay", "212"])
            else:
                # Same independently fixed input-idle phase alignment as the
                # complete146-frame gate; this never changes game timing.
                command.extend(["--setup-menu-delay", "713"])
            command.extend(["--rom", str(rom), "--assets", str(pack),
                            "--frames", str(frame + (717 if direction == "open" else 232)),
                            "--dump-frame", str(output)])
            result = subprocess.run(command, text=True, capture_output=True, check=True)
            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != expected_hash:
                raise AssertionError(f"submenu {direction} transition frame {frame} changed")
            if direction == "open" and frame == 313 and "transition=1/146" not in result.stdout:
                raise AssertionError("Set Rules final trace state was released before scanout")
            if direction == "open" and frame == 314 and "transition=0/146" not in result.stdout:
                raise AssertionError("Set Rules did not release after its 146-frame ROM build")
            if direction == "close" and frame == 450 and "transition=0/132" not in result.stdout:
                raise AssertionError("submenu return did not honor its 132-frame ROM build")

        # Options has a shorter ROM-authored BG3 construction than Rules.
        # Keep it independent so a future shared-timing shortcut cannot pass.
        options_open_script = legacy_options_script(directory)
        for frame, expected_hash in EXPECTED_OPTIONS_OPEN_TRANSITION_SHA256.items():
            output = Path(directory) / f"options_open_{frame}.bmp"
            result = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--input-script", str(options_open_script),
                 "--rom", str(rom), "--assets", str(pack), "--frames", str(frame),
                 "--dump-frame", str(output), "--debug-state"],
                text=True, capture_output=True, check=True,
            )
            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != expected_hash:
                raise AssertionError(f"Set Options open transition frame {frame} changed")
            if frame == 300 and "TR:OPEN TF:132" not in result.stdout:
                raise AssertionError("Set Options final trace state was released before scanout")
            if frame == 301 and "TR:NONE TF:132" not in result.stdout:
                raise AssertionError("Set Options did not release after its 132-frame ROM build")

        # Options has its own return capture (asset 151), so prove its directed
        # edge independently of the Rules -> Main trace.
        options_return_script = legacy_options_script(directory, confirm=True)
        for frame, expected_hash in EXPECTED_OPTIONS_RETURN_TRANSITION_SHA256.items():
            output = Path(directory) / f"options_return_{frame}.bmp"
            subprocess.run(
                [str(exe), "--headless", "--setup-only", "--input-script", str(options_return_script),
                 "--rom", str(rom), "--assets", str(pack),
                 "--frames", str(frame), "--dump-frame", str(output)],
                text=True, capture_output=True, check=True,
            )
            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != expected_hash:
                raise AssertionError(f"Set Options return transition frame {frame} changed")

        # The CSV is the human-readable counterpart of packed PPU trace 145.
        # It must expose every decoded state with no skipped or duplicated frame.
        trace_csv = Path(directory) / "rules_open_trace.csv"
        traced = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-menu", "rules",
             "--rom", str(rom), "--assets", str(pack), "--frames", "340",
             "--setup-transition-trace", str(trace_csv)],
            text=True, capture_output=True, check=True,
        )
        if "Wrote 147 Setup transition rows" not in traced.stdout:
            raise AssertionError("Setup transition CSV row count was not reported")
        with trace_csv.open(newline="", encoding="ascii") as trace_file:
            rows = list(csv.DictReader(trace_file))
        if len(rows) != 147 or [int(row["transition_frame"]) for row in rows] != \
                list(range(147)):
            raise AssertionError("Setup transition CSV skipped or duplicated a frame")
        if int(rows[0]["trace_frame"]) != -1 or \
                [int(row["trace_frame"]) for row in rows[1:]] != list(range(146)):
            raise AssertionError("Setup transition CSV is not aligned to packed trace frames")
        if any(int(row["route"]) != 1 for row in rows):
            raise AssertionError("Rules transition CSV changed directed route")
        blank_rows = [int(row["transition_frame"]) for row in rows
                      if int(row["forced_blank"])]
        # Canonical native521 asserts blank;522 releases at brightness0;
        # native523..546 assert blank again. Preserve the observed bit7,
        # rather than fitting a broad blank interval to screenshots.
        if blank_rows != [51] + list(range(53, 77)) or \
                len({row["rgb_fnv64"] for row in rows if int(row["forced_blank"])}) != 1:
            raise AssertionError("Rules transition did not preserve $2100 forced blank")

        _, packed_assets = load_pack(pack)
        packed_trace = packed_assets[145]
        packed_states = []
        offset = 16
        for _ in range(146):
            brightness, main, sub, _ = struct.unpack_from("<BBBB", packed_trace, offset)
            state = [brightness, main, sub]
            for layer in range(3):
                state.extend(struct.unpack_from(
                    "<HHHHBB", packed_trace, offset + 4 + layer * 10
                ))
            vram_count, cgram_count = struct.unpack_from("<HH", packed_trace, offset + 34)
            # NBSPPU3 adds a publication byte to VRAM writes only; CGRAM
            # entries retain their original three-byte encoding.
            offset += 38 + vram_count * 4 + cgram_count * 3
            packed_states.append(state)
        csv_fields = (
            "brightness", "main", "sub",
            "bg1h", "bg1v", "bg1map", "bg1chr", "bg1wide", "bg1tall",
            "bg2h", "bg2v", "bg2map", "bg2chr", "bg2wide", "bg2tall",
            "bg3h", "bg3v", "bg3map", "bg3chr", "bg3wide", "bg3tall",
        )
        visible_bg2_origin = int(rows[0]["bg2v"])
        packed_bg2_origin = packed_states[0][10]
        for trace_frame, (row, packed_state) in enumerate(zip(rows[1:], packed_states)):
            actual_state = tuple(int(row[field]) for field in csv_fields)
            expected_state = list(packed_state)
            # $80:A3B8 carries the live page's BG2 phase through the visible
            # exit.  Absolute capture coordinates become authoritative only
            # once $80:A2BF has entered the forced-blank rebuild (CSV t=51).
            if trace_frame + 1 < 51:
                expected_state[10] = (
                    expected_state[10] + visible_bg2_origin - packed_bg2_origin
                ) & 0x3FF
            expected_state = tuple(expected_state)
            if actual_state != expected_state:
                raise AssertionError(
                    f"Setup transition CSV diverged from packed PPU frame {trace_frame}"
                )
        if int(rows[1]["bg2v"]) != visible_bg2_origin:
            raise AssertionError("Rules transition reset BG2 before forced blank")
        if [int(row["bg2v"]) for row in rows[-3:]] != [21, 22, 22]:
            raise AssertionError("Rules transition lost the rebuilt BG2 settle cadence")

        # Prove the handoff from the last packed state back to the steady
        # updater. The ROM holds/increments according to the phase established
        # by $80:A3B8; it never jumps back to the lifetime Setup frame value.
        for menu, checkpoints in {
            # Native616..619:22,22,23,23. Do not preserve the stale delayed
            # trace's extra hold at315; native continuation is tested too.
            "rules": {313: 22, 314: 22, 315: 23, 316: 23},
            "options": {300: 18, 301: 19, 302: 19, 303: 19, 304: 20},
        }.items():
            for frame, expected_bg2v in checkpoints.items():
                route_args = (
                    ["--input-script", str(legacy_options_script(directory))]
                    if menu == "options" else
                    ["--setup-simulation-three-minute", "--setup-menu", menu]
                )
                state = subprocess.run(
                    [str(exe), "--headless", "--setup-only", *route_args,
                     "--rom", str(rom), "--assets", str(pack), "--frames", str(frame if menu == "options" else frame + 20),
                     "--debug-state"],
                    text=True, capture_output=True, check=True,
                ).stdout
                if f"Y2:{expected_bg2v:03d}" not in state:
                    raise AssertionError(
                        f"Set {menu.title()} BG2 cadence changed at frame {frame}"
                    )

        for (menu, row, rights), expected_hash in EXPECTED_MENU_ACTION_SHA256.items():
            output = Path(directory) / f"setup_{menu}_{row}_{rights}.bmp"
            result = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-menu", menu,
                 "--setup-menu-row", str(row), "--setup-menu-right", str(rights),
                 "--rom", str(rom), "--assets", str(pack), "--frames", "470",
                 "--dump-frame", str(output)],
                text=True, capture_output=True, check=True,
            )
            if "DSP menu SFX SRCN $1C" not in result.stdout:
                raise AssertionError(f"Set {menu.title()} did not play confirm SFX")
            if row and "DSP menu SFX SRCN $1B" not in result.stdout:
                raise AssertionError(f"Set {menu.title()} did not play move SFX")
            # The real Main configuration already emits two adjustment SFX.
            if rights and result.stdout.count("DSP menu SFX SRCN $1A") <= 2:
                raise AssertionError(f"Set {menu.title()} did not play adjust SFX")

            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != expected_hash:
                raise AssertionError(
                    f"Set {menu.title()} row {row} action frame changed"
                )
            image = Image.open(output).convert("RGB")
            if menu == "rules" and row == 7:
                offensive_meter = hashlib.sha256(
                    image.crop((144, 82, 192, 90)).tobytes()
                ).hexdigest()
                if offensive_meter != MESEN_MENU_PRIMITIVE_SHA256["rules_bar"]:
                    raise AssertionError(
                        "first Rules scroll did not move Offensive Fouls meter to slot 0"
                    )
                stale_second_meter = any(
                    red > 180 and blue < 100 and
                    (green < 80 or green > 180)
                    for red, green, blue in image.crop((144, 100, 192, 108)).getdata()
                )
                if stale_second_meter:
                    raise AssertionError("Defensive Fouls meter remained after first scroll")
            if menu == "rules" and row >= 8:
                stale_meter = any(
                    red > 180 and blue < 100 and
                    (green < 80 or green > 180)
                    for red, green, blue in image.crop((144, 82, 192, 108)).getdata()
                )
                if stale_meter:
                    raise AssertionError("Rules retained foul meters after both rows left")
            if menu == "options" and row in (5, 6) and rights:
                top = 68 + row * 18
                tail_x = 184 if row == 5 else 190
                stale_text = any(
                    red > 100 or green > 100
                    for red, green, _ in image.crop((tail_x, top, 248, top + 16)).getdata()
                )
                if stale_text:
                    raise AssertionError(
                        f"Set Options row {row} retained the previous value tail"
                    )

        # Original $87:8BA6-$8C18/$81:D327: an accepted Down highlights its
        # opaque arrow interior for exactly15 frames. Native normal-input
        # setup_rules_simulation_arrows_v1 frames700..716: pixel(24,191) is
        # idle(132,115,0), then15x(255,255,231), then idle. C's corresponding
        # accepted press is315, independently of the different entry timing.
        arrow_dir = Path(directory) / "arrow_timer"
        arrow_dir.mkdir()
        subprocess.run([str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-menu", "rules",
            "--setup-menu-row", "1", "--rom", str(rom), "--assets", str(pack),
            "--frames", "350", "--dump-sequence-from", "334", "--dump-sequence-dir", str(arrow_dir)],
            text=True, capture_output=True, check=True, timeout=30)
        for frame in range(314, 331):
            pixel = Image.open(arrow_dir / f"frame_{frame + 20:04d}.bmp").convert("RGB").getpixel((24, 191))
            expected = (255, 255, 231) if 315 <= frame <= 329 else (132, 115, 0)
            if pixel != expected:
                raise AssertionError(f"native arrow press lifetime/palette differs at C{frame}")

        # $7E:16FB is a working copy: an edit must not alter the committed
        # block until Start runs $81:D516 or $82:8CD9/$82:8D0A.
        cases = (
            ("rules", 2, False, 0, 1),
            ("rules", 2, True, 0, 0),
            ("options", 0, False, 31, 30),
            ("options", 0, True, 31, 31),
        )
        for menu, row, confirm, working, committed in cases:
            command = [
                str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-menu", menu,
                "--setup-menu-row", str(row), "--setup-menu-right", "1",
                "--rom", str(rom), "--assets", str(pack), "--frames", "470",
            ]
            if confirm:
                command.append("--setup-menu-confirm")
            result = subprocess.run(command, text=True, capture_output=True, check=True)
            match = re.search(
                rf"option_row={row} working=(\d+) committed=(\d+)", result.stdout
            )
            if not match or tuple(map(int, match.groups())) != (working, committed):
                raise AssertionError(
                    f"Set {menu.title()} working/commit behavior changed: {result.stdout}"
                )
            if result.stdout.count("DSP menu SFX SRCN $1A") != 3:
                raise AssertionError(f"Set {menu.title()} adjustment SFX was not dispatched")

        # Bar rows clamp at their endpoints and do not emit command $49 when
        # the requested direction cannot change the value.
        clamp = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-menu", "rules",
             "--setup-menu-right", "1", "--rom", str(rom), "--assets", str(pack),
             "--frames", "470"],
            text=True, capture_output=True, check=True,
        )
        if "option_row=0 working=45 committed=45" not in clamp.stdout:
            raise AssertionError("Rules slider no longer clamps at 45")
        if clamp.stdout.count("DSP menu SFX SRCN $1A") != 2:
            raise AssertionError("blocked Rules slider adjustment emitted SFX")

        # $82:8DDC -> $87:8C2D applies row 1 immediately. Prove the same
        # SRCN is actually rescaled, not merely that the displayed value moved.
        sfx_peaks = []
        for row in (0, 1):
            volume_run = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-menu", "options",
                 "--setup-menu-row", str(row), "--setup-menu-right", "1",
                 "--rom", str(rom), "--assets", str(pack), "--frames", "470"],
                text=True, capture_output=True, check=True,
            )
            matches = re.findall(
                r"SRCN \$1A pitch=\$05A8 ADSR1/2=\$8E/\$E0, "
                r"volume=(\d+)/45 DSPVOL=\$([0-9A-F]{2}) peak=(\d+)",
                volume_run.stdout,
            )
            if len(matches) != 3:
                raise AssertionError("menu SFX gain telemetry missing")
            # Select the submenu adjustment after the two Main adjustments.
            volume, dsp_volume, peak = matches[-1]
            sfx_peaks.append((int(volume), int(dsp_volume, 16), int(peak)))
        if sfx_peaks[0][:2] != (30, 0x40) or sfx_peaks[1][:2] != (31, 0x42) or \
           sfx_peaks[1][2] <= sfx_peaks[0][2]:
            raise AssertionError(f"SFX Volume did not change PCM gain: {sfx_peaks}")

        # Native $81:D318 clamps Rules at row12. It does not wrap like Options.
        wrapped = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-menu", "rules",
             "--setup-menu-row", "13", "--rom", str(rom), "--assets", str(pack),
             "--frames", "470"],
            text=True, capture_output=True, check=True,
        )
        if "page=1 menu_row=12" not in wrapped.stdout:
            raise AssertionError("Rules submenu cursor no longer clamps at row12")

        # B is consumed but ignored: the working edit remains uncommitted and
        # the page stays open, exactly as the submenu handler does.
        ignored_b = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-simulation-three-minute", "--setup-menu", "rules",
             "--setup-menu-row", "2", "--setup-menu-right", "1", "--setup-menu-b",
             "--rom", str(rom), "--assets", str(pack), "--frames", "470"],
            text=True, capture_output=True, check=True,
        )
        if "page=1 menu_row=2" not in ignored_b.stdout or \
           "option_row=2 working=0 committed=1" not in ignored_b.stdout:
            raise AssertionError("B no longer matches the ROM's ignored behavior")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    check_pack(Path(args.pack))
    check_frames(Path(args.exe), Path(args.rom), Path(args.pack))
    print("[TEST] PASS: C Setup/Rules/Options regression, session values and audio asset checks; independent native transition gates are separate")


if __name__ == "__main__":
    main()

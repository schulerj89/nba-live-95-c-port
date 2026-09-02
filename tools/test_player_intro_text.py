"""Render and fingerprint every Starting Lineup name/number card."""

import argparse
import concurrent.futures
import hashlib
import json
import struct
import subprocess
import tempfile
from pathlib import Path

from PIL import Image


TEAM_COUNT = 29
STARTERS_PER_TEAM = 5
ROSTER_ASSET = 251
ROSTER_HEADER_SIZE = 24
ROSTER_RECORD_SIZE = 64
TEXT_BOX = (76, 164, 240, 184)
EXPECTED_SENTINELS = {
    (1, 2): "PARISH",
    (16, 2): "BENJAMIN",
    (26, 2): "DUCKWORTH",
}

# Frozen after visual and alignment comparison with all 145 verified native
# captures. The full-card digest catches placement, glyph, divider, portrait
# and team/court selection; the text digest makes a name/number regression
# explicit in the error.
EXPECTED_FULL_CARD_SHA256 = \
    "29de6c244a0a859c6904eeda90c9ceedf57f88881d42fe144836ece1de194e8b"
EXPECTED_TEXT_CROP_SHA256 = \
    "17d892a576a199f308ddea3f7a203dd028e3e99fda4f2bc7b9b64c1b37759169"


def load_pack_asset(path, wanted_id):
    raw = path.read_bytes()
    if raw[:8] != b"NBA95PAK":
        raise AssertionError("invalid asset pack magic")
    version, count = struct.unpack_from("<II", raw, 8)
    if version != 31 or 16 + count * 24 > len(raw):
        raise AssertionError("invalid asset pack directory")
    for index in range(count):
        asset_id, offset, size, _width, _height, _flags = struct.unpack_from(
            "<6I", raw, 16 + index * 24)
        if asset_id == wanted_id:
            return raw[offset:offset + size]
    raise AssertionError(f"asset {wanted_id} is missing")


def starter_records(pack):
    roster = load_pack_asset(pack, ROSTER_ASSET)
    expected_size = ROSTER_HEADER_SIZE + TEAM_COUNT * 12 * ROSTER_RECORD_SIZE
    if roster[:8] != b"NBPROST2" or len(roster) != expected_size:
        raise AssertionError("player roster catalog changed")
    records = {}
    sentinels = {}
    for team in range(TEAM_COUNT):
        for slot in range(STARTERS_PER_TEAM):
            offset = ROSTER_HEADER_SIZE + (team * 12 + slot) * ROSTER_RECORD_SIZE
            record = roster[offset:offset + ROSTER_RECORD_SIZE]
            jersey = record[4]
            name = record[32:64].split(b"\0", 1)[0].decode("ascii").upper()
            if not name:
                raise AssertionError(f"blank starter name at team {team}, slot {slot}")
            if jersey == 0xFF:
                sentinels[(team, slot)] = name
                shown_jersey = "00"
            else:
                if jersey >= 100:
                    raise AssertionError(
                        f"three-digit starter jersey {jersey} at team {team}, slot {slot}")
                shown_jersey = str(jersey)
            records[(team, slot)] = (name, jersey, shown_jersey)
    if sentinels != EXPECTED_SENTINELS:
        raise AssertionError(f"lineup jersey sentinels changed: {sentinels}")
    return records


def render_card(exe, rom, pack, output, team, slot):
    command = [
        str(exe), "--headless", "--rom", str(rom), "--assets", str(pack),
        "--player-intro-only", "--player-intro-team", str(team),
        "--player-intro-slot", str(slot), "--frames", "1",
        "--dump-frame", str(output), "--debug-state",
    ]
    result = subprocess.run(command, text=True, capture_output=True, check=False)
    if result.returncode:
        raise AssertionError(result.stdout + result.stderr)
    expected_team = f"TEAM L:03 R:{team:02}"
    expected_card = f"CARD:{slot + 6:02}/10"
    if "SCN:PLAYER_INTRO" not in result.stdout or \
            expected_team not in result.stdout or expected_card not in result.stdout:
        raise AssertionError(
            f"lineup seed missed team {team}, slot {slot}\n{result.stdout}")
    return team, slot, output


def make_contacts(cards, output):
    contacts = []
    for page, first_team in enumerate(range(0, TEAM_COUNT, 6), 1):
        teams = range(first_team, min(first_team + 6, TEAM_COUNT))
        sheet = Image.new("RGB", (256 * STARTERS_PER_TEAM, 224 * len(teams)))
        for row, team in enumerate(teams):
            for slot in range(STARTERS_PER_TEAM):
                sheet.paste(cards[(team, slot)], (slot * 256, row * 224))
        path = output / f"player-intro-text-contact-{page}.png"
        sheet.save(path)
        contacts.append(path.name)
    return contacts


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--pack", required=True)
    parser.add_argument("--output")
    parser.add_argument("--workers", type=int, default=6)
    args = parser.parse_args()
    exe, rom, pack = map(Path, (args.exe, args.rom, args.pack))
    records = starter_records(pack)

    temporary = tempfile.TemporaryDirectory() if not args.output else None
    output = Path(args.output) if args.output else Path(temporary.name)
    raw = output / "raw"
    raw.mkdir(parents=True, exist_ok=True)
    tasks = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.workers)) as pool:
        for team in range(TEAM_COUNT):
            for slot in range(STARTERS_PER_TEAM):
                path = raw / f"team_{team:02}_slot_{slot}.bmp"
                tasks.append(pool.submit(
                    render_card, exe, rom, pack, path, team, slot))
        for task in concurrent.futures.as_completed(tasks):
            task.result()

    full_digest = hashlib.sha256()
    text_digest = hashlib.sha256()
    cards = {}
    for team in range(TEAM_COUNT):
        for slot in range(STARTERS_PER_TEAM):
            path = raw / f"team_{team:02}_slot_{slot}.bmp"
            card = Image.open(path).convert("RGB")
            cards[(team, slot)] = card.copy()
            full_digest.update(card.tobytes())
            text_digest.update(card.crop(TEXT_BOX).tobytes())
    full_value, text_value = full_digest.hexdigest(), text_digest.hexdigest()
    if full_value != EXPECTED_FULL_CARD_SHA256:
        raise AssertionError(
            f"Starting Lineup full-card aggregate changed: {full_value}")
    if text_value != EXPECTED_TEXT_CROP_SHA256:
        raise AssertionError(
            f"Starting Lineup name/number aggregate changed: {text_value}")

    contacts = make_contacts(cards, output) if args.output else []
    if args.output:
        report = {
            "result": "PASS",
            "cards": TEAM_COUNT * STARTERS_PER_TEAM,
            "teams": TEAM_COUNT,
            "starters_per_team": STARTERS_PER_TEAM,
            "sentinel_display": "00",
            "sentinels": [
                {"team": team, "slot": slot, "name": name}
                for (team, slot), name in sorted(EXPECTED_SENTINELS.items())
            ],
            "full_card_sha256": full_value,
            "text_crop_sha256": text_value,
            "contacts": contacts,
        }
        (output / "manifest.json").write_text(
            json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(
        "Player Introduction text smoke passed: "
        f"{TEAM_COUNT * STARTERS_PER_TEAM} cards, {len(EXPECTED_SENTINELS)} native 00 sentinels")


if __name__ == "__main__":
    main()

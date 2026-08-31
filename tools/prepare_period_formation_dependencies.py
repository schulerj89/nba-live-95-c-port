"""Recreate the reviewed private source closure from this checkout, not objects.

Historical paths in the pinned manifest are provenance only. They are never
opened: keys select this repository's include/ and src/ files, except the
explicitly archived asset header retained from before the HUD integration.
"""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST_SHA = "5d8867bdecd62bfb50d2dd0495fa9c5c7e007f3adbac043ac1e30a0263eab609"
ALIAS_SHA = "b036cc57deac2c57fd299909a656be4563ce6633771cc8dbd9cad5a0ab269a5e"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    aliases = (ROOT / "tools/period_formation_role_alias_map_v1.json").read_bytes()
    if digest(aliases) != ALIAS_SHA:
        raise ValueError("Reviewed alias map changed")
    alias_path = ROOT / ".analysis/period-formation-role-alias-map-v1.json"
    if alias_path.exists() and alias_path.read_bytes() != aliases:
        raise ValueError("Existing alias map differs")
    manifest = (ROOT / "tools/period_formation_dependencies_v1.json").read_bytes()
    if digest(manifest) != MANIFEST_SHA:
        raise ValueError("Reviewed dependency manifest changed")
    entries = json.loads(manifest)["files"]
    payloads = {"manifest.json": manifest}
    for key, identity in entries.items():
        if key in ("period_render_tail.c", "period_render_tail.h"):
            relative = Path("src" if key.endswith(".c") else "include") / key
        else:
            relative = Path(key)
        if (len(relative.parts) != 2 or relative.parts[0] not in ("src", "include")
                or relative.suffix not in (".c", ".h")):
            raise ValueError("Unexpected dependency path")
        source = ROOT / relative
        if key == "include/nba_assets.h":
            # Even its old WIP comment is part of the accepted byte identity.
            # Use an explicit tracked archive for this standalone component;
            # the live game continues to compile the current include/ header.
            source = ROOT / "tools/period_formation_dependencies_v1/nba_assets.h"
        data = source.read_bytes()
        if digest(data) != identity["sha256"]:
            raise ValueError("Source differs from reviewed dependency: " + key)
        if relative.name in payloads:
            raise ValueError("Duplicate dependency basename")
        payloads[relative.name] = data
    destination = ROOT / ".analysis/period-formation-dependencies-v1"
    # Validate the complete source closure before writing anything. Existing
    # snapshots must already match; never overwrite an older evidence packet.
    if destination.exists():
        if {p.name for p in destination.iterdir()} != set(payloads):
            raise ValueError("Existing dependency directory has another closure")
        for name, data in payloads.items():
            if (destination / name).read_bytes() != data:
                raise ValueError("Existing dependency changed: " + name)
    else:
        destination.mkdir(parents=True)
        for name, data in payloads.items():
            with (destination / name).open("xb") as output:
                output.write(data)
    if not alias_path.exists():
        with alias_path.open("xb") as output:
            output.write(aliases)
    print("Verified 30 source/header snapshots, pinned manifest and alias map")


if __name__ == "__main__":
    main()

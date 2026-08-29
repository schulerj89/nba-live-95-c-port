"""Audit the overlap-safe verified ledger against retained native captures.

This is an accounting gate, not behavioral evidence. Exact native vectors and
production regressions justify ledger entries; this tool proves that their
merged ranges leave the expected number of captured address positions.
"""

import argparse
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


def load_progress_module():
    spec = importlib.util.spec_from_file_location(
        "nba_progress", ROOT / "tools" / "progress.py"
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


COMPONENTS = (
    ("bank00_bootstrap_vectors", 0x008000, 0x00FFFF),
    ("bank80_bootstrap", 0x808000, 0x808159),
    ("bank80_ppu_tail", 0x808BF3, 0x808BFD),
    ("bank80_object_text_tail", 0x80BF01, 0x80BFFF),
    ("bank80_dispatch_table", 0x80C000, 0x80C467),
    ("bank80_resource_prelude", 0x80C468, 0x80C5AA),
    ("bank80_controller_tail", 0x80CE8E, 0x80CEFD),
    ("bank80_scene_tail", 0x80F101, 0x80FFFF),
    ("bank81_text_menu", 0x818000, 0x81AFFF),
    ("bank81_player_setup", 0x81B000, 0x81BFFF),
    ("bank81_transitions", 0x81C000, 0x81CFFF),
    ("bank81_rules", 0x81D000, 0x81DFFF),
    ("bank81_presentation_tail", 0x81E000, 0x81FFFF),
    ("bank82_scene_tail_d", 0x82D000, 0x82DFFF),
    ("bank82_scene_tail_e", 0x82E000, 0x82EFFF),
    ("bank82_ea_audio_tail", 0x82F000, 0x82FFFF),
    ("bank83_presentation_prefix", 0x838000, 0x83BFFF),
    ("bank84_gameplay_tables", 0x848000, 0x84BFFF),
    ("bank84_animation_dispatch", 0x84C000, 0x84DFFF),
    ("bank84_roster_graphics", 0x84E000, 0x84FFFF),
)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--expect-pending", type=int, required=True)
    args = parser.parse_args()
    progress = load_progress_module()
    executed = progress.load_executed(ROOT / ".analysis")
    if not executed:
        raise SystemExit("No retained exec_*.txt captures were found")
    verified, _ = progress.load_verified(ROOT / "docs")
    pending = progress.subtract(executed, verified)
    pending_total = progress.total(pending)
    accounted = 0
    for name, first, last in COMPONENTS:
        count = progress.total(progress.intersect(pending, [(first, last)]))
        accounted += count
        if count:
            print(f"[CLOSURE] {name} {count}")
    print(
        f"[CLOSURE] executed={progress.total(executed)} "
        f"verified={progress.total(progress.intersect(executed, verified))} "
        f"pending={pending_total} accounted={accounted}"
    )
    if accounted != pending_total:
        raise SystemExit(
            f"Closure component map missed {pending_total - accounted} positions"
        )
    if pending_total != args.expect_pending:
        raise SystemExit(
            f"Expected {args.expect_pending} pending positions, found {pending_total}"
        )


if __name__ == "__main__":
    main()

"""Negative integrity checks for the retained, independently captured witness.

These tests validate the verifier, not game/ROM equivalence. The positive
witness is the retained native capture; none of its state/RGB values is
generated from the C renderer being tested.
"""
import copy
import tempfile
import unittest
from pathlib import Path

from setup_transition_capture import read_ppu_states, read_rgb_flags, strict_json
from test_setup_rules_reveal import (STATE_KEYS, canonical_manifest_digest,
                                   read_port_trace, validate_witness)
from test_setup_rules_settled import (read_final_state,
                                    validate as validate_settled)
from test_setup_rules_return import (read_trace as read_return_trace,
                                    validate as validate_return)


ROOT = Path(__file__).resolve().parents[1]


class SetupTransitionVerifierIntegrity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.witness = strict_json((ROOT / "tests/fixtures/setup-rules-reveal-native.json").read_text())

    def rejected_witness(self, mutate, *, rehash_manifest=False):
        witness = copy.deepcopy(self.witness)
        mutate(witness)
        if rehash_manifest:
            witness["native_manifest_canonical_sha256"] = canonical_manifest_digest(
                witness["native_manifest"])
        with self.assertRaises((ValueError, TypeError)):
            validate_witness(witness)

    def test_complete_native_witness_is_valid(self):
        self.assertEqual(len(validate_witness(copy.deepcopy(self.witness))["rows"]), 71)

    def test_whole_open_contract_cannot_drop_frames_or_change_alignment(self):
        original = strict_json((ROOT / "tests/fixtures/setup-rules-open-native.json").read_text())
        self.assertEqual(len(validate_witness(copy.deepcopy(original))["rows"]), 147)
        mutations = [lambda w: w.__setitem__("contract", "reveal"),
                     lambda w: w.pop("contract"),
                     lambda w: w["rows"].pop(0),
                     lambda w: w["rows"].__setitem__(1, copy.deepcopy(w["rows"][0])),
                     lambda w: w["rows"].reverse(),
                     lambda w: w["rows"][0].__setitem__("port_step", w["rows"][0]["port_step"] + 1),
                     lambda w: w["rows"][0].__setitem__("native_frame", float(w["rows"][0]["native_frame"]))]
        for index, mutate in enumerate(mutations):
            witness = copy.deepcopy(original)
            mutate(witness)
            with self.subTest(mutation=index), self.assertRaises((ValueError, TypeError)):
                validate_witness(witness)

    def test_whole_open_requires_attested_dispatch_state(self):
        witness = strict_json((ROOT / "tests/fixtures/setup-rules-open-native.json").read_text())
        self.assertEqual(witness["rows"][0]["native_frame"], 470)
        witness["native_manifest"]["artifacts"]["files"].pop("dispatch_ppu_states.txt")
        witness["native_manifest_canonical_sha256"] = canonical_manifest_digest(witness["native_manifest"])
        with self.assertRaises(ValueError):
            validate_witness(witness)

    def test_dropped_duplicate_and_reordered_frames_are_rejected(self):
        for position in (0, 35, 70):
            with self.subTest(drop=position):
                self.rejected_witness(lambda w: w["rows"].pop(position))
        self.rejected_witness(lambda w: w["rows"].__setitem__(1, copy.deepcopy(w["rows"][0])))
        self.rejected_witness(lambda w: w["rows"].reverse())

    def test_all_required_ppu_fields_are_required(self):
        for key in self.witness["rows"][0]["state"]:
            with self.subTest(missing_field=key):
                self.rejected_witness(lambda w: w["rows"][0]["state"].pop(key))

    def test_invalid_or_missing_native_hashes_are_rejected(self):
        for value in ("", "0" * 63, "g" * 64, None, 5):
            with self.subTest(hash=value):
                self.rejected_witness(lambda w: w["rows"][0].__setitem__("rgb_sha256", value))

    def test_state_scalars_cannot_be_boolean_float_or_strings(self):
        for value in (True, False, 15.0, "15", None):
            with self.subTest(value=value):
                self.rejected_witness(lambda w: w["rows"][0]["state"].__setitem__("brightness", value))

    def test_manifest_edits_cannot_reuse_previous_digest(self):
        self.rejected_witness(lambda w: w["native_manifest"].__setitem__("kind", "C-generated"))
        self.rejected_witness(lambda w: w.__setitem__("native_manifest_canonical_sha256", "0" * 64))

    def test_wrong_rom_and_frame_skipping_are_rejected_even_with_fresh_digest(self):
        self.rejected_witness(
            lambda w: w["native_manifest"]["sources"]["rom"].__setitem__("sha256", "0" * 64),
            rehash_manifest=True)
        self.rejected_witness(
            lambda w: w["native_manifest"]["isolation"]["settings"]["Snes"].__setitem__(
                "DisableFrameSkipping", False), rehash_manifest=True)

    def test_duplicate_json_keys_are_rejected_at_any_level(self):
        for text in ('{"schema":1,"schema":1}', '{"nested":{"brightness":1,"brightness":2}}'):
            with self.subTest(text=text), self.assertRaises(ValueError):
                strict_json(text)

    def test_native_ppu_rows_reject_shape_order_and_range_corruption(self):
        # A literal parser specimen, not a game-state oracle or renderer output.
        first = "546 15 3 0 0 1023 0 12288 1 0 0 1023 2048 4096 1 0 0 1023 3072 16384 0 0"
        second = first.replace("546 ", "547 ", 1)
        cases = {
            "duplicate": first + "\n" + first,
            "reordered": second + "\n" + first,
            "trailing_column": first + " 0",
            "missing_column": first.rsplit(" ", 1)[0],
            "negative_scroll": first.replace("0 1023 0", "-1 1023 0", 1),
            "invalid_brightness": first.replace("546 15", "546 16", 1),
            "float": first.replace("546 15", "546 15.0", 1),
        }
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "states.txt"
            path.write_text(first + "\n" + second + "\n")
            self.assertEqual(list(read_ppu_states(path)), [546, 547])
            for name, text in cases.items():
                with self.subTest(case=name), self.assertRaises(ValueError):
                    path.write_text(text + "\n")
                    read_ppu_states(path)

    def test_native_rgb_flags_reject_duplicates_and_malformed_rows(self):
        header = "name,forced_blank,brightness,main,sub\n"
        row = "open_step_546.png,1,15,3,0\n"
        cases = {
            "duplicate": header + row + row,
            "extra_column": header + row.rstrip() + ",0\n",
            "missing_column": header + "open_step_546.png,1,15,3\n",
            "nonboolean_blank": header + "open_step_546.png,2,15,3,0\n",
            "negative_main": header + "open_step_546.png,1,15,-1,0\n",
            "wrong_header": header.replace("forced_blank", "blank") + row,
        }
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "flags.csv"
            path.write_text(header + row)
            self.assertEqual(read_rgb_flags(path)["open_step_546.png"]["forced_blank"], 1)
            for name, text in cases.items():
                with self.subTest(case=name), self.assertRaises(ValueError):
                    path.write_text(text)
                    read_rgb_flags(path)

    def test_port_trace_rejects_dropped_duplicate_reordered_and_malformed_rows(self):
        # Parser specimens only; these zeroes are never used as game expectations.
        fields = ["step", "forced_blank", *STATE_KEYS]
        header = ",".join(fields)
        rows = [str(step) + ",0" * (len(fields) - 1) for step in range(167, 314)]
        cases = {
            "dropped": [header, *rows[:70], *rows[71:]],
            "duplicate": [header, *rows, rows[-1]],
            "reordered": [header, rows[1], rows[0], *rows[2:]],
            "empty": [header],
            "duplicate_header": [header + ",step", *[row + ",0" for row in rows]],
            "missing_header": [",".join(fields[:-1]), *[row.rsplit(",", 1)[0] for row in rows]],
            "missing_field": [header, rows[0].rsplit(",", 1)[0], *rows[1:]],
            "trailing_field": [header, rows[0] + ",0", *rows[1:]],
        }
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "port.csv"
            path.write_text("\n".join([header, *rows]) + "\n")
            self.assertEqual(list(read_port_trace(path)), list(range(167, 314)))
            for name, lines in cases.items():
                with self.subTest(case=name), self.assertRaises(ValueError):
                    path.write_text("\n".join(lines) + "\n")
                    read_port_trace(path)


class SettledTransitionVerifierIntegrity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hold = strict_json((ROOT / "tests/fixtures/setup-rules-settled-native.json").read_text())
        cls.variants = [strict_json((ROOT / f"tests/fixtures/setup-rules-row{row}-native.json").read_text())
                        for row in (2, 7, 9, 12)]

    def rejected(self, original, mutate, *, rehash=False):
        witness = copy.deepcopy(original)
        mutate(witness)
        if rehash:
            witness["native_manifest_canonical_sha256"] = canonical_manifest_digest(witness["native_manifest"])
        with self.assertRaises((ValueError, TypeError)):
            validate_settled(witness)

    def test_native_hold_and_four_ui_witnesses_are_valid(self):
        self.assertEqual(len(validate_settled(copy.deepcopy(self.hold))["rows"]), 137)
        for witness in self.variants:
            self.assertEqual(len(validate_settled(copy.deepcopy(witness))["rows"]), 1)

    def test_hold_requires_all_consecutive_frames_and_complete_fields(self):
        self.rejected(self.hold, lambda w: w["rows"].pop(70))
        self.rejected(self.hold, lambda w: w["rows"].__setitem__(1, copy.deepcopy(w["rows"][0])))
        self.rejected(self.hold, lambda w: w["rows"].reverse())
        for key in self.hold["rows"][0]:
            with self.subTest(key=key):
                self.rejected(self.hold, lambda w: w["rows"][0].pop(key))

    def test_snapshot_navigation_and_value_effect_must_match_native(self):
        for witness in self.variants:
            self.rejected(witness, lambda w: w.__setitem__("menu_row", (w["menu_row"] + 1) % 13))
            self.rejected(witness, lambda w: w.__setitem__("menu_rights", (w["menu_rights"] + 1) % 3))
            self.rejected(witness, lambda w: w["native_values_after"].__setitem__(
                w["menu_row"], w["native_values_after"][w["menu_row"]] ^ 1))

    def test_native_value_arrays_reject_shape_type_and_out_of_domain_values(self):
        witness = self.variants[0]
        for key in ("native_values_before", "native_values_after"):
            self.rejected(witness, lambda w: w[key].pop())
            self.rejected(witness, lambda w: w[key].__setitem__(0, True))
        # Change both arrays consistently to ensure this tests the domain,
        # not merely before/after arithmetic mismatch.
        for slot, value in ((0, -1), (1, 46), (3, -1), (4, 2)):
            def mutate(w, slot=slot, value=value):
                w["native_values_before"][slot] = value
                w["native_values_after"][slot] = value
            with self.subTest(slot=slot, value=value):
                self.rejected(witness, mutate)

    def test_hold_and_targeted_navigation_are_mutually_exclusive(self):
        def mutate(w):
            w["native_manifest"]["configuration"]["target_row"] = 2
            w["native_manifest"]["configuration"]["target_rights"] = 1
        self.rejected(self.hold, mutate, rehash=True)

    def test_settled_hashes_and_provenance_cannot_be_omitted_or_changed(self):
        for witness in (self.hold, *self.variants):
            self.rejected(witness, lambda w: w["rows"][0].__setitem__("rgb_sha256", ""))
            self.rejected(witness, lambda w: w["native_manifest"].__setitem__("kind", "C generated"))

    def test_final_runtime_parser_rejects_ambiguous_and_malformed_output(self):
        setup = "[SETUP TEST] page=1 menu_row=0 transition=0/146 blank=0 gfx=1"
        ppu = "[DEBUG STATE] PPU B:15 X1:0 X2:0 Y2:68 Y3:0"
        valid = setup + "\n" + ppu + "\n"
        self.assertEqual(read_final_state(valid)[1], (15, 0, 0, 68, 0))
        for broken in (setup, ppu, valid + setup + "\n", valid + ppu + "\n",
                       setup + " page=2\n" + ppu, setup + " malformed\n" + ppu,
                       valid.replace("Y2:68", "Y2:68garbage")):
            with self.subTest(output=broken), self.assertRaises(ValueError):
                read_final_state(broken)


class ReturnTransitionVerifierIntegrity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hold = strict_json((ROOT / "tests/fixtures/setup-rules-return-hold-native.json").read_text())
        cls.custom = strict_json((ROOT / "tests/fixtures/setup-rules-return-custom-row2-native.json").read_text())

    def rejected(self, original, mutate, *, rehash=False):
        witness = copy.deepcopy(original)
        mutate(witness)
        if rehash:
            witness["native_manifest_canonical_sha256"] = canonical_manifest_digest(witness["native_manifest"])
        with self.assertRaises((ValueError, TypeError)):
            validate_return(witness)

    def test_two_native_return_witnesses_are_valid(self):
        for witness in (self.hold, self.custom):
            self.assertEqual(len(validate_return(copy.deepcopy(witness))["rows"]), 171)

    def test_return_requires_attested_dispatch_state(self):
        for witness in (self.hold, self.custom):
            self.assertEqual(witness["rows"][0]["native_frame"], 830)
            self.rejected(witness, lambda w: w["native_manifest"]["artifacts"]["files"].pop(
                "dispatch_ppu_states.txt"), rehash=True)
            self.rejected(witness, lambda w: w["native_manifest"]["artifacts"]["files"][
                "dispatch_ppu_states.txt"].__setitem__("sha256", "0" * 64))

    def test_return_cannot_drop_reorder_or_realign_any_frame(self):
        for original in (self.hold, self.custom):
            for index in (0, 132, 133, 170):
                self.rejected(original, lambda w, index=index: w["rows"].pop(index))
            self.rejected(original, lambda w: w["rows"].reverse())
            self.rejected(original, lambda w: w["rows"].__setitem__(1, copy.deepcopy(w["rows"][0])))
            self.rejected(original, lambda w: w["rows"][0].__setitem__("port_step", 529))
            self.rejected(original, lambda w: w["rows"][0].__setitem__("native_frame", 831.0))

    def test_return_requires_every_ppu_field_and_native_hash(self):
        for key in self.hold["rows"][0]["state"]:
            self.rejected(self.hold, lambda w, key=key: w["rows"][0]["state"].pop(key))
        for key, value in (("brightness", 16), ("forced_blank", True), ("bg2v", 1024),
                           ("bg3chr", 32769), ("main", -1), ("bg3tall", 2)):
            self.rejected(self.hold, lambda w, key=key, value=value: w["rows"][0]["state"].__setitem__(key, value))
        self.rejected(self.hold, lambda w: w["rows"][-1].__setitem__("rgb_sha256", ""))

    def test_return_committed_values_and_source_cannot_be_forged(self):
        for original in (self.hold, self.custom):
            for key in ("main", "rules"):
                for index in range(len(original["native_committed"][key])):
                    self.rejected(original, lambda w, key=key, index=index:
                                  w["native_committed"][key].__setitem__(index, 99))
            self.rejected(original, lambda w: w["native_committed"].__setitem__("sha256", "0" * 64))
            self.rejected(original, lambda w: w["native_committed"].__setitem__("source", "C-generated.bin"))

    def test_return_navigation_metadata_is_strict_and_consistent(self):
        for key, value in (("target_row", -1.0), ("target_rights", False),
                           ("target_row", 2), ("target_rights", 1)):
            self.rejected(self.hold, lambda w, key=key, value=value:
                          w["native_manifest"]["configuration"].__setitem__(key, value), rehash=True)
        self.rejected(self.custom, lambda w: w["native_manifest"]["configuration"].__setitem__(
            "hold_menu_without_value_edits", True), rehash=True)
        self.rejected(self.custom, lambda w: w["native_manifest"]["sources"]["rom"].__setitem__(
            "sha256", "0" * 64), rehash=True)

    def test_return_trace_rejects_malformed_unused_and_compared_rows(self):
        fields = ["step", "forced_blank", *STATE_KEYS]
        header = ",".join(fields)
        steps = list(range(167, 314)) + list(range(527, 660))
        rows = [str(step) + ",0" * (len(fields) - 1) for step in steps]
        cases = {
            "dropped": [header, *rows[:-1]],
            "duplicate": [header, *rows, rows[-1]],
            "reordered": [header, rows[1], rows[0], *rows[2:]],
            "missing_field": [header, rows[0].rsplit(",", 1)[0], *rows[1:]],
            "extra_field": [header, rows[0] + ",0", *rows[1:]],
            "duplicate_header": [header + ",step", *[row + ",0" for row in rows]],
            "bad_opening_brightness": [header, rows[0].replace(",0,0,", ",0,16,", 1), *rows[1:]],
            "bad_return_boolean": [header, *rows[:-1], rows[-1].replace(",0,", ",2,", 1)],
        }
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "return.csv"
            path.write_text("\n".join([header, *rows]) + "\n")
            self.assertEqual(list(read_return_trace(path)), steps)
            for name, lines in cases.items():
                with self.subTest(case=name), self.assertRaises(ValueError):
                    path.write_text("\n".join(lines) + "\n")
                    read_return_trace(path)


if __name__ == "__main__":
    unittest.main()

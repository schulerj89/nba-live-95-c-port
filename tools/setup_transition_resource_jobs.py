"""Native DMA publication coverage for parameterized Setup resource traces.

Snapshot differences omit writes that happened to write the baseline's old
value. Those writes still clear a different live menu. This reader retains
the observed DMA destinations and partial-transfer boundaries; it does not
invent a timing budget or consume emulator-rendered images.
"""
import csv
from pathlib import Path

from setup_transition_capture import digest

JOB_KEYS = ("job", "label", "channel", "mode", "bbus", "source", "size",
            "word_address", "stride", "parity", "increment", "remap", "fill")
SEGMENT_KEYS = ("label", "job", "first", "count")


def rows(path, keys):
    with Path(path).open(newline="", encoding="ascii") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != list(keys):
            raise ValueError("unexpected native resource CSV header")
        result = []
        for row in reader:
            if None in row or any(value is None for value in row.values()):
                raise ValueError("missing/trailing native resource field")
            result.append({key: int(row[key]) for key in keys})
    if not result:
        raise ValueError("empty native resource publication evidence")
    return result


def read_publications(directory, manifest):
    directory = Path(directory)
    for name in ("resource_jobs.csv", "resource_writes.csv", "capture_environment.txt"):
        data = (directory / name).read_bytes()
        attestation = manifest.get("artifacts", {}).get("files", {}).get(name, {})
        if len(data) != attestation.get("bytes") or digest(data) != attestation.get("sha256"):
            raise ValueError(f"native resource provenance mismatch: {name}")
    observed = (directory / "capture_environment.txt").read_text().splitlines()
    if observed != manifest.get("isolation", {}).get("observed_environment") or \
            not observed or observed[0] != "directory=" + directory.resolve().as_posix():
        raise ValueError("native resource capture environment/folder mismatch")
    jobs = {}
    previous_label = -1
    for row in rows(directory / "resource_jobs.csv", JOB_KEYS):
        if row["job"] != len(jobs) + 1 or row["label"] < previous_label or \
                not 0 <= row["label"] <= 10000 or not 0 <= row["channel"] < 8 or \
                not 0 <= row["source"] <= 0xffffff or not 1 <= row["size"] <= 65536 or \
                not 0 <= row["word_address"] <= 65535 or row["increment"] != 1 or row["remap"] != 0:
            raise ValueError("invalid/reordered native DMA job")
        if row["mode"] == 1:
            valid = row["bbus"] == 0x18 and row["stride"] == 1 and row["parity"] == 0 and row["fill"] == -1
        elif row["mode"] == 8:
            valid = row["bbus"] in (0x18, 0x19) and row["stride"] == 2 and \
                    row["parity"] == row["bbus"] - 0x18 and 0 <= row["fill"] <= 255
        else:
            valid = False
        if not valid:
            raise ValueError("unsupported native DMA addressing contract")
        jobs[row["job"]] = row
        previous_label = row["label"]
    segments = rows(directory / "resource_writes.csv", SEGMENT_KEYS)
    progress = {job: 0 for job in jobs}
    previous_label = -1
    previous_job = 0
    for row in segments:
        job = jobs.get(row["job"])
        if job is None or not 0 <= row["label"] <= 10000 or row["label"] < previous_label or row["job"] < previous_job or \
                row["label"] < job["label"] or row["first"] != progress[row["job"]] or \
                row["count"] <= 0 or row["first"] + row["count"] > job["size"]:
            raise ValueError("missing/duplicate/reordered native DMA publication segment")
        progress[row["job"]] += row["count"]
        previous_label, previous_job = row["label"], row["job"]
    if any(progress[number] != job["size"] for number, job in jobs.items()):
        raise ValueError("native DMA publication evidence drops part of a transfer")
    return jobs, segments


def read_native_differences(path, first_frame, last_frame, memory_size):
    """Validate every raw row before selecting the production frame range."""
    result = {}
    previous = (-1, -1)
    for line in Path(path).read_text(encoding="ascii").splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        fields = line.split()
        if len(fields) != 3:
            raise ValueError("missing/trailing native memory-difference field")
        frame, address, value = int(fields[0]), int(fields[1], 16), int(fields[2], 16)
        if not first_frame <= frame <= 10000 or not 0 <= address < memory_size or \
                not 0 <= value <= 255 or (frame, address) <= previous:
            raise ValueError("invalid/duplicate/reordered native memory difference")
        previous = (frame, address)
        if frame <= last_frame:
            result.setdefault(frame - first_frame, []).append((address, value))
    return result


def expand_native_writes(directory, manifest, prefix, first_frame, last_frame, base, differences):
    if prefix not in ("open", "return") or type(first_frame) is not int or type(last_frame) is not int or first_frame > last_frame:
        raise ValueError("invalid native publication route/frame bounds")
    if len(base) != 65536:
        raise ValueError("native publication base must contain all VRAM bytes")
    if type(differences) is not dict:
        raise ValueError("native memory differences must be indexed by relative frame")
    for frame, writes in differences.items():
        if type(frame) is not int or not 0 <= frame <= last_frame - first_frame or type(writes) is not list:
            raise ValueError("invalid native memory-difference frame")
        previous = -1
        for write in writes:
            if not isinstance(write, (tuple, list)) or len(write) != 2:
                raise ValueError("invalid native memory-difference row")
            address, value = write
            if type(address) is not int or type(value) is not int or \
                    not previous < address <= 65535 or not 0 <= value <= 255:
                raise ValueError("invalid/duplicate/reordered native memory-difference byte")
            previous = address
    jobs, segments = read_publications(directory, manifest)
    scopes = {}
    generation = 0
    font_groups = []
    for number, job in jobs.items():
        if not first_frame <= job["label"] <= last_frame:
            continue
        source = job["source"]
        if job["mode"] == 1 and 0x7f2360 <= source < 0x7f4b60 and \
                job["word_address"] * 2 == source - 0x7e9ff0:
            if source == 0x7f2360:
                generation += 1
                font_groups.append([])
            if not font_groups:
                raise ValueError("native font upload starts in the middle of a canvas")
            font_groups[-1].append((source, job["size"]))
            scopes[number] = 1 if prefix == "open" and generation == 1 else 2
    expected_group = [(0x7f2360, 4096), (0x7f3360, 4096), (0x7f4360, 2048)] \
        if prefix == "open" else [(0x7f2360, 4096), (0x7f3360, 1024)]
    if font_groups != [expected_group] * (2 if prefix == "open" else 1):
        raise ValueError("native font publication jobs differ from the translated callers")
    coverage = {}
    for segment in segments:
        frame = segment["label"]
        if not first_frame <= frame <= last_frame:
            continue
        job = jobs[segment["job"]]
        # The current endFrame observer proves completed mode8 fill bytes
        # from the PPU address. Mode1's pending low-byte bus phase has not
        # been independently observed; never export that uncertain boundary.
        if job["mode"] == 1 and (segment["first"] != 0 or segment["count"] != job["size"]):
            raise ValueError("split mode1 DMA needs an independently observed completed-byte boundary")
        addresses = coverage.setdefault(frame - first_frame, {})
        for index in range(segment["first"], segment["first"] + segment["count"]):
            address = (job["word_address"] * 2 + job["parity"] + index * job["stride"]) & 0xffff
            addresses[address] = segment["job"]
    canvas = bytearray(base)
    expanded = {}
    for frame in range(last_frame - first_frame + 1):
        written = coverage.get(frame, {})
        for address, value in differences.get(frame, []):
            if address not in written:
                raise ValueError(f"observed VRAM change lacks a native DMA owner: {frame + first_frame}:{address:04X}")
            canvas[address] = value
        if len(written) > 65535:
            raise ValueError("native frame exceeds the version3 publication count")
        for address, number in written.items():
            if jobs[number]["mode"] == 8 and canvas[address] != jobs[number]["fill"]:
                raise ValueError("native fixed-source fill disagrees with the resulting VRAM byte")
        expanded[frame] = [(address, canvas[address], scopes.get(written[address], 0))
                           for address in sorted(written)]
    return expanded

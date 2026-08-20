"""Small, phase-tolerant PCM fingerprints for regression tests."""

import math
import struct
import wave


def wav_fingerprint(path, seconds=10):
    with wave.open(str(path), "rb") as wav:
        rate = wav.getframerate()
        frames = wav.getnframes()
        channels = wav.getnchannels()
        sample_width = wav.getsampwidth()
        raw = wav.readframes(frames)

    if sample_width != 2 or channels != 2:
        return (rate, frames, channels, sample_width), [], 0
    samples = struct.unpack(f"<{len(raw) // 2}h", raw)
    mono = [(samples[i] + samples[i + 1]) * 0.5
            for i in range(0, len(samples), 2)]
    rms = []
    for second in range(min(seconds, frames // rate)):
        window = mono[second * rate:(second + 1) * rate]
        rms.append(round(math.sqrt(sum(value * value for value in window) /
                                   len(window))))
    return (rate, frames, channels, sample_width), rms, max(map(abs, samples), default=0)


def assert_wav_fingerprint(path, expected_frames, expected_rms,
                           min_peak, max_peak, tolerance=5):
    metadata, actual_rms, peak = wav_fingerprint(path, len(expected_rms))
    expected_metadata = (32000, expected_frames, 2, 2)
    if metadata != expected_metadata:
        raise AssertionError(f"WAV metadata changed: {metadata} != {expected_metadata}")
    if len(actual_rms) != len(expected_rms) or any(
        abs(actual - expected) > tolerance
        for actual, expected in zip(actual_rms, expected_rms)
    ):
        raise AssertionError(f"PCM energy fingerprint changed: {actual_rms}")
    if not min_peak <= peak <= max_peak:
        raise AssertionError(f"PCM peak changed: {peak}")

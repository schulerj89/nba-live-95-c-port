"""Phase-tolerant temporal, spectral, and stereo PCM regression features."""

import wave

import numpy as np


def wav_fingerprint(path, seconds=10):
    with wave.open(str(path), "rb") as wav:
        rate = wav.getframerate()
        frames = wav.getnframes()
        channels = wav.getnchannels()
        sample_width = wav.getsampwidth()
        raw = wav.readframes(frames)

    metadata = (rate, frames, channels, sample_width)
    if sample_width != 2 or channels != 2:
        return metadata, {}, 0

    all_samples = np.frombuffer(raw, dtype="<i2").reshape(-1, 2)
    samples = all_samples[:min(frames, seconds * rate)].astype(np.float64)
    mono = samples.mean(axis=1)

    # Eighth-second windows catch reordered notes/onsets that one-second RMS
    # buckets miss, while remaining insensitive to a few samples of SPC phase.
    rms_eighths = []
    for index in range(seconds * 8):
        begin = index * rate // 8
        end = (index + 1) * rate // 8
        window = mono[begin:end]
        rms_eighths.append(round(np.sqrt(np.mean(window * window))))

    # Coarse spectral energy protects BRR pitch/filter behavior without tying
    # the test to the phase of a fresh emulator snapshot.
    spectrum = np.abs(np.fft.rfft(mono * np.hanning(len(mono)))) ** 2
    frequencies = np.fft.rfftfreq(len(mono), 1.0 / rate)
    edges = (0, 250, 500, 1000, 2000, 4000, 8000, 16001)
    bands = np.array([
        spectrum[(frequencies >= low) & (frequencies < high)].sum()
        for low, high in zip(edges, edges[1:])
    ])
    band_ppm = [round(value * 1_000_000 / bands.sum()) for value in bands]

    channel_rms = [round(np.sqrt(np.mean(samples[:, index] ** 2)))
                   for index in range(2)]
    correlation = round(float(np.corrcoef(samples[:, 0], samples[:, 1])[0, 1]), 4)
    features = {
        "rms_eighths": rms_eighths,
        "band_ppm": band_ppm,
        "channel_rms": channel_rms,
        "correlation": correlation,
    }
    peak = int(np.abs(all_samples.astype(np.int32)).max(initial=0))
    return metadata, features, peak


def _assert_vector_close(actual, expected, tolerance, label):
    if len(actual) != len(expected) or any(
        abs(a - e) > tolerance for a, e in zip(actual, expected)
    ):
        raise AssertionError(f"PCM {label} changed: {actual}")


def assert_wav_fingerprint(path, expected_frames, expected_rms_eighths,
                           expected_band_ppm, expected_channel_rms,
                           expected_correlation, min_peak, max_peak):
    metadata, features, peak = wav_fingerprint(path)
    expected_metadata = (32000, expected_frames, 2, 2)
    if metadata != expected_metadata:
        raise AssertionError(f"WAV metadata changed: {metadata} != {expected_metadata}")
    _assert_vector_close(
        features["rms_eighths"], expected_rms_eighths, 8, "onset/RMS profile"
    )
    _assert_vector_close(
        features["band_ppm"], expected_band_ppm, 2000, "spectral bands"
    )
    _assert_vector_close(
        features["channel_rms"], expected_channel_rms, 10, "stereo energy"
    )
    if abs(features["correlation"] - expected_correlation) > 0.02:
        raise AssertionError(f"PCM stereo correlation changed: {features['correlation']}")
    if not min_peak <= peak <= max_peak:
        raise AssertionError(f"PCM peak changed: {peak}")

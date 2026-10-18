#----------------------
# | audio_compare.py
# | Description: Compares a Dreamcast capture of a song with a DuckStation dump of the same song: alignment, loudness envelope, per-channel level, loop period and spectral centroid.
# | Author: suinevere
# | Dependencies: numpy, wave, audio_capture.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import json
import sys
import wave

import numpy

#----------------------
# | RATE / ENVELOPE_SECONDS / LOOP_ENVELOPE_SECONDS / MAX_LAG_SECONDS / MIN_LOOP_SECONDS / ACTIVE_DB
# | Description: Working rate, envelope window for alignment and level, finer window for loop periods, alignment search range, shortest loop considered, and the level below which a window counts as silent.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RATE = 44100
ENVELOPE_SECONDS = 0.05
LOOP_ENVELOPE_SECONDS = 0.01
MAX_LAG_SECONDS = 5.0
MIN_LOOP_SECONDS = 5.0
ACTIVE_DB = -50.0


#----------------------
# | read_wav
# | Description: A 16-bit WAV as float frames in -1..1 with two channels at RATE; mono is duplicated and other rates are resampled linearly.
# | Author: suinevere
# | Dependencies: wave, numpy
# | Globals: N/A
# | Params: path -- WAV file
# | Returns: (frames, 2) float array
#----------------------
def read_wav(path):
    with wave.open(path, "rb") as handle:
        channels, rate, frames = handle.getnchannels(), handle.getframerate(), handle.getnframes()
        data = numpy.frombuffer(handle.readframes(frames), dtype="<i2").astype(numpy.float64) / 32768.0
    data = data.reshape(-1, channels)
    if channels == 1:
        data = numpy.repeat(data, 2, axis=1)
    data = data[:, :2]
    if rate != RATE:
        old = numpy.arange(len(data)) / rate
        new = numpy.arange(int(len(data) * RATE / rate)) / RATE
        data = numpy.stack([numpy.interp(new, old, data[:, c]) for c in range(2)], axis=1)
    return data


#----------------------
# | envelope
# | Description: RMS of each window of a signal.
# | Author: suinevere
# | Dependencies: numpy
# | Globals: N/A
# | Params: signal -- 1-D float array, seconds -- window length
# | Returns: float array
#----------------------
def envelope(signal, seconds):
    size = int(RATE * seconds)
    count = len(signal) // size
    return numpy.sqrt(numpy.mean(signal[:count * size].reshape(count, size) ** 2, axis=1))


#----------------------
# | to_db
# | Description: Amplitudes in dBFS, floored at -120.
# | Author: suinevere
# | Dependencies: numpy
# | Globals: N/A
# | Params: values -- amplitudes
# | Returns: float array
#----------------------
def to_db(values):
    return 20.0 * numpy.log10(numpy.maximum(values, 1e-6))


#----------------------
# | best_lag
# | Description: The shift in windows of b against a with the highest correlation of their mean-removed envelopes, within max_lag windows; positive means b is late.
# | Author: suinevere
# | Dependencies: numpy
# | Globals: N/A
# | Params: a / b -- envelopes, max_lag -- windows
# | Returns: lag in windows
#----------------------
def best_lag(a, b, max_lag):
    best, best_score = 0, -numpy.inf
    for lag in range(-max_lag, max_lag + 1):
        x, y = (a[:len(a) - lag], b[lag:]) if lag >= 0 else (a[-lag:], b[:len(b) + lag])
        n = min(len(x), len(y))
        if n < 10:
            continue
        x, y = x[:n] - x[:n].mean(), y[:n] - y[:n].mean()
        score = float(numpy.dot(x, y) / (numpy.linalg.norm(x) * numpy.linalg.norm(y) + 1e-12))
        if score > best_score:
            best, best_score = lag, score
    return best


#----------------------
# | aligned
# | Description: The overlapping parts of two sequences after shifting b by lag.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: a / b -- arrays, lag -- shift of b in elements
# | Returns: (a part, b part) of equal length
#----------------------
def aligned(a, b, lag):
    x, y = (a[:len(a) - lag] if lag > 0 else a[-lag:], b[lag:] if lag > 0 else b[:len(b) + lag])
    n = min(len(x), len(y))
    return x[:n], y[:n]


#----------------------
# | loop_period
# | Description: A song's loop period in seconds: the strongest autocorrelation peak of its fine envelope at lags from MIN_LOOP_SECONDS to half its length, refined by a parabola; None when no lag correlates above 0.5.
# | Author: suinevere
# | Dependencies: envelope, numpy
# | Globals: N/A
# | Params: mono -- 1-D float array
# | Returns: seconds or None
#----------------------
def loop_period(mono):
    env = envelope(mono, LOOP_ENVELOPE_SECONDS)
    env = env - env.mean()
    first = int(MIN_LOOP_SECONDS / LOOP_ENVELOPE_SECONDS)
    last = len(env) // 2
    if last <= first + 2:
        return None
    scores = numpy.array([numpy.dot(env[:-lag], env[lag:]) / (numpy.linalg.norm(env[:-lag]) * numpy.linalg.norm(env[lag:]) + 1e-12)
                          for lag in range(first, last)])
    peak = int(numpy.argmax(scores))
    if scores[peak] < 0.5:
        return None
    offset = 0.0
    if 0 < peak < len(scores) - 1:
        left, mid, right = scores[peak - 1], scores[peak], scores[peak + 1]
        denominator = left - 2 * mid + right
        if denominator != 0:
            offset = 0.5 * (left - right) / denominator
    return (first + peak + offset) * LOOP_ENVELOPE_SECONDS


#----------------------
# | centroid
# | Description: Spectral centroid in Hz of a signal's power spectrum averaged over 4096-frame blocks.
# | Author: suinevere
# | Dependencies: numpy
# | Globals: N/A
# | Params: mono -- 1-D float array
# | Returns: Hz
#----------------------
def centroid(mono):
    size = 4096
    count = len(mono) // size
    power = (numpy.abs(numpy.fft.rfft(mono[:count * size].reshape(count, size) * numpy.hanning(size), axis=1)) ** 2).sum(axis=0)
    freqs = numpy.fft.rfftfreq(size, 1.0 / RATE)
    return float((freqs * power).sum() / (power.sum() + 1e-24))


#----------------------
# | CHROMA_HOP_SECONDS / CHROMA_WINDOW
# | Description: Time between chroma frames, and the FFT size each frame takes.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CHROMA_HOP_SECONDS = 0.1
CHROMA_WINDOW = 8192


#----------------------
# | chroma
# | Description: Pitch-class profile of each frame (80 Hz to 3.5 kHz), unit length with the frame mean removed; silent frames are zero.
# | Author: suinevere
# | Dependencies: CHROMA_HOP_SECONDS, CHROMA_WINDOW, RATE, numpy
# | Globals: N/A
# | Params: mono -- 1-D float array
# | Returns: (12, frames) float array
#----------------------
def chroma(mono):
    hop = int(RATE * CHROMA_HOP_SECONDS)
    count = max(0, (len(mono) - CHROMA_WINDOW) // hop + 1)
    freqs = numpy.fft.rfftfreq(CHROMA_WINDOW, 1.0 / RATE)
    keep = (freqs > 80) & (freqs < 3500)
    pitch = numpy.round(12 * numpy.log2(freqs[keep] / 440.0)).astype(int) % 12
    window = numpy.hanning(CHROMA_WINDOW)
    out = numpy.zeros((12, count))
    for i in range(count):
        mag = numpy.abs(numpy.fft.rfft(mono[i * hop:i * hop + CHROMA_WINDOW] * window))[keep]
        out[:, i] = numpy.bincount(pitch, weights=mag, minlength=12)
    energy = out.sum(axis=0)
    out = numpy.log1p(out * 100)
    out -= out.mean(axis=0, keepdims=True)
    out /= numpy.linalg.norm(out, axis=0, keepdims=True) + 1e-12
    out[:, energy < max(energy.max() if count else 0, 1e-9) * 1e-3] = 0
    return out


#----------------------
# | locate_notes
# | Description: Where a song's notes best match inside a capture, by mean chroma similarity over the song's sounding frames; independent of level and timbre.
# | Author: suinevere
# | Dependencies: chroma, CHROMA_HOP_SECONDS, numpy
# | Globals: N/A
# | Params: song / capture -- 1-D float arrays
# | Returns: (seconds into the capture, score -1..1)
#----------------------
def locate_notes(song, capture):
    a, b = chroma(song), chroma(capture)
    sounding = numpy.flatnonzero(numpy.abs(a).sum(axis=0) > 0)
    span = b.shape[1] - a.shape[1] + 1
    if span <= 0 or len(sounding) == 0:
        return 0.0, -1.0
    scores = numpy.zeros(span)
    for k in sounding:
        scores += (a[:, k:k + 1] * b[:, k:k + span]).sum(axis=0)
    scores /= len(sounding)
    best = int(numpy.argmax(scores))
    return best * CHROMA_HOP_SECONDS, float(scores[best])


#----------------------
# | locate
# | Description: Where a song starts inside a longer capture: the offset with the highest normalized correlation of their loudness envelopes.
# | Author: suinevere
# | Dependencies: envelope, numpy
# | Globals: N/A
# | Params: song / capture -- 1-D float arrays
# | Returns: seconds into the capture
#----------------------
def locate(song, capture):
    a = envelope(song, ENVELOPE_SECONDS)
    b = envelope(capture, ENVELOPE_SECONDS)
    a = a - a.mean()
    a_norm = numpy.linalg.norm(a) + 1e-12
    best, best_score = 0, -numpy.inf
    for offset in range(0, max(1, len(b) - len(a) + 1)):
        window = b[offset:offset + len(a)]
        window = window - window.mean()
        score = float(numpy.dot(a[:len(window)], window) / (a_norm * (numpy.linalg.norm(window) + 1e-12)))
        if score > best_score:
            best, best_score = offset, score
    return best * ENVELOPE_SECONDS


#----------------------
# | compare
# | Description: Metrics of a Dreamcast capture against a PS1 dump: lag_s (Dreamcast late is positive), envelope_corr after alignment, level_db_left/right (median Dreamcast minus PS1 over windows where both play), loop_period_ratio (Dreamcast over PS1, None when either has no loop), centroid_ratio.
# | Author: suinevere
# | Dependencies: read_wav, envelope, best_lag, aligned, to_db, loop_period, centroid
# | Globals: N/A
# | Params: ps1_wav / dc_wav -- WAV paths
# | Returns: dict
#----------------------
def compare(ps1_wav, dc_wav):
    ps1, dc = read_wav(ps1_wav), read_wav(dc_wav)
    ps1_mono, dc_mono = ps1.mean(axis=1), dc.mean(axis=1)
    ps1_env, dc_env = envelope(ps1_mono, ENVELOPE_SECONDS), envelope(dc_mono, ENVELOPE_SECONDS)
    lag = best_lag(ps1_env, dc_env, int(MAX_LAG_SECONDS / ENVELOPE_SECONDS))
    x, y = aligned(ps1_env, dc_env, lag)
    result = {"lag_s": lag * ENVELOPE_SECONDS, "envelope_corr": float(numpy.corrcoef(x, y)[0, 1])}
    for side, name in ((0, "left"), (1, "right")):
        a, b = aligned(to_db(envelope(ps1[:, side], ENVELOPE_SECONDS)), to_db(envelope(dc[:, side], ENVELOPE_SECONDS)), lag)
        active = (a > ACTIVE_DB) & (b > ACTIVE_DB)
        result["level_db_" + name] = float(numpy.median(b[active] - a[active])) if active.any() else None
    ps1_loop, dc_loop = loop_period(ps1_mono), loop_period(dc_mono)
    result["loop_period_ratio"] = None if ps1_loop is None or dc_loop is None else dc_loop / ps1_loop
    result["centroid_ratio"] = centroid(dc_mono) / centroid(ps1_mono)
    return result


#----------------------
# | main
# | Description: Command-line entry: prints compare's metrics as JSON.
# | Author: suinevere
# | Dependencies: compare
# | Globals: N/A
# | Params: argv -- argument list
# | Returns: process exit code
#----------------------
def main(argv):
    parser = argparse.ArgumentParser(description="Compare a Dreamcast song capture with a DuckStation dump")
    parser.add_argument("ps1_wav")
    parser.add_argument("dc_wav")
    args = parser.parse_args(argv)
    print(json.dumps(compare(args.ps1_wav, args.dc_wav), indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

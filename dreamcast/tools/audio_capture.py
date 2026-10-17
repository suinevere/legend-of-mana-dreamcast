#----------------------
# | audio_capture.py
# | Description: Reads Flycast's raw audio capture (s16le stereo 44100 Hz from SDL's disk driver), writes it as WAV, and measures it in short windows: level per channel and dominant frequency.
# | Author: suinevere
# | Dependencies: numpy, wave, flycast_run.py (audio_out)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import sys
import wave

import numpy

#----------------------
# | RATE / CHANNELS
# | Description: The capture's sample rate and channel count.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RATE = 44100
CHANNELS = 2

#----------------------
# | SILENCE_DB
# | Description: Level reported for an all-zero window.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SILENCE_DB = -120.0


#----------------------
# | read_raw
# | Description: A raw capture as a (frames, 2) int16 array; a trailing partial frame is dropped.
# | Author: suinevere
# | Dependencies: numpy
# | Globals: N/A
# | Params: path -- capture file
# | Returns: numpy array
#----------------------
def read_raw(path):
    data = numpy.fromfile(path, dtype="<i2")
    frames = len(data) // CHANNELS
    return data[:frames * CHANNELS].reshape(frames, CHANNELS)


#----------------------
# | write_wav
# | Description: Writes a (frames, 2) int16 array as a 44100 Hz stereo WAV.
# | Author: suinevere
# | Dependencies: wave
# | Globals: N/A
# | Params: path -- output, samples -- array
# | Returns: N/A
#----------------------
def write_wav(path, samples):
    with wave.open(path, "wb") as handle:
        handle.setnchannels(CHANNELS)
        handle.setsampwidth(2)
        handle.setframerate(RATE)
        handle.writeframes(samples.astype("<i2").tobytes())


#----------------------
# | windows
# | Description: Splits one channel into consecutive windows of a given length in seconds, as float in -1..1.
# | Author: suinevere
# | Dependencies: numpy
# | Globals: N/A
# | Params: channel -- 1-D int16 array, seconds -- window length
# | Returns: (count, window) float array
#----------------------
def windows(channel, seconds):
    size = int(RATE * seconds)
    count = len(channel) // size
    return channel[:count * size].reshape(count, size).astype(numpy.float64) / 32768.0


#----------------------
# | levels_db
# | Description: RMS level of each window in dBFS.
# | Author: suinevere
# | Dependencies: windows, SILENCE_DB
# | Globals: N/A
# | Params: channel -- 1-D int16 array, seconds -- window length
# | Returns: float array
#----------------------
def levels_db(channel, seconds=0.05):
    rms = numpy.sqrt(numpy.mean(windows(channel, seconds) ** 2, axis=1))
    with numpy.errstate(divide="ignore"):
        return numpy.where(rms > 0, 20.0 * numpy.log10(numpy.maximum(rms, 1e-12)), SILENCE_DB)


#----------------------
# | dominant_hz
# | Description: Frequency of the strongest spectral bin in each window (Hann window).
# | Author: suinevere
# | Dependencies: windows
# | Globals: N/A
# | Params: channel -- 1-D int16 array, seconds -- window length
# | Returns: float array
#----------------------
def dominant_hz(channel, seconds=0.05):
    blocks = windows(channel, seconds)
    spectrum = numpy.abs(numpy.fft.rfft(blocks * numpy.hanning(blocks.shape[1]), axis=1))
    spectrum[:, 0] = 0
    return numpy.argmax(spectrum, axis=1) * RATE / blocks.shape[1]


#----------------------
# | tone_db
# | Description: Level of one frequency in each window, in dBFS of a full-scale sine (Hann window, the nearest bin).
# | Author: suinevere
# | Dependencies: windows, SILENCE_DB
# | Globals: N/A
# | Params: channel -- 1-D int16 array, hz -- frequency, seconds -- window length
# | Returns: float array
#----------------------
def tone_db(channel, hz, seconds=0.1):
    blocks = windows(channel, seconds)
    window = numpy.hanning(blocks.shape[1])
    spectrum = numpy.abs(numpy.fft.rfft(blocks * window, axis=1))
    amplitude = 2.0 * spectrum[:, int(round(hz * blocks.shape[1] / RATE))] / window.sum()
    with numpy.errstate(divide="ignore"):
        return numpy.where(amplitude > 0, 20.0 * numpy.log10(numpy.maximum(amplitude, 1e-12)), SILENCE_DB)


#----------------------
# | band_db
# | Description: Energy above a frequency in each window, in dBFS.
# | Author: suinevere
# | Dependencies: windows, SILENCE_DB
# | Globals: N/A
# | Params: channel -- 1-D int16 array, low_hz -- band floor, seconds -- window length
# | Returns: float array
#----------------------
def band_db(channel, low_hz, seconds=0.01):
    blocks = windows(channel, seconds)
    spectrum = numpy.abs(numpy.fft.rfft(blocks, axis=1)) ** 2
    first = int(numpy.ceil(low_hz * blocks.shape[1] / RATE))
    energy = 2.0 * spectrum[:, first:].sum(axis=1) / blocks.shape[1] ** 2
    with numpy.errstate(divide="ignore"):
        return numpy.where(energy > 0, 10.0 * numpy.log10(numpy.maximum(energy, 1e-24)), SILENCE_DB)


#----------------------
# | main
# | Description: Command-line entry: converts a raw capture to WAV.
# | Author: suinevere
# | Dependencies: read_raw, write_wav
# | Globals: N/A
# | Params: argv -- argument list
# | Returns: process exit code
#----------------------
def main(argv):
    parser = argparse.ArgumentParser(description="Convert a Flycast raw audio capture to WAV")
    parser.add_argument("raw")
    parser.add_argument("wav")
    args = parser.parse_args(argv)
    write_wav(args.wav, read_raw(args.raw))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

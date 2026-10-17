#----------------------
# | sweep_check.py
# | Description: Checks a capture of the sound sweep: maps the log's vblank stamps onto capture time with the two 1 kHz marker tones, then finds songs that stay silent after they start and measures the gaps after each stop.
# | Author: suinevere
# | Dependencies: audio_capture.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import re

import numpy

import audio_capture

#----------------------
# | MARKER_HZ / MARKER_DB / MARKER_SECONDS / TONE_WINDOW / LEVEL_WINDOW / ONSET_SECONDS / SOUNDING_DB / GAP_FROM / GAP_TO
# | Description: Marker tone frequency, level and shortest run; window lengths for the marker search and for levels; how soon a song must sound and how loud; and the stretch after each stop measured as the gap.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MARKER_HZ = 1000.0
MARKER_DB = -12.0
MARKER_SECONDS = 0.5
TONE_WINDOW = 0.02
LEVEL_WINDOW = 0.05
ONSET_SECONDS = 1.5
SOUNDING_DB = -40.0
GAP_FROM = 0.5
GAP_TO = 0.7

#----------------------
# | INTRO_SECONDS
# | Description: Songs whose sequences open on rests, with the first sound measured on the Dreamcast (5.3, 29.4 and 9.1 s; 28 repeats 23); each gets that long on top of ONSET_SECONDS.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
INTRO_SECONDS = {23: 5.5, 28: 5.5, 56: 29.5, 61: 9.5}


#----------------------
# | marker_starts
# | Description: Capture times where the marker tone holds for MARKER_SECONDS on either side, one per run.
# | Author: suinevere
# | Dependencies: audio_capture.tone_db
# | Globals: N/A
# | Params: samples -- capture frames
# | Returns: list of seconds
#----------------------
def marker_starts(samples):
    level = numpy.maximum(audio_capture.tone_db(samples[:, 0], MARKER_HZ, TONE_WINDOW),
                          audio_capture.tone_db(samples[:, 1], MARKER_HZ, TONE_WINDOW))
    need = int(MARKER_SECONDS / TONE_WINDOW)
    starts = []
    run = 0
    for index, loud in enumerate(level > MARKER_DB):
        run = run + 1 if loud else 0
        if run == need:
            starts.append((index - need + 1) * TONE_WINDOW)
    return starts


#----------------------
# | stamps
# | Description: (number, vblank) pairs of every log line matching pattern.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: lines -- log lines, pattern -- regex with the vblank as its last group
# | Returns: list of tuples of ints
#----------------------
def stamps(lines, pattern):
    out = []
    for line in lines:
        match = re.search(pattern, line)
        if match:
            out.append(tuple(int(group) for group in match.groups()))
    return out


#----------------------
# | check
# | Description: Aligns the sweep's log with its capture by the first and last marker and reports which songs stayed under SOUNDING_DB for ONSET_SECONDS (plus any INTRO_SECONDS) after starting, plus the median gap level after the stops.
# | Author: suinevere
# | Dependencies: marker_starts, stamps, INTRO_SECONDS, audio_capture.levels_db
# | Globals: N/A
# | Params: lines -- log lines, samples -- capture frames
# | Returns: dict with songs, silent, gap_db and seconds_per_vblank
#----------------------
def check(lines, samples):
    tones = [v for (_, v) in stamps(lines, r"LOM: tone start (\d+) at (\d+)")]
    if len(tones) < 2:
        raise ValueError("need two marker tones in the log, found %d" % len(tones))
    first, last = tones[0], tones[-1]
    found = marker_starts(samples)
    if not found:
        raise ValueError("no marker tone in the capture")
    t0 = found[0]
    t1 = max(found)
    if t1 <= t0:
        raise ValueError("only one marker tone in the capture")
    per_vblank = (t1 - t0) / float(last - first)

    def time(vblank):
        return t0 + (vblank - first) * per_vblank

    level = numpy.maximum(audio_capture.levels_db(samples[:, 0], LEVEL_WINDOW),
                          audio_capture.levels_db(samples[:, 1], LEVEL_WINDOW))

    def loudest(start, end):
        a, b = int(start / LEVEL_WINDOW), int(numpy.ceil(end / LEVEL_WINDOW))
        part = level[max(a, 0):max(b, a + 1)]
        return float(part.max()) if len(part) else audio_capture.SILENCE_DB

    songs = stamps(lines, r"LOM: music (\d+) at (\d+)")
    silent = [r for (r, v) in songs
              if loudest(time(v), time(v) + INTRO_SECONDS.get(r, 0.0) + ONSET_SECONDS) <= SOUNDING_DB]
    gaps = [loudest(time(v) + GAP_FROM, time(v) + GAP_TO) for (v,) in stamps(lines, r"LOM: musicoff at (\d+)")]
    return {"songs": [r for (r, _) in songs], "silent": silent,
            "gap_db": float(numpy.median(gaps)) if gaps else None, "seconds_per_vblank": per_vblank}

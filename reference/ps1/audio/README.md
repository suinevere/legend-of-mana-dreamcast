# PS1 audio references

DuckStation WAV dumps of the US version for `dreamcast/tests/test_sound.py::test_new_game_songs`. They stay local (git-ignored): about 10 MB each.

Record each with DuckStation → Tools → Start Media Capture, from the song's first note for about 60 seconds, then stop. Newer builds have no audio dumping; set the Media Capture settings to capture audio (video optional) at 320 kbps, drop the `.mp4` files here under the names below, and convert them with `ffmpeg -i title.mp4 -vn -af silenceremove=start_periods=1:start_threshold=-50dB -c:a pcm_s16le title.wav`:

| File | Where |
|---|---|
| `title.wav` | Title screen |
| `gname.wav` | Character and name selection (GNAME) |
| `wsel.wav` | Home-location choice (WSEL) |
| `wmap.wav` | World map (WMAP) |
| `field.wav` | The first field scene after placing Home (scene 857) |
| `cursor.wav` | One menu cursor sound, a second or two |

The test finds each song inside one Dreamcast capture of the new-game path by its loudness envelope, then compares alignment, per-channel level, loop period and spectral centroid (`dreamcast/tools/audio_compare.py`).

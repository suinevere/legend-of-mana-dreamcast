# PS1 audio references

PS1 song references for `dreamcast/tests/test_sound.py::test_new_game_songs`. They stay local (git-ignored).

They are cut from Zophar's MP3 render of the game's PSF rip (`fi.zophar.net/soundfiles/playstation-psf/legend-of-mana`), trimmed to how long each song plays on the scripted new-game path:

| File | Where | Track | Command |
|---|---|---|---|
| `title.wav` | Title, GNAME and WSEL (one song) | 102 Nostalgic Song | `ffmpeg -i "102 Nostalgic song.mp3" -af silenceremove=start_periods=1:start_threshold=-50dB -t 25 -ar 44100 -ac 2 -c:a pcm_s16le title.wav` |
| `wmap.wav` | World map (WMAP) | 103 World of Mana | the same with `"103 World of Mana.mp3"`, `-t 10` and `wmap.wav` |

The test finds each song's notes in one Dreamcast capture of the new-game path by chroma (`dreamcast/tools/audio_compare.py` `locate_notes`) and needs a score of at least 0.8; right songs score about 0.88 and wrong tracks at most 0.54. It does not compare level or timbre, since the render is another player's mix and the Dreamcast has no reverb until D8.

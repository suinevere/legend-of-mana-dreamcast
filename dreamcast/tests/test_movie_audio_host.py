#----------------------
# | test_movie_audio_host.py
# | Description: Builds and runs the host C test for movie audio's ring positions, block decoding and AICA stream feeding.
# | Author: suinevere
# | Dependencies: tests/host/test_movie_audio.c, src/movie/movie_audio_dc.c, ../port/sound/spu_sample_map.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import shutil
import subprocess
import tempfile

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)


def test_movie_audio_host_suite():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_movie_audio_test.exe")
    movie = os.path.join(DC, "src", "movie")
    sound = os.path.join(REPO, "port", "sound")
    cmd = [cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-DLOM_HOST_TEST",
           "-I", movie, "-I", sound, "-I", os.path.join(DC, "src", "sound"),
           "-o", exe, os.path.join(DC, "tests", "host", "test_movie_audio.c"), os.path.join(movie, "movie_audio_dc.c"),
           os.path.join(sound, "spu_sample_map.c")]
    subprocess.run(cmd, check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout

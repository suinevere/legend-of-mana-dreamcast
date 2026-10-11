#----------------------
# | test_toolchain.py
# | Description: Checks the lom-dc Docker image provides the KallistiOS compiler and mkdcdisc.
# | Author: suinevere
# | Dependencies: dockerfiles/dreamcast.Dockerfile
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import shutil
import subprocess

import pytest


def docker_run(command):
    if shutil.which("docker") is None:
        pytest.skip("no docker")
    probe = subprocess.run(["docker", "image", "inspect", "lom-dc"], capture_output=True)
    if probe.returncode != 0:
        pytest.skip("lom-dc image not built")
    return subprocess.run(["docker", "run", "--rm", "lom-dc", "bash", "-lc", command],
                          capture_output=True, text=True)


def test_kos_compiler_targets_sh4():
    result = docker_run("kos-cc -dumpmachine")
    assert result.returncode == 0, result.stderr
    assert result.stdout.strip().startswith("sh-elf")


def test_kos_base_is_set():
    result = docker_run("echo $KOS_BASE && test -f $KOS_BASE/lib/dreamcast/libkallisti.a")
    assert result.returncode == 0, result.stderr
    assert result.stdout.strip() == "/opt/toolchains/dc/kos"


def test_mkdcdisc_runs():
    result = docker_run("mkdcdisc --help")
    assert "elf" in (result.stdout + result.stderr).lower()

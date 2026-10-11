#----------------------
# | flycast_run.py
# | Description: Boots a Dreamcast image in headless Linux Flycast (the lom-flycast Docker image) and captures the SCIF serial log from its stdout.
# | Author: suinevere
# | Dependencies: docker, dockerfiles/flycast.Dockerfile
# | Globals: N/A
# | Params: [--seconds N] [--until TEXT] [image]
# | Returns: 0 when TEXT appears (or N seconds pass with no TEXT), 2 on a "LOM HALT:" line, 3 on timeout waiting for TEXT
#----------------------
import argparse
import os
import queue
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import uuid

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

#----------------------
# | DOCKER_IMAGE
# | Description: Tag of the Docker image that holds the headless Flycast build.
# | Author: suinevere
# | Dependencies: dockerfiles/flycast.Dockerfile
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DOCKER_IMAGE = "lom-flycast"

#----------------------
# | EMU_CFG
# | Description: Flycast settings written to the job-local config dir; SerialConsoleEnabled sends guest SCIF bytes to Flycast's stdout.
# | Author: suinevere
# | Dependencies: Flycast v2.7 core/cfg/option.cpp
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EMU_CFG = "[config]\nDebug.SerialConsoleEnabled = yes\n"

#----------------------
# | HALT_MARKER
# | Description: Prefix dc_halt puts on its serial line.
# | Author: suinevere
# | Dependencies: src/system/dc_system.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
HALT_MARKER = "LOM HALT:"


#----------------------
# | find_flycast
# | Description: Returns the Docker image name of the headless Flycast when docker is installed and the image is built, else None.
# | Author: suinevere
# | Dependencies: docker
# | Globals: DOCKER_IMAGE
# | Params: N/A
# | Returns: "lom-flycast" or None
#----------------------
def find_flycast():
    if shutil.which("docker") is None:
        return None
    probe = subprocess.run(["docker", "image", "inspect", DOCKER_IMAGE], capture_output=True)
    return DOCKER_IMAGE if probe.returncode == 0 else None


#----------------------
# | _pump
# | Description: Copies decoded lines from a pipe into a queue, then queues None at end of stream.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: stream -- binary pipe, out -- queue receiving str lines
# | Returns: N/A
#----------------------
def _pump(stream, out):
    for raw in iter(stream.readline, b""):
        out.put(raw.decode("utf-8", "replace").rstrip("\r\n"))
    out.put(None)


#----------------------
# | run_flycast
# | Description: Boots image in lom-flycast with a job-local config, collecting stdout lines until until appears, a halt line appears or seconds pass; always kills the container.
# | Author: suinevere
# | Dependencies: find_flycast, _pump, docker
# | Globals: DOCKER_IMAGE, EMU_CFG, HALT_MARKER
# | Params: image -- path to a .cdi/.gdi, seconds -- time limit, until -- text to wait for or None
# | Returns: (exit_code, lines) -- 0 found or plain timeout, 2 halt, 3 timeout waiting for until
#----------------------
def run_flycast(image, seconds, until):
    image = os.path.abspath(image)
    job = tempfile.mkdtemp(prefix="lom-flycast-")
    os.makedirs(os.path.join(job, "config", "flycast"))
    os.makedirs(os.path.join(job, "data", "flycast"))
    with open(os.path.join(job, "config", "flycast", "emu.cfg"), "w", newline="\n") as f:
        f.write(EMU_CFG)
    name = "lom-flycast-" + uuid.uuid4().hex[:12]
    cmd = ["docker", "run", "--rm", "--init", "--name", name,
           "-v", job + ":/job", "-v", os.path.dirname(image) + ":/disc:ro",
           "-e", "XDG_CONFIG_HOME=/job/config", "-e", "XDG_DATA_HOME=/job/data",
           DOCKER_IMAGE, "xvfb-run", "-a", "flycast", "/disc/" + os.path.basename(image)]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
    lines = []
    code = 3 if until else 0
    feed = queue.Queue()
    threading.Thread(target=_pump, args=(proc.stdout, feed), daemon=True).start()
    deadline = time.monotonic() + seconds
    try:
        while True:
            left = deadline - time.monotonic()
            if left <= 0:
                break
            try:
                line = feed.get(timeout=min(left, 0.5))
            except queue.Empty:
                continue
            if line is None:
                break
            lines.append(line)
            if until and until in line:
                code = 0
                break
            if HALT_MARKER in line:
                code = 2
                break
    finally:
        subprocess.run(["docker", "kill", name], capture_output=True)
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        shutil.rmtree(job, ignore_errors=True)
    return code, lines


#----------------------
# | main
# | Description: Command-line entry: runs the image, prints every captured line and returns the run's exit code.
# | Author: suinevere
# | Dependencies: find_flycast, run_flycast
# | Globals: DC
# | Params: argv -- argument list without the program name
# | Returns: process exit code (1 when lom-flycast is unavailable)
#----------------------
def main(argv):
    parser = argparse.ArgumentParser(description="Boot a Dreamcast image in headless Flycast and capture its serial log.")
    parser.add_argument("--seconds", type=float, default=20)
    parser.add_argument("--until", default=None)
    parser.add_argument("image", nargs="?", default=os.path.join(DC, "lom.cdi"))
    args = parser.parse_args(argv)
    if find_flycast() is None:
        print("flycast_run: docker image %s not found; build dockerfiles/flycast.Dockerfile" % DOCKER_IMAGE, file=sys.stderr)
        return 1
    code, lines = run_flycast(args.image, args.seconds, args.until)
    for line in lines:
        print(line)
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

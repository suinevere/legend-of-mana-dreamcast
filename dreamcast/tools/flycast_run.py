#----------------------
# | flycast_run.py
# | Description: Boots a Dreamcast image in headless Linux Flycast (the lom-flycast Docker image) and captures the SCIF serial log from its stdout.
# |              Grabs a PNG screenshot at each "LOM: capture <name>" marker.
# | Author: suinevere
# | Dependencies: docker, dockerfiles/flycast.Dockerfile (x11-apps, imagemagick)
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

#----------------------
# | DC
# | Description: Absolute path to the dreamcast/ directory, this script's parent.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
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
# | Description: Flycast settings written to the job-local config dir for a plain boot check; SerialConsoleEnabled sends guest SCIF bytes to Flycast's stdout.
# | Author: suinevere
# | Dependencies: Flycast v2.7 core/cfg/option.cpp
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EMU_CFG = "[config]\nDebug.SerialConsoleEnabled = yes\n"

#----------------------
# | EMU_CFG_VMU / EMU_CFG_NO_VMU / VMU_FILE / MAPLE_NONE
# | Description: Settings appended to every run: one VMU in port A slot 1 under a fixed file name (PerGameVmu off) and nothing in slot 2, or with no_vmu nothing in either slot.
# |              Flycast v2.7 names the slot-1 card vmu_save_A1.bin under XDG_DATA_HOME/flycast; maple device type 10 is "none" (checked against KallistiOS's attached-device log).
# | Author: suinevere
# | Dependencies: Flycast v2.7 [config] PerGameVmu, [input] device1.1 / device1.2
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MAPLE_NONE = 10
VMU_FILE = "vmu_save_A1.bin"
EMU_CFG_VMU = "PerGameVmu = no\n\n[input]\ndevice1.2 = %d\n" % MAPLE_NONE
EMU_CFG_NO_VMU = "PerGameVmu = no\n\n[input]\ndevice1.1 = %d\ndevice1.2 = %d\n" % (MAPLE_NONE, MAPLE_NONE)

#----------------------
# | EMU_CFG_CAPTURE
# | Description: Flycast settings used while capturing: SerialConsoleEnabled, plus a fullscreen 640x480 SDL window that fills the fixed-size Xvfb screen exactly.
# |              Flycast v2.7 core/sdl/sdl.cpp reads [window] width/height/fullscreen via config::loadInt/loadBool.
# | Author: suinevere
# | Dependencies: Flycast v2.7 core/sdl/sdl.cpp (window.width, window.height, window.fullscreen, window.maximized)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EMU_CFG_CAPTURE = (
    "\n[window]\nwidth = 640\nheight = 480\nfullscreen = yes\nmaximized = no\n"
    "\n[config]\nDebug.SerialConsoleEnabled = yes\n"
)

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
# | CAPTURE_PREFIX
# | Description: Prefix pad_script.c's log_capture puts on its serial line, followed by the capture's name.
# | Author: suinevere
# | Dependencies: src/input/pad_script.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CAPTURE_PREFIX = "LOM: capture "

#----------------------
# | CAPTURE_DISPLAY
# | Description: Fixed X display number the capturing Xvfb runs on, so docker exec's xwd can target it by number.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CAPTURE_DISPLAY = ":99"

#----------------------
# | CAPTURE_XAUTHORITY
# | Description: Fixed, job-mount-visible path xvfb-run -f writes its Xauthority cookie to, instead of xvfb-run's normal random per-run file.
# |              Lets a later docker exec authenticate against CAPTURE_DISPLAY.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CAPTURE_XAUTHORITY = "/job/xvfb.Xauthority"


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
# | _grab_capture
# | Description: Dumps the capturing Xvfb's root window to a PNG inside the running container, then copies it from the job-local bind mount into the caller's captures directory.
# | Author: suinevere
# | Dependencies: docker exec, xwd, convert (imagemagick), CAPTURE_DISPLAY
# | Globals: N/A
# | Params: container -- running container name, cap_name -- capture marker name, cap_dir -- /job/cap's host-side path, captures -- destination directory
# | Returns: N/A
#----------------------
def _grab_capture(container, cap_name, cap_dir, captures):
    subprocess.run(["docker", "exec", "-e", "XAUTHORITY=" + CAPTURE_XAUTHORITY, container, "sh", "-c",
                     "xwd -root -display %s -silent | convert xwd:- /job/cap/%s.png" % (CAPTURE_DISPLAY, cap_name)],
                    capture_output=True)
    src = os.path.join(cap_dir, cap_name + ".png")
    if os.path.exists(src):
        shutil.copy(src, os.path.join(captures, cap_name + ".png"))


#----------------------
# | run_flycast
# | Description: Boots image in lom-flycast with a job-local config, collecting stdout lines until until appears, a halt line appears or seconds pass.
# |              Grabs a PNG at each capture marker before checking until when captures is given; always kills the container.
# |              With audio_out, SDL's disk driver writes Flycast's mixed output (s16le stereo 44100 Hz, per core/audio/audiobackend_sdl2.cpp) to /job/audio.raw, copied to audio_out afterwards.
# | Author: suinevere
# | Dependencies: find_flycast, _pump, _grab_capture, docker
# | Globals: DOCKER_IMAGE, EMU_CFG, EMU_CFG_CAPTURE, HALT_MARKER, CAPTURE_PREFIX, CAPTURE_DISPLAY
# | Params: image -- path to a .cdi/.gdi, seconds -- time limit, until -- text to wait for or None, captures -- directory to save "<name>.png" grabs into, or None for a plain boot check,
# |         vmu_in -- VMU image bytes to start with (Flycast formats a blank one otherwise), vmu_out -- path to copy the VMU image to afterwards, no_vmu -- run with no VMU attached, audio_out -- raw audio path or None
# | Returns: (exit_code, lines) -- 0 found or plain timeout, 2 halt, 3 timeout waiting for until
#----------------------
def run_flycast(image, seconds, until, captures=None, vmu_in=None, vmu_out=None, no_vmu=False, audio_out=None):
    image = os.path.abspath(image)
    job = tempfile.mkdtemp(prefix="lom-flycast-")
    os.makedirs(os.path.join(job, "config", "flycast"))
    os.makedirs(os.path.join(job, "data", "flycast"))
    vmu_path = os.path.join(job, "data", "flycast", VMU_FILE)
    if vmu_in is not None:
        with open(vmu_path, "wb") as f:
            f.write(vmu_in)
    cap_dir = None
    if captures is not None:
        cap_dir = os.path.join(job, "cap")
        os.makedirs(cap_dir)
    cfg = (EMU_CFG_CAPTURE if captures is not None else EMU_CFG) + (EMU_CFG_NO_VMU if no_vmu else EMU_CFG_VMU)
    with open(os.path.join(job, "config", "flycast", "emu.cfg"), "w", newline="\n") as f:
        f.write(cfg)
    name = "lom-flycast-" + uuid.uuid4().hex[:12]
    if captures is not None:
        xvfb_args = ["xvfb-run", "-n", "99", "-f", CAPTURE_XAUTHORITY, "-s", "-screen 0 640x480x24", "flycast"]
    else:
        xvfb_args = ["xvfb-run", "-a", "flycast"]
    cmd = ["docker", "run", "--rm", "--init", "--name", name,
           "-v", job + ":/job", "-v", os.path.dirname(image) + ":/disc:ro",
           "-e", "XDG_CONFIG_HOME=/job/config", "-e", "XDG_DATA_HOME=/job/data"]
    if audio_out is not None:
        cmd += ["-e", "SDL_AUDIODRIVER=disk", "-e", "SDL_DISKAUDIOFILE=/job/audio.raw"]
    cmd += [DOCKER_IMAGE] + xvfb_args + ["/disc/" + os.path.basename(image)]
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
            if captures is not None and line.startswith(CAPTURE_PREFIX):
                _grab_capture(name, line[len(CAPTURE_PREFIX):].strip(), cap_dir, captures)
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
        if vmu_out is not None and os.path.exists(vmu_path):
            shutil.copy(vmu_path, vmu_out)
        if audio_out is not None and os.path.exists(os.path.join(job, "audio.raw")):
            shutil.copy(os.path.join(job, "audio.raw"), audio_out)
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

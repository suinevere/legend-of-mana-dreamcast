#----------------------
# | gen_pad_script.py
# | Description: Compiles a .pad script into a C header defining pad_script_steps[] and pad_script_step_count for pad_dc.c to run under LOM_PAD_SCRIPT.
# | Author: suinevere
# | Dependencies: src/input/pad_script.h
# | Globals: N/A
# | Params: <in.pad> <out.h>
# | Returns: process exit code (1 on a bad verb or button, with the line number on stderr)
#----------------------
import os
import sys

#----------------------
# | BUTTONS
# | Description: Script button names mapped to pad_script.h's PAD_SCRIPT_BTN_* macros.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
BUTTONS = {
    "UP": "PAD_SCRIPT_BTN_UP",
    "DOWN": "PAD_SCRIPT_BTN_DOWN",
    "LEFT": "PAD_SCRIPT_BTN_LEFT",
    "RIGHT": "PAD_SCRIPT_BTN_RIGHT",
    "CROSS": "PAD_SCRIPT_BTN_CROSS",
    "CIRCLE": "PAD_SCRIPT_BTN_CIRCLE",
    "SQUARE": "PAD_SCRIPT_BTN_SQUARE",
    "TRIANGLE": "PAD_SCRIPT_BTN_TRIANGLE",
    "START": "PAD_SCRIPT_BTN_START",
    "SELECT": "PAD_SCRIPT_BTN_SELECT",
    "L1": "PAD_SCRIPT_BTN_L1",
    "R1": "PAD_SCRIPT_BTN_R1",
    "L2": "PAD_SCRIPT_BTN_L2",
    "R2": "PAD_SCRIPT_BTN_R2",
}


#----------------------
# | ScriptError
# | Description: A bad verb or button at a known source line, carrying the message gen_pad_script prints to stderr.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class ScriptError(Exception):
    pass


#----------------------
# | parse_press
# | Description: Parses a "press <BUTTON>[+<BUTTON>...] <vblanks>" line into a step dict.
# | Author: suinevere
# | Dependencies: BUTTONS, ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "press", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "press", arg, buttons, name ""
#----------------------
def parse_press(tokens, lineno):
    if len(tokens) != 2:
        raise ScriptError("line %d: press needs <BUTTON>[+<BUTTON>...] <vblanks>" % lineno)
    for name in tokens[0].split("+"):
        if name.upper() not in BUTTONS:
            raise ScriptError("line %d: unknown button %r" % (lineno, name))
    try:
        vblanks = int(tokens[1])
    except ValueError:
        raise ScriptError("line %d: press vblank count must be an integer" % lineno)
    return {"op": "PAD_OP_PRESS", "arg": vblanks, "buttons_macros": [BUTTONS[n.upper()] for n in tokens[0].split("+")], "name": ""}


#----------------------
# | parse_wait
# | Description: Parses a "wait state <n>" or "wait <vblanks>" line into a step dict.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "wait", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "wait_state" or "wait", arg, buttons_macros [], name ""
#----------------------
def parse_wait(tokens, lineno):
    if len(tokens) == 2 and tokens[0] == "state":
        try:
            state = int(tokens[1])
        except ValueError:
            raise ScriptError("line %d: wait state needs an integer" % lineno)
        return {"op": "PAD_OP_WAIT_STATE", "arg": state, "buttons_macros": [], "name": ""}
    if len(tokens) == 1:
        try:
            vblanks = int(tokens[0])
        except ValueError:
            raise ScriptError("line %d: wait needs an integer vblank count" % lineno)
        return {"op": "PAD_OP_WAIT", "arg": vblanks, "buttons_macros": [], "name": ""}
    raise ScriptError("line %d: wait needs 'state <n>' or '<vblanks>'" % lineno)


#----------------------
# | parse_mash
# | Description: Parses a "mash <BUTTON> <period> state <n>" or "mash <BUTTON> <period> marker <text...>" line into a step dict; marker text may contain spaces, since watcher lines are "<name> <value>".
# | Author: suinevere
# | Dependencies: BUTTONS, ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "mash", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_MASH", arg (period), buttons_macros, name ("" for state mode), stop_state
#----------------------
def parse_mash(tokens, lineno):
    if len(tokens) < 4:
        raise ScriptError("line %d: mash needs <BUTTON> <period> state <n> or <BUTTON> <period> marker <text>" % lineno)
    button, period_tok, mode = tokens[0], tokens[1], tokens[2]
    rest = tokens[3:]
    if button.upper() not in BUTTONS:
        raise ScriptError("line %d: unknown button %r" % (lineno, button))
    try:
        period = int(period_tok)
    except ValueError:
        raise ScriptError("line %d: mash period must be an integer" % lineno)
    if period < 8:
        raise ScriptError("line %d: mash period must be at least 8" % lineno)
    if mode == "state":
        if len(rest) != 1:
            raise ScriptError("line %d: mash state needs exactly one integer" % lineno)
        try:
            state = int(rest[0])
        except ValueError:
            raise ScriptError("line %d: mash state needs an integer" % lineno)
        return {"op": "PAD_OP_MASH", "arg": period, "buttons_macros": [BUTTONS[button.upper()]], "name": "", "stop_state": state}
    if mode == "marker":
        return {"op": "PAD_OP_MASH", "arg": period, "buttons_macros": [BUTTONS[button.upper()]], "name": " ".join(rest), "stop_state": 0}
    raise ScriptError("line %d: mash needs 'state <n>' or 'marker <text>'" % lineno)


#----------------------
# | parse_capture
# | Description: Parses a "capture <name>" line into a step dict.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "capture", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "capture", arg 0, buttons_macros [], name
#----------------------
def parse_capture(tokens, lineno):
    if len(tokens) != 1:
        raise ScriptError("line %d: capture needs exactly one name" % lineno)
    return {"op": "PAD_OP_CAPTURE", "arg": 0, "buttons_macros": [], "name": tokens[0]}


#----------------------
# | parse_scene
# | Description: Parses "scene <id> <spawn>" into a test-only scene-jump step, carrying the spawn in stop_state.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "scene", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_SCENE", arg scene id, stop_state spawn
#----------------------
def parse_scene(tokens, lineno):
    if len(tokens) != 2:
        raise ScriptError("line %d: scene needs <id> <spawn>" % lineno)
    try:
        scene, spawn = int(tokens[0]), int(tokens[1])
    except ValueError:
        raise ScriptError("line %d: scene id and spawn must be integers" % lineno)
    return {"op": "PAD_OP_SCENE", "arg": scene, "buttons_macros": [], "name": "", "stop_state": spawn}


#----------------------
# | parse_battle
# | Description: Parses "battle" into a test-only step that starts the current scene's first monster group.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "battle", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_BATTLE"
#----------------------
def parse_battle(tokens, lineno):
    if tokens:
        raise ScriptError("line %d: battle takes no arguments" % lineno)
    return {"op": "PAD_OP_BATTLE", "arg": 0, "buttons_macros": [], "name": "", "stop_state": 0}


#----------------------
# | parse_gameover
# | Description: Parses "gameover" into a test-only step that starts the game-over transition.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "gameover", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_GAMEOVER"
#----------------------
def parse_gameover(tokens, lineno):
    if tokens:
        raise ScriptError("line %d: gameover takes no arguments" % lineno)
    return {"op": "PAD_OP_GAMEOVER", "arg": 0, "buttons_macros": [], "name": "", "stop_state": 0}


#----------------------
# | parse_tone
# | Description: Parses "tone <hz> <seconds>" into a test-only step that plays a sine on the AICA stream channel; 100-20000 Hz, 1-60 s.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "tone", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_TONE", arg the frequency and stop_state the seconds
#----------------------
def parse_tone(tokens, lineno):
    if len(tokens) != 2 or not all(t.isdigit() for t in tokens):
        raise ScriptError("line %d: tone needs <hz> <seconds>" % lineno)
    hz, seconds = int(tokens[0]), int(tokens[1])
    if not 100 <= hz <= 20000 or not 1 <= seconds <= 60:
        raise ScriptError("line %d: tone needs 100-20000 Hz and 1-60 s, not %d Hz %d s" % (lineno, hz, seconds))
    return {"op": "PAD_OP_TONE", "arg": hz, "buttons_macros": [], "name": "", "stop_state": seconds}


#----------------------
# | parse_longvoice
# | Description: Parses "longvoice <seconds>" into a test-only step that plays a streamed long sample on SPU voice 23; 1-60 s.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "longvoice", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_LONGVOICE" and arg the seconds
#----------------------
def parse_longvoice(tokens, lineno):
    if len(tokens) != 1 or not tokens[0].isdigit() or not 1 <= int(tokens[0]) <= 60:
        raise ScriptError("line %d: longvoice needs <seconds> 1-60" % lineno)
    return {"op": "PAD_OP_LONGVOICE", "arg": int(tokens[0]), "buttons_macros": [], "name": "", "stop_state": 0}


#----------------------
# | parse_effects
# | Description: Parses "effects" into a test-only step that plays the reverb, noise and pitch-modulation sequence.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "effects", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_EFFECTS"
#----------------------
def parse_effects(tokens, lineno):
    if tokens:
        raise ScriptError("line %d: effects takes no arguments" % lineno)
    return {"op": "PAD_OP_EFFECTS", "arg": 0, "buttons_macros": [], "name": "", "stop_state": 0}


#----------------------
# | parse_movies
# | Description: Parses "movies" into a test-only step that stops the script holding Start through the intro, so the intro plays.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "movies", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_MOVIES"
#----------------------
def parse_movies(tokens, lineno):
    if tokens:
        raise ScriptError("line %d: movies takes no arguments" % lineno)
    return {"op": "PAD_OP_MOVIES", "arg": 0, "buttons_macros": [], "name": "", "stop_state": 0}


#----------------------
# | parse_fieldmovie
# | Description: Parses "fieldmovie" into a test-only step that starts the current scene's first movie animation through FIELD's own restart op.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "fieldmovie", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_FIELD_MOVIE"
#----------------------
def parse_fieldmovie(tokens, lineno):
    if tokens:
        raise ScriptError("line %d: fieldmovie takes no arguments" % lineno)
    return {"op": "PAD_OP_FIELD_MOVIE", "arg": 0, "buttons_macros": [], "name": "", "stop_state": 0}


#----------------------
# | PROBE_TARGETS
# | Description: Probes "probe <name>" runs, in the order of their PAD_OP_PROBE_* ops' arguments.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PROBE_TARGETS = ["movie"]


#----------------------
# | parse_probe
# | Description: Parses "probe movie" into a test-only step that times software STR decoding of the intro.
# | Author: suinevere
# | Dependencies: PROBE_TARGETS, ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "probe", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_PROBE_MOVIE"
#----------------------
def parse_probe(tokens, lineno):
    if len(tokens) != 1 or tokens[0] not in PROBE_TARGETS:
        raise ScriptError("line %d: probe needs one of %s, not %r" % (lineno, ", ".join(PROBE_TARGETS), " ".join(tokens)))
    return {"op": "PAD_OP_PROBE_MOVIE", "arg": 0, "buttons_macros": [], "name": "", "stop_state": 0}


#----------------------
# | OPEN_TARGETS
# | Description: Secondary overlays "open <name>" reaches, in the index order field_watch_unit.c's opener table uses.
# | Author: suinevere
# | Dependencies: dreamcast/src/overlays/field_watch_unit.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OPEN_TARGETS = ["shop", "golem", "zukan", "gosub", "carda", "niki", "addhero", "carda_send", "carda_return", "ranch_menu"]


#----------------------
# | parse_landfx
# | Description: Parses "landfx <n>" into a test-only step that plays land placement effect n on the world map.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "landfx", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_LANDFX" and arg the effect number
#----------------------
def parse_landfx(tokens, lineno):
    if len(tokens) != 1 or not tokens[0].isdigit() or int(tokens[0]) > 33:
        raise ScriptError("line %d: landfx needs a land effect number 0-33" % lineno)
    return {"op": "PAD_OP_LANDFX", "arg": int(tokens[0]), "buttons_macros": [], "name": "", "stop_state": 0}


#----------------------
# | parse_open
# | Description: Parses "open <name>" into a test-only step that opens a secondary overlay through FIELD's own opener.
# | Author: suinevere
# | Dependencies: OPEN_TARGETS, ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "open", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_OPEN" and arg the OPEN_TARGETS index
#----------------------
def parse_open(tokens, lineno):
    if len(tokens) != 1 or tokens[0] not in OPEN_TARGETS:
        raise ScriptError("line %d: open needs one of %s, not %r" % (lineno, ", ".join(OPEN_TARGETS), " ".join(tokens)))
    return {"op": "PAD_OP_OPEN", "arg": OPEN_TARGETS.index(tokens[0]), "buttons_macros": [], "name": "", "stop_state": 0}


#----------------------
# | MUSIC_RESOURCES / SFXSET_RESOURCES
# | Description: CD resources the music and sfxset steps accept: convert_sound.SOUND_RESOURCES' music kind, and its effect_set, bank, sfx_set and weapon_sfx kinds.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MUSIC_RESOURCES = frozenset(list(range(23, 74)) + [146] + list(range(147, 167)))
SFXSET_RESOURCES = frozenset([21, 22] + list(range(81, 119)) + list(range(131, 142)))


#----------------------
# | parse_resource_step
# | Description: Parses "<verb> <resource>" into a test-only step whose arg is a CD resource from the allowed set.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: verb -- step verb, op -- PAD_OP_* name, allowed -- accepted resources, tokens -- tokens after the verb, lineno -- 1-based source line
# | Returns: step dict with op and arg the resource
#----------------------
def parse_resource_step(verb, op, allowed, tokens, lineno):
    if len(tokens) != 1 or not tokens[0].isdigit() or int(tokens[0]) not in allowed:
        raise ScriptError("line %d: %s needs one of its sound resources, not %r" % (lineno, verb, " ".join(tokens)))
    return {"op": op, "arg": int(tokens[0]), "buttons_macros": [], "name": "", "stop_state": 0}


#----------------------
# | parse_musicoff
# | Description: Parses "musicoff" into a test-only step that stops the playing song.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "musicoff", lineno -- 1-based source line
# | Returns: step dict with op "PAD_OP_MUSICOFF"
#----------------------
def parse_musicoff(tokens, lineno):
    if tokens:
        raise ScriptError("line %d: musicoff takes no arguments" % lineno)
    return {"op": "PAD_OP_MUSICOFF", "arg": 0, "buttons_macros": [], "name": "", "stop_state": 0}


#----------------------
# | parse_boothold
# | Description: Parses "boothold" into a step that marks a script as wanting 50 Hz PAL held through pad_script_boot_hold(); callers must reject it off the first line.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "boothold", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_BOOTHOLD"
#----------------------
def parse_boothold(tokens, lineno):
    if tokens:
        raise ScriptError("line %d: boothold takes no arguments" % lineno)
    return {"op": "PAD_OP_BOOTHOLD", "arg": 0, "buttons_macros": [], "name": "", "stop_state": 0}


#----------------------
# | parse_sweep
# | Description: Parses "sweep" into a test-only step that starts the FIELD scene sweep over the VMU's LOMSWEEP range.
# | Author: suinevere
# | Dependencies: ScriptError
# | Globals: N/A
# | Params: tokens -- tokens after "sweep", lineno -- 1-based source line, for error messages
# | Returns: step dict with op "PAD_OP_SWEEP"
#----------------------
def parse_sweep(tokens, lineno):
    if tokens:
        raise ScriptError("line %d: sweep takes no arguments" % lineno)
    return {"op": "PAD_OP_SWEEP", "arg": 0, "buttons_macros": [], "name": "", "stop_state": 0}


#----------------------
# | parse_script
# | Description: Parses every non-comment, non-blank line of a .pad script into step dicts; boothold is only accepted as the first step.
# | Author: suinevere
# | Dependencies: parse_wait, parse_press, parse_mash, parse_capture, parse_scene, parse_battle, parse_open, parse_gameover, parse_tone, parse_longvoice, parse_effects, parse_resource_step, parse_musicoff, parse_probe, parse_movies, parse_fieldmovie, parse_boothold, parse_sweep, ScriptError
# | Globals: N/A
# | Params: text -- script file contents
# | Returns: list of step dicts
#----------------------
def parse_script(text):
    steps = []
    for lineno, raw in enumerate(text.splitlines(), start=1):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        tokens = line.split()
        verb, rest = tokens[0], tokens[1:]
        if verb == "wait":
            steps.append(parse_wait(rest, lineno))
        elif verb == "press":
            steps.append(parse_press(rest, lineno))
        elif verb == "mash":
            steps.append(parse_mash(rest, lineno))
        elif verb == "capture":
            steps.append(parse_capture(rest, lineno))
        elif verb == "scene":
            steps.append(parse_scene(rest, lineno))
        elif verb == "battle":
            steps.append(parse_battle(rest, lineno))
        elif verb == "open":
            steps.append(parse_open(rest, lineno))
        elif verb == "landfx":
            steps.append(parse_landfx(rest, lineno))
        elif verb == "gameover":
            steps.append(parse_gameover(rest, lineno))
        elif verb == "tone":
            steps.append(parse_tone(rest, lineno))
        elif verb == "longvoice":
            steps.append(parse_longvoice(rest, lineno))
        elif verb == "effects":
            steps.append(parse_effects(rest, lineno))
        elif verb == "sweep":
            steps.append(parse_sweep(rest, lineno))
        elif verb == "music":
            steps.append(parse_resource_step(verb, "PAD_OP_MUSIC", MUSIC_RESOURCES, rest, lineno))
        elif verb == "musicoff":
            steps.append(parse_musicoff(rest, lineno))
        elif verb == "sfxset":
            steps.append(parse_resource_step(verb, "PAD_OP_SFXSET", SFXSET_RESOURCES, rest, lineno))
        elif verb == "probe":
            steps.append(parse_probe(rest, lineno))
        elif verb == "movies":
            steps.append(parse_movies(rest, lineno))
        elif verb == "fieldmovie":
            steps.append(parse_fieldmovie(rest, lineno))
        elif verb == "boothold":
            if steps:
                raise ScriptError("line %d: boothold must be the first step" % lineno)
            steps.append(parse_boothold(rest, lineno))
        else:
            raise ScriptError("line %d: unknown verb %r" % (lineno, verb))
    return steps


#----------------------
# | render_header
# | Description: Renders parsed scripts as a C header: one pad_script_steps_<name>[] per script and pad_script_scripts[] naming them, first script first.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: scripts -- list of (name, steps) with steps from parse_script
# | Returns: header text
#----------------------
def render_header(scripts):
    lines = [
        "#ifndef LOM_PAD_SCRIPT_TABLE_H",
        "#define LOM_PAD_SCRIPT_TABLE_H",
        "",
        '#include "pad_script.h"',
        "",
    ]
    for name, steps in scripts:
        lines.append("static const PadScriptStep pad_script_steps_%s[] = {" % name)
        for step in steps:
            buttons = " | ".join(step["buttons_macros"]) if step["buttons_macros"] else "0"
            stop_state = step.get("stop_state", 0)
            lines.append("    { %s, %d, %s, \"%s\", %d }," % (step["op"], step["arg"], buttons, step["name"], stop_state))
        lines.append("};")
        lines.append("")
    lines.append("static const PadScriptChoice pad_script_scripts[] = {")
    for name, steps in scripts:
        lines.append('    { "%s", pad_script_steps_%s, %d },' % (name, name, len(steps)))
    lines.append("};")
    lines.append("")
    lines.append("static const unsigned pad_script_script_count = %d;" % len(scripts))
    lines.append("")
    lines.append("#endif")
    lines.append("")
    return "\n".join(lines)


#----------------------
# | main
# | Description: Command-line entry: compiles every <in.pad> argument, named by its file stem, into the last argument's header.
# | Author: suinevere
# | Dependencies: parse_script, render_header, ScriptError
# | Globals: N/A
# | Params: argv -- argument list without the program name
# | Returns: 0 on success, 1 on a bad verb or button (message on stderr), 2 on bad usage
#----------------------
def main(argv):
    if len(argv) < 2:
        print("usage: gen_pad_script.py <in.pad>... <out.h>", file=sys.stderr)
        return 2
    in_paths, out_path = argv[:-1], argv[-1]
    scripts = []
    for in_path in in_paths:
        with open(in_path, "r") as f:
            text = f.read()
        try:
            scripts.append((os.path.splitext(os.path.basename(in_path))[0], parse_script(text)))
        except ScriptError as exc:
            print("%s: %s" % (in_path, exc), file=sys.stderr)
            return 1
    out_dir = os.path.dirname(out_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    with open(out_path, "w", newline="\n") as f:
        f.write(render_header(scripts))
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

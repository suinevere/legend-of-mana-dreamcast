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
# | parse_script
# | Description: Parses every non-comment, non-blank line of a .pad script into step dicts.
# | Author: suinevere
# | Dependencies: parse_wait, parse_press, parse_capture, ScriptError
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
        else:
            raise ScriptError("line %d: unknown verb %r" % (lineno, verb))
    return steps


#----------------------
# | render_header
# | Description: Renders parsed steps as a C header defining pad_script_steps[] and pad_script_step_count.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: steps -- list of step dicts from parse_script
# | Returns: header text
#----------------------
def render_header(steps):
    lines = [
        "#ifndef LOM_PAD_SCRIPT_TABLE_H",
        "#define LOM_PAD_SCRIPT_TABLE_H",
        "",
        '#include "pad_script.h"',
        "",
        "static const PadScriptStep pad_script_steps[] = {",
    ]
    for step in steps:
        buttons = " | ".join(step["buttons_macros"]) if step["buttons_macros"] else "0"
        stop_state = step.get("stop_state", 0)
        lines.append("    { %s, %d, %s, \"%s\", %d }," % (step["op"], step["arg"], buttons, step["name"], stop_state))
    lines.append("};")
    lines.append("")
    lines.append("static const unsigned pad_script_step_count = %d;" % len(steps))
    lines.append("")
    lines.append("#endif")
    lines.append("")
    return "\n".join(lines)


#----------------------
# | main
# | Description: Command-line entry: reads argv[1], writes argv[2]'s compiled header.
# | Author: suinevere
# | Dependencies: parse_script, render_header, ScriptError
# | Globals: N/A
# | Params: argv -- argument list without the program name
# | Returns: 0 on success, 1 on a bad verb or button (message on stderr), 2 on bad usage
#----------------------
def main(argv):
    if len(argv) != 2:
        print("usage: gen_pad_script.py <in.pad> <out.h>", file=sys.stderr)
        return 2
    in_path, out_path = argv
    with open(in_path, "r") as f:
        text = f.read()
    try:
        steps = parse_script(text)
    except ScriptError as exc:
        print(str(exc), file=sys.stderr)
        return 1
    out_dir = os.path.dirname(out_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    with open(out_path, "w", newline="\n") as f:
        f.write(render_header(steps))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

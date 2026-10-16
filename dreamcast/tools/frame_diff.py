#----------------------
# | frame_diff.py
# | Description: Crops the 4:3 picture out of pillarboxed PS1 reference screenshots and diffs a Dreamcast Flycast capture against them.
# | Author: suinevere
# | Dependencies: Pillow, numpy, reference/ps1/*.png, reference/ps1/crops.json
# | Globals: DC, REPO, REF_DIR, CROPS_PATH, BLACK_LEVEL, OUTLIER_LEVEL, MEAN_LIMIT, OUTLIER_LIMIT, CROP_SIBLINGS
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import json
import os
import sys

import numpy as np
from PIL import Image, ImageFilter

#----------------------
# | DC
# | Description: Absolute path to the dreamcast/ directory, this script's grandparent.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

#----------------------
# | REPO
# | Description: Absolute path to the repository root, DC's parent.
# | Author: suinevere
# | Dependencies: DC
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
REPO = os.path.dirname(DC)

#----------------------
# | REF_DIR
# | Description: Directory holding the PS1 reference screenshots and crops.json.
# | Author: suinevere
# | Dependencies: REPO
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
REF_DIR = os.path.join(REPO, "reference", "ps1")

#----------------------
# | CROPS_PATH
# | Description: Path to the JSON file mapping each reference screenshot's name to its 4:3 crop box.
# | Author: suinevere
# | Dependencies: REF_DIR
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CROPS_PATH = os.path.join(REF_DIR, "crops.json")

#----------------------
# | BLACK_LEVEL
# | Description: Max channel value at or below which a pixel is treated as pillarbox black, not picture.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
BLACK_LEVEL = 24

#----------------------
# | OUTLIER_LEVEL
# | Description: Largest-channel absolute difference above which a pixel counts as an outlier in compare().
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OUTLIER_LEVEL = 48

#----------------------
# | MEAN_LIMIT
# | Description: Highest mean absolute difference compare() may report for matches() to still accept the pair.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MEAN_LIMIT = 14.0

#----------------------
# | OUTLIER_LIMIT
# | Description: Highest outlier fraction compare() may report for matches() to still accept the pair.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OUTLIER_LIMIT = 0.05

#----------------------
# | CAPTURE_ROWS
# | Description: Grab rows kept before reducing (display lines 8..231 at 2x), mirroring DuckStation's default NTSC overscan crop the references were taken with.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CAPTURE_ROWS = (16, 464)


#----------------------
# | _widen_to_4_3
# | Description: Recomputes left/right so the box is exactly 4:3 of its own height, centred on the original box, clamped to image_width.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: left/top/right/bottom -- detected picture box, image_width -- source image width in pixels
# | Returns: (left, top, right, bottom) with top/bottom unchanged
#----------------------
def _widen_to_4_3(left, top, right, bottom, image_width):
    height = bottom - top
    target_width = round(height * 4 / 3)
    center = (left + right) / 2.0
    new_left = round(center - target_width / 2.0)
    new_right = new_left + target_width
    if new_left < 0:
        shift = -new_left
        new_left += shift
        new_right += shift
    if new_right > image_width:
        shift = new_right - image_width
        new_left -= shift
        new_right -= shift
    new_left = max(new_left, 0)
    return (int(new_left), int(top), int(new_right), int(bottom))


#----------------------
# | _fallback_crop
# | Description: When a screenshot has no pixel above BLACK_LEVEL, reuses the detected box of the brightest sibling PNG at the same height.
# | Author: suinevere
# | Dependencies: detect_crop
# | Globals: N/A
# | Params: path -- screenshot that failed detection, height -- its pixel height
# | Returns: (left, top, right, bottom) copied from the brightest same-height sibling
#----------------------
def _fallback_crop(path, height):
    directory = os.path.dirname(path)
    candidates = []
    for name in sorted(os.listdir(directory)):
        if not name.endswith(".png"):
            continue
        candidate_path = os.path.join(directory, name)
        if os.path.abspath(candidate_path) == os.path.abspath(path):
            continue
        img = Image.open(candidate_path).convert("RGB")
        if img.size[1] != height:
            continue
        brightness = float(np.asarray(img, dtype=np.uint8).mean())
        candidates.append((brightness, candidate_path))
    if not candidates:
        raise ValueError("frame_diff: no sibling screenshot at height %d to fall back on for %s" % (height, path))
    candidates.sort(reverse=True)
    return detect_crop(candidates[0][1])


#----------------------
# | detect_crop
# | Description: Finds the 4:3 picture box inside a pillarboxed screenshot, as the outermost columns/rows above BLACK_LEVEL widened to 4:3.
# | Author: suinevere
# | Dependencies: PIL.Image, numpy, _widen_to_4_3, _fallback_crop
# | Globals: BLACK_LEVEL
# | Params: path -- PNG screenshot path
# | Returns: (left, top, right, bottom) pixel box
#----------------------
def detect_crop(path):
    img = Image.open(path).convert("RGB")
    arr = np.asarray(img, dtype=np.uint8)
    maxc = arr.max(axis=2)
    mask = maxc > BLACK_LEVEL
    cols = np.where(mask.any(axis=0))[0]
    rows = np.where(mask.any(axis=1))[0]
    width, height = img.size
    if len(cols) == 0 or len(rows) == 0:
        return _fallback_crop(path, height)
    left, right = int(cols.min()), int(cols.max()) + 1
    top, bottom = int(rows.min()), int(rows.max()) + 1
    return _widen_to_4_3(left, top, right, bottom, width)


#----------------------
# | load_reference
# | Description: Opens a PS1 reference screenshot, crops it by crops.json and resizes the crop to 320x240.
# | Author: suinevere
# | Dependencies: PIL.Image, numpy, REF_DIR, CROPS_PATH
# | Globals: REF_DIR, CROPS_PATH
# | Params: name -- screenshot name without extension, e.g. "01-title"
# | Returns: numpy uint8 array (240, 320, 3)
#----------------------
def load_reference(name):
    with open(CROPS_PATH) as f:
        crops = json.load(f)
    box = tuple(crops[name])
    img = Image.open(os.path.join(REF_DIR, name + ".png")).convert("RGB")
    img = img.crop(box).resize((320, 240), Image.LANCZOS)
    return np.asarray(img, dtype=np.uint8)


#----------------------
# | load_capture
# | Description: Opens a 640x480 Flycast grab, keeps the CAPTURE_ROWS DuckStation shows, and box-resizes the result to 320x240.
# | Author: suinevere
# | Dependencies: PIL.Image, numpy
# | Globals: CAPTURE_ROWS
# | Params: path -- capture PNG path
# | Returns: numpy uint8 array (240, 320, 3)
#----------------------
def load_capture(path):
    img = Image.open(path).convert("RGB")
    img = img.crop((0, CAPTURE_ROWS[0], img.width, CAPTURE_ROWS[1])).resize((320, 240), Image.BOX)
    return np.asarray(img, dtype=np.uint8)


#----------------------
# | compare
# | Description: Box-blurs both 320x240 frames then computes their mean absolute difference and the fraction of outlier pixels.
# | Author: suinevere
# | Dependencies: PIL.Image, PIL.ImageFilter, numpy
# | Globals: OUTLIER_LEVEL
# | Params: a/b -- numpy uint8 arrays (240, 320, 3)
# | Returns: dict(mean=float, outliers=float)
#----------------------
def compare(a, b):
    blur_a = np.asarray(Image.fromarray(a).filter(ImageFilter.BoxBlur(1)), dtype=np.int16)
    blur_b = np.asarray(Image.fromarray(b).filter(ImageFilter.BoxBlur(1)), dtype=np.int16)
    diff = np.abs(blur_a - blur_b)
    mean = float(diff.mean())
    outliers = float((diff.max(axis=2) > OUTLIER_LEVEL).mean())
    return {"mean": mean, "outliers": outliers}


#----------------------
# | matches
# | Description: Decides whether a compare() result counts as the same screen.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: MEAN_LIMIT, OUTLIER_LIMIT
# | Params: result -- dict(mean, outliers) from compare()
# | Returns: True when both the mean and outlier fraction are within their limits
#----------------------
def matches(result):
    return result["mean"] <= MEAN_LIMIT and result["outliers"] <= OUTLIER_LIMIT


#----------------------
# | CROP_SIBLINGS
# | Description: Screenshots whose picture leaves its own edge columns black, mapped to a sibling from the same capture session whose detected pillarbox they share.
# |              GNAME's name entry draws nothing in its leftmost columns, and 09-16's dark WMAP content leaves content detection latching onto a bright patch inside the picture instead of the window.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CROP_SIBLINGS = {"04-gname-name-entry": "03-gname-select-weapon",
                  "09-wmap-loading": "01-title",
                  "10-wmap-mailbox": "01-title",
                  "11-wmap-mailbox-carried": "01-title",
                  "12-wmap-place-mailbox-prompt": "01-title",
                  "13-wmap-land-flash": "01-title",
                  "14-wmap-home-rising": "01-title",
                  "15-wmap-home-dark": "01-title",
                  "16-wmap-board-home": "01-title"}


#----------------------
# | reference_crop
# | Description: The crop box for one reference screenshot: detected from the screenshot itself, or from its CROP_SIBLINGS sibling.
# | Author: suinevere
# | Dependencies: detect_crop, CROP_SIBLINGS, REF_DIR
# | Globals: CROP_SIBLINGS, REF_DIR
# | Params: name -- screenshot name without extension
# | Returns: (left, top, right, bottom) pixel box
#----------------------
def reference_crop(name):
    return detect_crop(os.path.join(REF_DIR, CROP_SIBLINGS.get(name, name) + ".png"))


#----------------------
# | _detect_all
# | Description: Runs reference_crop over every PNG in REF_DIR and rewrites crops.json with the results.
# | Author: suinevere
# | Dependencies: reference_crop, REF_DIR, CROPS_PATH
# | Globals: REF_DIR, CROPS_PATH
# | Params: N/A
# | Returns: N/A
#----------------------
def _detect_all():
    crops = {}
    for name in sorted(os.listdir(REF_DIR)):
        if not name.endswith(".png"):
            continue
        key = name[:-4]
        crops[key] = list(reference_crop(key))
    with open(CROPS_PATH, "w", newline="\n") as f:
        json.dump(crops, f, indent=2, sort_keys=True)
        f.write("\n")


#----------------------
# | main
# | Description: Command-line entry: "detect" regenerates crops.json, "compare" diffs a capture against a named reference.
# | Author: suinevere
# | Dependencies: _detect_all, load_capture, load_reference, compare, matches
# | Globals: N/A
# | Params: argv -- argument list without the program name
# | Returns: process exit code (0 on detect or MATCH, 1 on DIFFER)
#----------------------
def main(argv):
    parser = argparse.ArgumentParser(description="Detect PS1 reference crops or diff a Flycast capture against one.")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("detect")
    compare_parser = sub.add_parser("compare")
    compare_parser.add_argument("capture")
    compare_parser.add_argument("reference")
    args = parser.parse_args(argv)
    if args.command == "detect":
        _detect_all()
        return 0
    result = compare(load_capture(args.capture), load_reference(args.reference))
    verdict = "MATCH" if matches(result) else "DIFFER"
    print("mean=%.3f outliers=%.4f %s" % (result["mean"], result["outliers"], verdict))
    return 0 if matches(result) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

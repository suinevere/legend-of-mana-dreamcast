"""----------------------
 | conftest.py
 | Description: pytest options for the PocketStation tests: --regen rewrites references, --verify checks hooked C per call, --drive runs the C.
 | Author: suinevere
 | Dependencies: pytest
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""


def pytest_addoption(parser):
    parser.addoption("--regen", action="store_true", help="regenerate pocketstation/build/reference/*.pktr")
    parser.addoption("--verify", action="store_true", help="check every hooked C function against its ARM, call by call")
    parser.addoption("--drive", action="store_true", help="run every hooked C function in place of its ARM")
    parser.addoption("--hooks-exclude", default="", help="comma-separated hook addresses to leave in ARM")

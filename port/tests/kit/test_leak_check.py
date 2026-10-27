#----------------------
# | test_leak_check.py
# | Description: Checks port/kit/leak_check.py finds a copied chunk at any 4-byte alignment, ignores uniform padding and honours an explicit allow list.
# | Author: suinevere
# | Dependencies: port/kit/leak_check.py
#----------------------
import os
import random
import sys

sys.path.insert(0, os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..", "..")))
from port.kit import leak_check  # noqa: E402


def _random(seed, size=4096):
    rng = random.Random(seed)
    return bytes(rng.randrange(256) for _ in range(size))


def test_finds_copied_chunk():
    secret = _random(1)
    hashes = leak_check.chunk_hashes([secret])
    shipped = bytes(12) + secret[512:768] + bytes(100)
    assert leak_check.scan(shipped, hashes) == [12]


def test_finds_chunk_at_unaligned_offset():
    secret = _random(3)
    hashes = leak_check.chunk_hashes([secret])
    shipped = bytes(8) + secret[256:1024] + bytes(8)
    assert 8 in leak_check.scan(shipped, hashes)


def test_ignores_uniform_chunks():
    hashes = leak_check.chunk_hashes([bytes(4096), b"\xff" * 4096])
    assert not hashes
    assert leak_check.scan(bytes(8192), hashes) == []


def test_clean_data_passes():
    assert leak_check.scan(_random(4), leak_check.chunk_hashes([_random(5)])) == []


def test_allowed_range_is_skipped():
    secret = _random(6)
    hits = [("track03.lit@0", 12), ("track03.lit@0", 400), ("track01.bin", 12)]
    allow = [{"file": "track03.lit@0", "offset": 0, "length": 300, "source": "src/foo.c"}]
    assert leak_check.unallowed(hits, allow) == [("track03.lit@0", 400), ("track01.bin", 12)]


def test_ignores_near_empty_filler():
    lone_flag = b"\x00\x01" + bytes(254)
    terminator = b"\xffCD001\x01" + bytes(249)
    hashes = leak_check.chunk_hashes([lone_flag * 4, terminator * 4])
    assert not hashes
    assert leak_check.scan(lone_flag + terminator, hashes) == []


def test_sparse_but_varied_chunk_still_counts():
    varied = bytes(range(0, 256, 8)) + bytes(224)
    hashes = leak_check.chunk_hashes([varied])
    assert leak_check.scan(bytes(4) + varied, hashes) == [4]

"""Tests for the zone-analysis prototype.

- behaviour tests on synthetic frames (stairs, cluttered side)
- port-equivalence test: the Python version must give the SAME numbers as the
  firmware header (camera_zones.h) compiled on the host with g++.
Run:  python -m pytest vision/tests -q
"""
import shutil
import subprocess
import sys
from pathlib import Path

import numpy as np
import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from zone_analysis import GH, GW, analyze_zones  # noqa: E402

HERE = Path(__file__).parent


def flat(v=120):
    return np.full((GH, GW), v, np.uint8)


def test_flat_frame_no_suggestion():
    r = analyze_zones(flat())
    assert (r.left, r.center, r.right, r.stair_lines, r.freer_side) == (0, 0, 0, 0, 0)


def test_cluttered_left_suggests_right():
    img = flat()
    rng = np.random.default_rng(1)
    img[:, :50] = rng.integers(0, 255, (GH, 50), dtype=np.uint8)
    r = analyze_zones(img)
    assert r.left > r.right and r.freer_side == 1


def test_cluttered_right_suggests_left():
    img = flat()
    rng = np.random.default_rng(2)
    img[:, 110:] = rng.integers(0, 255, (GH, 50), dtype=np.uint8)
    assert analyze_zones(img).freer_side == -1


def test_stair_like_bands_trigger_cue():
    img = flat(60)
    for y in range(50, GH, 16):          # alternating tread/riser brightness
        img[y:y + 8, :] = 200
    r = analyze_zones(img)
    assert r.stairs, r


def test_vertical_stripes_do_not_trigger_cue():
    img = flat(60)
    img[:, ::8] = 200
    assert not analyze_zones(img).stairs


@pytest.mark.skipif(shutil.which("g++") is None, reason="g++ not installed")
def test_python_matches_firmware(tmp_path):
    exe = tmp_path / "harness"
    subprocess.run(["g++", "-O2", "-o", str(exe), str(HERE / "zone_harness.cpp")], check=True)
    rng = np.random.default_rng(42)
    frames = [flat()]
    for _ in range(30):
        f = rng.integers(0, 255, (GH, GW), dtype=np.uint8)
        # smooth some of them so densities aren't all ~1.0
        k = int(rng.integers(1, 6))
        f = np.repeat(np.repeat(f[::k, ::k], k, 0), k, 1)[:GH, :GW]
        frames.append(np.ascontiguousarray(f))
    s = flat(60)
    for y in range(50, GH, 16):
        s[y:y + 8, :] = 200
    frames.append(s)

    for i, fr in enumerate(frames):
        p = tmp_path / f"f{i}.raw"
        p.write_bytes(fr.tobytes())
        out = subprocess.run([str(exe), str(p)], capture_output=True, text=True, check=True).stdout.split()
        c = (np.float32(out[0]), np.float32(out[1]), np.float32(out[2]), int(out[3]), int(out[4]))
        r = analyze_zones(fr)
        py = (np.float32(r.left), np.float32(r.center), np.float32(r.right), r.stair_lines, r.freer_side)
        assert c == py, f"frame {i}: C={c} Python={py}"

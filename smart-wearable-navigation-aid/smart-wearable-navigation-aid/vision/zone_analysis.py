"""
zone_analysis.py — laptop prototype of the on-device camera logic.

Same algorithm and thresholds as firmware/navigation_aid/camera_zones.h,
so anything you tune here ports straight to the ESP32 (edit both files).

  1) frame -> 160x120 grayscale
  2) edge density in LEFT / CENTER / RIGHT zones of the lower ROI
     -> side with fewer edges = suggested "freer" side
  3) EXPERIMENTAL stair cue: count horizontal edge bands in the CENTER zone

Usage:
  python zone_analysis.py --image path/to/photo.jpg
  python zone_analysis.py --video path/to/walk.mp4
  python zone_analysis.py --webcam 0
  python zone_analysis.py --folder path/to/images --csv results.csv
Keys (video/webcam): q = quit, s = save annotated frame
"""
from __future__ import annotations

import argparse
import csv
import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np

# ---- keep in sync with firmware/navigation_aid/config.h ----
GW, GH = 160, 120
CAM_ROI_TOP_PCT = 35
ZONE_EDGE_THRESH = 40
ZONE_MIN_DIFF = np.float32(0.03)
STAIR_ROW_PCT = 45
STAIR_MIN_LINES = 3


@dataclass
class ZoneResult:
    left: float
    center: float
    right: float
    stair_lines: int
    freer_side: int  # -1 left, +1 right, 0 none

    @property
    def suggestion(self) -> str:
        return {-1: "GO LEFT", 1: "GO RIGHT", 0: "-"}[self.freer_side]

    @property
    def stairs(self) -> bool:
        return self.stair_lines >= STAIR_MIN_LINES


def to_gray_small(frame_bgr: np.ndarray) -> np.ndarray:
    """Approximate the firmware's conversion: green channel, 160x120.
    (The ESP32 uses the 6-bit green of RGB565; here we use 8-bit green and
    quantise it the same way so thresholds behave alike.)"""
    import cv2

    small = cv2.resize(frame_bgr, (GW, GH), interpolation=cv2.INTER_NEAREST)
    g = small[:, :, 1].astype(np.uint16)
    return (((g >> 2) & 0x3F) << 2).astype(np.uint8)


def analyze_zones(gray: np.ndarray) -> ZoneResult:
    """Vectorised version of analyzeZones() in camera_zones.h (bit-exact)."""
    assert gray.shape == (GH, GW), f"expected {(GH, GW)}, got {gray.shape}"
    g = gray.astype(np.int32)
    y0 = GH * CAM_ROI_TOP_PCT // 100
    zw = GW // 3

    ys = slice(y0 + 1, GH - 1)
    xs = slice(1, GW - 1)
    gx = np.abs(g[ys, 2:] - g[ys, :-2])            # p[x+1] - p[x-1]
    gy = np.abs(g[y0 + 2:, xs] - g[y0:GH - 2, xs])  # p[y+1] - p[y-1]

    x_idx = np.arange(1, GW - 1)
    zone = np.minimum(x_idx // zw, 2)
    edge = (gx + gy) > ZONE_EDGE_THRESH

    dens = []
    for z in range(3):
        m = zone == z
        total = edge[:, m].size
        dens.append(np.float32(edge[:, m].sum()) / np.float32(total) if total else np.float32(0))

    center = zone == 1
    horiz = (gy[:, center] > ZONE_EDGE_THRESH) & (gy[:, center] > 2 * gx[:, center])
    row_pix = int(center.sum())
    is_line = horiz.sum(axis=1) * 100 >= STAIR_ROW_PCT * row_pix
    # count rising edges (separate bands, not thick lines)
    lines = int(is_line[0]) + int(np.sum(is_line[1:] & ~is_line[:-1]))

    diff = np.float32(dens[0] - dens[2])
    side = 1 if diff > ZONE_MIN_DIFF else (-1 if diff < -ZONE_MIN_DIFF else 0)
    return ZoneResult(float(dens[0]), float(dens[1]), float(dens[2]), lines, side)


def annotate(frame_bgr: np.ndarray, r: ZoneResult) -> np.ndarray:
    import cv2

    out = frame_bgr.copy()
    h, w = out.shape[:2]
    y0 = int(h * CAM_ROI_TOP_PCT / 100)
    for i in (1, 2):
        cv2.line(out, (w * i // 3, y0), (w * i // 3, h), (0, 255, 255), 1)
    cv2.line(out, (0, y0), (w, y0), (0, 255, 255), 1)
    for i, d in enumerate((r.left, r.center, r.right)):
        cv2.putText(out, f"{d:.2f}", (w * i // 3 + 6, h - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
    label = r.suggestion + ("  | STAIRS?" if r.stairs else "")
    cv2.putText(out, label, (8, 28), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
    cv2.putText(out, f"lines={r.stair_lines}", (8, 56), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 0), 2)
    return out


def run_stream(src) -> None:
    import cv2

    cap = cv2.VideoCapture(src)
    if not cap.isOpened():
        sys.exit(f"cannot open {src}")
    n = 0
    while True:
        ok, frame = cap.read()
        if not ok:
            break
        r = analyze_zones(to_gray_small(frame))
        vis = annotate(frame, r)
        cv2.imshow("zone analysis", vis)
        k = cv2.waitKey(1) & 0xFF
        if k == ord("q"):
            break
        if k == ord("s"):
            cv2.imwrite(f"frame_{n:04d}.png", vis)
            n += 1
    cap.release()
    cv2.destroyAllWindows()


def run_folder(folder: Path, csv_path: Path | None) -> None:
    import cv2

    rows = []
    for p in sorted(folder.glob("*")):
        if p.suffix.lower() not in {".jpg", ".jpeg", ".png", ".bmp"}:
            continue
        img = cv2.imread(str(p))
        if img is None:
            continue
        r = analyze_zones(to_gray_small(img))
        rows.append([p.name, f"{r.left:.3f}", f"{r.center:.3f}", f"{r.right:.3f}", r.stair_lines,
                     int(r.stairs), r.suggestion])
        print(*rows[-1], sep="\t")
    if csv_path:
        with open(csv_path, "w", newline="") as f:
            wr = csv.writer(f)
            wr.writerow(["file", "left", "center", "right", "stair_lines", "stairs_cue", "suggestion"])
            wr.writerows(rows)
        print(f"saved {csv_path}")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--image")
    g.add_argument("--video")
    g.add_argument("--webcam", type=int)
    g.add_argument("--folder")
    ap.add_argument("--csv", help="with --folder: write results to CSV")
    a = ap.parse_args()

    if a.image:
        import cv2

        img = cv2.imread(a.image)
        if img is None:
            sys.exit(f"cannot read {a.image}")
        r = analyze_zones(to_gray_small(img))
        print(r, "->", r.suggestion, "| stairs cue" if r.stairs else "")
        cv2.imshow("zone analysis", annotate(img, r))
        cv2.waitKey(0)
    elif a.video:
        run_stream(a.video)
    elif a.webcam is not None:
        run_stream(a.webcam)
    else:
        run_folder(Path(a.folder), Path(a.csv) if a.csv else None)


if __name__ == "__main__":
    main()

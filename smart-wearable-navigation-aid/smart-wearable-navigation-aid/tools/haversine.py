"""Haversine distance — same formula as haversineM() in the firmware.
Use it to sanity-check landmark distances before a demo.

  python tools/haversine.py 8.5880 123.3410 8.5900 123.3450
"""
import math
import sys


def haversine_m(lat1, lon1, lat2, lon2):
    r = 6371000.0
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dp, dl = math.radians(lat2 - lat1), math.radians(lon2 - lon1)
    a = math.sin(dp / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
    return 2 * r * math.atan2(math.sqrt(a), math.sqrt(1 - a))


def bucket(d):
    return "less than 100 m" if d < 100 else "100 to 500 m" if d < 500 else "more than 500 m"


if __name__ == "__main__":
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    d = haversine_m(*map(float, sys.argv[1:]))
    print(f"{d:.1f} m  -> announced as '{bucket(d)}'")

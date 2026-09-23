// Host-side harness: runs the exact firmware header on a raw 160x120 gray file.
#include <cstdio>
#include <cstdint>
#include "../../firmware/navigation_aid/camera_zones.h"
int main(int argc, char** argv) {
  FILE* f = fopen(argv[1], "rb");
  if (!f || fread(g_gray, 1, GW * GH, f) != GW * GH) return 1;
  fclose(f);
  ZoneResult r; analyzeZones(&r);
  printf("%.9g %.9g %.9g %d %d\n", r.left, r.center, r.right, r.stairLines, r.freerSide);
}

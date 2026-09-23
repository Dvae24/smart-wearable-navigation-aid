// =====================================================================
//  camera_zones.h — lightweight CV, ported 1:1 from vision/zone_analysis.py
//  1) RGB565 frame -> half-size grayscale (160x120)
//  2) Edge density in LEFT / CENTER / RIGHT zones of the lower ROI
//     -> the side with fewer edges is the suggested "freer" side
//  3) EXPERIMENTAL stair cue: count strong horizontal edge rows in CENTER
//  No ML here. The Roboflow model (upstairs/downstairs) is a later swap-in.
// =====================================================================
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include "config.h"

#define GW 160
#define GH 120

struct ZoneResult {
  float left, center, right;   // edge density 0..1 per zone
  int   stairLines;            // horizontal edge rows found in center zone
  int8_t freerSide;            // -1 = left, +1 = right, 0 = no clear preference
};

static uint8_t g_gray[GW * GH];

// esp32-camera delivers RGB565 high byte first.
// We downsample 2x and approximate luma from the 6-bit green channel
// (cheap, and green carries most of the brightness information).
static inline void rgb565ToGrayHalf(const uint8_t* buf, int w, int h) {
  for (int y = 0; y < GH; y++) {
    const uint8_t* row = buf + (size_t)(y * 2) * w * 2;
    for (int x = 0; x < GW; x++) {
      uint16_t px = ((uint16_t)row[x * 4] << 8) | row[x * 4 + 1];
      g_gray[y * GW + x] = (uint8_t)(((px >> 5) & 0x3F) << 2);
    }
  }
}

static inline void analyzeZones(ZoneResult* r) {
  const int y0 = GH * CAM_ROI_TOP_PCT / 100;
  const int zw = GW / 3;
  uint32_t edges[3] = {0, 0, 0}, total[3] = {0, 0, 0};
  int stairLines = 0;
  bool prevRowWasLine = false;

  for (int y = y0 + 1; y < GH - 1; y++) {
    int rowPixCenter = 0, rowHorizCenter = 0;
    for (int x = 1; x < GW - 1; x++) {
      const uint8_t* p = &g_gray[y * GW + x];
      int gx = abs((int)p[1] - (int)p[-1]);
      int gy = abs((int)p[GW] - (int)p[-GW]);
      int z = x / zw; if (z > 2) z = 2;
      total[z]++;
      if (gx + gy > ZONE_EDGE_THRESH) edges[z]++;
      if (z == 1) {
        rowPixCenter++;
        if (gy > ZONE_EDGE_THRESH && gy > 2 * gx) rowHorizCenter++;  // mostly horizontal edge
      }
    }
    bool isLine = rowPixCenter > 0 &&
                  rowHorizCenter * 100 >= STAIR_ROW_PCT * rowPixCenter;
    if (isLine && !prevRowWasLine) stairLines++;   // count separate bands, not thick lines
    prevRowWasLine = isLine;
  }

  r->left   = total[0] ? (float)edges[0] / total[0] : 0;
  r->center = total[1] ? (float)edges[1] / total[1] : 0;
  r->right  = total[2] ? (float)edges[2] / total[2] : 0;
  r->stairLines = stairLines;

  float diff = r->left - r->right;
  if (diff > ZONE_MIN_DIFF)       r->freerSide = +1;  // left is busier -> go right
  else if (diff < -ZONE_MIN_DIFF) r->freerSide = -1;  // right is busier -> go left
  else                            r->freerSide = 0;
}

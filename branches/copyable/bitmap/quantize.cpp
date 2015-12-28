// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You
// may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

// Large parts of the code in this file is modified from original code
// from Leptonica (www.leptonica.com). The original Leptonica license
// statement follows:
/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -
 -  Redistribution and use in source and binary forms, with or without
 -  modification, are permitted provided that the following conditions
 -  are met:
 -  1. Redistributions of source code must retain the above copyright
 -     notice, this list of conditions and the following disclaimer.
 -  2. Redistributions in binary form must reproduce the above
 -     copyright notice, this list of conditions and the following
 -     disclaimer in the documentation and/or other materials
 -     provided with the distribution.
 -
 -  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 -  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 -  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 -  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 -  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 -  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 -  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 -  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 -  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 -  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 -  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================*/
#include <algorithm>
#include <cassert>
#include <cstring> // memcpy
#include "bitmap/alpha-map.hh"
#include "bitmap/bitmap.hh"
#include "bitmap/color-counting.hh"
#include "bitmap/draw.hh"
#include "bitmap/quantize.hh"
#include "util/color.hh"

namespace faint{

static const int EXTRA_RESERVED_COLORS = 25; // "To avoid running out"

// Octree levels (4, 5 and 6 allowed)
static const int CQ_NLEVELS = 5;

class IndexTables{
// Maps colors to indexes into an octree
public:
  IndexTables(int numLevels){
    assert(1 <= numLevels && numLevels <= 6);
    this->red = new unsigned int[256];
    this->green = new unsigned int[256];
    this->blue = new unsigned int[256];

    if (numLevels == 1){
      for (unsigned int i = 0; i != 256; i++) {
        red[i] = (i >> 5) & 0x0004;
        green[i] = (i >> 6) & 0x0002;
        blue[i] = (i >> 7);
      }
    }
    else if (numLevels == 2){
      for (int i = 0; i != 256; i++) {
        red[i] = ((i >> 2) & 0x0020) | ((i >> 4) & 0x0004);
        green[i] = ((i >> 3) & 0x0010) | ((i >> 5) & 0x0002);
        blue[i] = ((i >> 4) & 0x0008) | ((i >> 6) & 0x0001);
      }
    }
    else if (numLevels == 3){
      for (int i = 0; i != 256; i++) {
        red[i] = ((i << 1) & 0x0100) | ((i >> 1) & 0x0020) |
          ((i >> 3) & 0x0004);
        green[i] = (i & 0x0080) | ((i >> 2) & 0x0010) |
          ((i >> 4) & 0x0002);
        blue[i] = ((i >> 1) & 0x0040) | ((i >> 3) & 0x0008) |
          ((i >> 5) & 0x0001);
      }
    }
    else if (numLevels == 4){
      for (int i = 0; i != 256; i++) {
        red[i] = ((i << 4) & 0x0800) | ((i << 2) & 0x0100) |
          (i & 0x0020) | ((i >> 2) & 0x0004);
        green[i] = ((i << 3) & 0x0400) | ((i << 1) & 0x0080) |
          ((i >> 1) & 0x0010) | ((i >> 3) & 0x0002);
        blue[i] = ((i << 2) & 0x0200) | (i & 0x0040) |
          ((i >> 2) & 0x0008) | ((i >> 4) & 0x0001);
      }
    }
    else if (numLevels == 5){
      for (int i = 0; i != 256; i++) {
        red[i] = ((i << 7) & 0x4000) | ((i << 5) & 0x0800) |
          ((i << 3) & 0x0100) | ((i << 1) & 0x0020) |
          ((i >> 1) & 0x0004);
        green[i] = ((i << 6) & 0x2000) | ((i << 4) & 0x0400) |
          ((i << 2) & 0x0080) | (i & 0x0010) |
          ((i >> 2) & 0x0002);
        blue[i] = ((i << 5) & 0x1000) | ((i << 3) & 0x0200) |
          ((i << 1) & 0x0040) | ((i >> 1) & 0x0008) |
          ((i >> 3) & 0x0001);
      }
    }
    else if (numLevels == 6){
      for (int i = 0; i < 256; i++) {
        red[i] = ((i << 10) & 0x20000) | ((i << 8) & 0x4000) |
          ((i << 6) & 0x0800) | ((i << 4) & 0x0100) |
          ((i << 2) & 0x0020) | (i & 0x0004);
        green[i] = ((i << 9) & 0x10000) | ((i << 7) & 0x2000) |
          ((i << 5) & 0x0400) | ((i << 3) & 0x0080) |
          ((i << 1) & 0x0010) | ((i >> 1) & 0x0002);
        blue[i] = ((i << 8) & 0x8000) | ((i << 6) & 0x1000) |
          ((i << 4) & 0x0200) | ((i << 2) & 0x0040) |
          (i & 0x0008) | ((i >> 2) & 0x0001);
      }
    }
  }

  ~IndexTables(){
    delete[] this->red;
    delete[] this->green;
    delete[] this->blue;
  }

  int GetIndex(const Color& c){
    return static_cast<int>(red[c.r] | green[c.g] | blue[c.b]);
  }

  unsigned int* red;
  unsigned int* green;
  unsigned int* blue;
};

struct ColorNode{
  ColorNode()
    : numSamples(0),
      index(0),
      numLeaves(0),
      isLeaf(false)
  {}

  ColRGB center;
  int numSamples;
  int index;
  int numLeaves;
  bool isLeaf;
};

ColorMap::ColorMap(){
  m_colors.reserve(256);
}

void ColorMap::AddColor(const Color& color){
  m_colors.push_back(color);
}

Color ColorMap::GetColor(int index) const{
  assert(index < resigned(m_colors.size()));
  return m_colors[to_size_t(index)];
}

int ColorMap::GetNumColors() const{
  return resigned(m_colors.size());
}

struct CubeIndices{
  CubeIndices(int in_base, int in_sub)
    : base(in_base),
      sub(in_sub)
  {}

  int base;
  int sub;
};

class Octree{
  // An octree with color nodes
public:
  ColorNode*** colorNode_aa;
  ColorMap colorMap;

  Octree(){
    colorNode_aa = new ColorNode**[CQ_NLEVELS + 1];

    // Make array of accumulation node arrays from levels 1 to 5
    for (int level = 0; level <= CQ_NLEVELS; level++){
      int numNodes = 1 << (3 * level);
      ColorNode** colorNode_a = new ColorNode*[numNodes];
      colorNode_aa[level] = colorNode_a;
      for (int i = 0; i != numNodes; i++) {
        colorNode_a[i] = new ColorNode();
      }
    }
  }

  ~Octree(){
    for (int level = 0; level <= CQ_NLEVELS; level++) {
      ColorNode** colorNode_a = colorNode_aa[level];
      int numNodes = 1 << (3 * level);
      for (int i = 0; i < numNodes; i++){
        delete colorNode_a[i];
      }
      delete[] colorNode_a;
    }
    delete[] colorNode_aa;
  }

  const ColorNode& findNode(int octIndex) const{
    for (int level = 2; level < CQ_NLEVELS; level++) {
      CubeIndices ind = GetIndices(octIndex, level);
      ColorNode* node = colorNode_aa[level][ind.base];
      ColorNode* subNode = colorNode_aa[level + 1][ind.sub];

      if (!subNode->isLeaf) {  /* use cell at level above */
        return *node;
      }
      else if (level == CQ_NLEVELS - 1) {  /* reached the bottom */
        return *subNode;
      }
    }
    assert(false); // Cell not found
    return dummy;
  }

  Octree(const Octree&) = delete;
  Octree& operator=(const Octree&) = delete;

private:
  CubeIndices GetIndices(int rgbIndex, int level) const{
    assert(0 <= level && level <= CQ_NLEVELS - 1);
    return CubeIndices(rgbIndex >> (3 * (CQ_NLEVELS - level)),
      rgbIndex >> (3 * (CQ_NLEVELS - 1 - level)));
  }
  ColorNode dummy;
};

static ColRGB get_rgb_from_octcube(int cubeindex, int level){
  /* Bring to format in 21 bits: (r7 g7 b7 r6 g6 b6 ...) */
  /* This is valid for levels from 0 to 6 */
  int rgbindex = cubeindex << (3 * (7 - level));  // upper corner of cube
  rgbindex |= (0x7 << (3 * (6 - level)));   // index to center of cube

  int r = ((rgbindex >> 13) & 0x80) |
    ((rgbindex >> 11) & 0x40) |
    ((rgbindex >> 9) & 0x20) |
    ((rgbindex >> 7) & 0x10) |
    ((rgbindex >> 5) & 0x08) |
    ((rgbindex >> 3) & 0x04) |
    ((rgbindex >> 1) & 0x02);
  int g = ((rgbindex >> 12) & 0x80) |
    ((rgbindex >> 10) & 0x40) |
    ((rgbindex >> 8) & 0x20) |
    ((rgbindex >> 6) & 0x10) |
    ((rgbindex >> 4) & 0x08) |
    ((rgbindex >> 2) & 0x04) |
    (rgbindex & 0x02);
  int b = ((rgbindex >> 11) & 0x80) |
    ((rgbindex >> 9) & 0x40) |
    ((rgbindex >> 7) & 0x20) |
    ((rgbindex >> 5) & 0x10) |
    ((rgbindex >> 3) & 0x08) |
    ((rgbindex >> 1) & 0x04) |
    ((rgbindex << 1) & 0x02);
  return rgb_from_ints(r,g,b);
}

Octree* generate_octree(const Bitmap& bmp, int requestedNumColors, int reservedColors){
  assert(128 <=  requestedNumColors && requestedNumColors <= 256);

  // Canonical index table
  IndexTables tables(CQ_NLEVELS);

  // The octtree (and color map, lol)
  Octree* tree = new Octree();
  ColorMap& colorMap = tree->colorMap;
  const IntSize sz(bmp.GetSize());


  // number of remaining pixels to be assigned
  int numPixels = area(sz);

  // Number of remaining color cells to use
  int numColors = requestedNumColors - reservedColors - EXTRA_RESERVED_COLORS;

  // Average number of pixels left for each color cell
  int pixelsPerCell = numPixels / numColors;

  // Accumulate the centers of each cluster at level CQ_NLEVELS
  ColorNode*** colorNode_aa = tree->colorNode_aa;
  ColorNode** cqca = colorNode_aa[CQ_NLEVELS];
  for (int y = 0; y < sz.h; y++){
    for (int x = 0; x != sz.w; x++){
      int octIndex = tables.GetIndex(get_color_raw(bmp, x, y));
      ColorNode* cell = cqca[octIndex];
      cell->numSamples++;
    }
  }

  const float thresholdFactor[] = {0.01f, 0.01f, 1.0f, 1.0f, 1.0f, 1.0f};

  // Prune back from the lowest level and generate the colormap
  for (int level = CQ_NLEVELS - 1; level >= 2; level--){
    const float thresh = thresholdFactor[level];
    cqca = colorNode_aa[level];
    ColorNode** cqcasub = colorNode_aa[level + 1];
    int numNodes = 1 << (3 * level);

    // Traverse each octIndex (i) at level
    for (int i = 0; i != numNodes; i++) {
      ColorNode* cqc = cqca[i];

      // Check each subnode
      for (int j = 0; j != 8; j++) {
        // octindex at level + 1
        int isub = 8 * i + j;
        ColorNode* cqcsub = cqcasub[isub];

        if (cqcsub->isLeaf){
          // Count the subcube leaves
          cqc->numLeaves++;
          continue;
        }

        if (cqcsub->numSamples >= thresh * static_cast<float>(pixelsPerCell)) {
          // Make it a true leaf
          cqcsub->isLeaf = true;
          if (colorMap.GetNumColors() < 256) {
            // Assign the color index
            cqcsub->index = colorMap.GetNumColors();
            ColRGB rgb = get_rgb_from_octcube(isub, level + 1);
            colorMap.AddColor(Color(rgb,255));
            cqcsub->center = rgb;
          }
          else {
            assert(false); // possibly assigned pixels to wrong color
          }
          cqc->numLeaves++;
          numPixels -= cqcsub->numSamples;
          numColors--;
          if (numColors > 0){
            pixelsPerCell = numPixels / numColors;
          }
          else if (numColors + reservedColors > 0){
            pixelsPerCell = numPixels / (numColors + reservedColors);
          }
          else{
            pixelsPerCell = 1000000;  /* make it big */ // Fixme: What?
          }
        }
      }
      if (cqc->numLeaves > 0 || level == 2) {
        // Make the cube a leaf
        cqc->isLeaf = true;
        if (cqc->numLeaves < 8) {
          // residual CTE cube: acquire the remaining pixels
          for (int j = 0; j != 8; j++) {  // check all subnodes
            int isub = 8 * i + j;
            ColorNode* cqcsub = cqcasub[isub];
            if ( !cqcsub->isLeaf){
              cqc->numSamples += cqcsub->numSamples;
            }
          }
          if (colorMap.GetNumColors() < 256) {
            // assign the color index
            cqc->index = colorMap.GetNumColors();
            ColRGB rgb = get_rgb_from_octcube(i, level);
            colorMap.AddColor(Color(rgb,255));
            cqc->center = rgb;
          }
          else {
            assert(false);
            /* This is very bad.  It will only cause trouble
             * with dithering, and we try to avoid it with
             * EXTRA_RESERVED_PIXELS. */
            // - TODO
          }
          numPixels -= cqc->numSamples;
          numColors--;
          if (numColors > 0){
            pixelsPerCell = numPixels / numColors;
          }
          else if (numColors + reservedColors > 0){
            pixelsPerCell = numPixels / (numColors + reservedColors);
          }
          else{
            pixelsPerCell = 1000000;  /* make it big */ // Fixme: What
          }
        }
      }
      else {
        // absorb all subpixels but don't make it a leaf

        // absorb from all subnodes
        for (int j = 0; j < 8; j++) {
          int isub = 8 * i + j;
          ColorNode* cqcsub = cqcasub[isub];
          cqc->numSamples += cqcsub->numSamples;
        }
      }
    }
  }
  return tree;
}

void get_rgb_line(const Bitmap& bmp, int y, uchar* r, uchar* g, uchar* b){
  for (int x = 0; x != bmp.m_w; x++){
    Color rgb = get_color_raw(bmp, x, y);
    r[x] = rgb.r;
    g[x] = rgb.g;
    b[x] = rgb.b;
  }
}


std::pair<AlphaMap, ColorMap> apply_dithered_quantization(const Bitmap& bmp, const Octree& tree){
  uchar* r8 = new uchar[bmp.m_w];
  uchar* g8 = new uchar[bmp.m_w];
  uchar* b8 = new uchar[bmp.m_w];
  int* r1 = new int[bmp.m_w];
  int* g1 = new int[bmp.m_w];
  int* b1 = new int[bmp.m_w];
  int* r2 = new int[bmp.m_w];
  int* g2 = new int[bmp.m_w];
  int* b2 = new int[bmp.m_w];

  // Start by priming buf2; line 1 is above line 2
  get_rgb_line(bmp, 0, r8, g8, b8);
  for (int x = 0; x != bmp.m_w; x++){
    r2[x] = 64 * static_cast<int>(r8[x]);
    g2[x] = 64 * static_cast<int>(g8[x]);
    b2[x] = 64 * static_cast<int>(b8[x]);
  }

  AlphaMap dst(bmp.GetSize());
  IndexTables tables(CQ_NLEVELS);
  for (int y = 0; y < bmp.m_h - 1; y++) {
    // Swap data 2 --> 1, and read in new line 2
    memcpy(r1, r2, to_size_t(4 * bmp.m_w));
    memcpy(g1, g2, to_size_t(4 * bmp.m_w));
    memcpy(b1, b2, to_size_t(4 * bmp.m_w));
    get_rgb_line(bmp, y + 1, r8, g8, b8);
    for (int x = 0; x != bmp.m_w; x++) {
      r2[x] = 64 * static_cast<int>(r8[x]);
      g2[x] = 64 * static_cast<int>(g8[x]);
      b2[x] = 64 * static_cast<int>(b8[x]);
    }

    for (int x = 0; x != bmp.m_w - 1; x++) {
      Color color = color_from_ints(r1[x] / 64,
        g1[x] / 64,
        b1[x] / 64);
      int octIndex = tables.GetIndex(color);
      const ColorNode& node = tree.findNode(octIndex);
      assert(node.index < 256);
      dst.Set(x,y, static_cast<uchar>(node.index));

      int dif = r1[x] / 8 - 8 * static_cast<int>(node.center.r);
      if (dif != 0) {
        int val1 = r1[x + 1] + 3 * dif;
        int val2 = r2[x] + 3 * dif;
        int val3 = r2[x + 1] + 2 * dif;
        if (dif > 0) {
          r1[x + 1] = std::min(16383, val1);
          r2[x] = std::min(16383, val2);
          r2[x + 1] = std::min(16383, val3);
        }
        else if (dif < 0) {
          r1[x + 1] = std::max(0, val1);
          r2[x] = std::max(0, val2);
          r2[x + 1] = std::max(0, val3);
        }
      }

      dif = g1[x] / 8 - 8 * static_cast<int>(node.center.g);
      if (dif != 0) {
        int val1 = g1[x + 1] + 3 * dif;
        int val2 = g2[x] + 3 * dif;
        int val3 = g2[x + 1] + 2 * dif;
        if (dif > 0) {
          g1[x + 1] = std::min(16383, val1);
          g2[x] = std::min(16383, val2);
          g2[x + 1] = std::min(16383, val3);
        }
        else if (dif < 0) {
          g1[x + 1] = std::max(0, val1);
          g2[x] = std::max(0, val2);
          g2[x + 1] = std::max(0, val3);
        }
      }

      dif = b1[x] / 8 - 8 * static_cast<int>(node.center.b);
      if (dif != 0) {
        int val1 = b1[x + 1] + 3 * dif;
        int val2 = b2[x] + 3 * dif;
        int val3 = b2[x + 1] + 2 * dif;
        if (dif > 0) {
          b1[x + 1] = std::min(16383, val1);
          b2[x] = std::min(16383, val2);
          b2[x + 1] = std::min(16383, val3);
        }
        else if (dif < 0) {
          b1[x + 1] = std::max(0, val1);
          b2[x] = std::max(0, val2);
          b2[x + 1] = std::max(0, val3);
        }
      }
    }

    // Last pixel in row, no downward propagation
    int octIndex = tables.GetIndex(color_from_ints(r1[bmp.m_w-1]/64,
        g1[bmp.m_w-1]/64,
        b1[bmp.m_w-1]/64));
    const ColorNode& cell = tree.findNode(octIndex);
    dst.Set(bmp.m_w - 1,y, static_cast<uchar>(cell.index));
  }

  // Last row of pixels, no leftward propagation
  for (int x = 0; x != bmp.m_w; x++) {

    int octIndex = tables.GetIndex(color_from_ints(r2[x]/64,
        g2[x]/64,
        b2[x]/64));
    const ColorNode& node = tree.findNode(octIndex);
    dst.Set(x,bmp.m_h - 1 , static_cast<uchar>(node.index));
  }
  delete[] r8;
  delete[] g8;
  delete[] b8;
  delete[] r1;
  delete[] g1;
  delete[] b1;
  delete[] r2;
  delete[] g2;
  delete[] b2;
  return std::make_pair(std::move(dst), tree.colorMap);
}

// Based on octreeQuantizePixels
static std::pair<AlphaMap, ColorMap> apply_quantization(const Bitmap& bmp, const Octree& tree){
  // Traverse the tree from the root, looking for lowest cube that is a leaf,
  // and set destination pixel to its color table index value.
  const IntSize sz(bmp.GetSize());

  // Canonical index tables (again?)
  IndexTables tables(CQ_NLEVELS);
  AlphaMap dst(sz);
  for (int y = 0; y < sz.h; y++) {
    for (int x = 0; x != sz.w; x++) {
      Color color = get_color_raw(bmp, x, y);
      int octIndex = tables.GetIndex(color);
      const ColorNode& cell = tree.findNode(octIndex);
      assert(cell.index < 256);
      dst.Set(x,y, static_cast<uchar>(cell.index));
    }
  }
  ColorMap map = tree.colorMap;
  return std::make_pair(std::move(dst), map);
}

static Bitmap bitmap_from_indexed_colors(const AlphaMap& alphaMap, const ColorMap& colorMap){
  const IntSize sz(alphaMap.GetSize());
  Bitmap dst(sz);
  for (int y = 0; y != sz.h; y++){
    for (int x = 0; x != sz.w; x++){
      uchar index = alphaMap.Get(x,y);
      Color color = colorMap.GetColor(index);
      put_pixel_raw(dst, x, y, color);
    }
  }
  return dst;
}

std::pair<AlphaMap, ColorMap> simply_index_the_colors(const Bitmap& bmp){

  color_counts_t colorCounts;
  add_color_counts(bmp, colorCounts);

  ColorMap indexToColor;
  std::map<Color, uchar> colorToIndex;
  for (auto item : colorCounts){
    const Color& c = item.first;
    colorToIndex[c] = static_cast<uchar>(indexToColor.GetNumColors());
    indexToColor.AddColor(c);
  }

  const IntSize sz(bmp.GetSize());
  AlphaMap indexes(sz);
  for (int y = 0; y != sz.h; y++){
    for (int x = 0; x != sz.w; x++){
      indexes.Set(x,y, colorToIndex[get_color_raw(bmp,x,y)]);
    }
  }
  return std::make_pair(std::move(indexes), indexToColor);
}

std::pair<AlphaMap, ColorMap> quantized(const Bitmap& bmp){
  if (count_colors(bmp) <= 256){
    return simply_index_the_colors(bmp);
  }

  const int reserved = 64; // To allow level 2 remainder CTEs
  Octree* tree = generate_octree(bmp, 256, reserved);

  const bool dithering = bmp.m_w >= 250 || bmp.m_h >= 250;

  return dithering ?
    apply_dithered_quantization(bmp, *tree) :
    apply_quantization(bmp, *tree);
}

void quantize(Bitmap& bmp){
  std::pair<AlphaMap, ColorMap> indexed = quantized(bmp);
  bmp = bitmap_from_indexed_colors(indexed.first, indexed.second);
}

} // namespace

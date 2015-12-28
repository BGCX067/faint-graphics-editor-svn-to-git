// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#ifndef FAINT_BMP_TYPES_HH
#define FAINT_BMP_TYPES_HH
#include <cstdint>
#include <string>
#include <array>
#include "geo/int-point.hh"
#include "geo/int-size.hh"
#include "util/hot-spot.hh"

namespace faint {

template<size_t N>
using Buffer = std::array<unsigned char, N>;

class BitmapFileHeader{
  // Information about the type, size, and layout of a file that
  // contains a "DIB".
public:
  uint16_t fileType; // Must be "BM"
  uint32_t length; // Length of file, in bytes.
  uint16_t reserved1; // Must be 0
  uint16_t reserved2; // Must be 0

  // Offset in bytes from the beginning of BitmapFileHeader to the
  // pixel data.
  uint32_t dataOffset;
};

const int BITMAPFILEHEADER_BYTES = 14;

BitmapFileHeader deserialize_BitmapFileHeader(
  const Buffer<BITMAPFILEHEADER_BYTES>&);

Buffer<BITMAPFILEHEADER_BYTES> serialize(const BitmapFileHeader&);

enum class Compression : uint32_t {
  BI_RGB = 0,
  BI_RLE8 = 1,
  BI_RLE4 = 2,
  BI_BITFIELDS = 3,
  BI_JPEG = 4,
  BI_PNG = 5,
  BI_ALPHABITFIELDS = 6,
};

// Returns the compression name (e.g. "BI_RGB") - or "Unknown" if
// invalid.
const char* enum_str(Compression);

class BitmapInfoHeader{
  // A Windows BITMAPINFOHEADER (a.k.a. DIB-header).
public:
  uint32_t headerLen;
  int32_t width;
  int32_t height;
  uint16_t colorPlanes;
  uint16_t bpp;
  Compression compression;
  uint32_t rawDataSize;

  int32_t horizontalResolution;
  int32_t verticalResolution;
  uint32_t paletteColors;
  uint32_t importantColors;
};

const int BITMAPINFOHEADER_BYTES = 40;
static_assert(sizeof(BitmapInfoHeader) == BITMAPINFOHEADER_BYTES,
  "Unexpected size of BitmapInfoHeader");

BitmapInfoHeader deserialize_BitmapInfoHeader(
  const Buffer<BITMAPINFOHEADER_BYTES>&);

Buffer<BITMAPINFOHEADER_BYTES> serialize(const BitmapInfoHeader&);

enum class IconType : uint16_t {
  ICO = 1,
  CUR = 2
};

class IconDir{
  // The top-most header in an icon file.
public:
  uint16_t reserved; // Reserved, must always be 0
  IconType imageType;
  uint16_t imageCount; // Number of images in file
};

const int ICONDIR_BYTES = 6;
static_assert(sizeof(IconDir) == ICONDIR_BYTES, "IconDir struct not 6 bytes");

IconDir deserialize_IconDir(const Buffer<ICONDIR_BYTES>&);

Buffer<ICONDIR_BYTES> serialize(const IconDir&);

class IconDirEntry{
  // A list of these follow the IconDir-header for ico and cur.
public:
  uint8_t width; // Note: 0 should be interpreted as 256,
  uint8_t height; // Note: 0 should be interpreted as 256,
  uint8_t colorCount; // Number of colors in image (0 if bpp >= 8)
  uint8_t reserved;
  uint16_t colorPlanes; // 0 or 1
  uint16_t bpp;
  uint32_t bytes; // The length of the XOR-mask and the AND-mask?
  uint32_t offset; // The offset to the bmp-header or png-data
};

const int ICONDIRENTRY_BYTES = 16;
static_assert(sizeof(IconDirEntry) == ICONDIRENTRY_BYTES,
  "IconDirEntry struct not 16 bytes");

IconDirEntry deserialize_IconDirEntry(const Buffer<ICONDIRENTRY_BYTES>&);

Buffer<ICONDIRENTRY_BYTES> serialize(const IconDirEntry&);

IntSize get_size(const IconDirEntry&);
void set_size(IconDirEntry&, const IntSize& size);

// Cursors only.
HotSpot get_hot_spot(const IconDirEntry&);
void set_hot_spot(IconDirEntry&, const HotSpot&);

// Fixme: Move these somewhere else
class Bitmap; // Fixme
enum class BitmapQuality; // Fixme

inline bool invalid_header_length(int len){
  return len != BITMAPINFOHEADER_BYTES;
}

BitmapInfoHeader create_bitmap_header(const Bitmap& bmp, uint16_t bpp,
  bool andMap);

} // namespace

#endif

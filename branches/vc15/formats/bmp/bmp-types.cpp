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

#include <cassert>
#include "bitmap/bitmap.hh" // Fixme
#include "formats/bmp/file-bmp.hh" // Fixme
#include "formats/bmp/bmp-types.hh"
#include "util/serialize-tuple.hh"
#include "util/deserialize-tuple.hh"

namespace faint{

template<typename T>
auto tied(const T& h){
  return tied(const_cast<T&>(h));
}

template<std::size_t N, typename T>
std::size_t write(Buffer<N>& out, T value, size_t offset){
  assert(offset + sizeof(value) <= N); // Fixme: <= or <?
  memcpy(out.data() + offset, &value, sizeof(value));
  return sizeof(value);
}

auto tied(BitmapFileHeader& h){
  return std::tie(h.fileType, h.length, h.reserved1, h.reserved2, h.dataOffset);
}

BitmapFileHeader deserialize_BitmapFileHeader(
  const Buffer<BITMAPFILEHEADER_BYTES>& a)
{
  BitmapFileHeader h;
  auto t(tied(h));
  deserialize_tuple(t, a);
  return h;
}

Buffer<BITMAPFILEHEADER_BYTES> serialize(const BitmapFileHeader& h){
  return serialize_tuple(tied(h));
}

const char* enum_str(Compression c){
  switch (c){
  case Compression::BI_RGB: return "BI_RGB";
  case Compression::BI_RLE8: return "BI_RLE8";
  case Compression::BI_RLE4: return "BI_RLE4";
  case Compression::BI_BITFIELDS: return "BI_BITFIELDS";
  case Compression::BI_JPEG: return "BI_JPEG";
  case Compression::BI_PNG: return "BI_PNG";
  case Compression::BI_ALPHABITFIELDS: return "BI_ALPHABITFIELDS";
  default: return "Unknown";
  }
}

auto tied(BitmapInfoHeader& h){
  return std::tie(h.headerLen,
    h.width,
    h.height,
    h.colorPlanes,
    h.bpp,
    raw_enum_value(h.compression),
    h.rawDataSize,
    h.horizontalResolution,
    h.verticalResolution,
    h.paletteColors,
    h.importantColors);
}

BitmapInfoHeader deserialize_BitmapInfoHeader(
  const Buffer<BITMAPINFOHEADER_BYTES>& a)
{
  BitmapInfoHeader h;
  auto t(tied(h));
  deserialize_tuple(t, a);
  return h;
}

Buffer<BITMAPINFOHEADER_BYTES> serialize(const BitmapInfoHeader& h){
  return serialize_tuple(tied(h));
}

auto tied(IconDir& h){
  return std::tie(h.reserved,
    raw_enum_value(h.imageType),
    h.imageCount);
}

IconDir deserialize_IconDir(
  const Buffer<ICONDIR_BYTES>& a)
{
  IconDir h;
  auto t(tied(h));
  deserialize_tuple(t, a);
  return h;
}

Buffer<ICONDIR_BYTES> serialize(const IconDir& h){
  return serialize_tuple(tied(h));
}

auto tied(IconDirEntry& h){
  return std::tie(h.width,
    h.height,
    h.reserved,
    h.colorCount,
    h.colorPlanes,
    h.bpp,
    h.bytes,
    h.offset);
}

IconDirEntry deserialize_IconDirEntry(const Buffer<ICONDIRENTRY_BYTES>& a){
  IconDirEntry h;
  auto t(tied(h));
  deserialize_tuple(t, a);
  return h;
}

Buffer<ICONDIRENTRY_BYTES> serialize(const IconDirEntry& h){
  return serialize_tuple(tied(h));
}

static int from_icon_measure(uint8_t measure){
  return measure == 0 ? 256 : static_cast<int>(measure);
}

IntSize get_size(const IconDirEntry& e){
  return IntSize(from_icon_measure(e.width), from_icon_measure(e.height));
}

static uint8_t to_icon_measure(int value){
  assert(value <= 256); // Fixme: Proper error handling
  return value == 256 ? 0 : static_cast<uint8_t>(value);
}

void set_size(IconDirEntry& e, const IntSize& sz){
  e.width = to_icon_measure(sz.w);
  e.height = to_icon_measure(sz.h);
}

HotSpot get_hot_spot(const IconDirEntry& e){
  return HotSpot(e.colorPlanes, e.bpp);
}

void set_hot_spot(IconDirEntry& e, const HotSpot& p){
  e.colorPlanes = static_cast<uint16_t>(p.x); // Fixme: Error check
  e.bpp = static_cast<uint16_t>(p.y); // Fixme: Error check
}

BitmapInfoHeader create_bitmap_header(const Bitmap& bmp, uint16_t bpp,
  bool andMap)
{
  BitmapInfoHeader h;
  h.headerLen = BITMAPINFOHEADER_BYTES;
  IntSize bmpSize = bmp.GetSize();
  h.width = bmpSize.w;
  h.height = bmpSize.h;
  if (andMap){
    // Double height in bitmap header for some reason
    h.height *= 2;
  }
  h.colorPlanes = 1;
  h.bpp = bpp;
  h.compression = Compression::BI_RGB;
  h.rawDataSize = 0; // Dummy value 0 allowed for BI_RGB
  h.horizontalResolution = 1; // Fixme: OK?
  h.verticalResolution = 1; // Fixme: OK?
  h.paletteColors = bpp == 8 ? 256 : 0;
  h.importantColors = 0;
  return h;
}

} // namespace

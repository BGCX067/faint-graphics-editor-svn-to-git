// -*- coding: us-ascii-unix -*-
// Copyright 2012 Lukas Kemmer
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
#include <sstream>
#include <vector>
#include "bitmap/alpha-map.hh"
#include "bitmap/bitmap.hh"
#include "bitmap/draw.hh"
#include "bitmap/quantize.hh"
#include "formats/msw-bmp.hh"
#include "geo/intpoint.hh"
#include "text/formatting.hh"
#include "util/image-props.hh"
#include "util/serialize-bitmap.hh"
#include "util/stream.hh"

namespace faint{

const int g_iconDirLen = 6;
const int g_iconDirEntryLen = 16;
const int g_bmpHeaderLen = 40;

static int and_map_stride(int w){
  return ((w % 32) == 0) ? w / 8 :
    4 * (w / 32 + 1);
}

static int and_map_len(const IntSize& bmpSize){
  return and_map_stride(bmpSize.w) * bmpSize.h;
}

static std::string bmp_compression_name(int compression){
  if (compression == 0){
    return "BI_RGB";
  }
  else if (compression == 1){
    return "BI_RLE8";
  }
  else if (compression == 2){
    return "BI_RLE4";
  }
  else if (compression == 3){
    return "BI_BITFIELDS";
  }
  else if (compression == 4){
    return "BI_JPEG";
  }
  else if (compression == 5){
    return "BI_PNG";
  }
  else if (compression == 6){
    return "BI_ALPHABITFIELDS";
  }
  return "Unknown";
}

static utf8_string error_bpp(size_t num, int bpp){
  std::stringstream ss;
  ss << "The bits-per-pixel setting for this icon is not " <<
    "supported by Faint." << std::endl << std::endl <<
    "Image entry: " << num + 1 << std::endl <<
    "Bits per pixel: " << bpp;
  return utf8_string(ss.str());
}

static utf8_string error_color_planes(size_t num, int planes){
  std::stringstream ss;
  ss << "The number of color planes for this icon is invalid." <<
    "Image entry: " << num + 1 << std::endl <<
    "Color planes: " << planes << " (expected 1)";
  return utf8_string(ss.str());
}

static utf8_string error_compression(size_t num, int compression){
  std::stringstream ss;
  ss << "The compression for this icon is not supported by Faint." << std::endl << std::endl <<
    "Icon#: " << num + 1 << std::endl <<
    "Compression: " << compression << " (" << bmp_compression_name(compression) << ")";
  return utf8_string(ss.str());
}

static utf8_string error_dir_reserved(int value){
  std::stringstream ss;
  ss << "This icon appears broken." << std::endl <<
    "The reserved entry of the IconDir is " << value << " (expected: 0)";
  return utf8_string(ss.str());
}

static utf8_string error_icon_size(size_t num, const IntSize& sz){
  std::stringstream ss;
  ss << "This icon appears broken." << std::endl << std::endl <<
    "The icon size in an IconDirEntry is " << sz.w << "," << sz.h << std::endl <<
    "Image entry: " << num + 1;
  return utf8_string(ss.str());
}

static utf8_string error_image(size_t num){
  std::stringstream ss;
  ss << "This icon appears broken." << std::endl <<
    "Failed reading the header for image entry " << num + 1;
  return utf8_string(ss.str());
}

static utf8_string error_icon_is_cursor(){
  return utf8_string("This icon file contains cursors. This is not yet supported by Faint.");
}

static utf8_string error_cursor_is_icon(){
  return utf8_string("Error: Cursor file contains icons.");
}

static utf8_string error_open_file_read(const FilePath& path){
  return endline_sep("The file could not be opened for reading.", space_sep("Filename:", path.Str()));
}

static utf8_string error_open_file_write(const FilePath& path){
  return endline_sep("The file could not be opened for writing.",
    space_sep("Filename:", path.Str()));
}


static utf8_string error_premature_eof(std::string structure){
  std::stringstream ss;
  ss << "This icon appears broken." << std::endl <<
    "Reading the " << structure << " failed.";
  return utf8_string(ss.str());
}

static utf8_string error_no_images(){
  std::stringstream ss;
  ss << "This icon contains no images." << std::endl << std::endl << "The IconDir images entry is 0.";
  return utf8_string(ss.str());
}

static utf8_string error_truncated_bmp_header(size_t num, int len){
  std::stringstream ss;
  ss << "This icon file appears broken." << std::endl << std::endl <<
    "Image entry: " << num + 1 << std::endl <<
    "Header length: " << len << "(expected >= 40)";
  return utf8_string(ss.str());
}

static utf8_string error_bmp_data(size_t num){
  std::stringstream ss;
  ss << "This icon file appears broken." << std::endl << std::endl <<
    "Reading the data for a bitmap failed." << std::endl <<
    "Image entry: " << num + 1;
  return utf8_string(ss.str());
}

static utf8_string error_truncated_icon_dir(int len){
  std::stringstream ss;
  ss << "This icon file appears broken." << std::endl << std::endl
     << "The icon dir is truncated to " << len << " bytes (expected " << g_iconDirLen << ").";
  return utf8_string(ss.str());
}

static utf8_string error_truncated_png_data(size_t num){
  std::stringstream ss;
  ss << "This icon file appears broken." << std::endl <<
    "Reading an embedded png-compressed image failed." << std::endl <<
    "Image entry: " << num + 1;
  return utf8_string(ss.str());
}

static utf8_string error_read_to_offset(size_t num, int offset){
  std::stringstream ss;
  ss << "This icon file appears broken." << std::endl << std::endl
     << "Seeking to an image offset failed." << std::endl
     << "Image entry: " << num << std::endl
     << "At: " << std::hex << offset << std::endl;
  return utf8_string(ss.str());
}

static utf8_string error_unknown_image_type(int type){
  std::stringstream ss;
  ss << "The image type in this icon file is not recognized by Faint."
     << std::endl << std::endl
     << "Image type: " << type << " (expected 1 for icon or 2 for cursor).";
  return utf8_string(ss.str());
}

class LoadIconError{
public:
  LoadIconError(const utf8_string& error)
    : m_error(error)
  {}
  utf8_string GetString() const{
    return m_error;
  }
private:
  utf8_string m_error;
};

static bool invalid_bpp_allow_zero(int bpp){
  return bpp != 32 && bpp != 0; // Fixme: Support other alternatives
}

static bool invalid_bpp_no_zero(int bpp){
  return bpp != 32 && bpp != 1; // Fixme: Support other alternatives
}

static bool invalid_compression(int compression){
  const int BI_RGB = 0;
  if (compression == BI_RGB){
    return false; // No compression. This is supported
  }
  return true;
}

static bool invalid_header_length(int len){
  return len < g_bmpHeaderLen; // Fixme: In addition to truncation, check invalid > 40 lengths.
}

static int to_int(char b0, char b1){
  return resigned((uint(uchar(b1)) << 8) | (uint(uchar(b0))));
}

static int to_int(char b0, char b1, char b2, char b3){
  return resigned((uint(uchar(b3)) << 24) | (uint(uchar(b2)) << 16) |
    (uint(uchar(b1)) << 8) | (uint(uchar(b0))));
}

static uint to_uint(char b){
  return uint(uchar(b));
}

static uint to_uint(char low, char high){
  return (uint(uchar(high)) << 8) | (uint(uchar(low)));
}

static uint read_2u(BinaryReader& f){
  char data[2];
  f.read(data,2);
  return to_uint(data[0], data[1]);
}

static int read_4(BinaryReader& f){
  char data[4];
  f.read(data, 4);
  return to_int(data[0], data[1], data[2], data[3]);
}

static void write_1b(BinaryWriter& out, int v){
  char data((char)static_cast<uint>(v));
  out.write(&data, 1);
}

static void write_2b(BinaryWriter& out, int v){
  char data[2];
  data[0] = static_cast<char>(static_cast<uint>(v) & 0xffff);
  data[1] = static_cast<char>((static_cast<uint>(v) >> 8) & 0xffff);
  out.write(data, 2);
}

static void write_4b(BinaryWriter& out, int v){
  char data[4] = { static_cast<char>(static_cast<uint>(v) & 0xff),
                   static_cast<char>((static_cast<uint>(v) >> 8) & 0xff),
                   static_cast<char>((static_cast<uint>(v) >> 16) & 0xff),
                   static_cast<char>((static_cast<uint>(v) >> 24) & 0xff)};
  out.write(data,4);
}

static IntSize read_bmp_size(BinaryReader& f){
  IntSize sz(read_4(f), read_4(f));
  if (!f.good()){
    throw LoadIconError(error_premature_eof("BitmapHeader")); // Fixme
  }
  return sz;
}

static IntSize read_ico_size(BinaryReader& f){
  char data[2] = {0x00, 0x00};
  f.read(data, 2);
  if (!f.good()){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  int w = static_cast<int>((uchar)data[0]);
  int h = static_cast<int>((uchar)data[1]);
  return IntSize(w == 0 ? 256 : w,
    h == 0 ? 256 : h);
}

struct BitmapHeader{
  int headerLen;
  IntSize size;
  int colorPlanes;
  int bpp;
  int compression;
  int rawDataSize;
  int horizontalResolution;
  int verticalResolution;
  int paletteColors;
  int importantColors;
};

struct IconDir{
  // The top-most header in an icon file
  int len;
  int reserved;
  int imageType;
  int imageCount;
};

struct IconDirEntry{
  // A list of these follow the IconDir-header.
  IntSize size;
  bool palette;
  int reserved;
  int colorsInPalette;
  int colorPlanes;
  int bpp;
  int bytes; // The length of the XOR-mask and the AND-mask
  int offset; // The offset to the bmp-header or png-data
};

static IconDirEntry parse_icon_dir_entry(BinaryReader& f){
  IconDirEntry icon;
  icon.size = read_ico_size(f);
  char data[4];
  // Colors in palette
  f.read(data, 1); // Fixme: Check for read errors
  if (!f.good()){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  if (data[0] == 0){
    icon.palette = false;
    icon.colorsInPalette = 0;
  }
  else {
    icon.palette = true;
    icon.colorsInPalette = int(data[0]);
  }

  f.read(data, 1);
  if (!f.good()){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  icon.reserved = data[0];

  // Color planes (hotspot x for cursors, ignored)
  f.read(data, 2);
  if (!f.good()){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  icon.colorPlanes = to_int(data[0], data[1]);

  // Icons: bpp (maybe ignored in favor of bmp-header)
  // Cursors: hotspot y
  f.read(data,2); // Fixme: Check for read errors
  if (!f.good()){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  icon.bpp = to_int(data[0], data[1]);
  f.read(data,4); // Fixme: Check for read errors
  if (!f.good()){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  icon.bytes = to_int(data[0], data[1], data[2], data[3]);

  f.read(data,4); // Fixme: Check for read errors
  if (!f.good()){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  int offset = to_int(data[0], data[1], data[2], data[3]);
  icon.offset = offset;
  return icon;
}

static bool is_png(char* data){
  return to_uint(data[0]) == 0x89 &&
    to_uint(data[1]) == 0x50 &&
    to_uint(data[2]) == 0x4e &&
    to_uint(data[3]) == 0x47 &&
    to_uint(data[4]) == 0x0d &&
    to_uint(data[5]) == 0x0a &&
    to_uint(data[6]) == 0x1a &&
    to_uint(data[7]) == 0x0a;
}

static bool peek_png_signature(BinaryReader& f){
  std::streampos oldPos = f.tellg();
  char signature[8];
  f.read(signature, 8);
  if (!f.good() || f.eof()){
    return false;
  }
  f.seekg(oldPos);
  return is_png(signature);
}

static void test_bitmap_header(size_t iconNum, const BitmapHeader& h){
  if (invalid_header_length(h.headerLen)){
    throw LoadIconError(error_truncated_bmp_header(iconNum, h.headerLen));
  }
  if (invalid_compression(h.compression)){
    throw LoadIconError(error_compression(iconNum, h.compression));
  }
  if (invalid_bpp_no_zero(h.bpp)) {
    throw LoadIconError(error_bpp(iconNum, h.bpp));
  }
  if (h.colorPlanes != 1){
    throw LoadIconError(error_color_planes(iconNum, h.colorPlanes));
  }
}

static void test_icon_dir(const IconDir& dir, bool icon){
  if (dir.len != g_iconDirLen){
    throw LoadIconError(error_truncated_icon_dir(dir.len));
  }
  if (dir.reserved != 0){
    throw LoadIconError(error_dir_reserved(dir.reserved));
  }
  if (icon){
    if (dir.imageType == 2){
      throw LoadIconError(error_icon_is_cursor());
    }
    if (dir.imageType != 1){
      throw LoadIconError(error_unknown_image_type(dir.imageType));
    }
  }
  else {
    if (dir.imageType == 1){
      throw LoadIconError(error_cursor_is_icon());
    }
    if (dir.imageType != 2){
      throw LoadIconError(error_unknown_image_type(dir.imageType));
    }
  }
  if (dir.imageCount == 0){
    throw LoadIconError(error_no_images());
  }
}

static void test_icon_dir_entry(size_t iconNum, const IconDirEntry& icon, bool cursor){
  if (!cursor && invalid_bpp_allow_zero(icon.bpp)){
    throw LoadIconError(error_bpp(iconNum, icon.bpp));
  }
  if (icon.size.w <= 0 || icon.size.h <= 0){
    throw LoadIconError(error_icon_size(iconNum, icon.size));
  }
}

static void test_icon_dir_entries(std::vector<IconDirEntry>& entries, bool cursor){
  for (size_t i = 0; i != entries.size(); i++){
    test_icon_dir_entry(i, entries[i], cursor);
  }
}

static BitmapHeader parse_bitmap_header(BinaryReader& f){
  BitmapHeader h;
  h.headerLen = read_4(f);
  h.size = read_bmp_size(f);
  h.colorPlanes = static_cast<int>(read_2u(f));
  h.bpp = static_cast<int>(read_2u(f));
  h.compression = read_4(f);
  h.rawDataSize = read_4(f);
  h.horizontalResolution = read_4(f);
  h.verticalResolution = read_4(f);
  h.paletteColors = read_4(f);
  h.importantColors = read_4(f);
  return h;
}

static IconDir parse_icon_dir(BinaryReader& f){
  IconDir d;
  d.len = d.reserved = d.imageType = d.imageCount = 0;
  std::streampos oldPos = f.tellg();
  char data[6];
  f.read(data, 6);
  d.len = static_cast<int>(f.tellg() - oldPos);
  if (f.good()){
    d.reserved = to_int(data[0], data[1]);
    d.imageType = to_int(data[2], data[3]);
    d.imageCount = to_int(data[4], data[5]);
  }
  return d;
}

static std::vector<IconDirEntry> parse_icon_dir_entries(BinaryReader& f, int numIcons){
  assert(numIcons >= 0);
  std::vector<IconDirEntry> v;
  for (int i = 0; i != numIcons; i++){
    v.push_back(parse_icon_dir_entry(f));
    if (!f.good()){
      return v;
    }
  }
  return v;
}

// Returns the stride for rows padded to multiples of 4-bytes
static int get_bmp_row_stride(int bpp, int w){
  return ((bpp * w + 31) / 32) * 4;
}

static Bitmap ico_parse_1bpp(BinaryReader& f, const IntSize& bitmapSize){
  char* palette = new char[8];
  f.read(palette, 8);
  if (!f.good()){
    delete[] palette;
    return Bitmap();
  }

  Color c0(color_from_ints(palette[2],palette[1],palette[0],255));
  Color c1(color_from_ints(palette[6],palette[5],palette[4],255));

  int rowSize = ((1 * bitmapSize.w + 31) / 32) * 4;
  int bufLen = rowSize * bitmapSize.h;

  char* pixelData = new char[bufLen];
  f.read(pixelData, bufLen);
  if (!f.good()){
    delete[] pixelData;
    return Bitmap();
  }

  // The size from the IconDirEntry is the exact amount of pixels
  // this image should have.
  const IntSize& sz(bitmapSize);
  Bitmap bmp(sz);
  for (int y = 0; y != sz.h; y++){
    for (int x = 0; x != sz.w; x++){
      uint value = to_uint(pixelData[rowSize*y + x /8]);
      if (value & (1 << (7 - x % 8))){
        put_pixel_raw(bmp, x, bmp.m_h - y - 1, c1);
      }
      else{
        put_pixel_raw(bmp, x, bmp.m_h - y - 1, c0);
      }
    }
  }

  char* mask = new char[bufLen];
  f.read(mask, bufLen);
  if (!f.good()){
    delete[] mask;
    delete[] pixelData;
    return Bitmap();
  }
  for (int y = 0; y != sz.h; y++){
    for (int x = 0; x != sz.w; x++){
      uint value = to_uint(mask[rowSize*y + x /8]);
      if ((value & (1 << (7 - x % 8)))){
        put_pixel_raw(bmp, x, bmp.m_h - y - 1, Color(0,0,0,0));
      }
    }
  }

  delete[] mask;
  delete[] pixelData;
  return bmp;
}

static Bitmap ico_parse_32bpp_BI_RGB(BinaryReader& f, const IntSize& bitmapSize){
  int bypp = 4;
  // The size from the bmp-header. May have larger height than the size
  // in the IconDirEntry. (Fixme: Why?)
  int bufLen = area(bitmapSize) * bypp;
  char* pixelData = new char[bufLen];
  f.read(pixelData, bufLen);
  if (!f.good()){
    return Bitmap();
  }

  // The size from the IconDirEntry is the exact amount of pixels
  // this image should have.
  const IntSize& sz(bitmapSize);
  Bitmap bmp(sz);
  for (int y = 0; y != sz.h; y++){
    for (int x = 0; x != sz.w; x++){
      uint b = to_uint(pixelData[y * sz.w * bypp + x * bypp]);
      uint g = to_uint(pixelData[y * sz.w * bypp + x * bypp + 1]);
      uint r = to_uint(pixelData[y * sz.w * bypp + x * bypp + 2]);
      uint a = to_uint(pixelData[y * sz.w * bypp + x * bypp + 3]);
      put_pixel_raw(bmp, x, bmp.m_h - y - 1, color_from_uints(r,g,b,a));
    }
  }
  delete[] pixelData;
  return bmp;
}

static Bitmap ico_parse_png(BinaryReader& f, int len){
  assert(len > 0);
  char* data = new char[len];
  f.read(data, len);
  Bitmap bmp(from_png(data, to_size_t(len)));
  delete[] data;
  return bmp;
}

static void write_32bpp_BI_RGB(BinaryWriter& out, const Bitmap& bmp){
  // The size from the bmp-header. May have larger height than the size
  // in the IconDirEntry. (Fixme: Why?)

  // Write the pixel data (in archaic ICO-terms, the XOR-mask)
  for (int y = 0; y != bmp.m_h; y++){
    for (int x = 0; x != bmp.m_w; x++){
      Color c(get_color_raw(bmp, x, bmp.m_h - y - 1));
      write_1b(out, c.b);
      write_1b(out, c.g);
      write_1b(out, c.r);
      write_1b(out, c.a);
    }
  }

  // Write the AND-mask
  int len = and_map_len(bmp.GetSize());
  for (int i = 0; i != len; i++){
    write_1b(out,0xff);
  }
}

static void write_8bpp_BI_RGB(BinaryWriter& out, const Bitmap& bmp){
  std::pair<AlphaMap, ColorMap> indexed = quantized(bmp);
  const ColorMap& palette = indexed.second;
  int colorsInPalette = palette.GetNumColors();
  assert(colorsInPalette <= 256);
  for (int i = 0; i != palette.GetNumColors(); i++){
    const Color& c = palette.GetColor(i);
    write_1b(out, c.b);
    write_1b(out, c.g);
    write_1b(out, c.r);
    write_1b(out, 0);
  }
  for (int i = colorsInPalette; i != 256; i++){
    write_1b(out, 0);
    write_1b(out, 0);
    write_1b(out, 0);
    write_1b(out, 0);
  }

  const AlphaMap& indexMap = indexed.first;
  const IntSize sz(indexMap.GetSize());
  int padBytes = get_bmp_row_stride(8, sz.w) - sz.w;
  for (int y = 0; y != sz.h; y++){
    for (int x = 0; x != sz.w; x++){
      write_1b(out, indexMap.Get(x, sz.h - y - 1));
    }
    for (int i = 0; i != padBytes; i++){
      write_1b(out, 0);
    }
  }
}

static void write_8bpp_BI_RGB_grayscale(BinaryWriter& out, const Bitmap& bmp){
  int padBytes = get_bmp_row_stride(8, bmp.m_w) - bmp.m_w;;
  for (int y = 0; y != bmp.m_h; y++){
    for (int x = 0; x != bmp.m_w; x++){
      Color c(get_color_raw(bmp, x, bmp.m_h - y - 1));
      write_1b(out, (c.b + c.g + c.r) / 3);
    }
    for (int i = 0; i != padBytes; i++){
      write_1b(out, 0);
    }
  }
}

void load_icon(const FilePath& filename, ImageProps& props){
  try {
    BinaryReader f(filename);
    if (!f.good()){
      props.SetError(error_open_file_read(filename));
      return;
    }

    IconDir iconDir(parse_icon_dir(f));
    if (!f.good()){
      props.SetError(error_premature_eof("ICONDIR"));
      return;
    }
    test_icon_dir(iconDir, true);
    std::vector<IconDirEntry> iconEntries = parse_icon_dir_entries(f, iconDir.imageCount);
    if (!f.good()){
      throw LoadIconError(error_premature_eof("ICONDIRENTRY"));
    }
    test_icon_dir_entries(iconEntries, false);

    for (size_t i = 0; i != iconEntries.size(); i++){
      IconDirEntry& icon = iconEntries[i];
      f.seekg(icon.offset);
      if (!f.good() || f.eof()){
        throw LoadIconError(error_read_to_offset(i, icon.offset));
      }

      bool png_compressed = peek_png_signature(f);
      if (!f.good() || f.eof()){
        throw LoadIconError(error_image(i));
      }
      if (png_compressed){
        Bitmap bmp(ico_parse_png(f, icon.bytes));
        if (!f.good()){
          throw LoadIconError(error_truncated_png_data(i));
        }
        if (f.eof() && i < iconEntries.size() -1){
          throw LoadIconError(utf8_string("Premature EOF")); // Fixme
        }
        props.AddFrame(std::move(bmp), FrameInfo());
      }
      else {
        BitmapHeader bmpHeader(parse_bitmap_header(f));
        if (!f.good()){
          throw LoadIconError(error_image(i));
        }
        test_bitmap_header(i, bmpHeader);
        if (bmpHeader.bpp == 32){
          Bitmap bmp(ico_parse_32bpp_BI_RGB(f, icon.size));
          if (!f.good()){
            throw LoadIconError(error_bmp_data(i));
          }
          assert(bitmap_ok(bmp));
          props.AddFrame(std::move(bmp), FrameInfo());
        }
        else if (bmpHeader.bpp == 1){
          Bitmap bmp(ico_parse_1bpp(f, icon.size));
          if (!f.good()){
            throw LoadIconError(error_bmp_data(i));
          }
          assert(bitmap_ok(bmp));
          props.AddFrame(std::move(bmp), FrameInfo());
        }
        else {
          // Valid_bitmap_header should prevent this case
          assert(false); // Only 32bpp supported.
        }
      }
    }
  }
  catch (const LoadIconError& error){
    props.SetError(error.GetString());
  }
}

void load_cursor(const FilePath& filename, ImageProps& props){ // Fixme: Mostly duplicates load_icon
  try {
    BinaryReader f(filename);
    if (!f.good()){
      props.SetError(error_open_file_read(filename));
      return;
    }

    IconDir iconDir(parse_icon_dir(f));
    if (!f.good()){
      props.SetError(error_premature_eof("ICONDIR"));
      return;
    }

    test_icon_dir(iconDir, false);
    std::vector<IconDirEntry> iconEntries = parse_icon_dir_entries(f, iconDir.imageCount);
    if (!f.good()){
      throw LoadIconError(error_premature_eof("ICONDIRENTRY"));
    }
    test_icon_dir_entries(iconEntries, true);

    for (size_t i = 0; i != iconEntries.size(); i++){
      IconDirEntry& icon = iconEntries[i];
      f.seekg(icon.offset);
      if (!f.good() || f.eof()){
        throw LoadIconError(error_read_to_offset(i, icon.offset));
      }

      bool png_compressed = peek_png_signature(f);
      if (!f.good() || f.eof()){
        throw LoadIconError(error_image(i));
      }

      hot_spot_t hotSpot(IntPoint(icon.colorPlanes,icon.bpp));
      if (png_compressed){
        Bitmap bmp(ico_parse_png(f, icon.bytes));
        if (!f.good()){
          throw LoadIconError(error_truncated_png_data(i));
        }
        if (f.eof() && i < iconEntries.size() -1){
          throw LoadIconError(utf8_string("Premature EOF")); // Fixme
        }
        props.AddFrame(std::move(bmp), FrameInfo(hotSpot));
      }
      else {
        BitmapHeader bmpHeader(parse_bitmap_header(f));
        if (!f.good()){
          throw LoadIconError(error_image(i));
        }
        test_bitmap_header(i, bmpHeader);
        if (bmpHeader.bpp == 32){
          Bitmap bmp(ico_parse_32bpp_BI_RGB(f, icon.size));
          if (!f.good()){
            throw LoadIconError(error_bmp_data(i));
          }
          assert(bitmap_ok(bmp));
          props.AddFrame(std::move(bmp), FrameInfo(hotSpot));
        }
        else if (bmpHeader.bpp == 1){
          Bitmap bmp(ico_parse_1bpp(f, icon.size));
          if (!f.good()){
            throw LoadIconError(error_bmp_data(i));
          }
          assert(bitmap_ok(bmp));
          props.AddFrame(std::move(bmp), FrameInfo(hotSpot));
        }
        else {
          // Valid_bitmap_header should prevent this case
          assert(false); // Unsupported bpp
        }
      }
    }
  }
  catch (const LoadIconError& error){
    props.SetError(error.GetString());
  }
}

typedef std::vector<Bitmap> bmp_vec_t;

static BitmapHeader create_bitmap_header(const Bitmap& bmp, int bpp, bool andMap){
  BitmapHeader h;
  h.headerLen = g_bmpHeaderLen;
  IntSize bmpSize = bmp.GetSize();
  h.size = bmpSize;
  if (andMap){
    // Double height in bitmap header for some reason
    h.size.h *= 2;
  }
  h.colorPlanes = 1;
  h.bpp = bpp;
  h.compression = 0;
  h.rawDataSize = area(bmp.GetSize()) * bpp / 8 + (andMap ? and_map_len(bmpSize) : 0);
  h.horizontalResolution = 1; // Fixme: OK?
  h.verticalResolution = 1; // Fixme: OK?
  h.paletteColors = bpp == 8 ? 256 : 0;
  h.importantColors = 0;
  return h;
}

static std::vector<BitmapHeader> create_bitmap_headers(const bmp_vec_t& bitmaps){
  std::vector<BitmapHeader> v;
  for (size_t i = 0; i != bitmaps.size(); i++){
    v.push_back(create_bitmap_header(bitmaps[i], 32, true));
  }
  return v;
}

static IconDir create_icon_dir(const bmp_vec_t& bitmaps, int imageType){
  assert(imageType == 1 || imageType == 2); // 1 == Icon, 2 == Cursor
  IconDir iconDir;
  iconDir.len = g_iconDirLen;
  iconDir.reserved = 0;
  iconDir.imageType = imageType; // Icon
  iconDir.imageCount = resigned(bitmaps.size());
  return iconDir;
}

static std::vector<IconDirEntry> create_icon_dir_entries(const bmp_vec_t& bitmaps){
  std::vector<IconDirEntry> v;

  int offset = g_iconDirLen + resigned(bitmaps.size()) * g_iconDirEntryLen;
  for (size_t i = 0; i != bitmaps.size(); i++){
    assert(i < bitmaps.size());
    const Bitmap& bmp = bitmaps[i];
    IconDirEntry entry;
    entry.size = bmp.GetSize();
    entry.palette = false;
    entry.reserved = 0;
    entry.colorsInPalette = 0;
    entry.colorPlanes = 1;
    entry.bpp = 32;
    entry.bytes = area(entry.size) * 4 + and_map_len(bmp.GetSize()) + g_bmpHeaderLen;
    entry.offset = offset;
    v.push_back(entry);
    offset += entry.bytes;
  }
  return v;
}

static std::vector<IconDirEntry> create_cursor_dir_entries(const bmp_vec_t& bitmaps, const std::vector<IntPoint>& hotSpots){
  std::vector<IconDirEntry> v;
  int offset = g_iconDirLen + resigned(bitmaps.size()) * g_iconDirEntryLen;
  for (size_t i = 0; i != bitmaps.size(); i++){
    assert(i < bitmaps.size());
    const Bitmap& bmp = bitmaps[i];
    IconDirEntry entry;
    entry.size = bmp.GetSize();
    entry.palette = false;
    entry.reserved = 0;
    entry.colorsInPalette = 0;
    entry.colorPlanes = hotSpots[i].x; // Fixme: Check range
    entry.bpp = hotSpots[i].y; // Fixme: Check range
    entry.bytes = area(entry.size) * 4 + and_map_len(bmp.GetSize()) + g_bmpHeaderLen;
    entry.offset = offset;
    v.push_back(entry);
    offset += entry.bytes;
  }
  return v;
}

SaveResult save_icon(const FilePath& filePath,  const bmp_vec_t& bitmaps){
  assert(!bitmaps.empty());
  for (size_t i = 0; i != bitmaps.size(); i++){
    const Bitmap& bmp(bitmaps[i]);
    if (bmp.m_w > 256 || bmp.m_h > 256){
      return SaveResult::SaveFailed(utf8_string("Maximum size for icons is 256x256"));
    }
  }
  IconDir iconDir = create_icon_dir(bitmaps, 1);
  std::vector<IconDirEntry> iconDirEntries = create_icon_dir_entries(bitmaps);
  std::vector<BitmapHeader> bmpHeaders = create_bitmap_headers(bitmaps);
  BinaryWriter out(filePath);
  if (!out.good()){
    return SaveResult::SaveFailed(error_open_file_write(filePath));
  }
  write_2b(out, iconDir.reserved);
  write_2b(out, iconDir.imageType);
  write_2b(out, iconDir.imageCount);

  for (size_t i = 0; i != iconDirEntries.size(); i++){
    const IconDirEntry& entry(iconDirEntries[i]);
    write_1b(out, entry.size.w);
    write_1b(out, entry.size.h);
    write_1b(out, entry.colorsInPalette);
    write_1b(out, entry.reserved);
    write_2b(out, entry.colorPlanes);
    write_2b(out, entry.bpp);
    write_4b(out, entry.bytes);
    write_4b(out, entry.offset);
  }

  assert(bitmaps.size() == bmpHeaders.size());
  for (size_t i = 0; i != bitmaps.size(); i++){
    const BitmapHeader& bmpHdr = bmpHeaders[i];
    write_4b(out, bmpHdr.headerLen);
    write_4b(out, bmpHdr.size.w);
    write_4b(out, bmpHdr.size.h);
    write_2b(out, bmpHdr.colorPlanes);
    write_2b(out, bmpHdr.bpp);
    write_4b(out, bmpHdr.compression);
    write_4b(out, bmpHdr.rawDataSize);
    write_4b(out, bmpHdr.horizontalResolution);
    write_4b(out, bmpHdr.verticalResolution);
    write_4b(out, bmpHdr.paletteColors);
    write_4b(out, bmpHdr.importantColors);

    const Bitmap& bmp = bitmaps[i];
    write_32bpp_BI_RGB(out, bmp);
  }
  return SaveResult::SaveSuccessful();
}

SaveResult save_cursor(const FilePath& filePath,  const bmp_vec_t& bitmaps, const std::vector<IntPoint>& hotSpots){
  assert(!bitmaps.empty());
  for (size_t i = 0; i != bitmaps.size(); i++){
    const Bitmap& bmp(bitmaps[i]);
    if (bmp.m_w > 256 || bmp.m_h > 256){
      return SaveResult::SaveFailed(utf8_string("Maximum size for cursors is 256x256"));
    }
  }
  IconDir iconDir = create_icon_dir(bitmaps, 2);
  std::vector<IconDirEntry> iconDirEntries = create_cursor_dir_entries(bitmaps, hotSpots);
  std::vector<BitmapHeader> bmpHeaders = create_bitmap_headers(bitmaps);
  BinaryWriter out(filePath);
  if (!out.good()){
    return SaveResult::SaveFailed(error_open_file_write(filePath));
  }

  write_2b(out, iconDir.reserved);
  write_2b(out, iconDir.imageType);
  write_2b(out, iconDir.imageCount);

  for (size_t i = 0; i != iconDirEntries.size(); i++){
    const IconDirEntry& entry(iconDirEntries[i]);
    write_1b(out, entry.size.w);
    write_1b(out, entry.size.h);
    write_1b(out, entry.colorsInPalette);
    write_1b(out, entry.reserved);
    write_2b(out, entry.colorPlanes);
    write_2b(out, entry.bpp);
    write_4b(out, entry.bytes);
    write_4b(out, entry.offset);
  }

  assert(bitmaps.size() == bmpHeaders.size());
  for (size_t i = 0; i != bitmaps.size(); i++){
    const BitmapHeader& bmpHdr = bmpHeaders[i];
    write_4b(out, bmpHdr.headerLen);
    write_4b(out, bmpHdr.size.w);
    write_4b(out, bmpHdr.size.h);
    write_2b(out, bmpHdr.colorPlanes);
    write_2b(out, bmpHdr.bpp);
    write_4b(out, bmpHdr.compression);
    write_4b(out, bmpHdr.rawDataSize);
    write_4b(out, bmpHdr.horizontalResolution);
    write_4b(out, bmpHdr.verticalResolution);
    write_4b(out, bmpHdr.paletteColors);
    write_4b(out, bmpHdr.importantColors);

    const Bitmap& bmp = bitmaps[i];
    write_32bpp_BI_RGB(out, bmp);
  }
  return SaveResult::SaveSuccessful();
}

static void write_grayscale_color_table(BinaryWriter& out){
  for (unsigned int i = 0; i != 256; i++){
    write_1b(out, (char)i);
    write_1b(out, (char)i);
    write_1b(out, (char)i);
    write_1b(out, 0);
  }
}

SaveResult save_bitmap(const FilePath& filePath, const Bitmap& bmp, BitmapQuality quality){
  int bpp = quality == BitmapQuality::COLOR_24BIT ? 32 : 8;
  BitmapHeader bmpHdr = create_bitmap_header(bmp, bpp, false);
  BinaryWriter out(filePath);
  if (!out.good()){
    return SaveResult::SaveFailed(error_open_file_write(filePath));
  }

  out.write("BM", 2);
  write_4b(out, 54 + 256 * 4 + area(bmp.GetSize())); // FIXME: Total byte size
  write_4b(out, 0); // Unused
  write_4b(out, 54 + 256); // FIXME: pixel data offset
  write_4b(out, bmpHdr.headerLen);
  write_4b(out, bmpHdr.size.w);
  write_4b(out, bmpHdr.size.h);
  write_2b(out, bmpHdr.colorPlanes);
  write_2b(out, bmpHdr.bpp);
  write_4b(out, bmpHdr.compression);
  write_4b(out, bmpHdr.rawDataSize);
  write_4b(out, bmpHdr.horizontalResolution);
  write_4b(out, bmpHdr.verticalResolution);
  write_4b(out, bmpHdr.paletteColors);
  write_4b(out, bmpHdr.importantColors);

  switch(quality){
  case BitmapQuality::COLOR_8BIT:
    write_8bpp_BI_RGB(out, bmp);
    return SaveResult::SaveSuccessful();
  case BitmapQuality::COLOR_24BIT:
    write_32bpp_BI_RGB(out, bmp);
    return SaveResult::SaveSuccessful();
  case BitmapQuality::GRAY_8BIT:
    write_grayscale_color_table(out);
    write_8bpp_BI_RGB_grayscale(out, bmp);
    return SaveResult::SaveSuccessful();
  }

  assert(false); // Invalid quality
  return SaveResult::SaveFailed(utf8_string("Internal error in save_bitmap"));
}

} // namespace

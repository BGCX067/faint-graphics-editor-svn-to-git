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
#include <fstream>
#include <sstream>
#include <vector>
#include "bitmap/bitmap.hh"
#include "formats/msw_icon.hh"
#include "rendering/cairocontext.hh"
#include "util/image.hh"
#include "util/util.hh" // For from_png

using faint::uint;
using faint::uchar;

const int g_iconDirLen = 6;
const int g_iconDirEntryLen = 16;
const int g_bmpHeaderLen = 40;

size_t and_map_stride( int w ){
  return ((w % 32 ) == 0) ? w / 8 :
    4 * (w / 32 + 1);
}

size_t and_map_len( const IntSize& bmpSize ){
  return and_map_stride(bmpSize.w) * bmpSize.h;
}

std::string bmp_compression_name( int compression ){
  if ( compression == 0 ){
    return "BI_RGB";
  }
  else if ( compression == 1 ){
    return "BI_RLE8";
  }
  else if ( compression == 2 ){
    return "BI_RLE4";
  }
  else if ( compression == 3 ){
    return "BI_BITFIELDS";
  }
  else if ( compression == 4 ){
    return "BI_JPEG";
  }
  else if ( compression == 5 ){
    return "BI_PNG";
  }
  else if ( compression == 6 ){
    return "BI_ALPHABITFIELDS";
  }
  return "Unknown";
}

std::string error_bpp( size_t num, int bpp ){
  std::stringstream ss;
  ss << "The bits-per-pixel setting for this icon is not " <<
    "supported by Faint." << std::endl << std::endl <<
    "Image entry: " << num + 1 << std::endl <<
    "Bits per pixel: " << bpp;
  return ss.str();
}

std::string error_color_planes( size_t num, int planes ){
  std::stringstream ss;
  ss << "The number of color planes for this icon is invalid." <<
    "Image entry: " << num + 1 << std::endl <<
    "Color planes: " << planes << " (expected 1)";
  return ss.str();
}

std::string error_compression( size_t num, int compression ){
  std::stringstream ss;
  ss << "The compression for this icon is not supported by Faint." << std::endl << std::endl <<
    "Icon#: " << num + 1 << std::endl <<
    "Compression: " << compression << " (" << bmp_compression_name(compression) << ")";
  return ss.str();
}

std::string error_dir_reserved(int value){
  std::stringstream ss;
  ss << "This icon appears broken." << std::endl <<
    "The reserved entry of the IconDir is " << value << " (expected: 0)";
  return ss.str();
}

std::string error_icon_size(size_t num, const IntSize& sz){
  std::stringstream ss;
  ss << "This icon appears broken." << std::endl << std::endl <<
    "The icon size in an IconDirEntry is " << sz.w << "," << sz.h << std::endl <<
    "Image entry: " << num + 1;
  return ss.str();
}

std::string error_image(size_t num){
  std::stringstream ss;
  ss << "This icon appears broken." << std::endl <<
    "Failed reading the header for image entry " << num + 1;
  return ss.str();
}

std::string error_icon_is_cursor(){
  return "This icon file contains cursors. This is not yet supported by Faint.";
}

std::string error_open_file( const std::string& filename ){
  std::stringstream ss;
  ss << "The file could not be opened for reading." << std::endl <<
    "Filename: " << filename;
  return ss.str();

}

std::string error_premature_eof(std::string structure){
  std::stringstream ss;
  ss << "This icon appears broken." << std::endl <<
    "Reading the " << structure << " failed.";
  return ss.str();
}

std::string error_no_images(){
  std::stringstream ss;
  ss << "This icon contains no images." << std::endl << std::endl << "The IconDir images entry is 0.";
  return ss.str();
}

std::string error_truncated_bmp_header( size_t num, int len ){
  std::stringstream ss;
  ss << "This icon file appears broken." << std::endl << std::endl <<
    "Image entry: " << num + 1 << std::endl <<
    "Header length: " << len << "(expected >= 40)";
  return ss.str();
}

std::string error_bmp_data( size_t num ){
  std::stringstream ss;
  ss << "This icon file appears broken." << std::endl << std::endl <<
    "Reading the data for a bitmap failed." << std::endl <<
    "Image entry: " << num + 1;
  return ss.str();
}

std::string error_truncated_icon_dir( int len ){
  std::stringstream ss;
  ss << "This icon file appears broken." << std::endl << std::endl
     << "The icon dir is truncated to " << len << " bytes (expected " << g_iconDirLen << ").";
  return ss.str();
}

std::string error_truncated_png_data( size_t num ){
  std::stringstream ss;
  ss << "This icon file appears broken." << std::endl <<
    "Reading an embedded png-compressed image failed." << std::endl <<
    "Image entry: " << num + 1;
  return ss.str();
}

std::string error_read_to_offset( size_t num, int offset ){
  std::stringstream ss;
  ss << "This icon file appears broken." << std::endl << std::endl
     << "Seeking to an image offset failed." << std::endl
     << "Image entry: " << num << std::endl
     << "At: " << std::hex << offset << std::endl;
  return ss.str();
}

std::string error_unknown_image_type(int type){
  std::stringstream ss;
  ss << "The image type in this icon file is not recognized by Faint."
     << std::endl << std::endl
     << "Image type: " << type << " (expected 1 for icon or 2 for cursor).";
  return ss.str();
}

class LoadIconError{
public:
  LoadIconError( const std::string& error )
    : m_error(error)
  {}
  std::string GetString() const{
    return m_error;
  }
private:
  std::string m_error;
};

bool invalid_bpp_allow_zero( int bpp ){
  return bpp != 32 && bpp != 0; // Fixme: Support other alternatives
}

bool invalid_bpp_no_zero( int bpp ){
  return bpp != 32; // Fixme: Support other alternatives
}

bool invalid_compression( int compression ){
  const int BI_RGB = 0;
  if ( compression == BI_RGB ){
    return false; // No compression. This is supported
  }
  return true;
}

bool invalid_header_length( int len ){
  return len < g_bmpHeaderLen; // Fixme: In addition to truncation, check invalid > 40 lengths.
}

int to_int( char b0, char b1 ){
  return (uint(uchar(b1)) << 8) | (uint(uchar(b0)));
}

int to_int( char b0, char b1, char b2, char b3 ){
  return (uint(uchar(b3)) << 24) | (uint(uchar(b2)) << 16) |
    (uint(uchar(b1)) << 8) | (uint(uchar(b0)));
}

uint to_uint(char b){
  return uint(uchar(b));
}

uint to_uint( char low, char high ){
  return (uint(uchar(high)) << 8) | (uint(uchar(low)));
}

uint to_uint( char b0, char b1, char b2, char b3 ){
  return (uint((uchar)b3) << 24) | (uint((uchar)b2) << 16) |
    (uint((uchar)b1) << 8) | uint((uchar)b0);
}

uint read_2u( std::ifstream& f ){
  char data[2];
  f.read(data,2);
  return to_uint(data[0], data[1]);
}

uint read_4u( std::ifstream& f ){
  char data[4];
  f.read(data, 4);
  return to_uint(data[0], data[1], data[2], data[3]);
}

int read_4( std::ifstream& f ){
  char data[4];
  f.read(data, 4);
  return to_int(data[0], data[1], data[2], data[3]);
}

void write_1b(std::ostream& out, uint v){
  char data((char)v);
  out.write(&data, 1);
}

void write_2b(std::ostream& out, uint v ){
  char data[2];
  data[0] = static_cast<char>(v & 0xffff);
  data[1] = static_cast<char>((v >> 8) & 0xffff);
  out.write(data, 2);
}

void write_4b(std::ostream& out, uint v ){
  char data[4] = { static_cast<char>(v & 0xff),
		   static_cast<char>((v >> 8) & 0xff),
		   static_cast<char>((v >> 16) & 0xff),
		   static_cast<char>((v >> 24) & 0xff)};
  out.write(data,4);
}

IntSize read_bmp_size(std::ifstream& f){
  IntSize sz(read_4(f), read_4(f));
  if ( !f.good() ){
    throw LoadIconError(error_premature_eof("BitmapHeader")); // Fixme
  }
  return sz;
}

IntSize read_ico_size(std::ifstream& f){
  char data[2] = {0x00, 0x00};
  f.read(data, 2);
  if ( !f.good() ){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  int w = static_cast<int>((uchar)data[0]);
  int h = static_cast<int>((uchar)data[1]);
  return IntSize(w == 0 ? 256 : w,
    h == 0 ? 256 : h );
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
  uint bytes; // The length of the XOR-mask and the AND-mask
  int offset; // The offset to the bmp-header or png-data
};

IconDirEntry parse_icon_dir_entry( std::ifstream& f ){
  IconDirEntry icon;
  icon.size = read_ico_size(f);
  char data[4];
  // Colors in palette
  f.read(data, 1); // Fixme: Check for read errors
  if ( !f.good() ){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  if ( data[0] == 0 ){
    icon.palette = false;
    icon.colorsInPalette = 0;
  }
  else {
    icon.palette = true;
    icon.colorsInPalette = int(data[0]);
  }

  f.read(data, 1);
  if ( !f.good() ){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  icon.reserved = to_uint(data[0]);

  // Color planes (hotspot x for cursors, ignored)
  f.read(data, 2);
  if ( !f.good() ){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  icon.colorPlanes = to_uint(data[0], data[1]);

  // bpp (hotspot y for cursors, ignored)
  f.read(data,2); // Fixme: Check for read errors
  if ( !f.good() ){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  icon.bpp = to_uint(data[1]);

  f.read(data,4); // Fixme: Check for read errors
  if ( !f.good() ){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  icon.bytes = to_uint(data[0], data[1], data[2], data[3]);

  f.read(data,4); // Fixme: Check for read errors
  if ( !f.good() ){
    throw LoadIconError(error_premature_eof("IconDirEntry"));
  }
  int offset = to_uint(data[0], data[1], data[2], data[3]);
  icon.offset = offset;
  return icon;
}

bool is_png( char* data ){
  return to_uint(data[0]) == 0x89 &&
    to_uint(data[1]) == 0x50 &&
    to_uint(data[2]) == 0x4e &&
    to_uint(data[3]) == 0x47 &&
    to_uint(data[4]) == 0x0d &&
    to_uint(data[5]) == 0x0a &&
    to_uint(data[6]) == 0x1a &&
    to_uint(data[7]) == 0x0a;
}

bool peek_png_signature( std::istream& f ){
  std::streampos oldPos = f.tellg();
  char signature[8];
  f.read(signature, 8);
  if ( !f.good() || f.eof() ){
    return false;
  }
  f.seekg(oldPos);
  return is_png(signature);
}

void test_bitmap_header(size_t iconNum, const BitmapHeader& h){
  if ( invalid_header_length(h.headerLen) ){
    throw LoadIconError(error_truncated_bmp_header(iconNum, h.headerLen));
  }
  if ( invalid_compression( h.compression ) ){
    throw LoadIconError(error_compression( iconNum, h.compression ) );
  }
  if ( invalid_bpp_no_zero(h.bpp)) {
    throw LoadIconError(error_bpp( iconNum, h.bpp ) );
  }
  if ( h.colorPlanes != 1 ){
    throw LoadIconError(error_color_planes(iconNum, h.colorPlanes));
  }
}

void test_icon_dir(const IconDir& dir ){
  if ( dir.len != g_iconDirLen ){
    throw LoadIconError(error_truncated_icon_dir(dir.len));
  }
  if ( dir.reserved != 0 ){
    throw LoadIconError(error_dir_reserved(dir.reserved));
  }
  if ( dir.imageType == 2 ){
    throw LoadIconError(error_icon_is_cursor());
  }
  if ( dir.imageType != 1 ){
    throw LoadIconError(error_unknown_image_type(dir.imageType));
  }
  if ( dir.imageCount == 0 ){
    throw LoadIconError(error_no_images());
  }
}

void test_icon_dir_entry(size_t iconNum, const IconDirEntry& icon ){
  if ( invalid_bpp_allow_zero(icon.bpp) ){
    throw LoadIconError(error_bpp(iconNum, icon.bpp));
  }
  if ( icon.size.w <= 0 || icon.size.h <= 0 ){
    throw LoadIconError(error_icon_size(iconNum, icon.size));
  }
}

void test_icon_dir_entries(std::vector<IconDirEntry>& entries){
  for ( size_t i = 0; i != entries.size(); i++ ){
    test_icon_dir_entry(i, entries[i]);
  }
}

BitmapHeader parse_bitmap_header( std::ifstream& f ){
  BitmapHeader h;
  h.headerLen = read_4u(f);
  h.size = read_bmp_size(f);
  h.colorPlanes = read_2u(f);
  h.bpp = read_2u(f);
  h.compression = read_4u(f);
  h.rawDataSize = read_4u(f);
  h.horizontalResolution = read_4u(f);
  h.verticalResolution = read_4u(f);
  h.paletteColors = read_4u(f);
  h.importantColors = read_4u(f);
  return h;
}

IconDir parse_icon_dir( std::ifstream& f ){
  IconDir d;
  d.len = d.reserved = d.imageType = d.imageCount = 0;
  std::streampos oldPos = f.tellg();
  char data[6];
  f.read(data, 6);
  d.len = static_cast<int>(f.tellg() - oldPos);
  if ( f.good() ){
    d.reserved = to_uint(data[0], data[1]);
    d.imageType = to_uint(data[2], data[3]);
    d.imageCount = to_uint(data[4], data[5]);
  }
  return d;
}

std::vector<IconDirEntry> parse_icon_dir_entries(std::ifstream& f, size_t numIcons ){
  std::vector<IconDirEntry> v;
  for ( size_t i = 0; i != numIcons; i++ ){
    v.push_back(parse_icon_dir_entry(f));
    if ( !f.good() ){
      return v;
    }
  }
  return v;
}

BITMAPRETURN ico_parse_32bpp_BI_RGB( std::ifstream& f, const IntSize& bitmapSize ){
  int bypp = 4;
  // The size from the bmp-header. May have larger height than the size
  // in the IconDirEntry. (Fixme: Why?)
  int bufLen = area(bitmapSize) * bypp;
  char* pixelData = new char[bufLen];
  f.read(pixelData, bufLen);
  if ( !f.good() ){
    return faint::Bitmap(faint::get_null_bitmap());
  }

  // The size from the IconDirEntry is the exact amount of pixels
  // this image should have.
  const IntSize& sz(bitmapSize);
  faint::Bitmap bmp(faint::cairo_compatible_bitmap(sz));
  for ( int y = 0; y != sz.h; y++ ){
    for ( int x = 0; x != sz.w; x++ ){
      uint b = to_uint(pixelData[y * sz.w * bypp + x * bypp]);
      uint g = to_uint(pixelData[y * sz.w * bypp + x * bypp + 1]);
      uint r = to_uint(pixelData[y * sz.w * bypp + x * bypp + 2]);
      uint a = to_uint(pixelData[y * sz.w * bypp + x * bypp + 3]);
      put_pixel_raw(bmp, x, bmp.m_h - y - 1, faint::Color(r,g,b,a));
    }
  }
  delete[] pixelData;
  return bmp;
}

BITMAPRETURN ico_parse_png( std::ifstream& f, size_t len ){
  char* data = new char[len];
  f.read(data, len);
  faint::Bitmap bmp(from_png(data, len));
  delete[] data;
  return bmp;
}

void write_32bpp_BI_RGB( std::ostream& out, const faint::Bitmap& bmp ){
  // The size from the bmp-header. May have larger height than the size
  // in the IconDirEntry. (Fixme: Why?)

  // Write the pixel data (in archaic ICO-terms, the XOR-mask)
  for ( size_t y = 0; y != bmp.m_h; y++ ){
    for ( size_t x = 0; x != bmp.m_w; x++ ){
      faint::Color c( faint::get_color_raw( bmp, x, bmp.m_h - y - 1 ) );
      write_1b(out, c.b);
      write_1b(out, c.g);
      write_1b(out, c.r);
      write_1b(out, c.a);
    }
  }

  // Write the AND-mask
  size_t len = and_map_len(bmp.GetSize());
  for ( size_t i = 0; i != len; i++ ){
    write_1b(out,0xff);
  }
}

namespace faint{
void load_icon( const std::string& filename, ImageProps& props ){
  try {
    std::ifstream f(filename.c_str(), std::ios::binary);
    if ( !f.good() ){
      props.SetError(error_open_file(filename));
      return;
    }

    IconDir iconDir( parse_icon_dir(f) );
    if ( !f.good() ){
      props.SetError(error_premature_eof("ICONDIR"));
      return;
    }
    test_icon_dir(iconDir);
    std::vector<IconDirEntry> iconEntries = parse_icon_dir_entries(f, iconDir.imageCount);
    if ( !f.good() ){
      throw LoadIconError(error_premature_eof("ICONDIRENTRY"));
    }
    test_icon_dir_entries(iconEntries);

    for ( size_t i = 0; i != iconEntries.size(); i++ ){
      IconDirEntry& icon = iconEntries[i];
      f.seekg(icon.offset);
      if ( !f.good() || f.eof() ){
        throw LoadIconError(error_read_to_offset(i, icon.offset));
      }

      bool png_compressed = peek_png_signature(f);
      if ( !f.good() || f.eof() ){
        throw LoadIconError(error_image(i));
      }
      if ( png_compressed ){
        faint::Bitmap bmp(ico_parse_png(f, icon.bytes));
        if ( !f.good() ){
          throw LoadIconError(error_truncated_png_data(i));
        }
        if ( f.eof() && i < iconEntries.size() -1 ){
          throw LoadIconError("Premature EOF"); // Fixme
        }
        props.AppendBitmap(bmp, delay_t(0));
      }
      else {
        BitmapHeader bmpHeader(parse_bitmap_header(f));
        if ( !f.good() ){
          throw LoadIconError(error_image(i));
        }
        test_bitmap_header(i, bmpHeader);
        if ( bmpHeader.bpp == 32 ){
          faint::Bitmap bmp(ico_parse_32bpp_BI_RGB(f, icon.size));
          if ( !f.good() ){
            throw LoadIconError(error_bmp_data(i));
          }
          assert(faint::bitmap_ok(bmp));
          props.AppendBitmap(bmp, delay_t(0));
        }
        else {
          // Valid_bitmap_header should prevent this case
          assert( false ); // Only 32bpp supported.
        }
      }
    }
  }
  catch ( const LoadIconError& error ){
    props.SetError(error.GetString());
  }
}
} // namespace
typedef std::vector<const faint::Bitmap> bmp_vec_t;
std::vector<BitmapHeader> create_bitmap_headers(const bmp_vec_t& bitmaps){
  std::vector<BitmapHeader> v;
  for ( size_t i = 0; i != bitmaps.size(); i++ ){
    const faint::Bitmap& bmp = bitmaps[i];
    BitmapHeader h;
    h.headerLen = g_bmpHeaderLen;
    IntSize bmpSize = bmp.GetSize();
    h.size = bmpSize;
    h.size.h *= 2; // Double height in bitmap header for some reason
    h.colorPlanes = 1;
    h.bpp = 32;
    h.compression = 0;
    h.rawDataSize = area(bmp.GetSize()) * 4 + and_map_len(bmpSize);
    h.horizontalResolution = 1; // Fixme: OK?
    h.verticalResolution = 1; // Fixme: OK?
    h.paletteColors = 0;
    h.importantColors = 0;
    v.push_back(h);
  }
  return v;
}

IconDir create_icon_dir( const bmp_vec_t& bitmaps ){
  IconDir iconDir;
  iconDir.len = g_iconDirLen;
  iconDir.reserved = 0;
  iconDir.imageType = 1; // Icon
  iconDir.imageCount = bitmaps.size();
  return iconDir;
}

std::vector<IconDirEntry> create_icon_dir_entries(const bmp_vec_t& bitmaps){
  std::vector<IconDirEntry> v;

  int offset = g_iconDirLen + bitmaps.size() * g_iconDirEntryLen;
  for ( size_t i = 0; i != bitmaps.size(); i++ ){
    assert( i < bitmaps.size() );
    const faint::Bitmap& bmp = bitmaps[i];
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

namespace faint{
SaveResult save_icon( const std::string& filename,  const bmp_vec_t& bitmaps ){
  assert( !bitmaps.empty() );
  for ( size_t i = 0; i != bitmaps.size(); i++ ){
    const Bitmap& bmp(bitmaps[i]);
    if ( bmp.m_w > 256 || bmp.m_h > 256 ){
      return SaveResult::SaveFailed("Maximum size for icons is 256x256");
    }
  }
  IconDir iconDir = create_icon_dir(bitmaps);
  std::vector<IconDirEntry> iconDirEntries = create_icon_dir_entries(bitmaps);
  std::vector<BitmapHeader> bmpHeaders = create_bitmap_headers(bitmaps);
  std::ofstream out(filename.c_str(), std::ios::binary);
  write_2b(out, iconDir.reserved);
  write_2b(out, iconDir.imageType);
  write_2b(out, iconDir.imageCount);

  for ( size_t i = 0; i != iconDirEntries.size(); i++ ){
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
  for ( size_t i = 0; i != bitmaps.size(); i++ ){
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
    write_32bpp_BI_RGB( out, bmp );
  }
  return SaveResult::SaveSuccessful();
}

}

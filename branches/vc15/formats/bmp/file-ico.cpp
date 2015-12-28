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
#include <vector>
#include "bitmap/alpha-map.hh"
#include "bitmap/bitmap.hh"
#include "bitmap/color-list.hh"
#include "formats/bmp/bmp-errors.hh"
#include "formats/bmp/bmp-types.hh"
#include "formats/bmp/file-cur.hh"
#include "formats/bmp/file-ico.hh"
#include "formats/bmp/serialize-bmp-pixel-data.hh"
#include "geo/int-point.hh"
#include "text/formatting.hh"
#include "util-wx/encode-bitmap.hh" // from_png
#include "util-wx/stream.hh"
#include "util/image-props.hh"
#include "util/iter.hh"

namespace faint{

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

static uint to_uint(char b){
  return uint(uchar(b));
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

static void test_bitmap_header(size_t iconNum, const BitmapInfoHeader& h){
  if (invalid_header_length(h.headerLen)){
    throw LoadIconError(error_truncated_bmp_header(iconNum, h.headerLen));
  }
  if (h.compression != Compression::BI_RGB){
    throw LoadIconError(error_compression(iconNum, h.compression));
  }
  if (h.colorPlanes != 1){
    throw LoadIconError(error_color_planes(iconNum, h.colorPlanes));
  }
}

static void test_icon_dir_common(const IconDir& dir){
  if (dir.reserved != 0){
    throw LoadIconError(error_dir_reserved(dir.reserved));
  }
}

static void test_icon_dir_ico(const IconDir& dir){
  test_icon_dir_common(dir);
  if (dir.imageType == IconType::CUR){
    throw LoadIconError(error_icon_is_cursor());
  }
  if (dir.imageType != IconType::ICO){
    throw LoadIconError(error_unknown_image_type(dir.imageType));
  }
}

static void test_icon_dir_cur(const IconDir& dir){
  test_icon_dir_common(dir);
  if (dir.imageType == IconType::ICO){
    throw LoadIconError(error_cursor_is_icon());
  }
  if (dir.imageType != IconType::CUR){
    throw LoadIconError(error_unknown_image_type(dir.imageType));
  }
  if (dir.imageCount == 0){
    throw LoadIconError(error_no_images());
  }
}

static std::vector<IconDirEntry> parse_icon_dir_entries(BinaryReader& f,
  int numIcons)
{
  assert(numIcons >= 0);
  std::vector<IconDirEntry> v;
  for (int i = 0; i != numIcons; i++){
    // TODO: Add read-struct template.
    auto entry = deserialize_IconDirEntry(read_array<ICONDIRENTRY_BYTES>(f));
    if (!f.good()){
      throw LoadIconError(error_premature_eof("ICONDIRENTRY"));
    }
    v.push_back(entry);
  }
  return v;
}

static Bitmap masked(const Bitmap& bmp, const AlphaMap& mask){
  Bitmap out(bmp);
  auto size = mask.GetSize();
  for (int y = 0; y!= size.h; y++){
    for (int x = 0; x != size.w; x++){
      if (mask.Get(x,y) == 1){
        put_pixel(out, {x,y}, color_transparent_white());
      }
    }
  }
  return out;
}

static Optional<Bitmap> read_1bpp_ico(BinaryReader& in, const IntSize& size){
  auto colorTable = read_color_table(in, 2);
  if (colorTable.NotSet()){
    return {};
  }

  auto xorMask = read_1bpp_BI_RGB(in, size);
  if (xorMask.NotSet()){
    return {};
  }

  auto andMask = read_1bpp_BI_RGB(in, size);
  if (andMask.NotSet()){
    return {};
  }

  auto bmp = bitmap_from_indexed_colors(xorMask.Get(), colorTable.Get());
  return masked(bmp, andMask.Get());
}

static Optional<Bitmap> read_4bpp_ico(BinaryReader& in, const IntSize& size){
  auto colorTable = read_color_table(in, 16);
  if (colorTable.NotSet()){
    return {};
  }

  auto xorMask = read_4bpp_BI_RGB(in, size);
  if (xorMask.NotSet()){
    return {};
  }

  auto andMask = read_1bpp_BI_RGB(in, size);
  if (andMask.NotSet()){
    return {};
  }

  auto bmp = bitmap_from_indexed_colors(xorMask.Get(), colorTable.Get());
  return masked(bmp, andMask.Get());
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

OrError<bmp_vec> read_ico(const FilePath& filename){
  try {
    BinaryReader f(filename);
    std::vector<Bitmap> bitmaps;
    if (!f.good()){
      return error_open_file_read(filename);
    }

    // Fixme: Handle eof
    auto iconDir = deserialize_IconDir(read_array<ICONDIR_BYTES>(f));
    if (!f.good()){
      return error_premature_eof("ICONDIR");
    }
    test_icon_dir_ico(iconDir);
    auto iconEntries(parse_icon_dir_entries(f, iconDir.imageCount));
    if (!f.good()){
      return error_premature_eof("ICONDIRENTRY");
    }

    for (size_t i = 0; i != iconEntries.size(); i++){
      IconDirEntry& iconDirEntry = iconEntries[i];
      f.seekg(iconDirEntry.offset);
      if (!f.good() || f.eof()){
        return error_read_to_offset(i, iconDirEntry.offset);
      }

      bool png_compressed = peek_png_signature(f);
      if (!f.good() || f.eof()){
        return error_image(i);
      }
      if (png_compressed){
        Bitmap bmp(ico_parse_png(f, iconDirEntry.bytes));
        if (!f.good()){
          return error_truncated_png_data(i);
        }
        if (f.eof() && i < iconEntries.size() -1){
          return utf8_string("Premature EOF"); // Fixme
        }
        bitmaps.emplace_back(std::move(bmp));
      }
      else {
        auto bmpHeader = deserialize_BitmapInfoHeader(read_array<BITMAPINFOHEADER_BYTES>(f));
        if (!f.good()){
          throw LoadIconError(error_image(i));
        }
        test_bitmap_header(i, bmpHeader);
        if (bmpHeader.bpp == 32){
          Bitmap bmp(ico_parse_32bpp_BI_RGB(f, get_size(iconDirEntry)));
          if (!f.good()){
            throw LoadIconError(error_bmp_data(i));
          }
          assert(bitmap_ok(bmp));
          bitmaps.emplace_back(std::move(bmp));
        }
        else if (bmpHeader.bpp == 1){
          auto bmp(read_1bpp_ico(f, get_size(iconDirEntry)));
          if (bmp.NotSet()){
            return error_bmp_data(i);
          }
          bitmaps.emplace_back(bmp.Get());
        }
        else if (bmpHeader.bpp == 4){
          auto bmp(read_4bpp_ico(f, get_size(iconDirEntry)));
          if (bmp.NotSet()){
            return error_bmp_data(i);
          }
          bitmaps.emplace_back(bmp.Get());
        }
        else {
          return error_bpp(i, bmpHeader.bpp);
        }
      }
    }
    return bitmaps;
  }
  catch (const LoadIconError& error){
    return error.GetString();
  }
}

OrError<cur_vec> read_cur(const FilePath& filename){
  // Fixme: Mostly duplicates read_ico

  try {
    BinaryReader f(filename);
    if (!f.good()){
      return error_open_file_read(filename);
    }

    auto cursorDir = deserialize_IconDir(read_array<ICONDIR_BYTES>(f));
    if (!f.good()){
      return error_premature_eof("ICONDIR");
    }

    test_icon_dir_cur(cursorDir);
    auto iconDirEntries = parse_icon_dir_entries(f, cursorDir.imageCount);
    if (!f.good()){
      return error_premature_eof("ICONDIRENTRY");
    }

    cur_vec cursors;
    for (size_t i = 0; i != iconDirEntries.size(); i++){
      IconDirEntry& iconDirEntry = iconDirEntries[i];
      f.seekg(iconDirEntry.offset);
      if (!f.good() || f.eof()){
        return error_read_to_offset(i, iconDirEntry.offset);
      }

      bool pngCompressed = peek_png_signature(f);
      if (!f.good() || f.eof()){
        return error_image(i);
      }

      auto hotSpot(get_hot_spot(iconDirEntry));
      if (pngCompressed){
        Bitmap bmp(ico_parse_png(f, iconDirEntry.bytes));
        if (!f.good()){
          return error_truncated_png_data(i);
        }
        if (f.eof() && i < iconDirEntries.size() -1){
          return utf8_string("Premature EOF"); // Fixme
        }
        cursors.emplace_back(std::make_pair(std::move(bmp), hotSpot));
      }
      else {
        auto bmpHeader = deserialize_BitmapInfoHeader(read_array<BITMAPINFOHEADER_BYTES>(f));
        if (!f.good()){
          return error_image(i);
        }
        test_bitmap_header(i, bmpHeader);
        if (bmpHeader.bpp == 1){
          auto bmp = read_1bpp_ico(f, get_size(iconDirEntry));
          if (bmp.NotSet()){
            return error_bmp_data(i);
          }
          cursors.emplace_back(bmp.Get(), hotSpot);
        }
        else if (bmpHeader.bpp == 4){
          auto bmp = read_4bpp_ico(f, get_size(iconDirEntry));
          if (bmp.NotSet()){
            return error_bmp_data(i);
          }
          cursors.emplace_back(bmp.Get(), hotSpot);
        }
        else if (bmpHeader.bpp == 32){
          Bitmap bmp(ico_parse_32bpp_BI_RGB(f, get_size(iconDirEntry)));
          if (!f.good()){
            return error_bmp_data(i);
          }
          assert(bitmap_ok(bmp));
          cursors.emplace_back(std::make_pair(std::move(bmp), hotSpot));
        }
        else {
          return error_bpp(i, bmpHeader.bpp);
        }

      }
    }
    return cursors;
  }
  catch (const LoadIconError& error){
    return error.GetString();
  }
}

static std::vector<BitmapInfoHeader> create_bitmap_headers(const bmp_vec& bitmaps){
  std::vector<BitmapInfoHeader> v;
  for (const auto& bmp : bitmaps){
    v.push_back(create_bitmap_header(bmp, 32, true));
  }
  return v;
}

static std::vector<BitmapInfoHeader> create_bitmap_headers(const cur_vec& cursors){
  std::vector<BitmapInfoHeader> v;
  for (const auto& c : cursors){
    v.push_back(create_bitmap_header(c.first, 32, true));
  }
  return v;
}

static IconDir create_icon_dir_icon(const bmp_vec& bitmaps){
  IconDir iconDir;
  iconDir.reserved = 0;
  iconDir.imageType = IconType::ICO;
  assert(bitmaps.size() <= UINT16_MAX); // Fixme: Proper error handling
  iconDir.imageCount = static_cast<uint16_t>(bitmaps.size());
  return iconDir;
}

static IconDir create_icon_dir_cursor(const cur_vec& cursors){
  IconDir iconDir;
  iconDir.reserved = 0;
  iconDir.imageType = IconType::CUR;
  assert(cursors.size() <= UINT16_MAX); // Fixme: Proper error handling
  iconDir.imageCount = static_cast<uint16_t>(cursors.size());
  return iconDir;
}

static std::vector<IconDirEntry> create_icon_dir_entries(const bmp_vec& bitmaps){
  std::vector<IconDirEntry> v;

  int offset = ICONDIR_BYTES + resigned(bitmaps.size()) * ICONDIRENTRY_BYTES;
  for (size_t i = 0; i != bitmaps.size(); i++){
    assert(i < bitmaps.size());
    const Bitmap& bmp = bitmaps[i];
    IconDirEntry entry;
    set_size(entry, bmp.GetSize());
    entry.reserved = 0;
    entry.colorCount = 0; // 0 when >= 8bpp
    entry.colorPlanes = 1;
    entry.bpp = 32;
    entry.bytes =
      area(bmp.GetSize()) * 4 +
      and_map_len(bmp.GetSize()) +
      BITMAPINFOHEADER_BYTES;
    entry.offset = offset;
    v.push_back(entry);
    offset += entry.bytes;
  }
  return v;
}

static std::vector<IconDirEntry> create_cursor_dir_entries(const cur_vec& cursors){
  std::vector<IconDirEntry> v;
  int offset = ICONDIR_BYTES + resigned(cursors.size()) * ICONDIRENTRY_BYTES;
  for (size_t i = 0; i != cursors.size(); i++){
    const Bitmap& bmp = cursors[i].first;
    IconDirEntry entry;
    set_size(entry, bmp.GetSize());
    entry.reserved = 0;
    entry.colorCount = 0; // 0 when >= 8bpp
    set_hot_spot(entry, cursors[i].second);
    entry.bytes =
      area(bmp.GetSize()) * 4 +
      and_map_len(bmp.GetSize()) +
      BITMAPINFOHEADER_BYTES;
    entry.offset = offset;
    v.push_back(entry);
    offset += entry.bytes;
  }
  return v;
}

SaveResult write_ico(const FilePath& filePath,  const bmp_vec& bitmaps){
  assert(!bitmaps.empty());
  for (size_t i = 0; i != bitmaps.size(); i++){
    const Bitmap& bmp(bitmaps[i]);
    if (bmp.m_w > 256 || bmp.m_h > 256){
      return SaveResult::SaveFailed(utf8_string(
        "Maximum size for icons is 256x256"));
    }
  }

  IconDir iconDir = create_icon_dir_icon(bitmaps);
  std::vector<IconDirEntry> iconDirEntries = create_icon_dir_entries(bitmaps);
  std::vector<BitmapInfoHeader> bmpHeaders = create_bitmap_headers(bitmaps);
  BinaryWriter out(filePath);
  if (!out.good()){
    return SaveResult::SaveFailed(error_open_file_write(filePath));
  }

  write(out, serialize(iconDir));
  for (const auto& iconDirEntry : iconDirEntries){
    write(out, serialize(iconDirEntry));
  }

  assert(bitmaps.size() == bmpHeaders.size());
  for (size_t i = 0; i != bitmaps.size(); i++){
    write(out, serialize(bmpHeaders[i]));
    const Bitmap& bmp = bitmaps[i];
    write_32bpp_BI_RGB_ICO(out, bmp);
  }
  return SaveResult::SaveSuccessful();
}

SaveResult write_cur(const FilePath& filePath, const cur_vec& cursors){
  assert(!cursors.empty());
  for (size_t i = 0; i != cursors.size(); i++){
    const Bitmap& bmp(cursors[i].first);
    if (bmp.m_w > 256 || bmp.m_h > 256){
      return SaveResult::SaveFailed(utf8_string(
        "Maximum size for cursors is 256x256"));
    }
  }
  IconDir iconDir = create_icon_dir_cursor(cursors);
  auto iconDirEntries(create_cursor_dir_entries(cursors));
  auto bmpHeaders(create_bitmap_headers(cursors));
  BinaryWriter out(filePath);
  if (!out.good()){
    return SaveResult::SaveFailed(error_open_file_write(filePath));
  }

  write(out, serialize(iconDir));

  for (const auto& iconDirEntry : iconDirEntries){
    write(out, serialize(iconDirEntry));
  }

  assert(cursors.size() == bmpHeaders.size());
  for (size_t i = 0; i != cursors.size(); i++){
    write(out, serialize(bmpHeaders[i]));
    write_32bpp_BI_RGB_ICO(out, cursors[i].first);
  }
  return SaveResult::SaveSuccessful();
}

} // namespace

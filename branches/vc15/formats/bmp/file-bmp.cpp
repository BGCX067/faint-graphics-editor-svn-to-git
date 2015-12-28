// -*- coding: us-ascii-unix -*-
// Copyright 2015 Lukas Kemmer
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

#include "bitmap/alpha-map.hh"
#include "bitmap/color-list.hh"
#include "formats/bmp/bmp-errors.hh"
#include "formats/bmp/bmp-types.hh"
#include "formats/bmp/file-bmp.hh"
#include "formats/bmp/serialize-bmp-pixel-data.hh"
#include "util/serialize-tuple.hh"
#include "util-wx/stream.hh"

namespace faint{

static int palette_length_bytes(BitmapQuality quality){
  switch (quality){
  case BitmapQuality::COLOR_8BIT:
    // 8-Bit Color table
    return 1024;
  case BitmapQuality::COLOR_24BIT:
    // No palette, RGBA stored directly
    return 0;
  case BitmapQuality::GRAY_8BIT:
    // Grayscale color table
    return 1024;
  default:
    assert(false);
    return 0;
  }
}

static BitmapFileHeader create_bitmap_file_header(BitmapQuality quality,
  int rowStride,
  int height)
{
  BitmapFileHeader h;
  h.fileType = 0x4d42; // "BM" (reversed, endianness and all)
  h.length =
    BITMAPINFOHEADER_BYTES +
    BITMAPFILEHEADER_BYTES +
    palette_length_bytes(quality) +
    rowStride * height;

  h.reserved1 = 0;
  h.reserved2 = 0;

  h.dataOffset =
    BITMAPINFOHEADER_BYTES +
    BITMAPFILEHEADER_BYTES +
    palette_length_bytes(quality);
  return h;
}

static void write_grayscale_color_table(BinaryWriter& out){
  for (unsigned int i = 0; i != 256; i++){
    out.put(static_cast<char>(i));
    out.put(static_cast<char>(i));
    out.put(static_cast<char>(i));
    out.put(0);
  }
}

OrError<Bitmap> read_bmp(const FilePath& filePath){
  BinaryReader in(filePath);
  if (!in.good()){
    return error_open_file_read(filePath);
  }

  // Fixme: Handle eof
  auto bitmapFileHeader =
    deserialize_BitmapFileHeader(read_array<BITMAPFILEHEADER_BYTES>(in));

  // Fixme: Again reversed. Split into bytes
  if (bitmapFileHeader.fileType != 0x4d42){
    return error_bitmap_signature(bitmapFileHeader.fileType);
  }

  auto bitmapInfoHeader = deserialize_BitmapInfoHeader(
    read_array<BITMAPINFOHEADER_BYTES>(in));

  if (invalid_header_length(bitmapInfoHeader.headerLen)){
    return error_truncated_bmp_header(0, bitmapInfoHeader.headerLen);
  }


  if (bitmapInfoHeader.compression != Compression::BI_RGB){
    return error_compression(0, bitmapInfoHeader.compression);
  }

  if (bitmapInfoHeader.colorPlanes != 1){
    return error_color_planes(0, bitmapInfoHeader.colorPlanes);
  }

  const IntSize bmpSize(bitmapInfoHeader.width, bitmapInfoHeader.height);

  // Fixme: Unreadable mess
  if (bitmapInfoHeader.bpp == 8){
    return read_color_table(in, 256).Visit( // Fixme: Verify 256

      [&](const ColorList& colorList) -> OrError<Bitmap>{
        in.seekg(bitmapFileHeader.dataOffset);
        return read_8bpp_BI_RGB(in, bmpSize).Visit(
           [colorList](const AlphaMap& alphaMap) -> OrError<Bitmap>{
             return bitmap_from_indexed_colors(alphaMap, colorList);
           },
           []() -> OrError<Bitmap>{
             return utf8_string("Failed reading 8bpp-pixel data.");
           });
      },
      []() -> OrError<Bitmap>{
        return utf8_string("Failed reading color table.");
      });
  }
  else {
    in.seekg(bitmapFileHeader.dataOffset);

    if (bitmapInfoHeader.bpp == 24){
      return read_24bpp_BI_RGB(in, bmpSize).Visit(
      [](const Bitmap& bmp) -> OrError<Bitmap>{
        return bmp;
      },
      []() -> OrError<Bitmap>{
        return utf8_string("Failed reading 24-bpp-pixel data.");
      });
    }
    else if (bitmapInfoHeader.bpp == 32){
      assert(false);
      return read_32bpp_BI_RGB(in, bmpSize).Visit(
      [](const Bitmap& bmp) -> OrError<Bitmap>{
        return bmp;
      },
      []() -> OrError<Bitmap>{
        return utf8_string("Failed reading 32-bpp-pixel data.");
      });
    }
    return utf8_string("Unsupported bpp"); // Fixme: better info.
  }
}

SaveResult write_bmp(const FilePath& filePath,
  const Bitmap& bmp,
  BitmapQuality quality)
{
  BinaryWriter out(filePath);
  if (!out.good()){
    return SaveResult::SaveFailed(error_open_file_write(filePath));
  }

  uint16_t bpp = quality == BitmapQuality::COLOR_24BIT ? 24 : 8;
  BitmapInfoHeader bitmapInfoHeader = create_bitmap_header(bmp, bpp, false);

  const IntSize size(bmp.GetSize());
  const int rowStride = bmp_row_stride(bpp, size.w);

  write(out, serialize(create_bitmap_file_header(quality, rowStride, size.h)));
  write(out, serialize(bitmapInfoHeader));

  switch(quality){
    // Note: No default, to ensure warning if unhandled enum value
  case BitmapQuality::COLOR_8BIT:
    // Fixme: Add color table writing instead, and use the same
    // write function for grayscale as color.
    write_8bpp_BI_RGB(out, bmp);
    return SaveResult::SaveSuccessful();
  case BitmapQuality::GRAY_8BIT:
    write_grayscale_color_table(out);
    write_8bpp_BI_RGB_grayscale(out, bmp);
    return SaveResult::SaveSuccessful();
  case BitmapQuality::COLOR_24BIT:
    write_24bpp_BI_RGB(out, bmp); // Fixme
    return SaveResult::SaveSuccessful();
  }

  assert(false);
  return SaveResult::SaveFailed(utf8_string("Internal error in save_bitmap"));
}

} // namespace

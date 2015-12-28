// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "tests/test-util/bitmap-test-util.hh"
#include "tests/test-util/file-handling.hh"
#include "tests/test-util/print-objects.hh"
#include "bitmap/color.hh"
#include "formats/bmp/file-ico.hh"
#include "formats/bmp/bmp-types.hh"
#include "geo/int-point.hh"
#include "geo/int-rect.hh"

using namespace faint;

void check_bitmap(const Bitmap& bmp, IntSize expectedSize){
  auto size = bmp.GetSize();
  EQUAL(size, expectedSize);

  // Transparent border

  const Color green(34, 177, 76);
  const Color blue(47,54,153);

  // The icon has a 1-pixel wide transparent border
  is_uniformly(color_transparent_white(),
    bmp, IntRect(IntPoint(0,0), IntSize(size.w, 1)));
  is_uniformly(color_transparent_white(),
    bmp, IntRect(IntPoint(0,size.h - 1), IntSize(size.w, 1)));
  is_uniformly(color_transparent_white(),
    bmp, IntRect(IntPoint(0,0), IntSize(1, size.w)));
  is_uniformly(color_transparent_white(),
    bmp, IntRect(IntPoint(size.w -1,0), IntSize(1, size.h)));

  // Green top left square
  is_uniformly(green,
    bmp, IntRect(IntPoint(1, 1), size / 2));

  // Blue top right square
  is_uniformly(blue,
    bmp, IntRect(IntPoint(size.w /2, 1), size / 2));

  // Green bottom left square
  is_uniformly(blue,
    bmp, IntRect(IntPoint(1, size.h / 2), size / 2));

  // Transparent bottom right square
  is_uniformly(color_transparent_white(),
    bmp, IntRect(IntPoint(size.w / 2, size.h / 2), size / 2));
}

static std::vector<Bitmap> test_read_ico(const FilePath& filePath){
  return read_ico(filePath).Visit(
    [&](const std::vector<Bitmap>& bitmaps){
      return bitmaps;
    },
    [](const utf8_string& error) -> std::vector<Bitmap>{
      FAIL(error.c_str());
    });
}

void test_file_ico(){

  // Load the test icon
  FileName fileName("32-bpp_8-bit-alpha_no-palette-BI_RGB.ico");
  auto srcPath = get_test_load_path(fileName);
  auto bitmaps = test_read_ico(srcPath);

  // Verify that it looks as expected
  ABORT_IF(bitmaps.size() != 4);
  FWD(check_bitmap(bitmaps[0], {256, 256}));
  FWD(check_bitmap(bitmaps[1], {128, 128}));
  FWD(check_bitmap(bitmaps[2], {64, 64}));
  FWD(check_bitmap(bitmaps[3], {32, 32}));

  // Save it as a new icon
  FilePath dstPath = get_test_save_path(fileName);
  write_ico(dstPath, bitmaps);

  // Load the new icon
  auto bitmaps2 = test_read_ico(dstPath);

  // Verify that the new icon looks as expected
  ABORT_IF(bitmaps2.size() != 4);
  FWD(check_bitmap(bitmaps2[0], {256, 256}));
  FWD(check_bitmap(bitmaps2[1], {128, 128}));
  FWD(check_bitmap(bitmaps2[2], {64, 64}));
  FWD(check_bitmap(bitmaps2[3], {32, 32}));

  // Fixme: Add variants
}

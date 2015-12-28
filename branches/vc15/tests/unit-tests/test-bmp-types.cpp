// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "formats/bmp/bmp-types.hh"

void test_bmp_types(){
  using namespace faint;
  {
    BitmapFileHeader h1 = {0x424d,
                           500,
                           0,
                           0,
                           0x1000};
    auto a = serialize(h1);
    auto h2 = deserialize_BitmapFileHeader(a);
    VERIFY(h1.fileType == h2.fileType);
    VERIFY(h1.length == h2.length);
    VERIFY(h1.reserved1 == h2.reserved1);
    VERIFY(h1.reserved2 == h2.reserved2);
    VERIFY(h1.dataOffset == h2.dataOffset);
  }

  {
    BitmapInfoHeader h1 = {
      40,
      640, 480,
      1,
      32,
      Compression::BI_RGB,
      1000,
      1,
      1,
      0,
      0};
    auto a = serialize(h1);
    auto h2 = deserialize_BitmapInfoHeader(a);
    VERIFY(h1.headerLen == h2.headerLen);
    VERIFY(h1.width == h2.width);
    VERIFY(h1.height == h2.height);
    VERIFY(h1.colorPlanes == h2.colorPlanes);
    VERIFY(h1.bpp == h2.bpp);
    VERIFY(h1.compression == h2.compression);
    VERIFY(h1.rawDataSize == h2.rawDataSize);
    VERIFY(h1.horizontalResolution == h2.horizontalResolution);
    VERIFY(h1.verticalResolution == h2.verticalResolution);
    VERIFY(h1.paletteColors == h2.paletteColors);
    VERIFY(h1.importantColors == h2.importantColors);
  }

  {
    IconDir h1 = {0,
                  IconType::ICO,
                  1};
    auto a = serialize(h1);
    auto h2 = deserialize_IconDir(a);
    VERIFY(h1.reserved == h2.reserved);
    VERIFY(h1.imageType == h2.imageType);
    VERIFY(h1.imageCount == h2.imageCount);
  }

}

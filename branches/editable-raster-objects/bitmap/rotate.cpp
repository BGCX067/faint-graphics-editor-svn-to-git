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
#include <cmath>
#include "bitmap/bitmap.hh"
#include "rendering/cairocontext.hh"
#include "util/color.hh"

// The rotation code is adapted from this example for Windows GDI by
// Yves Maurer: http://www.codeguru.com/cpp/g-m/gdi/article.php/c3693
namespace faint {
  double min4(double a, double b, double c, double d){
    if (a < b) {
      if (c < a) {
        return (d < c) ? d : c;
      }
      else {
        return ( d < a ) ? d : a;
      }
    }
    else if (c < b) {
      return ( d < c ) ? d : c;
    }
    else {
      return ( d < b ) ? d : b;
    }
  }

  double max4(double a, double b, double c, double d) {
    if (a > b) {
      if (c > a) {
        return (d > c) ? d : c;
      }
      else {
        return ( d > a ) ? d : a;
      }
    }
    else if (c > b) {
      return ( d > c ) ? d : c;
    }
    else {
      return ( d > b ) ? d : b;
    }
  }

  faint::Bitmap rotate( const faint::Bitmap& bmp, faint::radian angle, const faint::Color& bgColor ) {
    // Use the upper-left corner as pivot
    double CtX = 0;
    double CtY = 0;

    // Find the corners to initialize the destination width and height
    const double cA = cos(angle);
    const double sA = sin(angle);

    const double x1 = CtX + (-CtX) * cA - (-CtY) * sA;
    const double x2 = CtX + (bmp.m_w - CtX) * cA - (-CtY) * sA;
    const double x3 = CtX + (bmp.m_w - CtX) * cA - (bmp.m_h - CtY) * sA;
    const double x4 = CtX + (-CtX) * cA - (bmp.m_h - CtY) * sA;

    const double y1 = CtY + (-CtY) * cA + (-CtX) * sA;
    const double y2 = CtY + (bmp.m_h - CtY) * cA + (-CtX) * sA;
    const double y3 = CtY + (bmp.m_h - CtY) * cA + (bmp.m_w - CtX) * sA;
    const double y4 = CtY + (-CtY) * cA + (bmp.m_w - CtX) * sA;

    int OfX = ((int) floor(min4(x1, x2, x3, x4)));
    int OfY = ((int) floor(min4(y1, y2, y3, y4))); // Fixme: I think OfY must be the same # as OfX

    // Create the new bitmap
    faint::Bitmap bmpDst( IntSize((int) ceil(max4(x1, x2, x3, x4)) - OfX,
      (int) ceil(max4(y1, y2, y3, y4)) - OfY ),
      bgColor );
    faint::uchar* dstRow = bmpDst.m_data;

    const faint::uchar* src = bmp.m_data;
    double divisor = cA*cA + sA*sA;

    // Iterate over the destination bitmap
    for ( int stepY = 0; stepY != bmpDst.m_h; stepY++ ) {
      for ( int stepX = 0; stepX != bmpDst.m_w; stepX++ ) {
        // Calculate the source coordinate
        int orgX = static_cast<int>((cA * (((double) stepX + OfX) + CtX * (cA - 1)) + sA * (((double) stepY + OfY) + CtY * (sA - 1))) / divisor );
        int orgY = static_cast<int>( CtY + (CtX - ((double) stepX + OfX)) * sA + cA *(((double) stepY + OfY ) - CtY + (CtY - CtX) * sA) );
        if ( orgX >= 0 && orgY >= 0 && orgX < bmp.m_w && orgY < bmp.m_h ) {
          // Inside source - copy the bits
          dstRow[ stepX * 4 + faint::iR] = src[orgX * 4 + orgY * bmp.m_row_stride + faint::iR];
          dstRow[ stepX * 4 + faint::iG] = src[orgX * 4 + orgY * bmp.m_row_stride + faint::iG];
          dstRow[ stepX * 4 + faint::iB] = src[orgX * 4 + orgY * bmp.m_row_stride + faint::iB];
          dstRow[ stepX * 4 + faint::iA] = src[orgX * 4 + orgY * bmp.m_row_stride + faint::iA];
        }
        else {
          // Outside source - set the color to bg color
          dstRow[ stepX * 4 + faint::iR] = bgColor.r;
          dstRow[ stepX * 4 + faint::iG] = bgColor.g;
          dstRow[ stepX * 4 + faint::iB] = bgColor.b;
          dstRow[ stepX * 4 + faint::iA] = bgColor.a;
        }
      }
      dstRow += bmpDst.m_row_stride;
    }

    return bmpDst;
  }

}

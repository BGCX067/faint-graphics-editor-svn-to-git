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

#include "bitmap.h"
#include "cairo_util.h"
#include "geo/geotypes.hh"
#include <exception>
#include <memory>

namespace faint{

  Bitmap CairoCompatibleBitmap( const IntSize& sz ){
    return CairoCompatibleBitmap(sz.w, sz.h);
  }

  Bitmap CairoCompatibleBitmap( uint w, uint h ){
    int stride = cairo_format_stride_for_width( CAIRO_FORMAT_ARGB32, w );
    if ( stride == -1 ){
      throw std::bad_alloc();
    }
    faint::Bitmap bmp( faint::ARGB32, w, h, stride );
    return bmp;
  }

  Bitmap CairoCompatibleSubBitmap( const Bitmap& orig, uint x0, uint y0, uint w, uint h ){
    uint origStride = orig.m_row_stride;
    const faint::uchar* origData = orig.m_data;

    Bitmap bmp( CairoCompatibleBitmap( w, h ) );
    faint::uchar* data = bmp.GetRaw();
    uint destStride = bmp.m_row_stride;

    for ( uint y = 0; y != h; y++ ){
      for ( uint x = 0; x != w; x++ ){
        int dstPos = y * destStride + x * 4;
        int srcPos = ( y + y0 ) * origStride + ( x + x0 ) * 4;
        data[ dstPos ] = origData[ srcPos ];
        data[ dstPos + 1 ] = origData[ srcPos + 1 ];
        data[ dstPos + 2 ] = origData[ srcPos + 2 ];
        data[ dstPos + 3 ] = origData[ srcPos + 3 ];
      }
    }
    return bmp;
  }

  Bitmap CairoCompatibleSubBitmap( const Bitmap& bmp, const IntRect& r ){
    return CairoCompatibleSubBitmap( bmp, r.x, r.y, r.w, r.h );
  }

  cairo_surface_t* GetCairoSurface( Bitmap& bmp ){
    cairo_surface_t* surface = cairo_image_surface_create_for_data (bmp.GetRaw(), CAIRO_FORMAT_ARGB32, bmp.m_w, bmp.m_h, bmp.m_row_stride );
    return surface;
  }

  Bitmap CairoSkew( faint::Bitmap& bmpSrc, faint::coord skew_angle, faint::coord skew_pixels ){
    faint::Bitmap bmpDst(CairoCompatibleBitmap( int(bmpSrc.m_w + fabs(skew_pixels)),int( bmpSrc.m_h) ));
    Clear( bmpDst, faint::Color(0,0,0,0));
    cairo_surface_t* s_dst = GetCairoSurface( bmpDst );
    cairo_surface_t* s_src = GetCairoSurface( bmpSrc );

    cairo_t* cr = cairo_create( s_dst );
    cairo_matrix_t m_skew;
    cairo_matrix_init( &m_skew, 1,0,skew_angle,1,0,0);
    if ( skew_pixels > 0 ){
      cairo_matrix_t m_translate;
      cairo_matrix_init( &m_translate, 1, 0, 0, 1, skew_pixels, 0 );
      cairo_matrix_multiply( &m_skew, &m_translate, &m_skew );
    }
    cairo_set_matrix( cr, &m_skew );
    cairo_set_source_surface( cr, s_src, 0, 0 );
    cairo_paint( cr );
    cairo_destroy( cr );
    cairo_surface_destroy( s_dst );
    cairo_surface_destroy( s_src );
    return bmpDst;
  }
}

namespace faint {
  std::string GetCairoVersion(){
    return CAIRO_VERSION_STRING;
  }
}

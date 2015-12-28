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
#include "objraster.hh"
#include "faintdc.hh"
#include "tools/settingid.hh"
#include "objrectangle.hh"
#include "objutil.hh"
#include "bitmap/rotate.h"
#include "bitmap/cairo_util.h"
#include "util/autocrop.hh"
#include "commands/cmdsetbitmap.hh"

const std::string s_TypeRaster = "Raster";

FaintSettings GetRasterSettings(){
  FaintSettings s;
  s.Set( ts_BgCol, faint::Color(255,255,255) );
  s.Set( ts_Transparency, OPAQUE_BG );
  return s;
}

ObjRaster::ObjRaster( const Rect& r, const faint::Bitmap& bitmap, const FaintSettings& s )
  : Object( &s_TypeRaster, TriFromRect(r), s ),
    m_bitmap( bitmap ),
    m_scaled( bitmap )
{
  assert( m_settings.Has( ts_Transparency ) );
  assert( m_settings.Has( ts_BgCol ) );
}

ObjRaster::ObjRaster( const ObjRaster& other )
  : Object( &s_TypeRaster, other.GetTri(), other.GetSettings() ),
    m_bitmap(other.m_bitmap),
    m_scaled(other.m_scaled)
{}

void ApplyTransform( const faint::Bitmap& src, const Tri& transform, faint::Bitmap& dst ){
  dst = src;
  Tri t2(transform);
  faint::coord skew = t2.Skew();
  faint::radian angle = t2.Angle();
  if ( !rather_zero(angle) ){
    t2 = Rotated( t2, -angle, t2.P0() );
  }
  if ( !rather_zero(skew) ){    
    t2 = Skewed( t2, - skew );
    faint::coord skew_pixels = skew / ( t2.Width() / src.m_w );
    faint::radian skew_angle = atan2( skew_pixels, ( t2.P0().y - t2.P2().y ) / ( t2.Height() / src.m_h ) );
    dst = CairoSkew( dst, tan(skew_angle), skew_pixels );
  }
  if ( !rather_zero(angle) ){
    t2 = Rotated( t2, angle, t2.P0() );
  }

  if ( !coord_eq(fabs(t2.Width()), src.m_w ) || !coord_eq(fabs(t2.Height()), src.m_h ) ){
    // Only scale bilinear if the width is truly different, not just inverted
    // ScaleBilinear can invert too, but with distortion
    dst = faint::ScaleBilinear( dst, t2.Width() / src.m_w, t2.Height() / src.m_h );
  }
  else {
    if ( t2.Width() < 0 ){
      dst = FlipHorizontal( dst );
    }
    if ( t2.Height() < 0 ){
      dst = FlipVertical( dst );
    }
  }

  if ( !rather_zero(angle) ){
    dst = faint::Rotate( dst, angle, faint::Color(0,0,0,0) );
  }
}

void ObjRaster::Draw( FaintDC& dc ){
  Rect r = BoundingRect( GetTri() );
  if ( m_settings.Get( ts_Transparency ) == TRANSPARENT_BG ){
    dc.Blend( m_scaled, m_settings.Get( ts_BgCol ), r.TopLeft() );
  }
  else {
    dc.Blend( m_scaled, r.TopLeft() );
  }
}

void ObjRaster::DrawMask( FaintDC& dc ){
  // Fixme: This draws a filled axis aligned rectangle.  Preferably,
  // the object's masked color (if any) should be excluded, and the area
  // aligned with the object (As for alignment, at the time the only
  // non-aligned rectangle function is blended)
  FaintSettings s(GetRectangleSettings());
  s.Set( ts_FillStyle, FILL );
  s.Set( ts_FgCol, mask_fill );
  s.Set( ts_AntiAlias, 0 );
  dc.Rectangle( GetTri(), s );
}

IntRect ObjRaster::GetRefreshRect(){
  return truncated(Inflated(GetRect(), LITCRD(2.0)));
}

void ObjRaster::OnSetTri(){
  Tri t(GetTri());
  ApplyTransform( m_bitmap, t, m_scaled );
}

Rect ObjRaster::GetRasterRect(){
  Rect r(GetRect());
  r.w += LITCRD(1.0);
  r.h += LITCRD(1.0);
  return r;
}

std::vector<Point> ObjRaster::GetResizePoints(){
  return Corners(GetRasterRect());
}

std::vector<Point> ObjRaster::GetAttachPoints(){
  return AttachPointsFromTri(Translated(GetTri(),LITCRD(0.5),LITCRD(0.5)) );
}

Object* ObjRaster::Clone() const{
  return new ObjRaster( *this );
}

bool ObjRaster::HitTest( const Point& p ){
  return GetRasterRect().Contains(p);
}

faint::Bitmap& ObjRaster::GetBitmap(){
  return m_bitmap;
}

void ObjRaster::SetBitmap( const faint::Bitmap& bmp ){
  m_bitmap = bmp;
  ApplyTransform( m_bitmap, GetTri(), m_scaled );
}

Command* AutoCropRaster( ObjRaster* obj ){
  const faint::Bitmap& bmp( obj->GetBitmap() );
  IntRect r;
  bool cropped = GetAutoCropRect( bmp, r );
  if ( cropped ){
    faint::coord old_w = static_cast<faint::coord>(bmp.m_w);
    faint::coord old_h = static_cast<faint::coord>(bmp.m_h);
    faint::coord new_w = static_cast<faint::coord>(r.w);
    faint::coord new_h = static_cast<faint::coord>(r.h);
    faint::coord new_x = static_cast<faint::coord>(r.x);
    faint::coord new_y = static_cast<faint::coord>(r.y);

    Tri t = obj->GetTri();
    t = Scaled( t, new_w / old_w, new_h / old_h, t.P0() );
    t = OffsetAligned( t, new_x, new_y );

    return new SetObjectBitmapCommand( obj,
      faint::CairoCompatibleSubBitmap( bmp, r ),
      t );
  }
  return 0;
}

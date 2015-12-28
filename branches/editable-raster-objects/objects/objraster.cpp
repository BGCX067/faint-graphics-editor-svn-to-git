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
#include <cmath>
#include "commands/set-bitmap-cmd.hh"
#include "objraster.hh"
#include "rendering/cairocontext.hh" // For cairo_skew
#include "rendering/faintdc.hh"
#include "util/autocrop.hh"
#include "util/objutil.hh"
#include "util/settingid.hh"
#include "util/settingutil.hh"

const std::string s_TypeRaster = "Raster";

ObjRaster::ObjRaster( const Rect& r, const faint::Bitmap& bitmap, const Settings& s )
  : Object( &s_TypeRaster, tri_from_rect(r), s ),
    m_bitmap( bitmap ),
    m_active( bitmap ),
    m_scaled( bitmap )
{
  assert( m_settings.Has( ts_BackgroundStyle ) );
  assert( m_settings.Has( ts_BgCol ) );
}

ObjRaster::ObjRaster( const ObjRaster& other )
  : Object( &s_TypeRaster, other.GetTri(), other.GetSettings() ),
    m_bitmap(other.m_bitmap),
    m_active(other.m_bitmap),
    m_scaled(other.m_scaled)
{}

static void apply_transform( const faint::Bitmap& src, const Tri& transform, faint::Bitmap& dst ){
  dst = src;
  Tri t2(transform);
  faint::coord skew = t2.Skew();
  faint::radian angle = t2.Angle();
  if ( !rather_zero(angle) ){
    t2 = rotated( t2, -angle, t2.P0() );
  }
  if ( !rather_zero(skew) ){
    t2 = skewed( t2, - skew );
    faint::coord skew_pixels = skew / ( t2.Width() / src.m_w );
    faint::radian skew_angle = atan2( skew_pixels, ( t2.P0().y - t2.P2().y ) / ( t2.Height() / src.m_h ) );
    dst = cairo_skew( dst, tan(skew_angle), skew_pixels );
  }
  if ( !rather_zero(angle) ){
    t2 = rotated( t2, angle, t2.P0() );
  }

  if ( !coord_eq(fabs(t2.Width()), src.m_w ) || !coord_eq(fabs(t2.Height()), src.m_h ) ){
    // Only scale bilinear if the width is truly different, not just inverted
    // ScaleBilinear can invert too, but with distortion
    dst = faint::scale_bilinear( dst, Scale(t2.Width() / src.m_w, t2.Height() / src.m_h) );
  }
  else {
    if ( t2.Width() < 0 ){
      dst = faint::flip(dst, Axis::HORIZONTAL );
    }
    if ( t2.Height() < 0 ){
      dst = faint::flip( dst, Axis::VERTICAL );
    }
  }

  if ( !rather_zero(angle) ){
    dst = faint::rotate( dst, angle, faint::Color(0,0,0,0) );
  }
}

void ObjRaster::Draw( FaintDC& dc ){
  Rect r = bounding_rect( GetTri() );
  dc.Bitmap( m_scaled, r.TopLeft(), m_settings );
}

void ObjRaster::DrawMask( FaintDC& dc ){
  // Fixme: This draws a filled axis aligned rectangle.  Preferably,
  // the object's masked color (if any) should be excluded, and the area
  // aligned with the object (As for alignment, at the time only the
  // non-aligned rectangle function is blended)
  Settings s(default_rectangle_settings());
  s.Set( ts_FillStyle, FillStyle::FILL );
  s.Set( ts_FgCol, faint::DrawSource(mask_fill) );
  s.Set( ts_AntiAlias, 0 );
  dc.Rectangle( GetTri(), s );
}

IntRect ObjRaster::GetRefreshRect(){
  return floored(inflated(GetRect(), LITCRD(2.0)));
}

void ObjRaster::OnSetTri(){
  Tri t(GetTri());
  apply_transform( m_active, t, m_scaled );
}

void ObjRaster::Update(){
  apply_transform(m_active, GetTri(), m_scaled);
}

void ObjRaster::Reset(){
  m_active = m_bitmap;
}

Rect ObjRaster::GetRasterRect() const{
  Rect r(GetRect());
  r.w += LITCRD(1.0);
  r.h += LITCRD(1.0);
  return r;
}

std::vector<Point> ObjRaster::GetAttachPoints() const{
  return get_attach_points(translated(GetTri(),LITCRD(0.5),LITCRD(0.5)) ); // Fixme: Why translated?
}

Object* ObjRaster::Clone() const{
  return new ObjRaster( *this );
}

bool ObjRaster::HitTest( const Point& p ) {
  return GetRasterRect().Contains(p);
}

faint::Bitmap& ObjRaster::GetBitmap(){
  return m_active;
}

void ObjRaster::SetBitmap( const faint::Bitmap& bmp ){
  m_bitmap = bmp;
  Reset();
  apply_transform( m_active, GetTri(), m_scaled );
}

// Fixme: Move?
Command* crop_raster_object_command( ObjRaster* obj ){
  const faint::Bitmap& bmp( obj->GetBitmap() );
  IntRect r;
  bool cropped = get_auto_crop_rect( bmp, r );
  if ( cropped ){
    faint::coord old_w = static_cast<faint::coord>(bmp.m_w);
    faint::coord old_h = static_cast<faint::coord>(bmp.m_h);
    faint::coord new_w = static_cast<faint::coord>(r.w);
    faint::coord new_h = static_cast<faint::coord>(r.h);
    faint::coord new_x = static_cast<faint::coord>(r.x);
    faint::coord new_y = static_cast<faint::coord>(r.y);

    Tri t = obj->GetTri();
    t = scaled( t, Scale(new_w / old_w, new_h / old_h), t.P0() );
    t = offset_aligned( t, new_x, new_y );

    return new SetObjectBitmapCommand( obj,
      sub_bitmap( bmp, r ),
      t, "Crop Raster Object" );
  }
  return nullptr;
}

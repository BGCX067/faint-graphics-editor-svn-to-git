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
#include "cairo.h"
#include "geo/tri.hh" // Fixme: For pango_text
#include "pango/pangocairo.h"
#include "rendering/cairocontext.hh"
#include "util/char.hh"

namespace faint{
BITMAPRETURN cairo_compatible_bitmap( const IntSize& sz ){
  int stride = cairo_format_stride_for_width( CAIRO_FORMAT_ARGB32, sz.w );
  if ( stride == -1 ){
    throw std::bad_alloc();
  }
  faint::Bitmap bmp( faint::ARGB32, sz, stride );
  return bmp;
}

BITMAPRETURN cairo_compatible_sub_bitmap( const Bitmap& orig, const IntRect& r ){
  uint origStride = orig.m_row_stride;
  const faint::uchar* origData = orig.m_data;

  Bitmap bmp( cairo_compatible_bitmap( r.GetSize() ) );
  faint::uchar* data = bmp.GetRaw();
  uint destStride = bmp.m_row_stride;

  for ( int y = 0; y != r.h; y++ ){
    for ( int x = 0; x != r.w; x++ ){
      int dstPos = y * destStride + x * 4;
      int srcPos = ( y + r.y ) * origStride + ( x + r.x ) * 4;
      data[ dstPos ] = origData[ srcPos ];
      data[ dstPos + 1 ] = origData[ srcPos + 1 ];
      data[ dstPos + 2 ] = origData[ srcPos + 2 ];
      data[ dstPos + 3 ] = origData[ srcPos + 3 ];
    }
  }
  return bmp;
}

cairo_surface_t* get_cairo_surface( Bitmap& bmp ){
  cairo_surface_t* surface = cairo_image_surface_create_for_data (bmp.GetRaw(), CAIRO_FORMAT_ARGB32, bmp.m_w, bmp.m_h, bmp.m_row_stride );
  return surface;
}

BITMAPRETURN cairo_skew( faint::Bitmap& bmpSrc, faint::coord skew_angle, faint::coord skew_pixels ){
  faint::Bitmap bmpDst(cairo_compatible_bitmap( IntSize(int(bmpSrc.m_w + fabs(skew_pixels)),int( bmpSrc.m_h))));
  clear( bmpDst, faint::Color(0,0,0,0));
  cairo_surface_t* s_dst = get_cairo_surface( bmpDst );
  cairo_surface_t* s_src = get_cairo_surface( bmpSrc );

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

std::string get_cairo_version(){
  return CAIRO_VERSION_STRING; // Note: Compile time.
}

std::string get_pango_version(){
  return PANGO_VERSION_STRING; // Note: Compile time.
}

class CairoContextImpl{
public:
  CairoContextImpl( faint::Bitmap& bmp ){
    this->surface = get_cairo_surface(bmp);
    this->cr = cairo_create(surface);
  }
  ~CairoContextImpl(){
    cairo_destroy(this->cr);
    cairo_surface_destroy(this->surface);
  }
  cairo_t* cr;
  cairo_surface_t* surface;
};

CairoContext::CairoContext( faint::Bitmap& bmp )
  : m_impl(new CairoContextImpl(bmp))
{}

CairoContext::~CairoContext(){
  delete m_impl;
}

void CairoContext::arc( faint::coord xc, faint::coord yc, faint::coord radius, faint::radian angle1, faint::radian angle2 ){
  cairo_arc(m_impl->cr, xc, yc, radius, angle1, angle2);
}

void CairoContext::close_path(){
  cairo_close_path(m_impl->cr);
}

void CairoContext::curve_to( const Point& p1, const Point& p2, const Point& p3 ){
  cairo_curve_to(m_impl->cr, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
}

void CairoContext::fill(){
  cairo_fill(m_impl->cr);
}

  void CairoContext::fill_preserve(){
    cairo_fill_preserve(m_impl->cr);
}

void CairoContext::line_to(const Point& p ){
  cairo_line_to(m_impl->cr, p.x, p.y);
}

void CairoContext::move_to( const Point& p ){
  cairo_move_to(m_impl->cr, p.x, p.y);
}

void CairoContext::pango_text( const Tri& t, const utf8_string& text, const Settings& s){
  const bool bold = s.Has( ts_FontBold ) && s.Get( ts_FontBold );
  const bool italic = s.Has( ts_FontItalic ) && s.Get( ts_FontItalic );
  const faint::Color fgCol( s.Get( ts_FgCol ) );
  const faint::Color bgCol( s.Get( ts_BgCol ) );

  save();
  faint::coord x0 = t.P0().x;
  faint::coord y0 = t.P0().y;

  PangoFontDescription* font_description = pango_font_description_new();
  pango_font_description_set_family(font_description, s.Get( ts_FontFace ).c_str());
  pango_font_description_set_weight(font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL );
  pango_font_description_set_style(font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL );
  pango_font_description_set_absolute_size (font_description, s.Get( ts_FontSize ) * PANGO_SCALE);

  // Offset to anchor at the top
  PangoLayout* referenceLayout = pango_cairo_create_layout(m_impl->cr);
  pango_layout_set_font_description( referenceLayout, font_description );
  pango_layout_set_text( referenceLayout, "M", -1 );
  PangoRectangle ink;
  PangoRectangle logical;
  pango_layout_get_extents(referenceLayout, &ink, &logical);
  g_object_unref( referenceLayout );

  PangoLayout* layout = pango_cairo_create_layout(m_impl->cr);
  pango_layout_set_font_description(layout, font_description);
  pango_layout_set_text (layout, text.c_str(), -1);

  //RealignCairoFill(x0,y0,m_sc);
  translate(Point(x0,y0));
  rotate(t.Angle());
  if ( t.Height() < 0 ){
    scale( 1, -1 );
  }

  faint::coord xOffset = 0;
  faint::coord yOffset = ink.height / PANGO_SCALE;
  if ( rather_zero( t.Angle() ) ){
    // RealignCairoFill(xOffset, yOffset, m_sc);
  }
  translate(Point(xOffset, yOffset));

  // Render text as a Cairo path. Rendering with Pango gives somewhat
  // nicer output on Windows (e.g. ClearType sub-pixel anti aliasing),
  // but I've found no convenient way to turn that of when
  // saving/flattening, and the Pango output also caused letters to
  // "wobble" when the text is rotated.
  PangoLayoutLine* line = pango_layout_get_line( layout, 0 );
  pango_cairo_layout_line_path( m_impl->cr, line );
  set_source_rgba( fgCol );
  fill();
  g_object_unref (layout);
  pango_font_description_free (font_description);
  restore();
}

Size CairoContext::pango_text_extents( const utf8_string& text, const Settings& s ) const{
  const bool bold = s.Has( ts_FontBold ) && s.Get( ts_FontBold );
  const bool italic = s.Has( ts_FontItalic ) && s.Get( ts_FontItalic );
  PangoFontDescription* font_description = pango_font_description_new();
  font_description = pango_font_description_new();
  pango_font_description_set_family(font_description, s.Get( ts_FontFace ).c_str());
  pango_font_description_set_weight(font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL );
  pango_font_description_set_style( font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL );
  pango_font_description_set_absolute_size(font_description, s.Get( ts_FontSize ) * PANGO_SCALE);
  PangoLayout* layout = pango_cairo_create_layout(m_impl->cr);
  pango_layout_set_font_description(layout, font_description);
  pango_layout_set_text(layout, text.c_str(), -1);

  PangoRectangle ink;
  PangoRectangle logical;
  pango_layout_get_extents(layout, &ink, &logical);
  pango_font_description_free( font_description );
  g_object_unref(layout);
  return Size( ink.width / PANGO_SCALE, ink.height / PANGO_SCALE );
}

void CairoContext::restore(){
  cairo_restore(m_impl->cr);
}

void CairoContext::rotate( faint::radian angle ){
  cairo_rotate(m_impl->cr, angle);
}

void CairoContext::save(){
  cairo_save(m_impl->cr);
}

void CairoContext::scale(faint::coord sx, faint::coord sy){
  cairo_scale(m_impl->cr, sx, sy);
}

void CairoContext::set_dash(const faint::coord* dashes, int num, double offset){
  cairo_set_dash(m_impl->cr, dashes, num, offset);
}

void CairoContext::set_line_cap(LineCap::type cap){
    if ( cap == LineCap::ROUND ){
    cairo_set_line_cap( m_impl->cr, CAIRO_LINE_CAP_ROUND );
  }
  else if ( cap == LineCap::BUTT ){
    cairo_set_line_cap( m_impl->cr, CAIRO_LINE_CAP_BUTT );
  }
  else {
    assert( false );
  }
}

void CairoContext::set_line_join(LineJoin::type join ){
  if ( join == LineJoin::ROUND ){
    cairo_set_line_join( m_impl->cr, CAIRO_LINE_JOIN_ROUND );
  }
  else if ( join == LineJoin::BEVEL ){
    cairo_set_line_join( m_impl->cr, CAIRO_LINE_JOIN_BEVEL );
  }
  else if ( join == LineJoin::MITER ){
    cairo_set_line_join( m_impl->cr, CAIRO_LINE_JOIN_MITER );
  }
  else {
    assert( false );
  }
}

void CairoContext::set_line_width(faint::coord width){
  cairo_set_line_width( m_impl->cr, width );
}

void CairoContext::set_source_rgba(const faint::Color& c){
  cairo_set_source_rgba( m_impl->cr, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0 );
}

void CairoContext::stroke(){
  cairo_stroke(m_impl->cr);
}

void CairoContext::translate(const Point& p){
  cairo_translate(m_impl->cr, p.x, p.y);
}

} // namespace faint

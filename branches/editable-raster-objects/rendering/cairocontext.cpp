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
#include <functional>
#include "cairo.h"
#include "geo/tri.hh"
#include "pango/pangocairo.h"
#include "rendering/cairocontext.hh"
#include "util/char.hh"
#include "util/gradient.hh"
#include "util/pattern.hh"
namespace faint{

Rect to_faint(const PangoRectangle& r ){
  return Rect(Point(r.x, r.y), Size(r.width, r.height));
}

static cairo_matrix_t faint_cairo_gradient_matrix( const LinearGradient& g, const Size& sz ){
  cairo_matrix_t m;
  cairo_matrix_init_identity(&m);
  cairo_matrix_translate(&m, 0.5, 0.5);
  cairo_matrix_rotate(&m, -g.GetAngle());
  cairo_matrix_translate(&m, -0.5, -0.5);
  cairo_matrix_scale(&m, 1.0 / sz.w, 1.0 / sz.h ); // Fixme: What of 0 size
  return m;
}

static cairo_matrix_t faint_cairo_gradient_matrix( const RadialGradient& g, const Size& sz ){
  cairo_matrix_t m;
  cairo_matrix_init_identity(&m);
  cairo_matrix_scale(&m, 1.0 / sz.w, 1.0 / sz.h ); // Fixme: What of 0 size
  Point center(g.GetCenter());
  cairo_matrix_translate(&m, -center.x, center.y);
  return m;
}

cairo_surface_t* get_cairo_surface( Bitmap& bmp ){
  cairo_surface_t* surface = cairo_image_surface_create_for_data (bmp.GetRaw(), CAIRO_FORMAT_ARGB32, bmp.m_w, bmp.m_h, bmp.m_row_stride );
  return surface;
}

int faint_cairo_stride( const IntSize& sz ){
  int stride = cairo_format_stride_for_width( CAIRO_FORMAT_ARGB32, sz.w );
  return stride;
}

cairo_pattern_t* faint_cairo_linear_gradient( const LinearGradient& g ){
  cairo_pattern_t* cg = cairo_pattern_create_linear(0.0, 0.0, 1.0, 0.0 );

  for ( int i = 0; i != g.GetNumStops(); i++ ){
    faint::ColorStop stop(g.GetStop(i));
    faint::Color c(stop.GetColor());
    double offset(stop.GetOffset());
    cairo_pattern_add_color_stop_rgba(cg,
      offset, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0 );
  }
  return cg;
}

cairo_pattern_t* faint_cairo_radial_gradient( const RadialGradient& g ){
  Radii r(g.GetRadii());
  cairo_pattern_t* cg =
    cairo_pattern_create_radial(0.5, 0.5, 0.0, // Inner circle
      0.5, 0.5, 0.5); // Outer

  for ( int i = 0; i != g.GetNumStops(); i++ ){
    faint::ColorStop stop(g.GetStop(i));
    faint::Color c(stop.GetColor());
    double offset(stop.GetOffset());
    cairo_pattern_add_color_stop_rgba( cg,
      offset, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0 );
  }
  return cg;
}

faint::Bitmap cairo_linear_gradient_bitmap( const faint::LinearGradient& gradient, const IntSize& sz ){
  cairo_pattern_t* p_gradient = faint_cairo_linear_gradient(gradient);
  Bitmap bmpDst(sz);
  faint::clear(bmpDst, faint::Color(255,255,255,0)); // Fixme: The gradient will blend towards transparent _white_
  cairo_surface_t* s_dst = get_cairo_surface(bmpDst);
  cairo_t* cr = cairo_create(s_dst);
  cairo_matrix_t m(faint_cairo_gradient_matrix(gradient, floated(sz)));
  cairo_pattern_set_matrix( p_gradient, &m);
  cairo_set_source( cr, p_gradient );
  // Fixme: Consider cairo_mask to draw gradient
  cairo_move_to( cr, 0, 0 );
  cairo_line_to( cr, sz.w, 0 );
  cairo_line_to( cr, sz.w, sz.h );
  cairo_line_to( cr, 0, sz.h );
  cairo_close_path( cr );
  cairo_fill( cr );
  cairo_destroy( cr );
  cairo_surface_destroy(s_dst);
  cairo_pattern_destroy(p_gradient);
  return bmpDst;
}

faint::Bitmap cairo_radial_gradient_bitmap( const faint::RadialGradient& gradient, const IntSize& sz ){
  cairo_pattern_t* p_gradient = faint_cairo_radial_gradient(gradient);
  Bitmap bmpDst(sz);
  faint::clear(bmpDst, faint::Color(255,255,255,0)); // Fixme
  cairo_surface_t* s_dst = get_cairo_surface(bmpDst);
  cairo_t* cr = cairo_create(s_dst);
  cairo_matrix_t m(faint_cairo_gradient_matrix(gradient, floated(sz)));
  cairo_pattern_set_matrix( p_gradient, &m);
  cairo_set_source( cr, p_gradient );
  cairo_move_to( cr, 0, 0 );
  cairo_line_to( cr, sz.w, 0 );
  cairo_line_to( cr, sz.w, sz.h );
  cairo_line_to( cr, 0, sz.h );
  cairo_close_path( cr );
  cairo_fill( cr );
  cairo_destroy( cr );
  cairo_surface_destroy(s_dst);
  cairo_pattern_destroy(p_gradient);
  return bmpDst;
}

faint::Bitmap cairo_gradient_bitmap( const faint::Gradient& gradient, const IntSize& sz ){
  if ( gradient.IsLinear() ){
    return cairo_linear_gradient_bitmap( gradient.GetLinear(), sz );
  }
  else {
    return cairo_radial_gradient_bitmap( gradient.GetRadial(), sz );
  }
}

faint::Bitmap cairo_skew( const faint::Bitmap& in_bmpSrc, faint::coord skew_angle, faint::coord skew_pixels ){

  faint::Bitmap& bmpSrc(const_cast<faint::Bitmap&>(in_bmpSrc)); // Needs non-const bitmap for source surface. Will not modify.
  faint::Bitmap bmpDst(IntSize(int(bmpSrc.m_w + fabs(skew_pixels)),int( bmpSrc.m_h)));
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

cairo_matrix_t cairo_linear_matrix_from_tri( const Tri& t, double radians, bool objectAligned ){
  double sx = t.Width();
  double sy = t.Height();
  double x0 = t.P0().x;
  double y0 = t.P0().y;

  if ( !objectAligned ){
    Rect r(bounding_rect(t));
    sx = r.w;
    sy = r.h;
    x0 = r.x;
    y0 = r.y;
  }
  cairo_matrix_t m;
  cairo_matrix_init_identity(&m);
  cairo_matrix_translate( &m, 0.5, 0.5);
  cairo_matrix_rotate( &m, -radians);
  cairo_matrix_translate( &m, -0.5, -0.5);
  cairo_matrix_scale(&m, 1.0 / sx, 1.0 / sy);
  if ( objectAligned ){
    cairo_matrix_rotate( &m, -t.Angle() );
  }
  cairo_matrix_translate( &m, -x0, -y0 );
  return m;
}

cairo_matrix_t cairo_radial_matrix_from_tri( const Tri& t, const RadialGradient& g ){
  if ( g.GetObjectAligned() ){
    double sx = t.Width();
    double sy = t.Height();
    Point center(g.GetCenter());
    double x0 = t.P0().x;
    double y0 = t.P0().y;
    cairo_matrix_t m;
    cairo_matrix_init_identity(&m);
    cairo_matrix_translate(&m, center.x, center.y);
    cairo_matrix_scale(&m, 1.0 / sx, 1.0 / sy );
    cairo_matrix_translate(&m, -center.x, -center.y);
    cairo_matrix_translate(&m, -x0, -y0 );
    return m;
  }
  else {
    Radii r(g.GetRadii());
    Point c(g.GetCenter());
    cairo_matrix_t m;
    cairo_matrix_init_identity(&m);
    cairo_matrix_translate(&m, 0.5, 0.5);
    cairo_matrix_scale(&m, 1.0 / ( 2 * r.x), 1.0 / ( 2 * r.y ) );
    cairo_matrix_translate(&m, -0.5, -0.5);
    cairo_matrix_translate(&m, -c.x, -c.y);
    return m;
  }
}

cairo_matrix_t cairo_pattern_matrix_from_tri( const Tri& t ){
  Point p0(t.P0());
  cairo_matrix_t m;
  cairo_matrix_init_identity(&m);
  cairo_matrix_translate( &m, -p0.x, -p0.y);
  return m;
}

std::string get_cairo_version(){
  return CAIRO_VERSION_STRING; // Note: Compile time.
}

std::string get_pango_version(){
  return PANGO_VERSION_STRING; // Note: Compile time.
}

class CairoContextImpl{
public:
  CairoContextImpl( faint::Bitmap& bmp )
    : patternTri(Point(0,0), Point(100,0), 100.0)
  {
    this->surface = get_cairo_surface(bmp);
    this->cr = cairo_create(surface);
    this->srcBmp = nullptr;
    this->srcSurface = nullptr;
    this->srcPattern = nullptr;
    this->gradientPattern = nullptr;
    this->linearGradient = nullptr;
    this->radialGradient = nullptr;
    this->pattern = nullptr;
  }
  ~CairoContextImpl(){
    cairo_destroy(this->cr);
    cairo_surface_destroy(this->surface);
    if ( this->srcSurface != nullptr ){
      cairo_pattern_destroy(this->srcPattern);
      cairo_surface_destroy(this->srcSurface);
      delete this->srcBmp;
    }
    if ( this->gradientPattern != nullptr ){
      cairo_pattern_destroy(this->gradientPattern);
    }
    delete this->linearGradient;
    delete this->radialGradient;
    delete this->pattern;
  }
  cairo_t* cr;
  cairo_surface_t* surface;

  Tri patternTri;

  faint::Bitmap* srcBmp;
  cairo_surface_t* srcSurface;
  cairo_pattern_t* srcPattern;
  faint::Pattern* pattern;

  cairo_pattern_t* gradientPattern;
  faint::LinearGradient* linearGradient;
  faint::RadialGradient* radialGradient;

};

CairoContext::CairoContext( faint::Bitmap& bmp )
  : m_impl(new CairoContextImpl(bmp))
{}

CairoContext::~CairoContext(){
  delete m_impl;
}

bool CairoContext::IsOk() const{
  return cairo_status(m_impl->cr) == CAIRO_STATUS_SUCCESS;
}

std::string CairoContext::ErrorString() const{
  return cairo_status_to_string(cairo_status(m_impl->cr));
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
  set_source( s.Get( ts_FgCol ) ); // Fixme

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

  translate(Point(x0,y0));
  rotate(t.Angle());
  if ( t.Height() < 0 ){
    scale( 1, -1 );
  }

  faint::coord xOffset = 0;
  faint::coord yOffset = ink.height / PANGO_SCALE;
  translate(Point(xOffset, yOffset));

  // Render text as a Cairo path. Rendering with Pango gives somewhat
  // nicer output on Windows (e.g. ClearType sub-pixel anti aliasing),
  // but I've found no convenient way to turn that of when
  // saving/flattening, and the Pango output also caused letters to
  // "wobble" when the text is rotated.
  PangoLayoutLine* line = pango_layout_get_line( layout, 0 );
  pango_cairo_layout_line_path( m_impl->cr, line );
  fill();
  g_object_unref (layout);
  pango_font_description_free (font_description);
  restore();
}

Size CairoContext::pango_text_size( const utf8_string& text, const Settings& s ) const{
  const bool bold = s.Has( ts_FontBold ) && s.Get( ts_FontBold );
  const bool italic = s.Has( ts_FontItalic ) && s.Get( ts_FontItalic );
  PangoFontDescription* font_description = pango_font_description_new();
  pango_font_description_set_family(font_description, s.Get( ts_FontFace ).c_str());
  pango_font_description_set_weight(font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL );
  pango_font_description_set_style( font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL );
  pango_font_description_set_absolute_size(font_description, s.Get( ts_FontSize ) * PANGO_SCALE);
  PangoLayout* layout = pango_cairo_create_layout(m_impl->cr);
  pango_layout_set_font_description(layout, font_description);
  utf8_string useText;
  if ( text.size() > 1 ){
    useText = text.substr(0, text.size() - 2);
  }
  else{
    useText = text;
  }
  pango_layout_set_text(layout, text.c_str(), -1);

  int w, h;
  pango_layout_get_pixel_size(layout, &w, &h);
  pango_font_description_free( font_description );
  g_object_unref(layout);
  return Size( w, h );
}

TextMeasures<Rect> CairoContext::pango_text_extents( const utf8_string& text, const Settings& s ) const{
  // Fixme: duplicates lots of pango_text_size
  const bool bold = s.Has( ts_FontBold ) && s.Get( ts_FontBold );
  const bool italic = s.Has( ts_FontItalic ) && s.Get( ts_FontItalic );
  PangoFontDescription* font_description = pango_font_description_new();
  pango_font_description_set_family(font_description, s.Get( ts_FontFace ).c_str());
  pango_font_description_set_weight(font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL );
  pango_font_description_set_style( font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL );
  pango_font_description_set_absolute_size(font_description, s.Get( ts_FontSize ) * PANGO_SCALE);
  PangoLayout* layout = pango_cairo_create_layout(m_impl->cr);
  pango_layout_set_font_description(layout, font_description);
  utf8_string useText;
  if ( text.size() > 1 ){
    useText = text.substr(0, text.size() - 2);
  }
  else{
    useText = text;
  }
  pango_layout_set_text(layout, text.c_str(), -1);
  PangoRectangle ink = {0,0,0,0};
  PangoRectangle logical = {0,0,0,0};
  pango_layout_get_pixel_extents(layout,
    &ink,
    &logical);
  pango_font_description_free( font_description );
  g_object_unref(layout);
  TextMeasures<Rect> result;
  result.ink = to_faint(ink);
  result.logical = to_faint(logical);
  return result;
}

std::vector<int> CairoContext::cumulative_text_width( const utf8_string& text, const Settings& s ) const{
  const bool bold = s.Has( ts_FontBold ) && s.Get( ts_FontBold );
  const bool italic = s.Has( ts_FontItalic ) && s.Get( ts_FontItalic );
  PangoFontDescription* font_description = pango_font_description_new();
  pango_font_description_set_family(font_description, s.Get( ts_FontFace ).c_str());
  pango_font_description_set_weight(font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL );
  pango_font_description_set_style( font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL );
  pango_font_description_set_absolute_size(font_description, s.Get( ts_FontSize ) * PANGO_SCALE);
  PangoLayout* layout = pango_cairo_create_layout(m_impl->cr);
  pango_layout_set_font_description(layout, font_description);

  std::vector<int> v;
  for ( size_t i = 1; i <= text.size(); i++ ){
    faint::utf8_string substr(text.substr(0, i));
    int w, h;
    pango_layout_set_text(layout, substr.c_str(), -1);
    pango_layout_get_pixel_size(layout, &w, &h);
    v.push_back( w );
  }
  return v;
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

void CairoContext::set_line_cap(LineCap cap){
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

void CairoContext::set_line_join(LineJoin join ){
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

void CairoContext::set_source( const faint::DrawSource& src ){
  dispatch(src,
    [&](const faint::Color& color){set_source_rgba(color);},
    [&](const faint::Pattern& pattern){set_source_surface(pattern);},
    [&](const faint::Gradient& gradient){set_source_gradient(gradient);});
}

void CairoContext::set_source_rgba(const faint::Color& c){
  cairo_set_source_rgba( m_impl->cr, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0 );
  if ( m_impl->srcSurface != nullptr ){
    cairo_surface_destroy(m_impl->srcSurface);
    cairo_pattern_destroy(m_impl->srcPattern);
    delete m_impl->srcBmp;
    m_impl->srcSurface = nullptr;
    m_impl->srcPattern = nullptr;
    m_impl->srcBmp = nullptr;
  }

  if ( m_impl->gradientPattern != nullptr ){
    cairo_pattern_destroy(m_impl->gradientPattern);
    m_impl->gradientPattern = nullptr;
    delete m_impl->linearGradient;
    m_impl->linearGradient = nullptr;
    delete m_impl->radialGradient;
    m_impl->radialGradient = nullptr;
  }
}

void CairoContext::set_source_surface( const faint::Pattern& pattern ){ // Fixme: Use anchor point
  if ( m_impl->srcSurface != nullptr ){
    cairo_surface_destroy(m_impl->srcSurface);
    cairo_pattern_destroy(m_impl->srcPattern);
    delete m_impl->pattern;
    delete m_impl->srcBmp;
    m_impl->srcSurface = nullptr;
    m_impl->srcPattern = nullptr;
    m_impl->srcBmp = nullptr;
  }
  if ( m_impl->gradientPattern != nullptr ){
    cairo_pattern_destroy(m_impl->gradientPattern);
    m_impl->gradientPattern = nullptr;
    delete m_impl->linearGradient;
    m_impl->linearGradient = nullptr;
    delete m_impl->radialGradient;
    m_impl->radialGradient = nullptr;
  }

  // Fixme: Probably no need to copy bitmap AND pattern.
  // Just clone the pattern, and use GetBitmap(), storing the pointer
  m_impl->srcBmp = new faint::Bitmap(pattern.GetBitmap()); // Fixme: check format
  m_impl->srcSurface = get_cairo_surface(*(m_impl->srcBmp));
  m_impl->srcPattern = cairo_pattern_create_for_surface(m_impl->srcSurface);
  m_impl->pattern = new faint::Pattern(pattern);
  cairo_pattern_set_extend( m_impl->srcPattern, CAIRO_EXTEND_REPEAT );
  if ( m_impl->pattern->GetObjectAligned() ){
    cairo_matrix_t matrix = cairo_pattern_matrix_from_tri( m_impl->patternTri);
    cairo_pattern_set_matrix(m_impl->srcPattern, &matrix);
  }
  cairo_set_source( m_impl->cr, m_impl->srcPattern ); // Fixme: use x and y
}

void CairoContext::set_source_gradient( const Gradient& in_gradient ){
  if ( in_gradient.IsLinear() ){
    faint::LinearGradient g(in_gradient.GetLinear());
    if ( m_impl->gradientPattern != nullptr ){
      cairo_pattern_destroy(m_impl->gradientPattern);
      m_impl->gradientPattern = nullptr;
      delete m_impl->linearGradient;
      m_impl->linearGradient = nullptr;
      delete m_impl->radialGradient;
      m_impl->radialGradient = nullptr;
    }

    if ( m_impl->srcSurface != nullptr ){
      cairo_surface_destroy(m_impl->srcSurface);
      cairo_pattern_destroy(m_impl->srcPattern);
      delete m_impl->srcBmp;
      m_impl->srcSurface = nullptr;
      m_impl->srcPattern = nullptr;
      m_impl->srcBmp = nullptr;
    }

    m_impl->linearGradient = new faint::LinearGradient(g);
    m_impl->gradientPattern = faint_cairo_linear_gradient(g);
    cairo_matrix_t matrix = cairo_linear_matrix_from_tri( m_impl->patternTri, m_impl->linearGradient->GetAngle(), m_impl->linearGradient->GetObjectAligned() );
    cairo_pattern_set_matrix(m_impl->gradientPattern, &matrix);
    cairo_set_source( m_impl->cr, m_impl->gradientPattern );
  }
  else if ( in_gradient.IsRadial() ){
    faint::RadialGradient g(in_gradient.GetRadial());
    if ( m_impl->gradientPattern != nullptr ){
      cairo_pattern_destroy(m_impl->gradientPattern);
      m_impl->gradientPattern = nullptr;
      delete m_impl->linearGradient;
      m_impl->linearGradient = nullptr;
      delete m_impl->radialGradient;
      m_impl->radialGradient = nullptr;
    }
    if ( m_impl->srcSurface != nullptr ){
      cairo_surface_destroy(m_impl->srcSurface);
      cairo_pattern_destroy(m_impl->srcPattern);
      delete m_impl->srcBmp;
      m_impl->srcSurface = nullptr;
      m_impl->srcPattern = nullptr;
      m_impl->srcBmp = nullptr;
    }
    m_impl->radialGradient = new faint::RadialGradient(g);
    m_impl->gradientPattern = faint_cairo_radial_gradient(g);
    cairo_matrix_t matrix = cairo_radial_matrix_from_tri( m_impl->patternTri, *m_impl->radialGradient );
    cairo_pattern_set_matrix(m_impl->gradientPattern, &matrix);
    cairo_set_source( m_impl->cr, m_impl->gradientPattern );
  }
  else{
    assert(false);
  }
}

void CairoContext::set_source_tri( const Tri& t ){
  m_impl->patternTri = t;
  if ( m_impl->linearGradient != 0 ){
    cairo_matrix_t matrix = cairo_linear_matrix_from_tri(t, m_impl->linearGradient->GetAngle(), m_impl->linearGradient->GetObjectAligned());
    cairo_pattern_set_matrix(m_impl->gradientPattern, &matrix);
    cairo_set_source( m_impl->cr, m_impl->gradientPattern );
  }
  else if ( m_impl->radialGradient != 0 ){
    cairo_matrix_t matrix = cairo_radial_matrix_from_tri(t, *(m_impl->radialGradient));
    cairo_pattern_set_matrix(m_impl->gradientPattern, &matrix);
    cairo_set_source(m_impl->cr, m_impl->gradientPattern);
  }
  else if ( m_impl->srcPattern != 0 ){
    bool objectAligned = m_impl->pattern->GetObjectAligned();
    if ( objectAligned ){
      cairo_matrix_t matrix = cairo_pattern_matrix_from_tri(t);
      cairo_pattern_set_matrix(m_impl->srcPattern, &matrix);
      cairo_set_source( m_impl->cr, m_impl->srcPattern );
    }
  }
}

void CairoContext::stroke(){
  cairo_stroke(m_impl->cr);
}

void CairoContext::translate(const Point& p){
  cairo_translate(m_impl->cr, p.x, p.y);
}

} // namespace faint

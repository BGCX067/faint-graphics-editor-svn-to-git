#include <cassert>
#include "objects/object.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

// Setting init functions, for presumably faster retrieval
Settings init_default_rectangle_settings(){
  Settings s;
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, 1 );
  s.Set( ts_FillStyle, FillStyle::BORDER );
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color(255,255,255) );
  s.Set( ts_SwapColors, false );
  s.Set( ts_AntiAlias, true );
  s.Set( ts_LineJoin, LineJoin::MITER );
  return s;
}

Settings init_settings_selection_rectangle(){
  Settings s( default_rectangle_settings() );
  s.Set( ts_FgCol, faint::Color( 255, 0, 255 ) );
  s.Set( ts_BgCol, faint::Color( 0, 0, 0 ) );
  s.Set( ts_FillStyle, FillStyle::BORDER );
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, LineStyle::LONG_DASH );
  s.Set( ts_AntiAlias, false );
  return s;
}

Settings init_default_ellipse_settings(){
  Settings s;
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, 1 );
  s.Set( ts_FillStyle, 0 );
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color( 255, 255, 255 ) );
  s.Set( ts_SwapColors, false );
  s.Set( ts_AntiAlias, true );
  return s;
}

Settings init_default_line_settings(){
  Settings s;
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, LineStyle::SOLID );
  s.Set( ts_LineArrowHead, 0 );
  s.Set( ts_LineCap, LineCap::ROUND );
  s.Set( ts_LineJoin, LineJoin::ROUND );
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_SwapColors, false );
  s.Set( ts_AntiAlias, true );
  return s;
}

Settings init_default_path_settings(){
  Settings s;
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, 1 );
  s.Set( ts_FillStyle, 0 );
  s.Set( ts_FgCol, faint::Color( 0, 0, 0 ) );
  s.Set( ts_BgCol, faint::Color( 255, 255, 255 ) );
  s.Set( ts_SwapColors, false );
  return s;
}

Settings init_default_polygon_settings(){
  Settings s;
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, 1 );
  s.Set( ts_FillStyle, 0 );
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color( 255, 255, 255 ) );
  s.Set( ts_SwapColors, false );
  s.Set( ts_LineJoin, LineJoin::MITER );
  return s;
}

Settings init_default_raster_settings(){
  Settings s;
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color(255,255,255) );
  s.Set( ts_BackgroundStyle, BackgroundStyle::SOLID );
  return s;
}

Settings init_default_spline_settings(){
  Settings s;
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, LineStyle::SOLID );
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color( 255, 255, 255 ) );
  s.Set( ts_SwapColors, false );
  s.Set( ts_LineCap, LineCap::BUTT );
  return s;
}

Settings init_default_text_settings(){
  Settings s;
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color( 255, 255, 255 ) );
  s.Set( ts_SwapColors, false );
  s.Set( ts_FontSize, get_default_font_size() );
  s.Set( ts_FontFace, get_default_font_name() );
  s.Set( ts_FontBold, false );
  s.Set( ts_FontItalic, false );
  s.Set( ts_TextAutoRect, false );
  return s;
}

Settings init_default_tool_settings(){
  Settings s;
  s.Set( ts_LineWidth, LITCRD(1.0) );
  s.Set( ts_LineArrowHead, 0 );
  s.Set( ts_LineStyle, LineStyle::SOLID );
  s.Set( ts_LineCap, LineCap::BUTT );
  s.Set( ts_FillStyle, FillStyle::BORDER );
  s.Set( ts_BackgroundStyle, BackgroundStyle::MASKED );
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color(255,255,255) );
  s.Set( ts_FontSize, get_default_font_size() );
  s.Set( ts_FontFace, get_default_font_name() );
  s.Set( ts_FontBold, false );
  s.Set( ts_FontItalic, false );
  s.Set( ts_BrushSize, 5 );
  s.Set( ts_PolyLine, false );
  s.Set( ts_SwapColors, false );
  s.Set( ts_BrushShape, BrushShape::SQUARE );
  s.Set( ts_AlphaBlending, false );
  return s;
}

void add_fill( Settings& s ){
  assert( s.Has(ts_FillStyle) );
  int fillStyle = s.Get(ts_FillStyle);
  if ( fillStyle == FillStyle::BORDER ){
    s.Set( ts_FillStyle, FillStyle::BORDER_AND_FILL );
  }
}

FillStyle::type add_fill( FillStyle::type fillStyle ){
  if ( fillStyle == FillStyle::BORDER ){
    return FillStyle::BORDER_AND_FILL;
  }
  return fillStyle;
}

bool alpha_blending( const Settings& s ){
  return s.GetDefault( ts_AlphaBlending, true );
}

bool anti_aliasing( const Settings& s ){
  return s.GetDefault( ts_AntiAlias, false );
}

bool border( const Settings& s ){
  int f = s.Get(ts_FillStyle);
  return f == FillStyle::BORDER_AND_FILL || f == FillStyle::BORDER;
}

Settings default_bitmap_settings(){
  // Fixme: Probably nutty
  Settings s;
  s.Set( ts_AlphaBlending, false );
  s.Set( ts_BackgroundStyle, BackgroundStyle::SOLID );
  s.Set( ts_BgCol, faint::Color(0, 0, 0));
  return s;
}

const Settings& default_ellipse_settings(){
  static Settings s(init_default_ellipse_settings());
  return s;
}

const Settings& default_line_settings(){
  static Settings s(init_default_line_settings());
  return s;
}

const Settings& default_path_settings(){
  static Settings s(init_default_path_settings());
  return s;
}

const Settings& default_polygon_settings(){
  static Settings s(init_default_polygon_settings());
  return s;
}

const Settings& default_rectangle_settings(){
  static Settings s(init_default_rectangle_settings());
  return s;
}

const Settings& default_spline_settings(){
  static Settings s(init_default_spline_settings());
  return s;
}

const Settings& default_text_settings(){
  static Settings s(init_default_text_settings());
  return s;
}

const Settings& default_raster_settings(){
  static Settings s(init_default_raster_settings());
  return s;
}

const Settings& default_tool_settings(){
  static Settings s(init_default_tool_settings());
  return s;
}

Settings eraser_rectangle_settings( const faint::Color& eraserColor ){
  Settings s(default_rectangle_settings());
  s.Set( ts_FillStyle, FillStyle::FILL );
  s.Set( ts_AntiAlias, false );
  s.Set( ts_FgCol, eraserColor );
  s.Set( ts_BgCol, eraserColor );
  return s;
}

bool filled( const Settings& s ){
  int fillStyle = s.GetDefault( ts_FillStyle, FillStyle::BORDER );
  return fillStyle == FillStyle::FILL || fillStyle == FillStyle::BORDER_AND_FILL;
}

faint::Color get_color_bg( const Settings& s ){
  bool explicitSwap = s.GetDefault( ts_SwapColors, false );
  bool fillOnlySwap = s.Has( ts_FillStyle ) && s.Get( ts_FillStyle ) == FillStyle::FILL;
  bool swap = explicitSwap ^ fillOnlySwap;
  return swap ? s.Get( ts_FgCol ) : s.Get( ts_BgCol );
}

faint::Color get_color_fg( const Settings& s ){
  bool explicitSwap = s.GetDefault( ts_SwapColors, false );
  return explicitSwap ? s.Get( ts_BgCol ) : s.Get( ts_FgCol );
}

LineCap::type get_line_cap( const Settings& s ){
  return static_cast<LineCap::type>( s.Get( ts_LineCap ) );
}

FillStyle::type get_fillstyle( const Settings& s ){
  return static_cast<FillStyle::type>( s.Get( ts_FillStyle ) );
}

bool is_object(Layer::type layer){
  return layer == Layer::OBJECT;
}

bool is_raster(Layer::type layer){
  return layer == Layer::RASTER;
}

bool dashed( const Settings& s ){
  return s.Has( ts_LineStyle ) && s.Get( ts_LineStyle ) == LineStyle::LONG_DASH;
}

bool masked_background( const Settings& s ){
  return s.Get( ts_BackgroundStyle ) == BackgroundStyle::MASKED;
}

Settings mask_settings_fill( const Settings& objSettings ){
  Settings s(objSettings);
  s.Set( ts_FgCol, mask_edge );
  if ( filled(s) ){
    s.Set( ts_BgCol, mask_fill );
  }
  else {
    // The transparent inside of the object should be filled with
    // the inside-indicating color in the mask
    s.Set( ts_BgCol, mask_no_fill );
    add_fill(s);
  }
  return s;
}

Settings mask_settings_line( const Settings& objSettings ){
  Settings s(objSettings);
  s.Set( ts_FgCol, mask_edge );
  return s;
}

Settings remove_background_color( const Settings& s ){
  Settings out(s);
  if ( out.Has( ts_BgCol ) ){
    out.Erase(ts_BgCol);
  }
  if ( out.GetDefault(ts_SwapColors, false) ){
    out.Set(ts_FgCol, s.Get(ts_BgCol));
    out.Erase(ts_SwapColors);
  }
  return out;
}

const Settings& selection_rectangle_settings(){
  static Settings s(init_settings_selection_rectangle());
  return s;
}

Settings with_point_editing(const Settings& s, bool enabled){
  Settings s2(s);
  s2.Set( ts_EditPoints, enabled);
  return s2;
}

Settings bitmap_mask_settings( bool mask, const faint::Color& bgCol, bool alpha ){
  Settings s(default_bitmap_settings());
  s.Set( ts_BgCol, bgCol );
  s.Set( ts_BackgroundStyle, mask ? BackgroundStyle::MASKED : BackgroundStyle::SOLID );
  s.Set( ts_AlphaBlending, alpha );
  return s;
}

Settings get_selection_settings( const RasterSelection& selection ){
  RasterSelectionOptions options = selection.GetOptions();
  Settings s;
  s.Set( ts_BgCol, options.bgCol  );
  s.Set( ts_BackgroundStyle, options.mask ? BackgroundStyle::MASKED : BackgroundStyle::SOLID  );
  s.Set( ts_AlphaBlending, options.alpha );
  return s;
}

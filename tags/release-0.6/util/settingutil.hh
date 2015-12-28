#ifndef FAINT_SETTINGUTIL_HH
#define FAINT_SETTINGUTIL_HH
#include "util/rasterselection.hh"
#include "util/settingid.hh"
class Settings;

// Adds filling to the ts_FillStyle settings without affecting the
// border.
// Note: Asserts that ts_FillStyle exists.
void add_fill( Settings& );

FillStyle::type add_fill( FillStyle::type );

bool alpha_blending( const Settings& );

bool anti_aliasing( const Settings& );

bool border( const Settings& );

bool dashed( const Settings& );

Settings default_bitmap_settings();

// Default settings for objects (shapes)
const Settings& default_ellipse_settings();
const Settings& default_line_settings();
const Settings& default_polygon_settings();
const Settings& default_rectangle_settings();
const Settings& default_spline_settings();
const Settings& default_path_settings();
const Settings& default_polygon_settings();
const Settings& default_raster_settings();
const Settings& default_text_settings();
const Settings& default_tool_settings();
Settings eraser_rectangle_settings( const faint::Color& eraserColor );

// True if the object has a ts_FillStyle and it indicates filled
// inside.
bool filled( const Settings& );

// Returns the background color, taking ts_SwapColors in account
faint::Color get_color_bg( const Settings& );

// Returns the foreground color, taking ts_SwapColors in account
faint::Color get_color_fg( const Settings& );

LineCap::type get_line_cap( const Settings& s );

FillStyle::type get_fillstyle( const Settings& s );

bool is_object( Layer::type );
bool is_raster( Layer::type );

// True if the background is masked (ts_Transparency)
bool masked_background( const Settings& );

// Creates hit-test mask settings for fillable objects (ellipses,
// polygons etc.), from the actual objSettings
Settings mask_settings_fill( const Settings& objSettings );

// Creates hit test mask settings for unclosed paths (non-fillable objects)
Settings mask_settings_line( const Settings& );

// Returns settings without the background color (ts_BgCol), and
// removes any swapping (ts_SwapColors)
Settings remove_background_color( const Settings& );

const Settings& selection_rectangle_settings();

// Add a a ts_EditPoints setting, possibly enabling it.  This can be
// used to add point editing to an object, which should have
// point-editing, created by a tool which should not.
Settings with_point_editing(const Settings&, bool enabled=false);

Settings bitmap_mask_settings( bool mask, const faint::Color& backgroundColor, bool alpha );

Settings get_selection_settings( const RasterSelection& );
#endif

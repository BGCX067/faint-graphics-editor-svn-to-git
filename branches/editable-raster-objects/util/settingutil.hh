#ifndef FAINT_SETTINGUTIL_HH
#define FAINT_SETTINGUTIL_HH
#include "util/rasterselection.hh"
#include "util/settingid.hh"
class Settings;

// Adds filling to the ts_FillStyle settings without affecting the
// border. Note: Asserts that ts_FillStyle exists.
void add_fill( Settings& );

bool alpha_blending( const Settings& );

bool anti_aliasing( const Settings& );

Settings bitmap_mask_settings( bool maskEnabled, const faint::DrawSource& bg, bool alpha );

bool border( const Settings& );

bool dashed( const Settings& );

Settings default_bitmap_settings();

// Default settings for objects (shapes)
const Settings& default_ellipse_settings();
const Settings& default_line_settings();
const Settings& default_path_settings();
const Settings& default_polygon_settings();
const Settings& default_raster_settings();
const Settings& default_rectangle_settings();
const Settings& default_spline_settings();
const Settings& default_text_settings();

const Settings& default_tool_settings();
Settings eraser_rectangle_settings( const faint::DrawSource& eraser );

// True if the fillstyle indicates filled inside
bool filled( FillStyle );

// True if the settings has a ts_FillStyle and it indicates filled
// inside.
bool filled( const Settings& );

// Tools can temporarily swap the foreground and background-colors
// (typically by drawing with right mouse button).
// For this, ts_SwapColors is set, rather than actually swapping
// ts_BgCol and ts_FgCol.  For objects (and some commands), the
// distinction is meaningless, so this function swaps ts_FgCol and
// ts_BgCol and clears ts_SwapColors.
void finalize_swap_colors( Settings& );

// Same as finalize_swap_colors, but erases the background and the
// swap colors flag.
void finalize_swap_colors_erase_bg( Settings& );

// Returns the background, taking ts_SwapColors in account
faint::DrawSource get_bg( const Settings& );

// Returns the foreground, ts_SwapColors in account
faint::DrawSource get_fg( const Settings& );

FillStyle get_fillstyle( const Settings& );
FillStyle get_fillstyle_default( const Settings&, FillStyle);
LineCap get_line_cap( const Settings& s);

Settings get_selection_settings( const RasterSelection& );

bool is_object( Layer );
bool is_raster( Layer );

// Creates hit-test mask settings for fillable objects (ellipses,
// polygons etc.), from the actual objSettings
Settings mask_settings_fill( const Settings& objSettings );

// Creates hit test mask settings for unclosed paths (non-fillable objects)
Settings mask_settings_line( const Settings& );

// True if the background is masked (ts_BackgroundStyle)
bool masked_background( const Settings& );

// Returns settings without the background color (ts_BgCol), and
// removes any swapping (ts_SwapColors)
Settings remove_background_color( const Settings& );

const Settings& selection_rectangle_settings();

// Returns whether the ts_FgCol or the ts_BgCol is used to fill
// with the given fill style.
ColorSetting setting_used_for_fill( FillStyle );

// Adds filling to the passed in style, without modifying the border
// style
FillStyle with_fill( FillStyle );

class category_settingutil;
typedef Unique<bool, category_settingutil, 0> start_enabled;
// Add a a ts_EditPoints setting, possibly enabling it.  This can be
// used to add point editing to an object, which should have
// point-editing, created by a tool which should not.
Settings with_point_editing(const Settings&, const start_enabled&);

#endif

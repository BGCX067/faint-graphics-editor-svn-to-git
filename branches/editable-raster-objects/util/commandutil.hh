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

#ifndef FAINT_COMMANDUTIL_HH
#define FAINT_COMMANDUTIL_HH
#include <deque>
#include "bitmap/bitmap.hh"
#include "bitmap/filter.hh"
#include "commands/command.hh"
#include "geo/tri.hh"
#include "util/commonfwd.hh"
#include "util/settingid.hh"

class ObjRaster;

class DeleteState{
  // For toggling two ways of doing raster delete.  When deleting the
  // first time, the selection is cleared with the secondary
  // color. The second time, if possible, by the color that covered
  // the edge of the selection on the first go.
public:
  DeleteState();
  void Clear();
  Command* GetDeleteCommand(const faint::Image&, const IntRect& selection, const faint::DrawSource& eraser);
  void Set( const faint::DrawSource& alternateColor );
private:
  bool m_lastWasDelete;
  faint::DrawSource m_alternateEraser;
  faint::DrawSource m_lastEraser;
};

// Gets an add object or a draw object command depending on the layer
Command* add_or_draw(Object*, Layer);

// Experimental custom-anti-aliased line
Command* get_aa_line_command(const IntPoint&, const IntPoint&, const faint::Color&);
bool affects_raster( const Command* );
Command* get_add_object_command( Object*, const select_added&, const std::string& name="Add" );
Command* get_add_objects_command( const objects_t&, const select_added&, const std::string& name="Add" );
Command* get_auto_crop_command( const faint::Image& );
Command* get_blend_alpha_command( const faint::Color& bgColor );
Command* get_blur_command();
Command* get_change_raster_background_command( ObjRaster*, const faint::Color& );

// Returns RASTER if all commands are raster, OBJECT if all are object
// or HYBRID if any is hybrid or there are both object and raster
// commands.
CommandType get_collective_command_type( const commands_t& );

// Returns a command which crops any croppable objects passed in or 0
// if no object could be cropped.
Command* get_crop_command( const objects_t& );
Command* get_crop_to_selection_command( const RasterSelection& );
Command* get_delete_objects_command( const objects_t&, const faint::Image&, const std::string& name="Delete" );
Command* get_delete_raster_command( const IntRect&, const faint::DrawSource&);
Command* get_delete_raster_selection_command( const faint::Image&, const faint::DrawSource& bgCol, DeleteState& );
Command* get_desaturate_simple_command();
Command* get_desaturate_weighted_command();
Command* get_erase_but_color_command( const faint::Color& keep, const faint::DrawSource& eraser );
Command* get_fill_boundary_command( Object*, const faint::DrawSource& );
Command* get_fill_inside_command( Object*, const faint::DrawSource& );
Command* get_flatten_command( const objects_t&, const faint::Image& );
Command* get_flood_fill_command( const IntPoint&, const faint::DrawSource& );

Command* get_brightness_and_contrast_command( const faint::brightness_contrast_t& );
Command* get_invert_command();

typedef Order<tris_t>::New NewTris;
typedef Order<tris_t>::Old OldTris;
Command* get_move_objects_command( const objects_t&, const NewTris&, const OldTris& );
Command* get_offset_objects_command( const objects_t&, const Point& delta );
Command* get_offset_raster_selection_command(const faint::Image&, const IntPoint& delta);
Command* get_objects_backward_command( const objects_t&, const faint::Image& );
Command* get_objects_forward_command( const objects_t&, const faint::Image& );
Command* get_objects_to_back_command( const objects_t&, const faint::Image& );
Command* get_objects_to_front_command( const objects_t&, const faint::Image& );
Command* get_paste_raster_bitmap_command( const faint::Bitmap&, const IntPoint&, const RasterSelection& oldSelection, const Settings&);

Command* get_pixelize_command( int width );
Command* get_replace_color_command( const OldColor&, const faint::DrawSource& );
Command* get_rescale_command( const IntSize&, ScaleQuality );
Command* get_set_alpha_command(faint::uchar alpha);

// Returns a ResizeCommand which uses the bgCol as background if expanding,
// possibly with a DWIM-option using the edge-color from the expand directions.
Command* get_resize_command( const faint::Bitmap&, const IntRect& newRect, const faint::DrawSource& bg );
Command* get_rotate_command( Object*, const faint::radian, const Point& origin );
Command* get_rotate_command( const objects_t&, const faint::radian, const Point& origin );
Command* get_scale_command( const objects_t&, const Scale&, const Point& origin );
Command* get_threshold_command( const faint::threshold_range_t& );

bool then_false( bool& );

Command* get_deselect_raster_command( const RasterSelection& );
Command* get_selection_rectangle_command( const IntRect&, const RasterSelection& );
Command* get_select_all_command( const faint::Image&, const RasterSelection& );
Command* get_rotate_selection_command( const faint::Image& );
Command* get_offset_selection_command( const faint::Image&, const IntPoint& delta );
Command* get_scale_raster_selection_command( const faint::Image&, const IntSize& newSize );
bool has_command( const std::deque<OldCommand>&, const Command* );

class OperationFlip : public Operation {
public:
  OperationFlip(Axis);
  Command* DoImage() const override;
  Command* DoObjects(const objects_t&) const override;
  Command* DoRasterSelection(const faint::Image&) const override;
private:
  Axis m_axis;
};

#endif

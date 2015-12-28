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

#include <algorithm>
#include <cassert>
#include "bitmap/aa_line.hh"
#include "bitmap/filter.hh"
#include "commands/add-object-cmd.hh"
#include "commands/change-setting-cmd.hh"
#include "commands/command-bunch.hh"
#include "commands/delete-object-cmd.hh"
#include "commands/draw-object-cmd.hh"
#include "commands/flip-rotate-cmd.hh"
#include "commands/function-cmd.hh"
#include "commands/order-object-cmd.hh"
#include "commands/rescale-cmd.hh"
#include "commands/resize-cmd.hh"
#include "commands/set-bitmap-cmd.hh"
#include "commands/set-raster-selection-cmd.hh"
#include "commands/tri-cmd.hh"
#include "objects/objraster.hh"
#include "objects/objrectangle.hh"
#include "objects/objtext.hh"
#include "rendering/cairocontext.hh"
#include "tools/tool.hh"
#include "util/autocrop.hh"
#include "util/commandutil.hh"
#include "util/formatting.hh"
#include "util/image.hh"
#include "util/imageutil.hh"
#include "util/objutil.hh"
#include "util/settingutil.hh"

bool then_false( bool& value ){
  if ( value ){
    value = false;
    return true;
  }
  return false;
}

class EnableDwimDeleteCommand : public Command {
// Command which always forwards Do(..) to the DeleteCommand.
// On the first call also sets the alternate color to the delete-state, enabling
// dwim-delete for the next delete, if nothing gets in the way.
public:
  EnableDwimDeleteCommand( DeleteState& state, const faint::Color& alternateColor, Command* cmd )
    : Command( CMD_TYPE_RASTER ),
      m_alternateColor(alternateColor),
      m_cmd(cmd),
      m_firstRun(true),
      m_state(state)
  {}
  ~EnableDwimDeleteCommand(){
    delete m_cmd;
  }
  void Do( CommandContext& context ){
    if ( then_false(m_firstRun) ){
      m_state.Set( m_alternateColor );
    }
    m_cmd->Do(context);
  }
  std::string Name() const{
    return m_cmd->Name();
  }

private:
  EnableDwimDeleteCommand& operator=(const EnableDwimDeleteCommand&);
  EnableDwimDeleteCommand(const EnableDwimDeleteCommand&);
  faint::Color m_alternateColor;
  Command* m_cmd;
  bool m_firstRun;
  DeleteState& m_state;
};

DeleteState::DeleteState()
  : m_lastWasDelete(false),
    m_alternateColor(faint::white_color()),
    m_lastDeleteColor(m_alternateColor)
{}

void DeleteState::Clear(){
  m_lastWasDelete = false;
}

Command* DeleteState::GetDeleteCommand(const faint::Image& image, const IntRect& selection, const faint::Color& bgColor){
  if ( m_lastWasDelete && m_lastDeleteColor == bgColor ){
    // Alternate delete-scheme for the second-delete: Use the
    // determined surounding color for the last delete instead of
    // the selected secondary color.
    return get_delete_raster_command(selection, m_alternateColor);
  }

  // When not following an identical delete, the command should use the selected
  // secondary color
  Command* deleteCommand = get_delete_raster_command(selection, bgColor);
  Optional<faint::Color> edgeColor( get_edge_color(image.GetBitmap(), selection) );
  if ( edgeColor.NotSet() ){
    // Could not determine an edge color, so DWIM-delete is not
    // possible
    return deleteCommand;
  }

  if ( faint::is_blank(cairo_compatible_sub_bitmap(image.GetBitmap(), selection))){
    return deleteCommand;
  }

  // An edge color could be determined, first do a regular delete. If another delete follows
  // it should use the edge color, unless the background color has changed.
  m_lastDeleteColor = bgColor;

  // Return a command which will call Set for this object when run.
  return new EnableDwimDeleteCommand(*this, edgeColor.Get(), deleteCommand);
}

void DeleteState::Set( const faint::Color& color ){
  m_alternateColor = color;
  m_lastWasDelete = true;
}

static bunch_name get_bunch_name( const std::string& command, const objects_t& objects ){
  return bunch_name(space_sep(command, get_collective_name(objects)));
}

Command* add_or_draw(Object* obj, Layer::type layer){
  if ( layer == Layer::RASTER ) {
    return new DrawObjectCommand(its_yours(obj));
  }
  else {
    return new AddObjectCommand(obj, select_added(false));
  }
}

bool affects_raster( const Command* cmd ){
  CommandType type( cmd->Type() );
  return type == CMD_TYPE_RASTER || type == CMD_TYPE_HYBRID;
}

Command* crop_one_object( Object* obj ){
  ObjRaster* raster = dynamic_cast<ObjRaster*>( obj );
  if ( raster != 0 ){
    return crop_raster_object_command( raster );
  }
  ObjText* text = dynamic_cast<ObjText*>( obj );
  if ( text != 0 ){
    return crop_text_region_command( text );
  }
  return 0;
}
Command* get_aa_line_command( const IntPoint& p0, const IntPoint& p1, const faint::Color& color ){
  return new FunctionCommand3<IntPoint, IntPoint, faint::Color, faint::draw_line_aa_Wu>("Draw Wu-line", p0, p1, color);
}

Command* get_add_object_command( Object* object, const select_added& select, const std::string& name ){
  return new AddObjectCommand( object, select, name );
}

Command* get_add_objects_command( const objects_t& objects, const select_added& select, const std::string& name ){
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    commands.push_back( new AddObjectCommand(objects[i], select, name ) );
  }
  return perhaps_bunch( CMD_TYPE_OBJECT, get_bunch_name(name, objects), commands );
}

Command* get_auto_crop_command( const faint::Image& image ){
  const bool hasObjects = !(image.GetObjects().empty());
  std::vector<IntRect> rects = hasObjects ?
    get_auto_crop_rectangles(faint::flatten(image)) :
    get_auto_crop_rectangles(image.GetBitmap());

  if ( rects.empty() ){
    return 0;
  }
  else if ( rects.size() == 1 ){
    return new ResizeCommand(rects[0], faint::white_color(), "Auto Crop Image");
  }
  else if ( rects.size() == 2 ){
    IntRect r1(rects[0]);
    IntRect r2(rects[1]);
    if ( area(r2) < area(r1) ){
      // Default to keeping the smaller region, cutting away the
      // larger region
      std::swap(r1, r2);
    }
    return new ResizeCommand(r1, Alternate(r2), faint::white_color(), "Auto Crop Image");
  }

  assert( false );
  return 0;
}

Command* get_blend_alpha_command( const faint::Color& bgColor ){
  typedef FunctionCommand1<faint::Color, faint::blend_alpha > BlendAlphaCommand;
  return new BlendAlphaCommand("Replace Alpha", bgColor);
}

Command* get_blur_command(){
  return new FunctionCommand<faint::blur>("Blur");
}

Command* get_change_raster_background_command( ObjRaster* obj, const faint::Color& color ){
  return command_bunch(CMD_TYPE_OBJECT, bunch_name("Set Raster Object Background"),
    change_setting_command( obj, ts_BgCol, color ),
    change_setting_command( obj, ts_BackgroundStyle, BackgroundStyle::MASKED ) );
}

CommandType get_collective_command_type( const commands_t& cmds ){
  assert(!cmds.empty());
  commands_t::const_iterator it = cmds.begin();
  CommandType prev = (*it)->Type();
  for ( ; it != cmds.end(); ++it ){
    CommandType current = (*it)->Type();
    if ( prev == CMD_TYPE_HYBRID || prev != current ){
      return CMD_TYPE_HYBRID;
    }
    prev = current;
  }
  return prev;
}

Command* get_crop_command( objects_t& objects ){
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Command* cmd = crop_one_object( objects[i] );
    if ( cmd != 0 ){
      commands.push_back( cmd );
    }
  }

  if ( commands.empty() ){
    // No object supported autocrop
    return 0;
  }
  return perhaps_bunch( CMD_TYPE_OBJECT, get_bunch_name("Crop", objects), commands );
}

Command* get_crop_to_selection_command( const RasterSelection& selection ){
  assert( selection.Exists() );
  if ( selection.Floating() ){
    return new SetBitmapCommand( selection.GetBitmap(), selection.TopLeft(),
      "Crop to Selection" );
  }
  return new ResizeCommand( selection.GetRect(), faint::white_color(),
    "Crop to Selection" );
}

Command* get_delete_objects_command( const objects_t& objects, const faint::Image& image, const std::string& name ){
  std::vector<Command*> commands;

  // Note the reverse iteration, objects must be deleted in reverse
  // order so that the Z-order is preserved
  for ( int i = objects.size() - 1; i >= 0; i-- ){
    Object* object = objects[i];
    commands.push_back( new DelObjectCommand( object, image.GetObjectZ( object ), name ) );
  }
  return perhaps_bunch( CMD_TYPE_OBJECT, get_bunch_name(name, objects), commands );
}

Command* get_delete_raster_command( const IntRect& r, const faint::Color& bgColor ){
  typedef FunctionCommand2<IntRect, faint::Color, faint::fill_rect> DeleteRasterCommand;
  return new DeleteRasterCommand("Erase Selection", r, bgColor);
}

Command* get_delete_raster_selection_command( const faint::Image& image, const faint::Color& bgCol, DeleteState& deleteState ){
  const RasterSelection& selection(image.GetRasterSelection());
  assert(!selection.Empty());
  if ( selection.Floating() ){
    if ( selection.Copying() ){
      return new SetRasterSelection(New(RasterSelectionState()), Old(selection.GetState()),
	"Delete Floating Selection");
    }
    return command_bunch(CMD_TYPE_HYBRID, bunch_name("Delete Selected Region"),
      new DrawObjectCommand(its_yours(new ObjRectangle(tri_from_rect(floated(selection.GetOldRect())), eraser_rectangle_settings(selection.GetBgCol())))),
      new SetRasterSelection(New(RasterSelectionState()), Old(selection.GetState()), "") );
  }
  else {
    // Raster delete
    return deleteState.GetDeleteCommand(image, selection.GetRect(), bgCol);
  }
}

Command* get_deselect_raster_command( RasterSelection& selection ){
  if ( selection.Floating() ){
    // Stamp the floating selection before deselecting
    return command_bunch(CMD_TYPE_HYBRID, bunch_name("Deselect Raster (floating)"),
      selection.StampFloatingSelection(),
      new SetRasterSelection(New(RasterSelectionState()), Old(selection.GetState()), ""));
  }
  // Just deselect
  return new SetRasterSelection(New(RasterSelectionState()), Old(selection.GetState()), "Deselect Raster" );
}

Command* get_paste_raster_bitmap_command( const faint::Bitmap& bmp, const IntPoint& topLeft, const RasterSelection& oldSelection, const Settings& s ){
  return command_bunch(CMD_TYPE_HYBRID, bunch_name("Paste Bitmap"),
    new SetSelectionOptions( New(RasterSelectionOptions(masked_background(s), s.Get(ts_BgCol), s.Get(ts_AlphaBlending) )),
      Old(oldSelection.GetOptions()) ),
    new SetRasterSelection(New(RasterSelectionState(bmp, topLeft)), Old(oldSelection.GetState()), ""));
}

Command* get_selection_rectangle_command( const IntRect& r, const OldRasterSelectionState& oldState ){
  return new SetRasterSelection( New(RasterSelectionState(r)), oldState, "Select Rectangle" );
}

Command* get_desaturate_simple_command(){
  return new FunctionCommand<faint::desaturate_simple>("Desaturate");
}

Command* get_desaturate_weighted_command(){
  return new FunctionCommand<faint::desaturate_weighted>("Desaturate Weighted");
}

void draw_dumb_ellipse_fwd( faint::Bitmap& bmp, const IntRect& r, const int& /*lineWidth*/, const faint::Color& c){
  faint::draw_dumb_ellipse(bmp, r, c); // Fixme: Use line width
}
Command* get_dumb_ellipse_command( const IntRect& r, int lineWidth, const faint::Color& c ){
  return new FunctionCommand3<IntRect, int, faint::Color, draw_dumb_ellipse_fwd>("Draw dumb ellipse", r, lineWidth, c);
}

Command* get_erase_but_color_command( const faint::Color& keep, const faint::Color& eraserColor ){
  typedef FunctionCommand2<faint::Color, faint::Color, faint::erase_but> EraseButColorCommand;
  return new EraseButColorCommand("Replace Colors", keep, eraserColor );
}

Command* get_fill_boundary_command( Object* obj, const faint::Color& fillColor ){
  return change_setting_command(obj, ts_FgCol, fillColor);
}

Command* get_fill_inside_command( Object* obj, const faint::Color& fillColor ){
  const Settings& s(obj->GetSettings());
  commands_t commands;
  if ( !filled(s) ){
    commands.push_back(change_setting_command(obj, ts_FillStyle, add_fill(get_fillstyle(s))));
  }
  commands.push_back( change_setting_command(obj, ts_BgCol, fillColor ) );
  return perhaps_bunch( CMD_TYPE_OBJECT, bunch_name("Set Object Fill Color"), commands );
}

Command* get_flatten_command( const objects_t& objects, const faint::Image& image ){
  assert(!objects.empty());
  std::deque<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* obj = objects[i];
    commands.push_back( new DrawObjectCommand( just_a_loan(obj) ) );
    commands.push_front( new DelObjectCommand( obj, image.GetObjectZ( obj ) ) );
  }
  return command_bunch( CMD_TYPE_HYBRID, get_bunch_name("Flatten", objects ), commands );
}

Command* get_flood_fill_command( const IntPoint& pos, const faint::Color& fillColor ){
  typedef FunctionCommand2<IntPoint, faint::Color, faint::flood_fill> FloodFillCommand;
  return new FloodFillCommand("Flood Fill", pos, fillColor);
}

Command* get_invert_command(){
  return new FunctionCommand<faint::invert>("Invert Colors");
}

Command* get_move_objects_command( const objects_t& objects, const NewTris& in_newTris, const OldTris& in_oldTris ){
  const std::vector<Tri>& oldTris(in_oldTris.Get());
  const std::vector<Tri>& newTris(in_newTris.Get());
  assert(!objects.empty());
  assert( oldTris.size() == newTris.size() && newTris.size() == objects.size() );

  commands_t commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    commands.push_back( new TriCommand(objects[i],
	New(newTris[i]),
	Old(oldTris[i]), "Move") );
  }
  return perhaps_bunch(CMD_TYPE_OBJECT, get_bunch_name("Move", objects), commands);
}

Command* perhaps_bunched_reorder( const std::vector<Command*>& commands, const objects_t& objects, const NewZ& newZ, const OldZ& oldZ ){
  if ( commands.empty() ){
    // No object could be moved
    return 0;
  }
  else if ( commands.size() == 1 ){
    return commands.back();
  }
  const std::string objectsName(get_collective_name(objects));
  const std::string dirStr = forward_or_back_str(newZ, oldZ);
  const bunch_name commandName(space_sep(objectsName,dirStr));
  return command_bunch( CMD_TYPE_OBJECT, commandName, commands);
}

Command* get_objects_backward_command( const objects_t& objects, const faint::Image& image ){
  assert(!objects.empty());
  size_t zLimit = 0;
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* object = objects[i];
    size_t Z = image.GetObjectZ( object );

    if ( Z == 0 || Z - 1 < zLimit ){
      // Prevent the current object from moving below Z-depth 0, and
      // from moving below an object that was previously prevented to
      // move below Z=0 (etc.), so that the relative order of all
      // selected objects is retained.
      zLimit = Z + 1;
    }
    else {
      zLimit = Z - 1;
      commands.push_back( new OrderObjectCommand( object, New(Z - 1), Old(Z) ) );
    }
  }
  return perhaps_bunched_reorder(commands, objects, New(0u), Old(1u));
}

Command* get_objects_forward_command( const objects_t& objects, const faint::Image& image ){
  assert(!objects.empty());
  std::vector<Command*> commands;
  unsigned int zLimit = image.GetObjects().size();
  for ( int i = objects.size() - 1; i >= 0; i-- ){
    Object* object = objects[i];
    size_t Z = image.GetObjectZ( object );

    // Prevent objects from being moved on top of other selected objects that
    // could not be moved further up because of reaching top.
    if ( Z + 1 >= zLimit ){
      zLimit = Z;
    }
    else {
      zLimit = Z + 1;
      commands.push_back( new OrderObjectCommand( object, New(Z + 1), Old(Z) ) );
    }
  }
  return perhaps_bunched_reorder(commands, objects, New(1u), Old(0u));
}

Command* get_objects_to_back_command( const objects_t& objects, const faint::Image& image ){
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* object = objects[i];
    size_t currentPos = image.GetObjectZ( object );
    if ( i != currentPos ){
      commands.push_back( new OrderObjectCommand( object, New(i), Old(currentPos) ) );
    }
  }
  return perhaps_bunched_reorder(commands, objects, New(0u), Old(1u));
}

Command* get_objects_to_front_command( const objects_t& objects, const faint::Image& image ){
  assert( !objects.empty() );
  const size_t maxPos = get_highest_z(image);
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* object = objects[i];
    size_t currentPos = image.GetObjectZ(object);
    if ( maxPos != currentPos ){
      commands.push_back( new OrderObjectCommand( object, New(maxPos), Old(currentPos) ) );
    }
  }
  return perhaps_bunched_reorder(commands, objects, New(1u), Old(0u));
}

Command* get_replace_color_command(const OldColor& oldColor, const NewColor& newColor){
  typedef FunctionCommand2<OldColor, NewColor, faint::replace_color > ReplaceColorCommand;
  return new ReplaceColorCommand("Replace Color", oldColor, newColor);
}

Command* get_rescale_command( const IntSize& size, ScaleQuality::type quality ){
  return new RescaleCommand(size, quality);
}

Optional<faint::Color> get_resize_dwim_color( const faint::Bitmap& bmp, const IntRect& r ){
  std::vector<faint::Color> colors;
  faint::Color temp;
  const int bmpW(static_cast<int>(bmp.m_w));
  const int bmpH(static_cast<int>(bmp.m_h));
  if ( r.w - r.x > bmpW && get_right_edge_color(bmp, temp) ){
    colors.push_back(temp);
  }
  if ( r.x < 0 && get_left_edge_color(bmp, temp) ){
    colors.push_back(temp);
  }
  if ( r.h - r.y > bmpH && get_bottom_edge_color(bmp, temp) ){
    colors.push_back(temp);
  }
  if ( r.y < 0 && get_top_edge_color(bmp, temp ) ){
    colors.push_back(temp);
  }
  if ( colors.empty() ){
    return Optional<faint::Color>();
  }
  faint::Color result = colors.front();
  for ( size_t i = 1; i != colors.size(); i++ ){
    if ( colors[i] != result ){
      return Optional<faint::Color>();
    }
  }
  return Optional<faint::Color>(result);
}

Command* get_resize_command( const faint::Bitmap& bmp, const IntRect& rect, const faint::Color& bgCol ){
  Optional<faint::Color> altCol = get_resize_dwim_color(bmp, rect);
  if ( altCol.IsSet() ){
    return new ResizeCommand( rect, bgCol, Alternate(altCol.Get()) );
  }
  return new ResizeCommand( rect, bgCol );
}

Command* get_rotate_command( const objects_t& objects, const faint::radian angle, const Point& origin ){
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* object = objects[i];
    const Tri& tri = object->GetTri();
    commands.push_back( new TriCommand( object, New(rotated( tri, angle, origin )), Old(tri), "Rotate" ) );
  }
  return perhaps_bunch( CMD_TYPE_OBJECT, get_bunch_name("Rotate", objects), commands );
}

Command* get_rotate_selection_command( const faint::Image& image ){
  const RasterSelection& selection(image.GetRasterSelection());
  assert(!selection.Empty());

  if ( selection.Floating() ){
    faint::Bitmap bmp(faint::rotate_90cw(selection.GetBitmap()));

    RasterSelectionState newState( selection.Copying() ?
      RasterSelectionState(bmp, selection.TopLeft() ) :
      RasterSelectionState(bmp, selection.TopLeft(), selection.GetOldRect() ) );
    return new SetRasterSelection( New(newState), Old(selection.GetState()), "Rotate Selection Clockwise" );
  }

  RasterSelectionState newState(faint::rotate_90cw(cairo_compatible_sub_bitmap(image.GetBitmap(), selection.GetRect())),
    selection.TopLeft(), selection.GetRect());
  return new SetRasterSelection(New(newState), Old(selection.GetState()), "Rotate Selection Clockwise");
}

Command* get_scale_command( const objects_t& objects, const Scale& scale, const Point& origin ){
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* obj = objects[i];
    const Tri& tri = obj->GetTri();
    commands.push_back( new TriCommand( obj,
        New(scaled(tri, scale, origin)), Old(tri), "Scale" ) );
  }
  return perhaps_bunch( CMD_TYPE_OBJECT, get_bunch_name("Scale", objects), commands );
}

Command* get_scale_raster_selection_command( const faint::Image& image, const IntSize& newSize ){
  const RasterSelection& selection(image.GetRasterSelection());
  assert(!selection.Empty());
  Scale scale(New(floated(newSize)), floated(selection.GetSize()));
  if ( selection.Floating() ){
    // Fixme: Mem use
    // Fixme: Copying-state
    faint::Bitmap bmp(faint::scale_bilinear(selection.GetBitmap(), scale));

    RasterSelectionState newState( selection.Copying() ?
      RasterSelectionState( bmp, selection.TopLeft() ) :
      RasterSelectionState( bmp, selection.TopLeft(), selection.GetOldRect() ) );
    return new SetRasterSelection( New(newState), Old(selection.GetState()), "Scale Selection" );
  }
  // Fixme: Initialize mask settings and what not
  RasterSelectionState newState(faint::scale_bilinear(cairo_compatible_sub_bitmap(image.GetBitmap(), selection.GetRect()), scale), selection.TopLeft(), selection.GetRect());
  return new SetRasterSelection(New(newState), Old(selection.GetState()), "Scale Selection" );
}

bool has_command( const std::deque<old_command_t>& list, const Command* cmd ){
  for ( size_t i = 0; i != list.size(); i++ ){
    if ( list[i].first == cmd ){
      return true;
    }
  }
  return false;
}

OperationFlip::OperationFlip(Axis::type axis)
  : m_axis(axis)
{}

Command* OperationFlip::DoImage() const{
  return flip_image_command(m_axis);
}

Command* OperationFlip::DoObjects(const objects_t& objects) const{
  faint::coord xScale = m_axis == Axis::HORIZONTAL ? -LITCRD(1.0) : LITCRD(1.0);
  Scale scale(xScale, -xScale);
  Point origin = bounding_rect(objects).Center();
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* o = objects[i];
    const Tri& tri = o->GetTri();
    commands.push_back( new TriCommand( o, New(scaled( tri, scale, origin)), Old(tri), "Flip" ) );
  }
  return perhaps_bunch( CMD_TYPE_OBJECT, get_bunch_name( "Flip", objects), commands );
}

Command* OperationFlip::DoRasterSelection( const faint::Image& image ) const{
  const RasterSelection& selection(image.GetRasterSelection());
  assert(!selection.Empty());
  if ( selection.Floating() ){
    // Fixme: Mem use, reversible - consider saving the process instead of saving bitmap.
    faint::Bitmap bmp(faint::flip(selection.GetBitmap(), m_axis));
    // Fixme: Add a way to create a floating state with replaced bitmap from an existing floating state/selection
    RasterSelectionState newState( selection.Copying() ?
      RasterSelectionState(bmp, selection.TopLeft()) :
      RasterSelectionState(bmp, selection.TopLeft(), selection.GetOldRect()) );

    return new SetRasterSelection( New(newState), Old(selection.GetState()),
      space_sep("Flip Selection", str_axis_adverb(m_axis) ) );
  }

  RasterSelectionState newState(faint::flip(cairo_compatible_sub_bitmap(image.GetBitmap(), selection.GetRect()), m_axis),
    selection.TopLeft(), selection.GetRect());
  return new SetRasterSelection(New(newState), Old(selection.GetState()),
    space_sep("Flip Selection", str_axis_adverb(m_axis)));
}

CommandType get_command_type(const std::vector<Command*>& cmds){
  assert(!cmds.empty());
  CommandType type = cmds.front()->Type();
  for ( size_t i = 1; i != cmds.size(); i++ ){
    if ( cmds[i]->Type() != type ){
      return CMD_TYPE_HYBRID;
    }
  }
  return type;
}

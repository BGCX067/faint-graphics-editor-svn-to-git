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
#include "commands/addobjectcommand.hh"
#include "commands/cmdchangesetting.hh"
#include "commands/cmdsetbitmap.hh"
#include "commands/cmdfliprot.hh"
#include "commands/commandbunch.hh"
#include "commands/delobjectcommand.hh"
#include "commands/drawobjectcommand.hh"
#include "commands/functioncommand.hh"
#include "commands/orderobject.hh"
#include "commands/rescalecommand.hh"
#include "commands/resizecommand.hh"
#include "commands/set-raster-selection.hh"
#include "commands/tricommand.hh"
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

bunch_name get_bunch_name( const std::string& command, const objects_t& objects ){
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

Command* get_add_object_command( Object* object, const select_added& select, const std::string& name ){
  return new AddObjectCommand( object, select, name );
}

Command* get_add_objects_command( const objects_t& objects, const select_added& select, const std::string& name ){
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    commands.push_back( new AddObjectCommand(objects[i], select, name ) );
  }
  return perhaps_bunch( CMD_TYPE_OBJECT, commands, get_bunch_name(name, objects) );
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

Command* get_change_raster_background_command( ObjRaster* obj, const faint::Color& color ){
  commands_t commands;
  commands.push_back( change_setting_command( obj, ts_BgCol, color ) );
  commands.push_back( change_setting_command( obj, ts_BackgroundStyle, BackgroundStyle::MASKED ) );
  return new CommandBunch( CMD_TYPE_OBJECT, commands, bunch_name("Set Raster Object Background") );
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
  return perhaps_bunch( CMD_TYPE_OBJECT, commands, get_bunch_name("Crop", objects) );
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
  return perhaps_bunch( CMD_TYPE_OBJECT, commands, get_bunch_name(name, objects) );
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
      return new SetRasterSelection(RasterSelectionState(), selection.GetState(), "Delete Floating Selection");
    }
    std::vector<Command*> commands;
    commands.push_back( new DrawObjectCommand(its_yours(new ObjRectangle(tri_from_rect(floated(selection.GetOldRect())), eraser_rectangle_settings(selection.GetBgCol())))) );
    commands.push_back( new SetRasterSelection(RasterSelectionState(), selection.GetState(), "Delete selected region") );
    return new CommandBunch(CMD_TYPE_HYBRID, commands, bunch_name("Delete Selected Region"));
  }
  else {
    // Raster delete
    return deleteState.GetDeleteCommand(image, selection.GetRect(), bgCol);
  }
}

Command* get_deselect_raster_command( RasterSelection& selection ){
  if ( selection.Floating() ){
    std::vector<Command*> commands;
    RasterSelectionState oldState(selection.GetState());
    commands.push_back( selection.StampFloatingSelection() );
    commands.push_back(new SetRasterSelection(RasterSelectionState(), selection.GetState(), ""));
    return new CommandBunch(CMD_TYPE_HYBRID, commands, bunch_name("Deselect Raster (floating)"));
  }
  return new SetRasterSelection(RasterSelectionState(), selection.GetState(), "Deselect Raster" );
}

Command* get_paste_raster_bitmap_command( const faint::Bitmap& bmp, const IntPoint& topLeft, const RasterSelection& oldSelection, const Settings& s ){
  return new SetRasterSelection(RasterSelectionState(bmp, topLeft, RasterSelectionOptions(masked_background(s), s.Get(ts_BgCol), s.Get(ts_AlphaBlending))),
    oldSelection.GetState(), "Paste Bitmap");
}

Command* get_selection_rectangle_command( const IntRect& r, const RasterSelectionOptions& options, const RasterSelectionState& oldState ){
  return new SetRasterSelection( RasterSelectionState(r, options), oldState, "Select Rectangle" );
}

Command* get_desaturate_simple_command(){
  return new FunctionCommand<faint::desaturate_simple>("Desaturate");
}

Command* get_desaturate_weighted_command(){
  return new FunctionCommand<faint::desaturate_weighted>("Desaturate Weighted");
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
  return perhaps_bunch( CMD_TYPE_OBJECT, commands );
}

Command* get_flatten_command( const objects_t& objects, const faint::Image& image ){
  assert(!objects.empty());
  std::deque<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* obj = objects[i];
    commands.push_back( new DrawObjectCommand( just_a_loan(obj) ) );
    commands.push_front( new DelObjectCommand( obj, image.GetObjectZ( obj ) ) );
  }
  return new CommandBunch( CMD_TYPE_HYBRID, std::vector<Command*>(commands.begin(), commands.end() ),
    get_bunch_name("Flatten", objects ) );
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
  return perhaps_bunch(CMD_TYPE_OBJECT, commands,
    bunch_name(space_sep("Move", get_collective_name(objects))));
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
  return new CommandBunch( CMD_TYPE_OBJECT, commands, commandName);
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
  return perhaps_bunch( CMD_TYPE_OBJECT, commands, get_bunch_name("Rotate", objects) );
}

Command* get_rotate_selection_command( const faint::Image& image ){
  const RasterSelection& selection(image.GetRasterSelection());
  assert(!selection.Empty());

  if ( selection.Floating() ){
    faint::Bitmap bmp(faint::rotate_90cw(selection.GetBitmap()));

    RasterSelectionState newState( selection.Copying() ?
      RasterSelectionState(bmp, selection.TopLeft(), selection.GetOptions()) :
      RasterSelectionState(bmp, selection.TopLeft(), selection.GetOldRect(), selection.GetOptions()) );
    return new SetRasterSelection( newState, selection.GetState(), "Rotate Selection Clockwise" );
  }

  RasterSelectionState newState(faint::rotate_90cw(cairo_compatible_sub_bitmap(image.GetBitmap(), selection.GetRect())),
    selection.TopLeft(), selection.GetRect(), selection.GetOptions());
  return new SetRasterSelection(newState, selection.GetState(), "Rotate Selection Clockwise");
}

Command* get_scale_command( const objects_t& objects, const Scale& scale, const Point& origin ){
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* obj = objects[i];
    const Tri& tri = obj->GetTri();
    commands.push_back( new TriCommand( obj,
        New(scaled(tri, scale, origin)), Old(tri), "Scale" ) );
  }
  return perhaps_bunch( CMD_TYPE_OBJECT, commands, get_bunch_name("Scale", objects) );
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
      RasterSelectionState( bmp, selection.TopLeft(), selection.GetOptions() ) :
      RasterSelectionState( bmp, selection.TopLeft(), selection.GetOldRect(), selection.GetOptions() ) );
    return new SetRasterSelection( newState, selection.GetState(), "Scale Selection" );
  }
  // Fixme: Initialize mask settings and what not
  RasterSelectionState newState(faint::scale_bilinear(cairo_compatible_sub_bitmap(image.GetBitmap(), selection.GetRect()), scale), selection.TopLeft(), selection.GetRect(), selection.GetOptions());
  return new SetRasterSelection(newState, selection.GetState(), "Scale Selection" );
}

bool compare_cmd( const old_command_t& other, const Command* cmd ){
  return other.first == cmd;
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
  return perhaps_bunch( CMD_TYPE_OBJECT, commands );
}

Command* OperationFlip::DoRasterSelection( const faint::Image& image ) const{
  const RasterSelection& selection(image.GetRasterSelection());
  assert(!selection.Empty());
  if ( selection.Floating() ){
    // Fixme: Mem use, reversible - consider saving the process instead of saving bitmap.
    faint::Bitmap bmp(faint::flip(selection.GetBitmap(), m_axis));
    // Fixme: Add a way to create a floating state with replaced bitmap from an existing floating state/selection
    RasterSelectionState newState( selection.Copying() ?
      RasterSelectionState(bmp, selection.TopLeft(), selection.GetOptions()) :
      RasterSelectionState(bmp, selection.TopLeft(), selection.GetOldRect(), selection.GetOptions()) );

    return new SetRasterSelection( newState, selection.GetState(),
      space_sep("Flip Selection", str_axis_adverb(m_axis) ) );
  }

  RasterSelectionState newState(faint::flip(cairo_compatible_sub_bitmap(image.GetBitmap(), selection.GetRect()), m_axis),
    selection.TopLeft(), selection.GetRect(), selection.GetOptions());
  return new SetRasterSelection(newState, selection.GetState(),
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

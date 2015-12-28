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
#include "bitmap/color-counting.hh"
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
#include "tools/tool.hh"
#include "util/autocrop.hh"
#include "util/commandutil.hh"
#include "util/formatting.hh"
#include "util/image.hh"
#include "util/imageutil.hh"
#include "util/iter.hh"
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
  EnableDwimDeleteCommand( DeleteState& state, const faint::DrawSource& alternateEraser, Command* cmd )
    : Command( CommandType::RASTER ),
      m_alternateEraser(alternateEraser),
      m_cmd(cmd),
      m_firstRun(true),
      m_state(state)
  {}
  ~EnableDwimDeleteCommand(){
    delete m_cmd;
  }
  void Do( CommandContext& context ) override{
    if ( then_false(m_firstRun) ){
      m_state.Set( m_alternateEraser );
    }
    m_cmd->Do(context);
  }
  std::string Name() const override{
    return m_cmd->Name();
  }

private:
  EnableDwimDeleteCommand& operator=(const EnableDwimDeleteCommand&);
  EnableDwimDeleteCommand(const EnableDwimDeleteCommand&);
  faint::DrawSource m_alternateEraser;
  Command* m_cmd;
  bool m_firstRun;
  DeleteState& m_state;
};

DeleteState::DeleteState()
  : m_lastWasDelete(false),
    m_alternateEraser(faint::color_white()),
    m_lastEraser(m_alternateEraser)
{}

void DeleteState::Clear(){
  m_lastWasDelete = false;
}

Command* DeleteState::GetDeleteCommand(const faint::Image& image, const IntRect& selection, const faint::DrawSource& eraser){
  if ( m_lastWasDelete && m_lastEraser == eraser ){
    // Alternate delete-scheme for the second-delete: Use the
    // determined surounding color for the last delete instead of
    // the selected secondary color.
    return get_delete_raster_command(selection, m_alternateEraser);
  }

  // When not following an identical delete, the command should use the selected
  // secondary color
  Command* deleteCommand = get_delete_raster_command(selection, eraser);

  // Try to determine a color to use for an upcoming delete of the same region
  Optional<faint::Color> edgeColor( get_edge_color(image.GetBitmap(), selection) );
  if ( edgeColor.NotSet() ){
    // Could not determine an edge color, so DWIM-delete is not
    // possible
    return deleteCommand;
  }

  if ( faint::is_blank(sub_bitmap(image.GetBitmap(), selection))){
    // The entire deleted region is blank, so deleting with the edge
    // color would have no effect, so DWIM would be useless.
    return deleteCommand;
  }

  // An edge color could be determined, first do a regular delete. If another delete follows
  // it should use the edge color, unless the background color has changed.
  m_lastEraser = eraser;

  // Return a command which will call Set for this object when run.
  return new EnableDwimDeleteCommand(*this, faint::DrawSource(edgeColor.Get()), deleteCommand);
}

void DeleteState::Set( const faint::DrawSource& alternate ){
  m_alternateEraser = alternate;
  m_lastWasDelete = true;
}

static bunch_name get_bunch_name( const std::string& command, const objects_t& objects ){
  return bunch_name(space_sep(command, get_collective_name(objects)));
}

Command* add_or_draw(Object* obj, Layer layer){
  if ( layer == Layer::RASTER ) {
    return new DrawObjectCommand(its_yours(obj));
  }
  else {
    return new AddObjectCommand(obj, select_added(false));
  }
}

bool affects_raster( const Command* cmd ){
  // Fixme: Sort of duplicates functions in command.hh (e.g. has_raster_steps).
  CommandType type( cmd->Type() );
  return ( type == CommandType::RASTER || type == CommandType::HYBRID ) && cmd->ModifiesState();
}

Command* crop_one_object( Object* obj ){
  ObjRaster* raster = dynamic_cast<ObjRaster*>( obj );
  if ( raster != nullptr ){
    return crop_raster_object_command( raster );
  }
  ObjText* text = dynamic_cast<ObjText*>( obj );
  if ( text != nullptr ){
    return crop_text_region_command( text );
  }
  return nullptr;
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
  return perhaps_bunch( CommandType::OBJECT, get_bunch_name(name, objects), commands );
}

Command* get_auto_crop_command( const faint::Image& image ){
  const bool hasObjects = !(image.GetObjects().empty());
  std::vector<IntRect> rects = hasObjects ?
    get_auto_crop_rectangles(faint::flatten(image)) :
    get_auto_crop_rectangles(image.GetBitmap());

  if ( rects.empty() ){
    return nullptr;
  }
  else if ( rects.size() == 1 ){
    return new ResizeCommand(rects[0], faint::DrawSource(faint::color_white()), "Auto Crop Image");
  }
  else if ( rects.size() == 2 ){
    IntRect r1(rects[0]);
    IntRect r2(rects[1]);
    if ( area(r2) < area(r1) ){
      // Default to keeping the smaller region, cutting away the
      // larger region
      std::swap(r1, r2);
    }
    return new ResizeCommand(r1, Alternate(r2), faint::DrawSource(faint::color_white()), "Auto Crop Image");
  }

  assert( false );
  return nullptr;
}

Command* get_blend_alpha_command( const faint::Color& bgColor ){
  typedef FunctionCommand1<faint::Color, faint::blend_alpha > BlendAlphaCommand;
  return new BlendAlphaCommand("Replace Alpha", bgColor);
}

Command* get_blur_command(){
  return new FunctionCommand<faint::blur>("Blur");
}

Command* get_change_raster_background_command( ObjRaster* obj, const faint::Color& color ){
  return command_bunch(CommandType::OBJECT, bunch_name("Set Raster Object Background"),
    change_setting_command( obj, ts_BgCol, faint::DrawSource(color) ),
    change_setting_command( obj, ts_BackgroundStyle, BackgroundStyle::MASKED ) );
}

CommandType get_collective_command_type( const commands_t& cmds ){
  assert(!cmds.empty());
  CommandType type = cmds.front()->Type();
  for ( const Command* cmd : but_first(cmds) ){
    if ( cmd->Type() != type ){
      return CommandType::HYBRID;
    }
  }
  return type;
}

Command* get_crop_command( const objects_t& objects ){
  std::vector<Command*> commands;
  for ( Object* obj : objects ){
    Command* cmd = crop_one_object( obj );
    if ( cmd != nullptr ){
      commands.push_back( cmd );
    }
  }

  if ( commands.empty() ){
    // No object supported autocrop
    return nullptr;
  }
  return perhaps_bunch( CommandType::OBJECT, get_bunch_name("Crop", objects), commands );
}

Command* get_crop_to_selection_command( const RasterSelection& selection ){
  assert( selection.Exists() );
  if ( selection.Floating() ){
    return new SetBitmapCommand( selection.GetBitmap(), selection.TopLeft(),
      "Crop to Selection" );
  }
  return new ResizeCommand( selection.GetRect(), faint::DrawSource(faint::color_white()),
    "Crop to Selection" );
}

Command* get_delete_objects_command( const objects_t& objects, const faint::Image& image, const std::string& name ){
  std::vector<Command*> commands;

  // Objects must be deleted in reverse order so that the Z-order is
  // preserved.
  for ( Object* obj : reversed(objects) ){
    commands.push_back( new DelObjectCommand( obj, image.GetObjectZ( obj ), name ) );
  }
  return perhaps_bunch( CommandType::OBJECT, get_bunch_name(name, objects), commands );
}

Command* get_delete_raster_command( const IntRect& r, const faint::DrawSource& src ){
  typedef FunctionCommand2<IntRect, faint::DrawSource, faint::fill_rect> DeleteRasterCommand;
  return new DeleteRasterCommand("Erase Selection", r, src);
}

Command* get_delete_raster_selection_command( const faint::Image& image, const faint::DrawSource& bg, DeleteState& deleteState ){
  const RasterSelection& selection(image.GetRasterSelection());
  assert(!selection.Empty());
  if ( selection.Floating() ){
    if ( selection.Copying() ){
      return new SetRasterSelection(New(RasterSelectionState()), Old(selection.GetState()),
	"Delete Floating Selection");
    }
    return command_bunch(CommandType::HYBRID, bunch_name("Delete Selected Region"),
      new DrawObjectCommand(its_yours(new ObjRectangle(tri_from_rect(floated(selection.GetOldRect())), eraser_rectangle_settings(selection.GetBackground())))),
      new SetRasterSelection(New(RasterSelectionState()), Old(selection.GetState()), "") );
  }
  else {
    // Raster delete
    return deleteState.GetDeleteCommand(image, selection.GetRect(), bg);
  }
}

Command* get_deselect_raster_command( const RasterSelection& selection ){
  if ( selection.Floating() ){
    // Stamp the floating selection before deselecting
    return command_bunch(CommandType::HYBRID, bunch_name("Deselect Raster (floating)"),
      selection.StampFloatingSelection(),
      new SetRasterSelection(New(RasterSelectionState()), Old(selection.GetState()), ""));
  }
  // Just deselect
  return new SetRasterSelection(New(RasterSelectionState()), Old(selection.GetState()), "Deselect Raster" );
}

Command* get_paste_raster_bitmap_command( const faint::Bitmap& bmp, const IntPoint& topLeft, const RasterSelection& oldSelection, const Settings& s ){
  return command_bunch(CommandType::HYBRID, bunch_name("Paste Bitmap"),
    new SetSelectionOptions( New(RasterSelectionOptions(masked_background(s), s.Get(ts_BgCol), s.Get(ts_AlphaBlending) )),
      Old(oldSelection.GetOptions()) ),
    new SetRasterSelection(New(RasterSelectionState(bmp, topLeft)), Old(oldSelection.GetState()), ""));
}

Command* get_pixelize_command( int width ){
  return new FunctionCommand1<int, unref_forwarder<int, faint::pixelize> >("Pixelize", width);
}

Command* maybe_stamp_old_selection( Command* newCommand, const RasterSelection& currentSelection ){
  if ( currentSelection.Floating() ){
    return command_bunch( CommandType::HYBRID,
      bunch_name(newCommand->Name()),
      currentSelection.StampFloatingSelection(),
      newCommand );
  }
  return newCommand;
}

Command* get_selection_rectangle_command( const IntRect& r, const RasterSelection& currentSelection ){
  Command* selectRectangle = new SetRasterSelection( New(RasterSelectionState(r)),
    Old(currentSelection.GetState()),
    "Select Rectangle" );
  return maybe_stamp_old_selection( selectRectangle, currentSelection );
}

Command* get_select_all_command( const faint::Image& image, const RasterSelection& currentSelection ){
  Command* selectAll = new SetRasterSelection( New(RasterSelectionState(image_rect(image))),
    Old(currentSelection.GetState()),
    "Select All" );
  return maybe_stamp_old_selection( selectAll, currentSelection );
}

Command* get_set_alpha_command( faint::uchar alpha ){
  return new FunctionCommand1<faint::uchar, unref_forwarder<faint::uchar, faint::set_alpha> >("Set Alpha", alpha);
}

Command* get_desaturate_simple_command(){
  return new FunctionCommand<faint::desaturate_simple>("Desaturate");
}

Command* get_desaturate_weighted_command(){
  return new FunctionCommand<faint::desaturate_weighted>("Desaturate Weighted");
}

Command* get_erase_but_color_command( const faint::Color& keep, const faint::DrawSource& eraser ){
  typedef FunctionCommand2<faint::Color, faint::DrawSource, faint::erase_but> EraseButColorCommand;
  return new EraseButColorCommand("Replace Colors", keep, eraser );
}

Command* get_fill_boundary_command( Object* obj, const faint::DrawSource& src ){
  return change_setting_command(obj, ts_FgCol, src);
}

Command* get_fill_inside_command( Object* obj, const faint::DrawSource& src ){
  const Settings& s(obj->GetSettings());
  commands_t commands;
  FillStyle fillStyle = get_fillstyle(s);
  if ( !filled(fillStyle) ){
    // Change the object to filled
    fillStyle = with_fill(fillStyle);
    commands.push_back(change_setting_command(obj, ts_FillStyle, fillStyle));
  }

  commands.push_back( change_setting_command(obj,
      setting_used_for_fill(fillStyle), src ) );

  return perhaps_bunch( CommandType::OBJECT,
    bunch_name("Set Object Fill Color"), commands );
}

Command* get_flatten_command( const objects_t& objects, const faint::Image& image ){
  assert(!objects.empty());
  std::deque<Command*> commands;
  for ( Object* obj : objects ){
    commands.push_back( new DrawObjectCommand( just_a_loan(obj) ) );
    commands.push_front( new DelObjectCommand( obj, image.GetObjectZ( obj ) ) );
  }
  return command_bunch( CommandType::HYBRID, get_bunch_name("Flatten", objects ), commands );
}

Command* get_flood_fill_command( const IntPoint& pos, const faint::DrawSource& fill ){
  typedef FunctionCommand2<IntPoint, faint::DrawSource, faint::flood_fill2> FloodFillCommand;
  return new FloodFillCommand("Flood Fill", pos, fill);
}

Command* get_brightness_and_contrast_command(const faint::brightness_contrast_t& values){
  return new FunctionCommand1< faint::brightness_contrast_t, faint::apply_brightness_and_contrast>(
    "Brightness and Contrast", values);
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
  return perhaps_bunch(CommandType::OBJECT, get_bunch_name("Move", objects), commands);
}

class MergeIfSameObjects : public MergeCondition{
public:
  MergeIfSameObjects( const objects_t& objects )
    : m_objects(objects)
  {}

  bool Satisfied( MergeCondition* cond ) override{
    MergeIfSameObjects* candidate = dynamic_cast<MergeIfSameObjects*>(cond);
    if ( candidate == nullptr ){
      return false;
    }
    // Fixme: Verify object identity safety
    return m_objects == candidate->m_objects;
  }

  bool Append( Command* ) override{
    // MergeIfSameObjects is not used for appending
    return false;
  }

  bool AssumeName() const override{
    return false;
  }
private:
  MergeIfSameObjects& operator=(const MergeIfSameObjects&); // Prevent assignment
  const objects_t m_objects;
};

Command* get_offset_objects_command( const objects_t& objects, const Point& delta ){
  assert(!objects.empty());

  commands_t commands;
  for ( Object* obj : objects ){
    Tri tri = obj->GetTri();
    commands.push_back( new TriCommand( obj,
      New(translated(tri, delta.x, delta.y)),
      Old(tri),
      "Offset",
      MergeMode::SOCIABLE ) );
  }
  if ( commands.size() == 1 ){
    return commands.back();
  }

  return command_bunch(CommandType::OBJECT,
    get_bunch_name("Offset", objects), commands, new MergeIfSameObjects(objects));
}

Command* perhaps_bunched_reorder( const std::vector<Command*>& commands, const objects_t& objects, const NewZ& newZ, const OldZ& oldZ ){
  if ( commands.empty() ){
    // No object could be moved
    return nullptr;
  }
  else if ( commands.size() == 1 ){
    return commands.back();
  }
  const std::string objectsName(get_collective_name(objects));
  const std::string dirStr = forward_or_back_str(newZ, oldZ);
  const bunch_name commandName(space_sep(objectsName,dirStr));
  return command_bunch( CommandType::OBJECT, commandName, commands);
}

Command* get_objects_backward_command( const objects_t& objects, const faint::Image& image ){
  assert(!objects.empty());
  size_t zLimit = 0;
  std::vector<Command*> commands;
  for ( Object* object : objects ){
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
  for ( Object* object : reversed(objects) ){
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
  for ( Object* obj : objects ){
    size_t currentPos = image.GetObjectZ(obj);
    if ( maxPos != currentPos ){
      commands.push_back( new OrderObjectCommand( obj, New(maxPos), Old(currentPos) ) );
    }
  }
  return perhaps_bunched_reorder(commands, objects, New(1u), Old(0u));
}

Command* get_replace_color_command(const OldColor& oldColor, const faint::DrawSource& newColor){
  typedef FunctionCommand2<OldColor, faint::DrawSource, faint::replace_color > ReplaceColorCommand;
  return new ReplaceColorCommand("Replace Color", oldColor, newColor);
}

Command* get_rescale_command( const IntSize& size, ScaleQuality quality ){
  return new RescaleCommand(size, quality);
}

static faint::Bitmap copy_right_column( const faint::Bitmap& bmp ){
  return sub_bitmap( bmp, IntRect( IntPoint(int(bmp.m_w) - 1, 0),
      IntSize(1, int(bmp.m_h)) ) );
}

static faint::Bitmap copy_left_column( const faint::Bitmap& bmp ){
  return sub_bitmap( bmp, IntRect( IntPoint(0, 0),
      IntSize(1, bmp.m_h) ) );
}

static faint::Bitmap copy_top_row( const faint::Bitmap& bmp ){
  return sub_bitmap( bmp, IntRect( IntPoint(0, 0),
      IntSize(bmp.m_w, 1) ) );
}

static faint::Bitmap copy_bottom_row( const faint::Bitmap& bmp ){
  return sub_bitmap( bmp, IntRect( IntPoint(0, int(bmp.m_h) - 1),
      IntSize(int(bmp.m_w), 1) ) );
}

static Optional<faint::Color> get_resize_dwim_color( const faint::Bitmap& bmp, const IntRect& r ){
  faint::color_counts_t colors;
  if ( r.w - r.x > bmp.m_w ){
    add_color_counts( copy_right_column(bmp), colors );
  }
  if ( r.x < 0 ){
    add_color_counts( copy_left_column(bmp), colors );
  }
  if ( r.h - r.y > bmp.m_h ){
    add_color_counts( copy_bottom_row(bmp), colors );
  }
  if ( r.y < 0 ){
    add_color_counts( copy_top_row(bmp), colors );
  }
  if ( colors.empty() ){
    // No edge colors retrieved if shrinking or unchanged.
    return no_option();
  }
  return option(most_common(colors));
}

Command* get_resize_command( const faint::Bitmap& bmp, const IntRect& rect, const faint::DrawSource& src ){
  Optional<faint::Color> altCol = get_resize_dwim_color(bmp, rect);
  if ( altCol.IsSet() && altCol.Get() != src ){
    return new ResizeCommand( rect, src, Alternate(faint::DrawSource(altCol.Get())) );
  }
  return new ResizeCommand( rect, src );
}

Command* get_rotate_command( Object* obj, const faint::radian angle, const Point& origin ){
  const Tri& tri = obj->GetTri();
  return new TriCommand( obj, New(rotated( tri, angle, origin )), Old(tri), "Rotate" );
}

Command* get_rotate_command( const objects_t& objects, const faint::radian angle, const Point& origin ){
  std::vector<Command*> commands;
  for ( Object* obj : objects ){
    commands.push_back(get_rotate_command( obj, angle, origin ));
  }
  return perhaps_bunch( CommandType::OBJECT, get_bunch_name("Rotate", objects), commands );
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

  RasterSelectionState newState(faint::rotate_90cw(sub_bitmap(image.GetBitmap(), selection.GetRect())),
    selection.TopLeft(), selection.GetRect());
  return new SetRasterSelection(New(newState), Old(selection.GetState()), "Rotate Selection Clockwise");
}

class AppendIfMoveCommand : public MergeCondition{
public:
  bool Append(Command* cmd) override{
    return is_move_raster_selection_command(cmd);
  }
  bool AssumeName() const override{
    return false;
  }
  bool Satisfied(MergeCondition*) override{
    return false;
  }
};

Command* get_offset_raster_selection_command( const faint::Image& image, const IntPoint& delta ){
  const RasterSelection& selection(image.GetRasterSelection());
  assert(!selection.Empty());

  if ( selection.Floating() ){
    const IntPoint oldPos = selection.TopLeft();
    return move_raster_selection_command(oldPos + delta, oldPos);
  }
  else {
    // Begin floating, and offset
    RasterSelectionState newState(sub_bitmap(image.GetBitmap(), selection.GetRect()), selection.TopLeft() + delta, selection.GetRect());
    return command_bunch(CommandType::HYBRID,
      bunch_name("Offset Raster Selection"),
      new SetRasterSelection(New(newState), Old(selection.GetState()), ""), new AppendIfMoveCommand());
  }
}

Command* get_scale_command( const objects_t& objects, const Scale& scale, const Point& origin ){
  std::vector<Command*> commands;
  for ( Object* obj : objects ){
    const Tri& tri = obj->GetTri();
    commands.push_back( new TriCommand( obj,
        New(scaled(tri, scale, origin)), Old(tri), "Scale" ) );
  }
  return perhaps_bunch( CommandType::OBJECT, get_bunch_name("Scale", objects), commands );
}

Command* get_threshold_command( const faint::threshold_range_t& range ){
  return new FunctionCommand1<faint::threshold_range_t, faint::threshold>("Threshold", range);
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
  RasterSelectionState newState(faint::scale_bilinear(sub_bitmap(image.GetBitmap(), selection.GetRect()), scale), selection.TopLeft(), selection.GetRect());
  return new SetRasterSelection(New(newState), Old(selection.GetState()), "Scale Selection" );
}

bool has_command( const std::deque<OldCommand>& list, const Command* cmd ){
  for ( const OldCommand& item : list ){
    if ( item.command == cmd ){
      return true;
    }
  }
  return false;
}

OperationFlip::OperationFlip(Axis axis)
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
  for ( Object* obj : objects ){
    const Tri& tri = obj->GetTri();
    commands.push_back( new TriCommand( obj, New(scaled( tri, scale, origin)), Old(tri), "Flip" ) );
  }
  return perhaps_bunch( CommandType::OBJECT, get_bunch_name( "Flip", objects), commands );
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

  RasterSelectionState newState(faint::flip(sub_bitmap(image.GetBitmap(), selection.GetRect()), m_axis),
    selection.TopLeft(), selection.GetRect());
  return new SetRasterSelection(New(newState), Old(selection.GetState()),
    space_sep("Flip Selection", str_axis_adverb(m_axis)));
}

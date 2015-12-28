// -*- coding: us-ascii-unix -*-
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
#include "wx/dnd.h"
#include "wx/dcclient.h"
#include "app/app-context.hh"
#include "geo/geo-func.hh"
#include "geo/line.hh"
#include "geo/scale.hh"
#include "geo/size.hh"
#include "gui/canvas-panel.hh"
#include "gui/canvas-panel-contexts.hh"
#include "rendering/paint-canvas.hh"
#include "text/formatting.hh"
#include "tools/resize-canvas-tool.hh" // Fixme: Try to move to contexts
#include "util/convenience.hh"
#include "util/convert-wx.hh"
#include "util/gui-util.hh"
#include "util/hit-test.hh"
#include "util/image.hh"
#include "util/image-util.hh"
#include "util/iter.hh"
#include "util/mouse.hh"
#include "util/object-util.hh"
#include "util/resource-id.hh"

namespace faint{

const ColRGB g_canvasBg(160, 140, 160);
const int g_lineSize = 10;

const wxEventType FAINT_CANVAS_CHANGE = wxNewEventType();
const wxEventType FAINT_GRID_CHANGE = wxNewEventType();
const wxEventType FAINT_ZOOM_CHANGE = wxNewEventType();
const wxEventTypeTag<CanvasChangeEvent> EVT_FAINT_CANVAS_CHANGE(FAINT_CANVAS_CHANGE);
const wxEventTypeTag<CanvasChangeEvent> EVT_FAINT_GRID_CHANGE(FAINT_GRID_CHANGE);
const wxEventTypeTag<CanvasChangeEvent> EVT_FAINT_ZOOM_CHANGE(FAINT_ZOOM_CHANGE);

const long g_panel_style = static_cast<long>(wxVSCROLL|wxHSCROLL | wxWANTS_CHARS);

  CanvasPanel::CanvasPanel(wxWindow* parent, ImageList&& images, const initially_dirty& startDirty, wxFileDropTarget* fileDropTarget, AppContext& app, StatusInterface& statusInfo)
  : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, g_panel_style),
    m_contexts(app),
    m_images(std::move(images)),
    m_mouse(this, [=](){Preempt(PreemptOption::ALLOW_COMMAND);}),
    m_state(app.GetDefaultGrid()),
    m_statusInfo(statusInfo)
{
  Bind(wxEVT_CHAR, &CanvasPanel::OnChar, this);
  Bind(wxEVT_LEFT_DCLICK,
    [&](const wxMouseEvent& event){
      PosInfo info = HitTest(to_faint(event.GetPosition()),
        mouse_modifiers(event));
      HandleToolResult(m_contexts.GetTool().DoubleClick(info));
    });

  Bind(wxEVT_ERASE_BACKGROUND,
    [](const wxEraseEvent&){
      // This function intentionally left blank
    });

  Bind(wxEVT_IDLE, &CanvasPanel::OnIdle, this);
  Bind(wxEVT_KEY_DOWN, &CanvasPanel::OnKeyDown, this);
  Bind(wxEVT_KEY_UP, &CanvasPanel::OnKeyUp, this);
  Bind(wxEVT_LEFT_DOWN, &CanvasPanel::OnMouseDown, this);
  Bind(wxEVT_RIGHT_DOWN, &CanvasPanel::OnMouseDown, this);
  Bind(wxEVT_LEFT_UP, &CanvasPanel::OnMouseUp, this);
  Bind(wxEVT_RIGHT_UP, &CanvasPanel::OnMouseUp, this);
  Bind(wxEVT_MOTION,
    [&](const wxMouseEvent& evt){
      MousePosRefresh(to_faint(evt.GetPosition()), get_tool_modifiers());
    });


  Bind(wxEVT_LEAVE_WINDOW,
    [&](const wxMouseEvent&){
      // The cursor has left the canvas (the drawing area + the
      // border) onto a different control or outside the window, and
      // is not captured. Tools may need to be informed so they don't
      // draw for example a brush cursor.
      if (m_contexts.GetTool().RefreshOnMouseOut()){
        RefreshToolRect();
      }
      m_statusInfo.Clear();
    });

  Bind(wxEVT_MOUSEWHEEL, &CanvasPanel::OnMouseWheel, this);

  Bind(wxEVT_PAINT, [&](const wxPaintEvent&){
    auto selectionMirage = m_mirage.selection.lock();
    const RasterSelection& rasterSelection(selectionMirage == nullptr ?
      GetImageSelection() : *selectionMirage);

    wxPaintDC dc(this);
    paint_canvas(dc,
      m_images.Active(),
      m_state,
      to_faint(GetUpdateRegion().GetBox()),
      m_mirage.bitmap,
      g_canvasBg,
      mouse::image_position(m_state.geo, *this),
      rasterSelection,
      m_contexts.tool.GetTool(),
      m_contexts.app.GetTransparencyStyle(),
      m_contexts.app.GetLayerType());
  });

  Bind(wxEVT_SCROLLWIN_THUMBTRACK, &CanvasPanel::OnScrollDrag, this);
  Bind(wxEVT_SCROLLWIN_THUMBRELEASE, &CanvasPanel::OnScrollDragEnd, this);
  Bind(wxEVT_SCROLLWIN_LINEDOWN, &CanvasPanel::OnScrollLineDown, this);
  Bind(wxEVT_SCROLLWIN_LINEUP, &CanvasPanel::OnScrollLineUp, this);
  Bind(wxEVT_SCROLLWIN_PAGEDOWN, &CanvasPanel::OnScrollPageDown, this);
  Bind(wxEVT_SCROLLWIN_PAGEUP, &CanvasPanel::OnScrollPageUp, this);

  Bind(wxEVT_SIZE, [&](wxSizeEvent& event){
    AdjustScrollbars(m_state.geo.Pos());
    RefreshToolRect();
    event.Skip();
  });

  SetBackgroundColour(to_wx(g_canvasBg));
  SetBackgroundStyle(wxBG_STYLE_PAINT);

  m_contexts.canvas.reset(create_canvas_context(*this, m_contexts.app,
    m_images));
  m_contexts.command.reset(create_command_context(*this, m_images));

  SetDropTarget(fileDropTarget);

  if (startDirty.Get()){
    // Set a unique command id so that the image remains dirty until
    // the first save.
    m_document.savedAfter.Set(CommandId());
  }
}

bool CanvasPanel::AcceptsFocus() const{
  return true;
}

void CanvasPanel::AdjustHorizontalScrollbar(int pos){
  auto& geo = m_state.geo;
  geo.pos.x = pos;
  pos = std::max(0,pos);
  m_scroll.startX = geo.pos.x < 0 ? geo.pos.x : 0;
  SetScrollbar(wxHORIZONTAL, // Orientation
    pos, // Position
    GetHorizontalPageSize(), // Thumb size
    std::max(GetMaxScrollRight(), pos + GetHorizontalPageSize() - m_scroll.startX), // Range
    true); // Refresh
}

void CanvasPanel::AdjustScrollbars(const IntPoint& p){
  AdjustHorizontalScrollbar(p.x);
  AdjustVerticalScrollbar(p.y);
}

void CanvasPanel::AdjustVerticalScrollbar(int pos){
  auto& geo = m_state.geo;
  geo.pos.y = pos;
  pos = std::max(0,pos);
  m_scroll.startY = geo.pos.y < 0 ? geo.pos.y : 0;
  SetScrollbar(wxVERTICAL, // Orientation
    pos, // Position
    GetVerticalPageSize(), // Thumb size
    std::max(GetMaxScrollDown(), pos + GetVerticalPageSize() - m_scroll.startY) , // Range
    true); // Refresh
}

bool CanvasPanel::CanRedo() const{
  const Tool& tool = m_contexts.GetTool();
  return tool.CanRedo() ||
    (tool.AllowsGlobalRedo() && m_commands.CanRedo());
}

bool CanvasPanel::CanUndo() const{
  return m_commands.CanUndo() || m_contexts.GetTool().CanUndo();
}

void CanvasPanel::CenterView(const IntPoint& ptView){
  auto& geo = m_state.geo;
  IntPoint viewCenter = point_from_size(to_faint(GetClientSize())) / 2;
  IntPoint pos = ptView + geo.pos - viewCenter;
  if (pos == geo.pos){
    // Do nothing if the ptView refers to the currently centered image point, as
    // this would make it difficult to move the cursor while
    // continuously centering at large zoom.
    return;
  }
  geo.pos = pos;
  AdjustScrollbars(geo.pos);
  WarpPointer(viewCenter.x, viewCenter.y);
  MousePosRefresh();
  Refresh();
}

void CanvasPanel::CenterViewImage(const Point& ptImage){
  CenterView(mouse::image_to_view(floored(ptImage), m_state.geo));
}

void CanvasPanel::ChangeZoom(ZoomLevel::ChangeType changeType){
  auto& geo = m_state.geo;
  coord oldScale = geo.Scale();
  geo.zoom.Change(changeType);
  geo.pos = truncated(geo.pos / (oldScale / geo.Scale()));
  AdjustScrollbars(geo.pos);
  MousePosRefresh();
  SendZoomChangeEvent();
  Refresh();
}

void CanvasPanel::ClearPointOverlay(){
  m_state.pointOverlay.Clear();
}

void CanvasPanel::DeselectObject(Object* obj){
  DeselectObjects(as_list(obj));
}

void CanvasPanel::DeselectObjects(){
  m_images.Active().DeselectObjects();
  m_contexts.GetTool().SelectionChange();
  SendCanvasChangeEvent();
}

void CanvasPanel::DeselectObjects(const objects_t& objects){
  bool deselected = m_images.Active().Deselect(objects);
  if (!deselected){
    return;
  }
  m_contexts.GetTool().SelectionChange();
  SendCanvasChangeEvent();
}

PosInfo CanvasPanel::ExternalHitTest(const IntPoint& ptImage){
  IntPoint ptView = mouse::image_to_view(ptImage, m_state.geo);
  return HitTest(ptView, get_tool_modifiers());
}

IntPoint CanvasPanel::GetFaintScrollPos() const{
  return m_state.geo.Pos();
}

CanvasId CanvasPanel::GetCanvasId() const{
  return m_canvasId;
}

Optional<FilePath> CanvasPanel::GetFilePath() const{
  return m_document.filename;
}

Grid CanvasPanel::GetGrid() const{
  return m_state.grid;
}

const ImageList& CanvasPanel::GetImageList() const{
  return m_images;
}

Point CanvasPanel::GetImageViewStart() const{
  auto& geo = m_state.geo;
  const Point pos = (geo.pos - point_from_size(geo.border)) / geo.Scale();
  return max_coords(pos, Point::Both(0));
}

RasterSelection& CanvasPanel::GetImageSelection(){
  return m_images.Active().GetRasterSelection();
}

const RasterSelection& CanvasPanel::GetImageSelection() const{
  return m_images.Active().GetRasterSelection();
}

Canvas& CanvasPanel::GetInterface(){
  return *m_contexts.canvas;
}

int CanvasPanel::GetMaxScrollRight() const{
  auto scale = m_state.geo.Scale();
  auto w = m_images.Active().GetSize().w;
  return floored(std::max(0.0, w * scale));
}

int CanvasPanel::GetMaxScrollDown() const{
  auto scale =  m_state.geo.Scale();
  auto h = m_images.Active().GetSize().h;
  return floored(std::max(0.0, h * scale));
}

IntPoint CanvasPanel::GetMaxUsefulScroll() const{
  return IntPoint(GetMaxScrollRight(), GetMaxScrollDown()) -
    IntPoint(GetHorizontalPageSize(), GetVerticalPageSize());
}

MenuFlags CanvasPanel::GetMenuFlags() const{
  const Image& active(m_images.Active());
  bool shouldDrawRaster = should_draw_raster(m_contexts.tool,
    m_contexts.app.GetLayerType());
  bool shouldDrawVector = should_draw_vector(m_contexts.tool,
    m_contexts.app.GetLayerType());

  MenuFlags fl;
  fl.toolSelection = HasToolSelection();
  fl.rasterSelection = shouldDrawRaster && HasRasterSelection();
  fl.objectSelection = shouldDrawVector && HasObjectSelection();
  fl.numSelected = active.GetObjectSelection().size();
  fl.groupIsSelected = contains_group(active.GetObjectSelection());
  fl.hasObjects = active.GetNumObjects() != 0;
  fl.dirty = IsDirty();
  fl.canUndo = CanUndo();
  fl.canRedo = CanRedo();
  fl.undoLabel = GetUndoName();
  fl.redoLabel = GetRedoName();
  fl.canMoveForward = can_move_forward(active, active.GetObjectSelection());
  fl.canMoveBackward = can_move_backward(active, active.GetObjectSelection());
  return fl;
}

const objects_t& CanvasPanel::GetObjects(){
  return m_images.Active().GetObjects();
}

utf8_string CanvasPanel::GetRedoName() const{
  const Tool& tool = m_contexts.GetTool();
  if (tool.CanRedo()){
    return tool.GetRedoName();
  }
  else if (tool.PreventsGlobalRedo()){
    return "";
  }
  return m_commands.GetRedoName(m_images);
}

Point CanvasPanel::GetRelativeMousePos(){
  return mouse::image_position(m_state.geo, *this);
}

index_t CanvasPanel::GetSelectedFrame() const{
  return m_images.GetActiveIndex();
}

const objects_t& CanvasPanel::GetObjectSelection(){
  return m_images.Active().GetObjectSelection();
}

utf8_string CanvasPanel::GetUndoName() const{
  const Tool& tool = m_contexts.GetTool();
  if (tool.CanUndo()){
    return tool.GetUndoName();
  }
  return m_commands.GetUndoName(m_images);
}

coord CanvasPanel::GetZoom() const{
  return m_state.geo.zoom.GetScaleFactor();
}

const ZoomLevel& CanvasPanel::GetZoomLevel() const{
  return m_state.geo.zoom;
}

bool CanvasPanel::Has(const ObjectId& id){
  const objects_t& objects(m_images.Active().GetObjects());
  for (const Object* obj : objects){
    if (is_or_has(obj, id)){
      return true;
    }
  }
  return false;
}

bool CanvasPanel::HasObjectSelection() const{
  return !m_images.Active().GetObjectSelection().empty();
}

bool CanvasPanel::HasRasterSelection() const{
  return GetImageSelection().Exists();
}

bool CanvasPanel::HasSelection() const{
  return HasRasterSelection() || HasObjectSelection();
}

bool CanvasPanel::HasToolSelection() const{
  return m_contexts.GetTool().SupportsSelection();
}

bool CanvasPanel::InSelection(const Point& p){
  return GetImageSelection().Contains(floored(p));
}

bool CanvasPanel::IsDirty() const{
  const Optional<CommandId> lastModifying(m_commands.GetLastModifying());
  if (lastModifying.NotSet()){
    // No remaining undoable commands modified the image.
    // If the image was saved during this session, we've undone past
    // that point (and the image is dirty).
    // If the image has not been saved, we've undone back to the load
    // state, and the image is not dirty.
    return m_document.savedAfter.IsSet();
  }

  // There are undoable modifying commands, and the image hasn't been
  // saved during this session or was saved earlier or later.
  return m_document.savedAfter.NotSet() ||
    (lastModifying != m_document.savedAfter.Get());
}

void CanvasPanel::MousePosRefresh(){
  MousePosRefresh(mouse::view_position(*this), get_tool_modifiers());
}

static KeyPress char_event_to_keypress(const wxKeyEvent& event, bool& probablyCtrlEnter){
  int keyCode = event.GetKeyCode();
  int modifiers = event.GetModifiers();
  if ((modifiers & wxMOD_CONTROL) != 0){
    // Untranslate Ctrl+Letter keycode
    if (keyCode <= 26){
      if (keyCode == 10){
        keyCode = probablyCtrlEnter ? key::enter : key::J;
      }
      else{
        keyCode = 'A' + keyCode - 1;
      }
    }
  }

  // The workaround (see OnKeyDown) should only survive a single
  // OnChar
  probablyCtrlEnter = false;

  return KeyPress(Mod::Create(modifiers), Key(keyCode));
}

void CanvasPanel::OnChar(wxKeyEvent& event){
  KeyInfo info(GetInterface(),
    m_statusInfo,
    m_contexts.app.GetLayerType(),
    to_faint(event.GetUnicodeKey()),
    char_event_to_keypress(event, m_probablyCtrlEnter));

  if (!info.key.Alt()){
    if (HandleToolResult(m_contexts.GetTool().Char(info))){
      // The tool ate the character.
      return;
    }
  }
  // Key ignored by tool, or Alt held. Allow normal key handling.
  event.Skip();
}

void CanvasPanel::OnKeyDown(wxKeyEvent& event){
  int keycode = event.GetKeyCode();
  if (is_tool_modifier(keycode)){
    // Update the tool state if a modifier is pressed (e.g. for cursor update)
    // Note: This repeats while the key is held.
    MousePosRefresh();
    return;
  }

  // Ugly workaround: It is not possible to discern between
  // Ctrl+Enter and Ctrl+J in the OnChar-handler.
  m_probablyCtrlEnter = event.GetKeyCode() == WXK_RETURN &&
    (event.GetModifiers() & wxMOD_CONTROL);
  event.Skip();
}

void CanvasPanel::OnKeyUp(wxKeyEvent& event){
  int keycode = event.GetKeyCode();
  if (is_tool_modifier(keycode)){
    // Update the tool state if a modifier is released (e.g. for cursor update)
    // Note: This repeates while the key is held.
    MousePosRefresh();
    return;
  }
  event.Skip();
}

void CanvasPanel::OnMouseDown(wxMouseEvent& evt){
  SetFocus();
  IntPoint viewPos(to_faint(evt.GetPosition()));
  PosInfo info(HitTest(viewPos, mouse_modifiers(evt)));
  m_mouse.Capture();

  if (!IgnoreCanvasHandle(info)){
    // Check if a canvas handle was clicked, and if so switch to the
    // canvas resize tool
    IntSize imageSize(m_images.Active().GetSize());


    if (auto handle = canvas_handle_hit_test(viewPos, imageSize, m_state.geo)){
      Settings toolSettings(m_contexts.app.GetToolSettings());
      Tool* resizeTool = evt.GetButton() == wxMOUSE_BTN_LEFT ?
        resize_canvas_tool(handle.Get(), toolSettings) :
        scale_canvas_tool(handle.Get(), toolSettings);
      m_contexts.tool.SetSwitched(resizeTool);
      SetFaintCursor(handle.Get().GetCursor());
      return;
    }
  }
  HandleToolResult(m_contexts.GetTool().MouseDown(info));
  SetFaintCursor(m_contexts.GetTool().GetCursor(info));

  // Notify app to allow in-tool undo (e.g. Polygon tool point adding)
  SendCanvasChangeEvent();
}

void CanvasPanel::OnMouseUp(wxMouseEvent& evt){
  m_mouse.Release();
  SetFocus();
  PosInfo info = HitTest(to_faint(evt.GetPosition()), mouse_modifiers(evt));
  HandleToolResult(m_contexts.GetTool().MouseUp(info));
}

void CanvasPanel::OnMouseWheel(wxMouseEvent& evt){
  int rot = evt.GetWheelRotation();
  if (rot > 0){
    if (evt.ControlDown()){
      ScrollLineLeft();
    }
    else {
      ScrollLineUp();
    }
  }
  else if (rot < 0){
    if (evt.ControlDown()){
      ScrollLineRight();
    }
    else {
      ScrollLineDown();
    }
  }
}

void CanvasPanel::OnScrollDrag(wxScrollWinEvent& event){
  // This is a recurring event while a scrollbar-thumb is dragged
  auto& geo = m_state.geo;
  event.Skip();
  int orientation = event.GetOrientation();
  int pos = event.GetPosition();
  if (orientation == wxHORIZONTAL){
    geo.pos.x = pos + m_scroll.startX;
  }
  else if (orientation == wxVERTICAL){
    geo.pos.y = pos + m_scroll.startY;
  }
  Refresh();
}

void CanvasPanel::OnScrollDragEnd(wxScrollWinEvent& event){
  auto& geo = m_state.geo;
  event.Skip();
  if (event.GetOrientation() == wxHORIZONTAL){
    AdjustHorizontalScrollbar(geo.pos.x);
  }
  else {
    AdjustVerticalScrollbar(geo.pos.y);
  }
}

void CanvasPanel::OnScrollLineDown(wxScrollWinEvent& event){
  event.Skip();
  const int orientation = event.GetOrientation();
  if (orientation == wxHORIZONTAL){
    ScrollLineRight();
  }
  else if (orientation == wxVERTICAL){
    ScrollLineDown();
  }
  Refresh();
}

void CanvasPanel::OnScrollLineUp(wxScrollWinEvent& event){
  event.Skip();
  const int orientation = event.GetOrientation();
  if (orientation == wxHORIZONTAL){
    ScrollLineLeft();
  }
  else if (orientation == wxVERTICAL){
    ScrollLineUp();
  }
  Refresh();
}

void CanvasPanel::OnScrollPageDown(wxScrollWinEvent& event){
  event.Skip();
  if (event.GetOrientation() == wxVERTICAL){
    ScrollPageDown();
  }
  else if (event.GetOrientation() == wxHORIZONTAL){
    ScrollPageRight();
  }
}

void CanvasPanel::OnScrollPageUp(wxScrollWinEvent& event){
  event.Skip();
  if (event.GetOrientation() == wxVERTICAL){
    ScrollPageUp();
  }
  else if (event.GetOrientation() == wxHORIZONTAL){
    ScrollPageLeft();
  }
}

void CanvasPanel::OnIdle(wxIdleEvent&) {
  // Update the scrollbars in the idle handler to prevent various
  // crashes and mouse capture problems
  // (e.g. issue 78)

  if (neither(m_scroll.updateHorizontal, m_scroll.updateVertical)){
    return;
  }
  if (then_false(m_scroll.updateHorizontal)){
    AdjustHorizontalScrollbar(m_state.geo.pos.x);
  }
  if (then_false(m_scroll.updateVertical)){
    AdjustVerticalScrollbar(m_state.geo.pos.y);
  }
  Refresh();
}

PreemptResult CanvasPanel::Preempt(PreemptOption option){
  Tool& tool = m_contexts.GetTool();

  PosInfo info(HitTest(mouse::view_position(*this),
    get_tool_modifiers()));
  ToolResult r = tool.Preempt(info);
  if (r == TOOL_COMMIT){
    if (option == PreemptOption::ALLOW_COMMAND){
      CommitTool(tool, REFRESH);
    }
    return PreemptResult::COMMIT;
  }
  else if (r == TOOL_CANCEL){
    Refresh();
    return PreemptResult::CANCEL;
  }
  else if (r == TOOL_CHANGE){
    if (option == PreemptOption::ALLOW_COMMAND){
      CommitTool(tool, REFRESH);
    }
    m_contexts.tool.ClearSwitched();
    return PreemptResult::COMMIT;
  }
  else {
    Refresh();
    return PreemptResult::NONE;
  }
}

void CanvasPanel::PreviousFrame(){
  const index_t maxIndex = m_images.GetNumImages();
  const index_t oldIndex = m_images.GetActiveIndex();
  const index_t index(oldIndex.Get() == 0 ? maxIndex - 1 : oldIndex - 1);
  SelectFrame(index);
}

void CanvasPanel::NextFrame(){
  const index_t maxIndex = m_images.GetNumImages();
  const index_t oldIndex = m_images.GetActiveIndex();
  const index_t index((oldIndex.Get() + 1) % maxIndex.Get());
  SelectFrame(index);
}

void CanvasPanel::NotifySaved(const FilePath& path){
  m_document.filename.Set(path);
  m_document.savedAfter = m_commands.GetLastModifying();
  SendCanvasChangeEvent();
}

void CanvasPanel::Redo(){
  Tool& tool = m_contexts.GetTool();
  if (tool.CanRedo()){
    tool.Redo();
    MousePosRefresh();

    // For undo/redo state
    SendCanvasChangeEvent();
    return;
  }

  if (!m_commands.CanRedo() || tool.PreventsGlobalRedo()){
    return;
  }

  // Do not allow the preempted tool to commit, as this would
  // mess up undo/redo.
  Preempt(PreemptOption::DISCARD_COMMAND);

  m_commands.Redo(*m_contexts.command, m_state.geo, m_images);

  // For undo/redo state.
  SendCanvasChangeEvent();
}

void CanvasPanel::RunCommand(Command* cmd){
  // When a command is run, any commands in the redo list must be
  // cleared (See exception in CanvasPanel::Redo).
  RunCommand(cmd, clear_redo(true), nullptr);
}

void CanvasPanel::RunCommand(Command* cmd, const FrameId& frameId){
  // When a command is run, any commands in the redo list must be
  // cleared (See exception in CanvasPanel::Redo).
  RunCommand(cmd, clear_redo(true), &m_images.GetImageById(frameId));
}

void CanvasPanel::RunDWIM(){
  if (m_commands.ApplyDWIM(m_images, *m_contexts.command, m_state.geo)){
    Refresh();
    // Fixme: What else must be done?
  }
}

void CanvasPanel::ScrollMaxDown(){
  m_state.geo.pos.y = std::max(0, GetMaxScrollDown() - GetVerticalPageSize());
  AdjustScrollbars(m_state.geo.Pos());
  Refresh();
}

void CanvasPanel::ScrollMaxLeft(){
  m_state.geo.pos.x = 0;
  AdjustScrollbars(m_state.geo.Pos());
  Refresh();
}

void CanvasPanel::ScrollMaxRight(){
  m_state.geo.pos.x = std::max(0, GetMaxScrollRight() - GetHorizontalPageSize());
  AdjustScrollbars(m_state.geo.Pos());
  Refresh();
}

void CanvasPanel::ScrollMaxUp(){
  m_state.geo.pos.y = 0;
  AdjustScrollbars(m_state.geo.Pos());
  Refresh();
}

void CanvasPanel::ScrollPageDown(){
  auto& geo = m_state.geo;
  const int pageSize = GetVerticalPageSize();
  const int y = geo.pos.y + pageSize;
  geo.pos.y = std::max(0, std::min(y, GetMaxScrollDown() - pageSize));
  m_scroll.updateVertical = true;
}

void CanvasPanel::ScrollPageLeft(){
  auto& geo = m_state.geo;
  geo.pos.x = std::max(0, geo.pos.x - GetHorizontalPageSize());
  m_scroll.updateHorizontal = true;
}

void CanvasPanel::ScrollPageRight(){
  auto& geo = m_state.geo;
  const int pageSize = GetHorizontalPageSize();
  const int x = geo.pos.x + pageSize;
  geo.pos.x = std::max(0, std::min(x, GetMaxScrollRight() - pageSize));
  m_scroll.updateHorizontal = true;
}

void CanvasPanel::ScrollPageUp(){
  auto& geo = m_state.geo;
  geo.pos.y = std::max(0, geo.pos.y - GetVerticalPageSize());
  m_scroll.updateVertical = true;
}

void CanvasPanel::SelectObject(Object* obj, const deselect_old& deselectOld){
  SelectObjects(as_list(obj), deselectOld);
}

void CanvasPanel::SelectObjects(const objects_t& objects, const deselect_old& deselectOld){
  if (deselectOld.Get()){
    m_images.Active().DeselectObjects();
  }
  m_images.Active().SelectObjects(objects);
  m_contexts.GetTool().SelectionChange();
  SendCanvasChangeEvent();
  Refresh();
}

void CanvasPanel::SendCanvasChangeEvent(){
  CanvasChangeEvent event(FAINT_CANVAS_CHANGE, GetCanvasId());
  ProcessEvent(event);
}

void CanvasPanel::SendGridChangeEvent(){
  CanvasChangeEvent event(FAINT_GRID_CHANGE, GetCanvasId());
  ProcessEvent(event);
}

void CanvasPanel::SendZoomChangeEvent(){
  CanvasChangeEvent event(FAINT_ZOOM_CHANGE, GetCanvasId());
  ProcessEvent(event);
}

void CanvasPanel::SetFaintScrollPos(const IntPoint& pos){
  m_state.geo.SetPos(pos);
  m_scroll.updateHorizontal = true;
  m_scroll.updateVertical = true;
}

void CanvasPanel::SetGrid(const Grid& g){
  m_state.grid = g;
  SendGridChangeEvent();
}

void CanvasPanel::SetMirage(const std::weak_ptr<Copyable<Bitmap>>& bitmap){
  m_mirage.bitmap = bitmap;
  Refresh();
}

void CanvasPanel::SetMirage(const std::weak_ptr<RasterSelection>& selection){
  m_mirage.selection = selection;
  Refresh(); // Fixme: Consider RefreshRect
}

void CanvasPanel::SelectFrame(const index_t& index){
  assert(index < m_images.GetNumImages());
  if (index != m_images.GetActiveIndex()){
    Preempt(PreemptOption::ALLOW_COMMAND);
    m_images.SetActiveIndex(index);
    m_contexts.command->SetFrame(&m_images.Active());
    m_contexts.GetTool().SelectionChange();

    // For frame control
    SendCanvasChangeEvent();
    Refresh();
  }
}

void CanvasPanel::SetPointOverlay(const IntPoint& p){
  m_state.pointOverlay.Set(p);
}

void CanvasPanel::SetZoomLevel(const ZoomLevel& zoom){
  m_state.geo.zoom = zoom;
  SendCanvasChangeEvent();
  MousePosRefresh();
}

void CanvasPanel::Undo(){
  if (m_contexts.GetTool().CanUndo()){
    m_contexts.GetTool().Undo();
    MousePosRefresh();

    // For undo/redo-state
    SendCanvasChangeEvent();
    return;
  }

  // Preempt the current tool, but allow it to commit.
  // if committing, whatever it did will be undone.
  PreemptResult res = Preempt(PreemptOption::ALLOW_COMMAND);
  if (res == PreemptResult::CANCEL){
    // The tool was aborted by the undo
    return;
  }

  // The tool either committed on the preempt (and the command should
  // now be undone) or there was no effect. Proceed with normal
  // undo-behavior.
  if (m_commands.Undo(*m_contexts.command, m_state.geo)){
    SendCanvasChangeEvent();

    // Update the selection settings etc (originally added to make the
    // ObjSelectTool change the controls if a setting change was
    // undone)
    m_contexts.GetTool().SelectionChange();
  }
}

void CanvasPanel::ZoomFit(){
  auto& geo = m_state.geo;

  // Use the complete size rather than the client size, so that
  // scrollbars don't limit the available size
  const IntSize windowSize(to_faint(GetSize()));
  const IntSize borders(geo.border * 2);
  const Size availableSpace(floated(max_coords(windowSize - borders, IntSize(1,1))));
  const Size imageSize(floated(m_images.Active().GetSize()));
  Scale rel(NewSize(availableSpace), imageSize);
  geo.zoom.SetApproximate(std::min(rel.x, rel.y));
  AdjustScrollbars(geo.pos = {0,0});
  MousePosRefresh();
  SendZoomChangeEvent();
  Refresh();
}

int CanvasPanel::GetHorizontalPageSize() const {
  return GetClientSize().GetWidth() - m_state.geo.border.w * 2;
}

int CanvasPanel::GetVerticalPageSize() const {
  return GetClientSize().GetHeight() - m_state.geo.border.h * 2;
}

bool CanvasPanel::IgnoreCanvasHandle(const PosInfo& info){
  // Prefer a clicked object handle with object selection tool
  // over canvas resize-handles.
  return info.handle.IsSet() &&
    m_contexts.tool.GetToolId() == ToolId::OBJECT_SELECTION;
}

void CanvasPanel::MousePosRefresh(const IntPoint& viewPt, const ToolModifiers& modifiers){
  PosInfo info = HitTest(viewPt, modifiers);
  if (!m_mouse.HasCapture() && !IgnoreCanvasHandle(info)){
    auto canvasHandle = canvas_handle_hit_test(viewPt,
      m_images.Active().GetSize(), m_state.geo);

    if (canvasHandle){
      SetFaintCursor(canvasHandle.Get().GetCursor());
      m_statusInfo.SetMainText("Left click to resize, right click to rescale.");
      return;
    }
  }
  HandleToolResult(m_contexts.GetTool().MouseMove(info));
  SetFaintCursor(m_contexts.GetTool().GetCursor(info));
}

void CanvasPanel::ScrollLineUp(){
  // Fixme: Use m_scroll.startX et al.
  auto& geo = m_state.geo;
  const int y = geo.pos.y - g_lineSize;
  const int pageSize = GetVerticalPageSize();
  geo.pos.y = std::max(0, std::min(y, GetMaxScrollDown() - pageSize));
  AdjustVerticalScrollbar(geo.pos.y);
  Refresh();
}

void CanvasPanel::ScrollLineDown(){
  auto& geo = m_state.geo;
  const int y = geo.pos.y + g_lineSize;
  const int pageSize = GetVerticalPageSize();
  geo.pos.y = std::max(0, std::min(y, GetMaxScrollDown() - pageSize));
  AdjustVerticalScrollbar(geo.pos.y);
  Refresh();
}

void CanvasPanel::ScrollLineLeft(){
  auto& geo = m_state.geo;
  // Fixme: Why no std::min like ScrollLineUp?
  geo.pos.x = std::max(0, geo.pos.x - g_lineSize);
  AdjustHorizontalScrollbar(geo.pos.x);
}

void CanvasPanel::ScrollLineRight(){
  auto& geo = m_state.geo;
  const int x = geo.pos.x + g_lineSize;
  const int pageSize = GetHorizontalPageSize();
  geo.pos.x = std::max(0, std::min(x, GetMaxScrollRight() - pageSize));
  AdjustHorizontalScrollbar(geo.pos.x);
  Refresh();
}

static IntRect get_visible_rect(const IntSize& size, const CanvasGeo& geo){
  const coord zoom = geo.Scale();
  return IntRect(floored((geo.pos - point_from_size(geo.border)) / zoom),
    truncated(size / zoom + Size::Both(100)));
}

void CanvasPanel::RefreshToolRect(){
  Tool& tool = m_contexts.GetTool();
  IntRect toolRect(tool.GetRefreshRect(get_visible_rect(to_faint(GetSize()),
        m_state.geo), mouse::image_position(m_state.geo, *this)));
  InclusiveRefresh(mouse::image_to_view(toolRect, m_state.geo));
}

void CanvasPanel::CommitTool(Tool& tool, RefreshMode refreshMode){
  Command* cmd = tool.GetCommand();
  if (cmd == nullptr){
    return;
  }

  RunCommand(cmd);
  if (refreshMode == REFRESH){
    Refresh();
  }
}

void CanvasPanel::CloseUndoBundle(){
  m_commands.CloseUndoBundle();
  SendCanvasChangeEvent();
}

PosInfo CanvasPanel::HitTest(const IntPoint& ptView, const ToolModifiers& mod){
  ObjectInfo objInfo = hit_test(ptView,
    m_images.Active(),
    m_state.geo);

  Point ptImage = mouse::view_to_image(ptView, m_state.geo);
  PosInfo info = {
    GetInterface(),
    m_statusInfo,
    mod,
    ptImage,
    m_contexts.app.TabletGetCursor(),
    InSelection(ptImage),
    m_contexts.app.GetLayerType(),
    objInfo};
  return info;
}

void CanvasPanel::OpenUndoBundle(){
  m_commands.OpenUndoBundle();
}

void CanvasPanel::SetFaintCursor(Cursor cursor){
  if (cursor == Cursor::DONT_CARE){
    return;
  }
  SetCursor(to_wx_cursor(cursor));
}

void CanvasPanel::RunCommand(Command* cmd, const clear_redo& clearRedo, Image* activeImage){
  if (activeImage == nullptr){
    activeImage = &(m_images.Active());
  }

  Optional<IntPoint> offset = m_commands.Apply(cmd,
    clearRedo,
    activeImage,
    m_images,
    *m_contexts.command,
    m_state.geo);

  if (offset.IsSet()){
    AdjustScrollbars(offset.Get());
  }

  if (!m_commands.Bundling()){
    SendCanvasChangeEvent();
  }
  m_contexts.GetTool().SelectionChange();
}

// Refreshes the union of the rectangle parameter and the rectangle
// parameter from the previous call.
void CanvasPanel::InclusiveRefresh(const IntRect& r){
  const coord zoom = m_state.geo.zoom.GetScaleFactor();
  if (zoom >= 1.0){
    RefreshRect(to_wx(union_of(r, m_lastRefreshRect)).Inflate(floored(2.0 * zoom)));
    m_lastRefreshRect = r;
  }
  else {
    // Fixme - Workaround. RefreshRect does not work well when zoomed out,
    // leading to problems with offsets and other stuff.
    // Note: This is slow.
    Refresh();
  }
}

bool CanvasPanel::HandleToolResult(ToolResult ref){
  if (ref == TOOL_COMMIT){
    CommitTool(m_contexts.GetTool(), REFRESH);
  }
  else if (ref == TOOL_CHANGE){
    CommitTool(m_contexts.GetTool(), NO_REFRESH);
    m_contexts.tool.ClearSwitched();

    // Update settings, e.g. when cloning objects from the
    // ObjSelectTool
    m_contexts.GetTool().SelectionChange();
    Refresh();
  }
  else if (ref == TOOL_DRAW){
    RefreshToolRect();
  }
  else if (ref == TOOL_CANCEL){
    Refresh();
  }
  // TOOL_NONE might signify that the tool didn't care, which can in
  // some cases suggest further processing outside this function.
  return ref != TOOL_NONE;
}

} // namespace

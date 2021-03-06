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

#ifndef FAINT_CANVAS_PANEL_HH
#define FAINT_CANVAS_PANEL_HH

#include <deque>
#include <vector>
#include "wx/panel.h"
#include "bitmap/bitmap.hh"
#include "geo/canvas-geo.hh"
#include "geo/intrect.hh"
#include "gui/canvas-change-event.hh"
#include "gui/canvas-panel-contexts.hh"
#include "gui/canvas-state.hh"
#include "gui/menu-predicate.hh"
#include "gui/mouse-capture.hh"
#include "objects/object.hh"
#include "tools/tool.hh"
#include "tools/tool-wrapper.hh"
#include "util/canvas.hh"
#include "util/command-history.hh"
#include "util/command-util.hh"
#include "util/distinct.hh"
#include "util/file-path.hh"
#include "util/grid.hh"
#include "util/id-types.hh"
#include "util/image-list.hh"
class wxFileDropTarget;
class RectangleSelectTool;

namespace faint{
class CommandContextImpl;
class ImageProps;

enum class PreemptResult{ NONE, COMMIT, CANCEL };
enum class PreemptOption{ ALLOW_COMMAND, DISCARD_COMMAND };

wxDECLARE_EVENT(EVT_FAINT_CANVAS_CHANGE, CanvasChangeEvent);
wxDECLARE_EVENT(EVT_FAINT_GRID_CHANGE, CanvasChangeEvent);
wxDECLARE_EVENT(EVT_FAINT_ZOOM_CHANGE, CanvasChangeEvent);

class category_canvas_panel;
typedef Distinct<bool, category_canvas_panel, 1> initially_dirty;

class Contexts{
public:
  Contexts(AppContext& in_app) :
    app(in_app), tool(in_app)
  {}

  const Tool& GetTool() const{
    return tool.GetTool();
  }

  Tool& GetTool(){
    return tool.GetTool();
  }

  AppContext& app;
  std::unique_ptr<TargetableCommandContext> command;
  std::unique_ptr<Canvas> canvas;
  ToolWrapper tool;
};

class CanvasPanel : public wxPanel {
public:
  CanvasPanel(wxWindow* parent, ImageList&&, const initially_dirty&, wxFileDropTarget*, AppContext&, StatusInterface&);
  bool AcceptsFocus() const override;
  void AdjustHorizontalScrollbar(int pos);
  void AdjustScrollbars(const IntPoint&);
  void AdjustVerticalScrollbar(int pos);
  bool CanRedo() const;
  bool CanUndo() const;
  void CenterView(const IntPoint& ptView);
  void CenterViewImage(const Point& ptImage);
  void ChangeZoom(ZoomLevel::ChangeType);
  void ClearPointOverlay();
  void CloseUndoBundle();
  void DeselectObject(Object*);
  void DeselectObjects();
  void DeselectObjects(const objects_t&);
  PosInfo ExternalHitTest(const IntPoint&);
  IntPoint GetFaintScrollPos() const;
  CanvasId GetCanvasId() const;
  Optional<FilePath> GetFilePath() const;
  Grid GetGrid() const;
  RasterSelection& GetImageSelection();
  const ImageList& GetImageList() const;
  Point GetImageViewStart() const;
  Canvas& GetInterface();
  int GetMaxScrollRight() const;
  int GetMaxScrollDown() const;
  IntPoint GetMaxUsefulScroll() const;
  MenuFlags GetMenuFlags() const;
  const objects_t& GetObjects();
  const objects_t& GetObjectSelection();
  utf8_string GetRedoName() const;
  Point GetRelativeMousePos();
  index_t GetSelectedFrame() const;
  utf8_string GetUndoName() const;
  coord GetZoom() const;
  const ZoomLevel& GetZoomLevel() const;
  bool Has(const ObjectId&);
  bool HasObjectSelection() const;
  bool HasRasterSelection() const;
  bool HasSelection() const;
  bool HasToolSelection() const;
  bool InSelection(const Point&);
  bool IsDirty() const;
  void MousePosRefresh();
  void NextFrame();
  void NotifySaved(const FilePath&);
  void OpenUndoBundle();
  PreemptResult Preempt(PreemptOption);
  void PreviousFrame();
  void Redo();
  void RunCommand(Command*);
  void RunCommand(Command*, const FrameId&);
  void RunDWIM();
  void ScrollMaxDown();
  void ScrollMaxLeft();
  void ScrollMaxRight();
  void ScrollMaxUp();
  void ScrollPageDown();
  void ScrollPageLeft();
  void ScrollPageRight();
  void ScrollPageUp();
  void SelectFrame(const index_t&);
  void SelectObject(Object*, const deselect_old&);
  void SelectObjects(const objects_t&, const deselect_old&);
  void SetFaintScrollPos(const IntPoint&);
  void SetGrid(const Grid&);
  void SetMirage(const std::weak_ptr<Copyable<Bitmap>>&);
  void SetMirage(const std::weak_ptr<RasterSelection>&);
  void SetPointOverlay(const IntPoint&);
  void SetZoomLevel(const ZoomLevel&);
  void Undo();
  void ZoomFit();
private:
  friend class faint::CanvasToolInterface;
  const RasterSelection& GetImageSelection() const;
  enum RefreshMode{REFRESH, NO_REFRESH};
  void CommitTool(Tool&, RefreshMode);
  int GetHorizontalPageSize() const;
  int GetVerticalPageSize() const;
  bool HandleToolResult(ToolResult);
  PosInfo HitTest(const IntPoint&, const ToolModifiers&);
  bool IgnoreCanvasHandle(const PosInfo&);
  void InclusiveRefresh(const IntRect&);
  void MousePosRefresh(const IntPoint&, const ToolModifiers&);
  void OnChar(wxKeyEvent&);
  void OnKeyDown(wxKeyEvent&);
  void OnKeyUp(wxKeyEvent&);
  void OnMouseDown(wxMouseEvent&);
  void OnMouseUp(wxMouseEvent&);
  void OnMotion(wxMouseEvent&);
  void OnMouseWheel(wxMouseEvent&);
  void OnScrollDrag(wxScrollWinEvent&);
  void OnScrollDragEnd(wxScrollWinEvent&);
  void OnScrollLineDown(wxScrollWinEvent&);
  void OnScrollLineUp(wxScrollWinEvent&);
  void OnScrollPageDown(wxScrollWinEvent&);
  void OnScrollPageUp(wxScrollWinEvent&);
  void OnIdle(wxIdleEvent&);
  void RefreshToolRect();
  void RunCommand(Command*, const clear_redo&, Image*);
  void ScrollLineUp();
  void ScrollLineDown();
  void ScrollLineLeft();
  void ScrollLineRight();
  void SendCanvasChangeEvent();
  void SendZoomChangeEvent();
  void SendGridChangeEvent();
  void SetFaintCursor(Cursor);
  void UndoObject(Command*);

  CanvasId m_canvasId;
  CommandHistory m_commands;
  Contexts m_contexts;

  struct {
    Optional<FilePath> filename;
    Optional<CommandId> savedAfter;
  } m_document;

  ImageList m_images;
  IntRect m_lastRefreshRect;

  struct{
    std::weak_ptr<Copyable<Bitmap>> bitmap;
    std::weak_ptr<RasterSelection> selection;
  } m_mirage;

  MouseCapture m_mouse;

  bool m_probablyCtrlEnter = false;

  struct {
    int startX = 0;
    int startY = 0;
    bool updateHorizontal = false;
    bool updateVertical = false;
  } m_scroll;

  CanvasState m_state;
  StatusInterface& m_statusInfo;
};

}

#endif

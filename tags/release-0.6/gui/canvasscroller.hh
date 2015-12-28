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

#ifndef FAINT_CANVASSCROLLER_HH
#define FAINT_CANVASSCROLLER_HH

#include <vector>
#include <deque>
#include "wx/wx.h"
#include "bitmap/bitmap.hh"
#include "geo/canvasgeo.hh"
#include "geo/grid.hh"
#include "gui/menupredicate.hh"
#include "objects/object.hh"
#include "tools/tool-wrapper.hh"
#include "util/canvasinterface.hh"
#include "util/commandutil.hh"
#include "util/idtypes.hh"
#include "util/image.hh"
#include "util/settings.hh"

class ImageProps;
class RectangleSelectTool;
class CommandContextImpl;

enum PreemptResult{ PREEMPT_NONE, PREEMPT_COMMIT, PREEMPT_CANCEL };
struct PreemptOption{
  enum type{ ALLOW_COMMAND, DISCARD_COMMAND };
};

class CanvasScroller : public wxPanel {
public:
  CanvasScroller( wxWindow* parent, ImageProps& );
  CanvasScroller( wxWindow* parent, std::vector<ImageProps>& );
  ~CanvasScroller();
  void AddFrame(faint::Image*);
  void AddFrame(faint::Image*, size_t index);
  void Apply( const Operation& );
  bool AcceptsFocus() const;
  void AdjustHorizontalScrollbar( int pos );
  void AdjustScrollbars( const IntPoint& );
  void AdjustVerticalScrollbar( int pos );

  // Bundles the given amount of commands from the undo list,
  // using the given name as the new undo name
  void BundleUndo( size_t, const std::string& name );
  bool CanRedo() const;
  bool CanUndo() const;
  void CenterView( const IntPoint& ptView );
  void CenterViewImage( const Point& ptImage );
  void ChangeZoom( ZoomLevel::ChangeType );
  void ClearDirty();
  void ClearPointOverlay();
  void ClearRasterSelectionChimera();
  bool CopySelection();
  void Crop();
  void CutSelection();
  void DeleteSelection();
  void Deselect();
  void DeselectObject( Object* );
  void DeselectObjects();
  void DeselectObjects( const objects_t& );
  void DeselectRaster();
  void ExternalRefresh();
  CursorPositionInfo ExternalHitTest(const IntPoint&);
  void Flatten();
  ApplyTarget GetApplyTarget();
  Size GetApplyTargetSize();
  faint::Bitmap GetSubBitmap( const IntRect& ) const;
  IntSize GetBitmapSize() const;
  CanvasId GetCanvasId() const;
  std::string GetFilename() const;
  Grid GetGrid() const;
  faint::Image& GetImage();
  RasterSelection& GetImageSelection();
  faint::ImageList& GetImageList();
  const faint::ImageList& GetImageList() const;
  Point GetImageViewStart() const;
  CanvasInterface& GetInterface();
  int GetMaxScrollRight() const;
  int GetMaxScrollDown() const;
  faint::MenuFlags GetMenuFlags() const;
  objects_t& GetObjects();
  const objects_t& GetObjectSelection();
  RasterSelection& GetRasterSelection();
  std::string GetRedoName() const;
  Point GetRelativeMousePos();
  size_t GetSelectedFrame() const;
  std::string GetUndoName() const;
  faint::coord GetZoom() const;
  const ZoomLevel& GetZoomLevel() const;
  void GroupSelected();
  bool Has( const ObjectId& );
  bool HasObjectSelection() const;
  bool HasRasterSelection() const;
  bool HasSelection() const;
  bool HasToolSelection() const;
  bool InSelection( const Point& );
  bool IsDirty() const;
  void MousePosRefresh();
  void MoveObjectBackward();
  void MoveObjectForward();
  void MoveObjectToBack();
  void MoveObjectToFront();
  void NextFrame();
  void Paste();
  PreemptResult Preempt( PreemptOption::type );
  void PreviousFrame();
  void Redo();
  void RemoveFrame( faint::Image* );
  void RemoveFrame( size_t index );
  void ReorderFrame( size_t, size_t );
  void Rescale( const IntSize& );
  void Resize( const IntRect& );
  void Rotate90CW();  // Fixme - remove
  void RunCommand( Command*);
  void RunDWIM();
  void ScaleRasterSelection( const IntSize& );
  void ScaleSelectedObject( const Size& );
  void ScrollMaxDown();
  void ScrollMaxLeft();
  void ScrollMaxRight();
  void ScrollMaxUp();
  void ScrollPageDown();
  void ScrollPageLeft();
  void ScrollPageRight();
  void ScrollPageUp();
  void SelectAll();
  void SelectFrame(size_t);
  void SelectObject( Object*, const deselect_old& );
  void SelectObjects( const objects_t&, const deselect_old& );
  void SetChimera( RasterSelection* );
  void SetDirty();
  void SetFilename(const std::string& );
  void SetGrid( const Grid& );
  void SetObjectSelection( const objects_t& );
  void SetPointOverlay( const IntPoint& );
  void SetRasterSelection( const IntRect& );
  void SetZoomLevel( const ZoomLevel& );
  bool ShouldDrawRaster() const;
  bool ShouldDrawVector() const;
  void ShowResizeDialog();
  void ShowRotateDialog();
  void Undo();
  void UngroupSelected();
  void ZoomFit();
private:
  const RasterSelection& GetImageSelection() const;
  void CommitTool( Tool*, bool refresh=true );
  int GetHorizontalPageSize() const;
  int GetVerticalPageSize() const;
  wxRect GetVisibleImage();
  IntRect GetVisibleImageRect() const;
  bool HandleToolResult( ToolResult );
  CursorPositionInfo HitTest( const wxPoint&, int );
  bool IgnoreCanvasHandle( const CursorPositionInfo& );
  void InclusiveRefresh( const IntRect& r );
  void Initialize();
  void MousePosRefresh( const wxPoint&, int modifiers );
  std::pair<Object*, HitInfo> ObjectAt( const Point& );
  void OnCaptureLost( wxMouseCaptureLostEvent& );
  void OnChar( wxKeyEvent& );
  void OnDoubleClick( wxMouseEvent& );
  void OnEraseBackground( wxEraseEvent& );
  void OnKeyDown( wxKeyEvent& );
  void OnKeyUp( wxKeyEvent& );
  void OnLeftDown( wxMouseEvent& );
  void OnLeftUp( wxMouseEvent& );
  void OnMotion( wxMouseEvent& );
  void OnMouseOut( wxMouseEvent& );
  void OnMouseWheel( wxMouseEvent& );
  void OnPaint( wxPaintEvent& );
  void OnScrollDrag( wxScrollWinEvent& );
  void OnScrollDragEnd( wxScrollWinEvent& );
  void OnScrollLineDown( wxScrollWinEvent& );
  void OnScrollLineUp( wxScrollWinEvent& );
  void OnScrollPageDown( wxScrollWinEvent& );
  void OnScrollPageUp( wxScrollWinEvent& );
  void OnSize( wxSizeEvent& );
  void OnIdle( wxIdleEvent& );
  void PasteBitmap( const faint::Bitmap& );
  void PasteBitmapAsObject( const faint::Bitmap& );
  void PasteObjects( objects_t& );
  void RefreshToolRect();
  void RunCommand( Command*, bool, faint::Image* activeImage=0 );
  void ScrollLineUp();
  void ScrollLineDown();
  void ScrollLineLeft();
  void ScrollLineRight();
  bool SelectionToClipboard( bool );
  void SetFaintCursor( Cursor::type );
  RectangleSelectTool* SwitchToRasterSelection();
  void UndoObject( Command* );
  std::deque<old_command_t> m_undoList;
  std::deque<old_command_t> m_redoList;
  CanvasInterface* m_interface;
  CanvasId m_canvasId;
  std::string m_filename;
  bool m_dirty;
  Geometry m_geo;
  ToolWrapper m_toolWrap;
  faint::ImageList m_images;
  IntRect m_lastRefreshRect;
  faint::Bitmap m_hitTestBuffer;
  DeleteState m_dwimDelete;
  Grid m_grid;
  CommandContextImpl* m_cmdContext;
  struct {
    int startX;
    int startY;
    bool updateHorizontal;
    bool updateVertical;
  } m_scroll;

  RasterSelection* m_rasterSelectionChimera;
  Optional<IntPoint> m_pointOverlay;
  DECLARE_EVENT_TABLE()
};

#endif

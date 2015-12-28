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
#include "util/path.hh"
#include "util/settings.hh"
#include "util/unique.hh"

class ImageProps;
class RectangleSelectTool;
class CommandContextImpl;

enum class PreemptResult{ NONE, COMMIT, CANCEL };
enum class PreemptOption{ ALLOW_COMMAND, DISCARD_COMMAND };

class CanvasScroller;
typedef Unique<bool, CanvasScroller, 0> clear_redo;
typedef Unique<bool, CanvasScroller, 1> initially_dirty;

class CanvasScroller : public wxPanel {
public:
  CanvasScroller( wxWindow* parent, ImageProps&, const initially_dirty& );
  CanvasScroller( wxWindow* parent, std::vector<ImageProps>&, const initially_dirty& );
  ~CanvasScroller();
  void AddFrame(faint::Image*);
  void AddFrame(faint::Image*, const index_t&);
  bool AcceptsFocus() const;
  void AdjustHorizontalScrollbar( int pos );
  void AdjustScrollbars( const IntPoint& );
  void AdjustVerticalScrollbar( int pos );
  bool CanRedo() const;
  bool CanUndo() const;
  void CenterView( const IntPoint& ptView );
  void CenterViewImage( const Point& ptImage );
  void ChangeZoom( ZoomLevel::ChangeType );
  void ClearPointOverlay();
  void ClearRasterSelectionChimera();
  bool CopySelection();
  void CutSelection();
  void DeleteSelection();
  void DeselectObject( Object* );
  void DeselectObjects();
  void DeselectObjects( const objects_t& );
  void DeselectRaster();
  void ExternalRefresh();
  CursorPositionInfo ExternalHitTest(const IntPoint&);
  IntPoint GetFaintScrollPos() const;
  faint::Bitmap GetSubBitmap( const IntRect& ) const;
  IntSize GetBitmapSize() const;
  CanvasId GetCanvasId() const;
  Optional<faint::FilePath> GetFilePath() const;
  Grid GetGrid() const;
  faint::Image& GetImage();
  RasterSelection& GetImageSelection();
  faint::ImageList& GetImageList();
  const faint::ImageList& GetImageList() const;
  Point GetImageViewStart() const;
  CanvasInterface& GetInterface();
  int GetMaxScrollRight() const;
  int GetMaxScrollDown() const;
  IntPoint GetMaxUsefulScroll() const;
  faint::MenuFlags GetMenuFlags() const;
  const objects_t& GetObjects();
  const objects_t& GetObjectSelection();
  RasterSelection& GetRasterSelection();
  std::string GetRedoName() const;
  Point GetRelativeMousePos();
  index_t GetSelectedFrame() const;
  std::string GetUndoName() const;
  faint::coord GetZoom() const;
  const ZoomLevel& GetZoomLevel() const;
  bool Has( const ObjectId& );
  bool HasObjectSelection() const;
  bool HasRasterSelection() const;
  bool HasSelection() const;
  bool HasToolSelection() const;
  bool InSelection( const Point& );
  bool IsDirty() const;
  void MousePosRefresh();
  void NextFrame();
  void NotifySaved( const faint::FilePath& );
  void Paste();
  PreemptResult Preempt( PreemptOption );
  void PreviousFrame();
  void Redo();
  void RemoveFrame( faint::Image* );
  void RemoveFrame( const index_t& );
  void ReorderFrame( const new_index_t&, const old_index_t& );
  void RunCommand( Command* );
  void RunCommand( Command*, const FrameId& );
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
  void SelectObject( Object*, const deselect_old& );
  void SelectObjects( const objects_t&, const deselect_old& );
  void SetBitmapChimera( faint::Bitmap* );
  void SetChimera( RasterSelection* );
  void SetFaintScrollPos( const IntPoint& );
  void SetGrid( const Grid& );
  void SetPointOverlay( const IntPoint& );
  void SetZoomLevel( const ZoomLevel& );
  bool ShouldDrawRaster() const;
  bool ShouldDrawVector() const;
  void Undo();
  void UnwrapLast();
  void ZoomFit();
private:
  friend class CanvasToolInterface;
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
  void Initialize(const initially_dirty&);
  void MousePosRefresh( const wxPoint&, int modifiers );
  std::pair<Object*, Hit> ObjectAt( const Point& );
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
  void RunCommand( Command*, const clear_redo&, faint::Image* activeImage=0 );
  void ScrollLineUp();
  void ScrollLineDown();
  void ScrollLineLeft();
  void ScrollLineRight();
  bool SelectionToClipboard( bool );
  void SetRasterSelection( const IntRect& ); // Fixme: Relic?
  void SetFaintCursor( Cursor );
  void UndoObject( Command* );
  std::deque<OldCommand> m_undoList;
  std::deque<OldCommand> m_redoList;
  CanvasInterface* m_interface;
  CanvasId m_canvasId;
  Optional<faint::FilePath> m_filename;
  Geometry m_geo;
  ToolWrapper m_toolWrap;
  CommandContextImpl* m_cmdContext;
  faint::ImageList m_images;
  faint::Bitmap m_hitTestBuffer;
  IntRect m_lastRefreshRect;
  DeleteState m_dwimDelete;
  Grid m_grid;
  struct {
    int startX;
    int startY;
    bool updateHorizontal;
    bool updateVertical;
  } m_scroll;

  RasterSelection* m_rasterSelectionChimera;
  faint::Bitmap* m_bitmapChimera;
  Optional<IntPoint> m_pointOverlay;
  Optional<CommandId> m_savedAfter;
  DECLARE_EVENT_TABLE()
};

#endif

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
#include "wx/wx.h"
#include <vector>
#include <deque>
#include "settings.hh"
#include "canvasinterface.hh"
#include "tools/settingid.hh"
#include "tools/toolwrapper.hh"
#include "util/idtypes.hh"
#include "geo/canvasgeo.hh"

enum PreemptResult{ PREEMPT_NONE, PREEMPT_COMMIT, PREEMPT_CANCEL };

class CanvasScroller : public wxPanel {
public:
  CanvasScroller( wxWindow* parent, const faint::Bitmap& ); // Fixme: Remove
  CanvasScroller( wxWindow* parent, const CanvasInfo&, const std::vector<Object*>& objects ); // Fixme: Remove
  CanvasScroller( wxWindow* parent, ImageProps& );
  ~CanvasScroller();
  std::string GetFilename() const;

  virtual bool AcceptsFocus() const;
  bool CanUndo() const;
  bool CanRedo() const;
  void CenterView( const IntPoint& ptView );
  void CenterViewImage( const Point& ptImage );
  void SetDirty();
  void ClearDirty();
  bool CopySelection();
  void Crop();
  void CutSelection();
  void DeleteSelection();
  void Deselect();
  void DeselectObject( Object* );
  void DeselectObjects();
  void DeselectRaster();
  void Flatten();
  template<typename Operation> void ApplyOperation();
  faint::Bitmap GetBitmap( const IntRect& ) const;
  IntSize GetBitmapSize() const;
  CanvasId GetCanvasId() const;
  faint::Image& GetImage();
  ApplyTarget GetApplyTarget();
  Size GetApplyTargetSize();
  Point GetImageViewStart() const;
  CanvasInterface& GetInterface();
  IntRect GetRasterSelection();
  Point GetRelativeMousePos();
  std::vector<Object*>& GetSelectedObjects();
  std::vector<Object*>& GetObjects();
  faint::coord GetZoom() const;
  const ZoomLevel& GetZoomLevel() const;
  void GroupSelected();
  bool Has( const ObjectId& );
  bool HasSelection() const;
  bool HasRasterSelection() const;
  bool HasObjectSelection() const;
  bool HasToolSelection();
  bool InSelection( const Point& p );

  void AdjustScrollbars( int x=0, int y=0 );
  void AdjustHorizontalScrollbar( int pos );
  void AdjustVerticalScrollbar( int pos );

  void ChangeZoom( ZoomLevel::ChangeType );

  void MoveObjectDown();
  void MoveObjectUp();
  void MoveObjectFront();
  void MoveObjectBack();

  bool IsDirty() const;

  enum HitType{ HT_MISS, HT_INSIDE, HT_EDGE, HT_NEAR }; // Fixme: Remove, use object hit definitions directly
  std::pair<Object*, HitType>  ObjectAt( const Point& p );

  void OnChar( wxKeyEvent& );
  void OnDoubleClick( wxMouseEvent& );
  void OnEnter( wxMouseEvent& );
  void OnEraseBackground( wxEraseEvent& );
  void OnMouseOut( wxMouseEvent& );
  void OnKeyDown( wxKeyEvent& );
  void OnKeyUp( wxKeyEvent& );
  void OnLeftDown( wxMouseEvent& );
  void OnLeftUp( wxMouseEvent& );
  void OnMotion( wxMouseEvent& );
  void OnMouseWheel( wxMouseEvent& );
  void OnCaptureLost( wxMouseCaptureLostEvent& );
  void ScrollPageDown();
  void ScrollPageLeft();
  void ScrollPageRight();
  void ScrollPageUp();
  void ScrollMaxLeft();
  void ScrollMaxRight();
  void ScrollMaxUp();
  void ScrollMaxDown();
  void OnPaint( wxPaintEvent& );
  void OnScrollDrag( wxScrollWinEvent& );
  void OnScrollDragEnd( wxScrollWinEvent& );
  void OnScrollLineUp( wxScrollWinEvent& );
  void OnScrollLineDown( wxScrollWinEvent& );
  void OnScrollPageDown( wxScrollWinEvent& );
  void OnScrollPageUp( wxScrollWinEvent& );
  int GetMaxScrollRight() const;
  int GetMaxScrollDown() const;
  void OnSize( wxSizeEvent& );
  void Paste();
  PreemptResult Preempt( bool discard=false);
  void Redo();
  void Resize( const IntSize& );
  void Resize( const IntSize&, const faint::Color& expandColor );
  void Resize( const IntRect& );
  void Rescale( const IntSize& );  
  void Rotate90CW();  // Fixme - remove
  void RunCommand( Command*, bool clearRedoList=true );
  void SelectAll();
  void SelectObject( Object*, bool deselectOld=true );
  void SetFaintCursor( int cursorId );
  void SetFilename(const std::string& );
  virtual void SetFocus();
  void SetObjectSelection( const std::vector<Object*>& );
  void SetRasterSelection( const IntRect& );
  void ScaleRasterSelection( const Size& );
  void ScaleSelectedObject( const Size& );
  void ScaleToolBitmap( const IntSize& );
  void Undo();
  void UngroupSelected();
  void UpdateToolSettings(){}
  void ClearOverlays();
  void ExternalRefresh();
  bool ShouldDrawRaster();
  bool ShouldDrawVector();
  void SetGrid( const Grid& );
  Grid GetGrid() const;
  void CommitTool( ToolBehavior*, bool refresh=true );
private:
  void RotateTool90CW();
  bool HandleToolResult( ToolRefresh );
  void RefreshTool( ToolBehavior* );
  ToolBehavior* StartDrag();
  void MousePosRefresh( const wxPoint&, int modifiers );
  void ScrollLineUp();
  void ScrollLineDown();
  void ScrollLineLeft();
  void ScrollLineRight();
  int GetHorizontalPageSize() const;
  int GetVerticalPageSize() const;
  wxRect GetVisibleImage();
  CursorPositionInfo HitTest( const wxPoint& p );
  // Point GetResizePoint( BoundaryHandle* handle );
  void InclusiveRefresh( const IntRect& r );
  void Initialize( faint::Image* );
  void UndoObject( Command* );
  void OnIdle( wxIdleEvent& );
  void PasteObjects( std::vector<Object*>& );
  void PasteBitmap( const faint::Bitmap& );
  void PasteBitmapAsObject( const faint::Bitmap& );
  IntRect GetVisibleImageRect() const;
  std::deque<Command*> m_undoList;
  std::deque<Command*> m_redoList;
  CanvasInterface* m_interface;
  CanvasId m_canvasId;
  std::string m_filename;
  bool m_dirty;
  Geometry m_geo;
  ToolWrapper m_toolWrap;
  faint::Image* m_image;
  std::vector<Object*> m_objectSelection;
  IntRect m_rasterSelection;
  IntRect m_lastRefreshRect;
  faint::Bitmap m_hitTestBuffer;
  bool m_updateHorizontal;
  bool m_updateVertical;

  // Fixme: This is a hack implementation which lets me toggle two
  // ways of doing raster delete.  When deleting the first time, the
  // rectangle is filled with bgColor. The second time, if possible, by
  // the color that covered the edge of the rectangle on the first go.
  bool m_lastWasDelete;
  faint::Color m_alternateColor;

  Grid m_grid;

  DECLARE_EVENT_TABLE()
};

#endif

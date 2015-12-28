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

#include "wx/dnd.h"
#include "app/getappcontext.hh"
#include "commands/command-bunch.hh"
#include "commands/python-command-list.hh"
#include "commands/set-bitmap-cmd.hh"
#include "gui/canvasscroller.hh"
#include "gui/gui-constants.hh"
#include "objects/objraster.hh"
#include "rendering/faintdc.hh"
#include "rendering/overlaydcwx.hh"
#include "tools/resize-canvas-tool.hh"
#include "util/angle.hh"
#include "util/char.hh"
#include "util/clipboard.hh"
#include "util/convertwx.hh"
#include "util/formatting.hh"
#include "util/guiutil.hh"
#include "util/image.hh"
#include "util/imageutil.hh"
#include "util/iter.hh"
#include "util/mouse.hh"
#include "util/objutil.hh"
#include "util/settingutil.hh"
#include "util/toolutil.hh"
#include "util/util.hh"

const wxColour g_canvasBg( 160, 140, 160 );
const int g_lineSize = 10;

class CommandContextImpl : public CommandContext{
public:
  CommandContextImpl( CanvasScroller& canvas )
    : m_canvas(canvas),
      m_dc(nullptr),
      m_frame(nullptr)
  {}

  ~CommandContextImpl(){
    delete m_dc;
  }

  void SetFrame(faint::Image* frame){ // Non-virtual
    Reset();
    m_frame = frame;
  }

  void Add( Object* obj, const select_added& select, const deselect_old& deselect ) override {
    m_frame->Add(obj);
    if ( select.Get() ){
      m_canvas.SelectObject( obj, deselect );
    }
  }

  void Add( Object* obj, size_t z, const select_added& select, const deselect_old& deselect ) override{
    m_frame->Add(obj, z);
    if ( select.Get() ){
      m_canvas.SelectObject( obj, deselect );
    }
  }

  void AddFrame( faint::Image* image ) override{
    m_canvas.AddFrame(image);
  }

  void AddFrame( faint::Image* image, const index_t& index ) override{
    m_canvas.AddFrame(image, index);
  }

  const faint::Bitmap& GetBitmap() const override{
    return m_frame->GetBitmap();
  }

  FaintDC& GetDC() override{
    if ( m_dc == nullptr ){
      assert(m_frame != nullptr);
      m_dc = new FaintDC(m_frame->GetBitmap());
    }
    return *m_dc;
  }

  faint::Image& GetFrame(const index_t& index) override{
    return m_canvas.GetImageList().GetImage(index);
  }

  IntSize GetImageSize() const override{
    return m_frame->GetSize();
  }

  const objects_t& GetObjects() override{
    return m_frame->GetObjects();
  }

  size_t GetObjectZ( const Object* obj ) override{
    return m_frame->GetObjectZ(obj);
  }

  faint::Bitmap& GetRawBitmap() override{
    return m_frame->GetBitmap();
  }

  bool HasObjects() const override{
    return m_frame->GetNumObjects() != 0;
  }

  void MoveRasterSelection( const IntPoint& topLeft ) override{
    RasterSelection& selection( m_canvas.GetImageSelection() );
    assert( selection.Exists() );
    selection.Move( topLeft );
  }

  void OffsetRasterSelectionOrigin( const IntPoint& delta ) override{
    RasterSelection& selection( m_canvas.GetImageSelection() );
    if ( selection.Exists() ){
      selection.OffsetOrigin( delta );
    }
  }

  void Remove( Object* obj ) override{
    m_canvas.DeselectObject(obj);
    m_frame->Remove(obj);
  }

  void RemoveFrame( const index_t& index ) override{
    m_canvas.RemoveFrame(index);
  }

  void RemoveFrame( faint::Image* image ) override{
    m_canvas.RemoveFrame(image);
  }

  void ReorderFrame(const new_index_t& newIndex, const old_index_t& oldIndex) override{
    m_canvas.ReorderFrame(newIndex, oldIndex);
  }

  void SetBitmap( const faint::Bitmap& bmp ) override{
    m_frame->SetBitmap(bmp);
  }

  void SetFrameDelay( const index_t& index, const delay_t& delay ) override{
    faint::ImageList& images(m_canvas.GetImageList());
    assert(index < images.GetNumImages() );
    images.GetImage(index).SetDelay(delay);
  }

  void SetFrameHotSpot( const index_t& index, const IntPoint& hotSpot ) override{
    faint::ImageList& images(m_canvas.GetImageList());
    assert(index < images.GetNumImages() );
    images.GetImage(index).SetHotSpot(hotSpot);
  }

  void SetObjectZ( Object* obj, size_t z ) override{
    m_frame->SetObjectZ(obj, z);
  }

  void SetRasterSelection( const RasterSelectionState& state ) override{
    m_frame->GetRasterSelection().SetState(state);
  }

  void SetRasterSelectionOptions( const RasterSelectionOptions& options ) override{
    m_frame->GetRasterSelection().SetOptions(options);
  }

  // Non virtual
  void Reset(){
    delete m_dc;
    m_dc = nullptr;
  }

private:
  CanvasScroller& m_canvas;
  FaintDC* m_dc;
  faint::Image* m_frame;
};

static bool aint_active_canvas( CanvasScroller* canvas ){
  return GetAppContext().GetActiveCanvas().GetId() != canvas->GetCanvasId();
}

static Layer get_tool_layer( ToolId tool, Layer defaultLayer ){
  if ( raster_layer_only(tool) ){
    return Layer::RASTER;
  }
  else if ( object_layer_only(tool) ){
    return Layer::OBJECT;
  }
  return defaultLayer;
}

class CanvasToolInterface : public ToolInterface{
public:
  CanvasToolInterface( CanvasScroller* canvas )
    : m_canvas(canvas)
  {}
  bool Delete() override{
    if ( aint_active_canvas(m_canvas) ){
      return false;
    }
    Tool* tool = m_canvas->m_toolWrap.GetTool();
    if ( !tool->HasSelection() ){
      return false;
    }
    m_canvas->HandleToolResult(tool->Delete());
    return true;
  }

  bool Deselect() override{
    if ( aint_active_canvas(m_canvas) ){
      return false;
    }
    Tool* tool = m_canvas->m_toolWrap.GetTool();
    if ( !tool->HasSelection() ){
      return false;
    }
    m_canvas->HandleToolResult(tool->Deselect());
    return true;
  }

  bool SelectAll() override{
    if ( aint_active_canvas(m_canvas) ){
      return false;
    }
    Tool* tool = m_canvas->m_toolWrap.GetTool();
    if ( !tool->HasSelection() ){
      return false;
    }
    // Fixme: Consider always returning true, not sure..
    // This determines if context commands will stop even if
    // the tool didn't do anything
    return m_canvas->HandleToolResult(tool->SelectAll());
  }

  Layer GetLayerType() const override{
    ToolId tool = m_canvas->m_toolWrap.GetToolId();
    Layer layer = GetAppContext().GetLayerType();
    return get_tool_layer(tool, layer);
  }
private:
  CanvasScroller* m_canvas;
};

class CanvasContext : public CanvasInterface {
public:
  CanvasContext( CanvasScroller* canvas ) :
    m_canvas( canvas ),
    m_toolInterface( canvas )
  {}

  void CenterView( const Point& pt ) override{
    m_canvas->CenterViewImage( pt );
  }

  void ClearPointOverlay() override{
    m_canvas->ClearPointOverlay();
  }

  void ClearRasterSelectionChimera() override{
    m_canvas->ClearRasterSelectionChimera();
  }

  void CloseUndoBundle() override{
    m_canvas->RunCommand(close_python_command_list());
    m_canvas->UnwrapLast();
  }

  void ContextDelete() override{
    m_canvas->DeleteSelection();
  }

  void CopySelection() override{
    m_canvas->CopySelection();
  }

  void CutSelection() override{
    m_canvas->CutSelection();
  }

  void DeselectObject( Object* obj ) override{
    m_canvas->DeselectObject( obj );
  }

  void DeselectObjects() override{
    m_canvas->DeselectObjects();
  };

  void DeselectObjects(const objects_t& objs) override{
    m_canvas->DeselectObjects(objs);
  }

  const faint::Bitmap& GetBitmap() override{
    return m_canvas->GetImage().GetBitmap();
  }

  Optional<faint::FilePath> GetFilePath() const override{
    return m_canvas->GetFilePath();
  }

  const faint::Image& GetFrame(const index_t& index) override{
    return m_canvas->GetImageList().GetImage(index);
  }

  faint::Image& GetFrameById(const FrameId& id) override{
    return m_canvas->GetImageList().GetImageById(id);
  }

  index_t GetFrameIndex(const faint::Image& image) const override{
    return m_canvas->GetImageList().GetIndex(image);
  }

  Grid GetGrid() const override{
    return m_canvas->GetGrid();
  }

  CanvasId GetId() const override{
    return m_canvas->GetCanvasId();
  }

  const faint::Image& GetImage() override{
    return m_canvas->GetImage();
  }

  IntPoint GetMaxScrollPos() override{
    return m_canvas->GetMaxUsefulScroll();
  }

  index_t GetNumFrames() const override{
    return m_canvas->GetImageList().GetNumImages();
  }

  const objects_t& GetObjects() override{
    return m_canvas->GetObjects();
  }

  CursorPositionInfo GetPosInfo( const IntPoint& pos ) override{
    return m_canvas->ExternalHitTest(pos);
  }

  const RasterSelection& GetRasterSelection() override{
    return m_canvas->GetRasterSelection();
  }

  Point GetRelativeMousePos() override{
    return m_canvas->GetRelativeMousePos();
  }

  IntPoint GetScrollPos() override{
    return m_canvas->GetFaintScrollPos();
  }

  index_t GetSelectedFrame() const override{
    return m_canvas->GetSelectedFrame();
  }

  const objects_t& GetObjectSelection() override{
    return m_canvas->GetObjectSelection();
  }

  IntSize GetSize() const override{
    return m_canvas->GetBitmapSize();
  }

  faint::Bitmap GetSubBitmap( const IntRect& r ) const override{
    return m_canvas->GetSubBitmap( r );
  }

  ToolInterface& GetTool() override{
    return m_toolInterface;
  }

  faint::coord GetZoom() const override{
    return m_canvas->GetZoom();
  }

  ZoomLevel GetZoomLevel() const override{
    return m_canvas->GetZoomLevel();
  }

  bool Has( const ObjectId& id ) const override{
    return m_canvas->Has( id );
  }

  bool Has( const FrameId& id ) const override{
    return m_canvas->GetImageList().Has( id );
  }

  bool IsSelected( const Object* obj ) const override{
    return contains(m_canvas->GetObjectSelection(), obj);
  }

  void NextFrame() override{
    m_canvas->NextFrame();
  }

  void NotifySaved( const faint::FilePath& filePath ) override{
    m_canvas->NotifySaved( filePath );
  }

  void OpenUndoBundle() override{
    RunCommand(open_python_command_list());
  }

  void Paste() override{
    m_canvas->Paste();
  }

  void PreviousFrame() override{
    m_canvas->PreviousFrame();
  }

  void Redo() override{
    m_canvas->Redo();
  }

  void Refresh() override{
    m_canvas->Refresh();
  }

  void RunCommand( Command* cmd ) override{
    if ( cmd != nullptr ){
      // Preempt so that for example(!) a move-object tool is
      // preempted before the moved objects are removed.  This is a
      // bit harsh, it should be possible to trigger commands without
      // disabling tools, but this requires more advanced handling,
      // e.g. checking if the tool is disturbed by the effects of the
      // command.
      m_canvas->Preempt(PreemptOption::ALLOW_COMMAND);
      m_canvas->RunCommand( cmd );
    }
  }

  void RunCommand( Command* cmd, const FrameId& id ) override{
    if ( cmd == nullptr ){
      return;
    }
    // Preempt so that for example(!) a move-object tool is
    // preempted before the moved objects are removed.  This is a
    // bit harsh, it should be possible to trigger commands without
    // disabling tools, but this requires more advanced handling,
    // e.g. checking if the tool is disturbed by the effects of the
    // command.
    m_canvas->Preempt(PreemptOption::ALLOW_COMMAND);
    m_canvas->RunCommand(cmd, id);
  }

  void RunDWIM() override{
    m_canvas->RunDWIM();
  }

  void ScrollMaxDown() override{
    m_canvas->ScrollMaxDown();
  }

  void ScrollMaxLeft() override{
    m_canvas->ScrollMaxLeft();
  }

  void ScrollMaxRight() override{
    m_canvas->ScrollMaxRight();
  }

  void ScrollMaxUp() override{
    m_canvas->ScrollMaxUp();
  }

  void ScrollPageDown() override{
    m_canvas->ScrollPageDown();
  }

  void ScrollPageLeft() override{
    m_canvas->ScrollPageLeft();
  }

  void ScrollPageRight() override{
    m_canvas->ScrollPageRight();
  }

  void ScrollPageUp() override{
    m_canvas->ScrollPageUp();
  }

  void SelectFrame( const index_t& index ) override{
    m_canvas->SelectFrame(index);
  }

  void SelectObject( Object* obj, const deselect_old& deselectOld ) override{
    m_canvas->SelectObject( obj, deselectOld );
  }

  void SelectObjects( const objects_t& objs, const deselect_old& deselectOld ) override{
    m_canvas->SelectObjects( objs, deselectOld );
  }

  void SetChimera( RasterSelection* selection) override{
    m_canvas->SetChimera(selection);
  }

  void SetGrid( const Grid& g ) override{
    m_canvas->SetGrid(g);
  }

  void SetPointOverlay( const IntPoint& p ) override{
    m_canvas->SetPointOverlay(p);
  }

  void SetScrollPos( const IntPoint& p ) override{
    m_canvas->SetFaintScrollPos(p);
  }

  void SetZoom( const ZoomLevel& zoom ) override{
    m_canvas->SetZoomLevel( zoom );
  }

  void Undo() override{
    m_canvas->Undo();
  }

  void ZoomDefault() override{
    m_canvas->ChangeZoom( ZoomLevel::DEFAULT );
  }

  void ZoomFit() override{
    m_canvas->ZoomFit();
  }

  void ZoomIn() override{
    m_canvas->ChangeZoom( ZoomLevel::NEXT );
  }

  void ZoomOut() override{
    m_canvas->ChangeZoom( ZoomLevel::PREVIOUS );
  }
private:
  CanvasScroller* m_canvas;
  CanvasToolInterface m_toolInterface;
};

// Returns the latest command that modified the image state (or an
// unset Optional if there's no modifying command in the list)
static Optional<CommandId> get_last_modifying( const std::deque<OldCommand>& commands ){
  for ( const OldCommand& item : reversed(commands) ){
    Command* cmd = item.command;
    if ( cmd->ModifiesState() ){
      return option(cmd->GetId());
    }
  }
  return no_option();
}

static wxBitmap& get_back_buffer( const IntSize& minSize ){
  static wxBitmap backBuffer( minSize.w,minSize.h );
  wxSize oldSize = backBuffer.GetSize();
  if ( oldSize.GetWidth() < minSize.w || oldSize.GetHeight() < minSize.h ){
    backBuffer = wxBitmap( std::max( minSize.w, oldSize.GetWidth() ),
      std::max( minSize.h, oldSize.GetHeight() ) );
  }
  return backBuffer;
}

static void paint_object_handles( const objects_t& objects, OverlayDC& dc ){
  for ( const Object* obj : objects ){
    // Draw handles for objects, excluding currently adjusted objects
    // so the handles don't get in the way
    if ( obj->Inactive() && resize_handles_enabled(obj) ){
      if ( object_aligned_resize(obj) ){
        dc.Handles( obj->GetTri() );
      }
      else{
        dc.Handles( obj->GetRect() );
      }
    }
  }
}

static int point_hit_test( const std::vector<Point>& points, const wxPoint& pos, const Geometry& g ){
  for ( size_t i = 0; i != points.size(); i++ ){
    const Point& ptHandle = points[i];
    const faint::coord zoom = g.zoom.GetScaleFactor();
    Point topLeft( ptHandle.x * zoom - faint::objectHandleWidth / 2, ptHandle.y * zoom - faint::objectHandleWidth/ 2 );
    Rect r( topLeft, Size(faint::objectHandleWidth, faint::objectHandleWidth ) );
    if ( r.Contains( Point( pos.x - g.left_border + g.x0, pos.y - g.top_border + g.y0 ) ) ){
      return i;
    }
  }
  return -1;
}

static int get_side_index( const std::vector<Point>& points, const Point& pos, const Geometry& g ){
  if ( points.size() != 4 ){
    return -1;
  }

  faint::coord zoom = g.zoom.GetScaleFactor();

  // Fixme: Need better handle handling.
  Point prev = points.front();
  faint::coord minDistance = 5;
  int nearestIndex = -1;

  // Left side
  bool withinTopBottom = ( side(points[0]*zoom, points[1]*zoom, pos) != side(points[2] * zoom, points[3] * zoom, pos) );
  if (withinTopBottom){
    faint::coord dist = distance(pos, unbounded(LineSegment(points[0]*zoom, points[2]*zoom)));
    if ( dist < minDistance ){
      minDistance = dist;
      nearestIndex = 4;
    }
    // Right side
    dist = distance(pos, unbounded(LineSegment(points[1]*zoom, points[3]*zoom)));
    if ( dist < minDistance ){
      minDistance = dist;
      nearestIndex = 5;
    }
  }

  bool withinLeftRight = ( side(points[0]*zoom, points[2]*zoom, pos) != side(points[1]*zoom, points[3]*zoom, pos) );
  if (withinLeftRight){
    faint::coord dist = distance(pos, unbounded(LineSegment(points[0]*zoom, points[1]*zoom)));
    if ( dist < minDistance ){
      minDistance = dist;
      nearestIndex = 6;
    }

    dist = distance(pos, unbounded(LineSegment(points[2]*zoom, points[3]*zoom)));
    if ( dist < minDistance ){
      minDistance = dist;
      nearestIndex = 7;
    }
  }
  return nearestIndex;
}

static int get_handle_index( const std::vector<Point>& points, const wxPoint& pos, const Geometry& g ){
  Point p2( pos.x - g.left_border + g.x0, pos.y - g.top_border + g.y0 );
  const faint::coord zoom = g.zoom.GetScaleFactor();
  for ( size_t i = 0; i != points.size(); i++ ){
    const Point& ptHandle = points[i];
    Point topLeft( ptHandle.x * zoom - faint::objectHandleWidth / 2, ptHandle.y * zoom - faint::objectHandleWidth/ 2 );
    Rect r( topLeft, Size(faint::objectHandleWidth, faint::objectHandleWidth ) );
    if ( r.Contains( p2 ) ){
      return i;
    }
  }

  // Not near a handle, check if near a side
  return get_side_index(points, p2, g);
}

static void clear_list( std::deque<OldCommand>& list ){
  for ( auto& item : list ){
    delete item.command;
  }
  list.clear();
}

static void paint_canvas_handles( wxDC& dc, const IntSize& canvasSize, const Geometry& geo ){
  dc.SetPen( wxPen(wxColour(128, 128, 128), 1 ) );
  dc.SetBrush( wxBrush(wxColour(181, 165, 213) ) );

  for ( HandlePos handlePos : iterable<HandlePos>() ){
    CanvasResizeHandle handle(handlePos, canvasSize, geo);
    dc.DrawRectangle( to_wx(handle.GetRect() ) );
  }
}

static Rect Paint_InImageCoordinates( const Rect& rView, const Geometry& geo ){
  Rect rImageCoord( Point(rView.x + geo.x0 - geo.left_border, rView.y + geo.y0 - geo.top_border), Size(rView.w + geo.left_border, rView.h + geo.top_border ));
  const faint::coord zoom = geo.zoom.GetScaleFactor();
  rImageCoord.x /= zoom;
  rImageCoord.y /= zoom;
  rImageCoord.w = rImageCoord.w / zoom;
  rImageCoord.h = rImageCoord.h / zoom;
  return rImageCoord;
}

static void Paint_EarlyReturn( wxDC& dc, const IntRect& rView, const Geometry& geo, const IntSize& imageSize ){
  dc.SetBrush( wxBrush( g_canvasBg ) );
  dc.SetPen( wxPen( g_canvasBg ) );
  dc.DrawRectangle( rView.x, rView.y, rView.w, rView.h );
  paint_canvas_handles( dc, imageSize, geo );
}

static void Paint_CheckeredBackground( wxDC& dc, const IntPoint& topLeft, const IntPoint& imageRegionTopLeft, const Geometry& geo, int w, int h  ){
  const int tileSz = 20;
  const wxColor c1(192,192,192);
  const wxColor c2(255,255,255);
  const faint::coord zoom = geo.zoom.GetScaleFactor();
  int xOff = truncated( imageRegionTopLeft.x * zoom ) % (tileSz * 2);
  int yOff = truncated( imageRegionTopLeft.y * zoom ) % (tileSz * 2);
  const int row_max = h / tileSz + 2;
  const int col_max = w / (tileSz * 2) + 2;
  for ( int row = 0; row <= row_max ; row++ ){
    dc.SetBrush(wxBrush(c1));
    dc.SetPen(wxPen(c1));
    int shift = ( row % 2 ) * tileSz;
    for ( int col = 0; col <= col_max; col++ ){
      dc.DrawRectangle( 0 - xOff + col * 40 + shift + topLeft.x,
        0 - yOff + row * 20 + topLeft.y,
        tileSz,
        tileSz );
    }
    dc.SetBrush(wxBrush(c2));
    dc.SetPen(wxPen(c2));
    for ( int col = 0; col <= col_max; col++ ){
      dc.DrawRectangle( 0 - xOff + col * 40 - 20 + shift + topLeft.x, 0 - yOff + row * 20 + topLeft.y, tileSz, tileSz );
    }
  }
}

static void Paint_FillBackground( wxDC& dc, const IntPoint& topLeft, const IntSize& sz, const wxColour& bg ){
  dc.SetBrush(wxBrush(bg));
  dc.SetPen(wxPen(bg));
  dc.DrawRectangle( topLeft.x, topLeft.y, sz.w, sz.h );
}

static IntRect Paint_GetImageRegion( Rect rView, IntSize bmpSize, const Geometry& geo ){
  const faint::coord zoom = geo.zoom.GetScaleFactor();
  Rect rImage( Point(rView.x + geo.x0 - geo.left_border, rView.y + geo.y0 - geo.top_border), Size(rView.w, rView.h) );
  rImage.x /= zoom;
  rImage.y /= zoom;
  rImage.w = rImage.w / zoom + 2;
  rImage.h = rImage.h / zoom + 2;
  rImage = intersection( rImage, Rect( Point(0, 0), floated(bmpSize) ) );
  int dbgX0( rounded_down(rImage.x) );
  int dbgY0( rounded_down(rImage.y) );
  dbgX0 = std::max( 0, dbgX0 );
  dbgY0 = std::max( 0, dbgY0 );
  int dbgW(truncated(rImage.w + 5));
  int dbgH(truncated(rImage.h + 5));
  dbgW = std::min( dbgW, bmpSize.w - dbgX0 );
  dbgH = std::min( dbgH, bmpSize.h - dbgY0 );
  return IntRect( IntPoint(dbgX0, dbgY0), IntSize( dbgW, dbgH ) );
}

class CanvasFileDropTarget : public wxFileDropTarget {
public:
  bool OnDropFiles( wxCoord /*x*/, wxCoord /*y*/, const wxArrayString& files) override{
    GetAppContext().Load(to_FileList(files));
    return true;
  }
};

CanvasScroller::CanvasScroller( wxWindow* parent, ImageProps& props, const initially_dirty& startDirty )
  : wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL|wxHSCROLL | wxWANTS_CHARS ),
    m_cmdContext(nullptr),
    m_images(props),
    m_hitTestBuffer(faint::Bitmap(IntSize(20, 20)))
{
  Initialize(startDirty);
  m_grid = GetAppContext().GetDefaultGrid();
}

CanvasScroller::CanvasScroller( wxWindow* parent, std::vector<ImageProps>& props, const initially_dirty& startDirty )
  : wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL|wxHSCROLL | wxWANTS_CHARS ),
    m_cmdContext(nullptr),
    m_images(props),
    m_hitTestBuffer(IntSize(20, 20))
{
  Initialize(startDirty);
  m_grid = GetAppContext().GetDefaultGrid();
}

CanvasScroller::~CanvasScroller(){
  if ( HasCapture() ){
    ReleaseMouse();
  }

  clear_list( m_redoList );
  clear_list( m_undoList );
  delete m_interface;
  delete m_cmdContext;
}

void CanvasScroller::AddFrame( faint::Image* image ){
  m_images.Add(image);
}

void CanvasScroller::AddFrame( faint::Image* image, const index_t& index ){
  m_images.Insert(image, index);
}

bool CanvasScroller::AcceptsFocus() const{
  return true;
}

void CanvasScroller::AdjustHorizontalScrollbar( int pos ){
  m_geo.x0 = pos;
  pos = std::max(0,pos);
  m_scroll.startX = m_geo.x0 < 0 ? m_geo.x0 : 0;
  SetScrollbar( wxHORIZONTAL, // Orientation
    pos, // Position
    GetHorizontalPageSize(), // Thumb size
    std::max( GetMaxScrollRight(), pos + GetHorizontalPageSize() - m_scroll.startX ), // Range
    true ); // Refresh
}

void CanvasScroller::AdjustScrollbars( const IntPoint& p ){
  AdjustHorizontalScrollbar( p.x );
  AdjustVerticalScrollbar( p.y );
}

void CanvasScroller::AdjustVerticalScrollbar( int pos ){
  m_geo.y0 = pos;
  pos = std::max(0,pos);
  m_scroll.startY = m_geo.y0 < 0 ? m_geo.y0 : 0;
  SetScrollbar( wxVERTICAL, // Orientation
    pos, // Position
    GetVerticalPageSize(), // Thumb size
    std::max( GetMaxScrollDown(), pos + GetVerticalPageSize() - m_scroll.startY ) , // Range
    true ); // Refresh
}

bool CanvasScroller::CanRedo() const{
  const Tool& tool = m_toolWrap.ToolRef();
  return tool.CanRedo() ||
    ( tool.AllowsGlobalRedo() && !m_redoList.empty() );
}

bool CanvasScroller::CanUndo() const{
  return !m_undoList.empty() || m_toolWrap.ToolRef().CanUndo();
}

void CanvasScroller::CenterView( const IntPoint& ptView ){
  wxSize sz = GetClientSize();
  int x0 = ptView.x + m_geo.x0 - sz.GetWidth() / 2;
  int y0 = ptView.y + m_geo.y0 - sz.GetHeight() / 2;
  if ( x0 == m_geo.x0 && y0 == m_geo.y0 ){
    // Do nothing if the ptView refers to the currently centered image point, as
    // this would make it difficult to move the cursor while
    // continuously centering at large zoom.
    return;
  }
  m_geo.x0 = x0;
  m_geo.y0 = y0;
  AdjustScrollbars( IntPoint(m_geo.x0, m_geo.y0) );
  WarpPointer( sz.GetWidth() / 2, sz.GetHeight() / 2 );
  MousePosRefresh();
  Refresh();
}

void CanvasScroller::CenterViewImage( const Point& ptImage ){
  CenterView( faint::mouse::image_to_view_tr( ptImage, m_geo ) );
}

void CanvasScroller::ChangeZoom( ZoomLevel::ChangeType changeType ){
  faint::coord oldScale = m_geo.zoom.GetScaleFactor();
  m_geo.zoom.Change( changeType );
  faint::coord newScale = m_geo.zoom.GetScaleFactor();
  m_geo.x0 = truncated(m_geo.x0 / ( oldScale / newScale ));
  m_geo.y0 = truncated(m_geo.y0 / ( oldScale / newScale ));
  AdjustScrollbars( IntPoint(m_geo.x0, m_geo.y0) );
  MousePosRefresh();
  GetAppContext().OnZoomChange();
  Refresh();
}

void CanvasScroller::ClearPointOverlay(){
  m_pointOverlay.Clear();
}

bool CanvasScroller::CopySelection(){
  bool cut = false;
  return SelectionToClipboard(cut);
}

void CanvasScroller::ClearRasterSelectionChimera(){
  m_rasterSelectionChimera = nullptr;
}

void CanvasScroller::CutSelection(){
  bool cut = true;
  SelectionToClipboard(cut);
}

void CanvasScroller::DeleteSelection(){
  // Todo: Extract to context_commands, but figure out a way to
  // handle dwim-delete and bg-col first.
  // Fixme: Must check if tool is active for current canvas
  if ( m_toolWrap.GetTool()->HasSelection() ){
    HandleToolResult( m_toolWrap.GetTool()->Delete() );
    return;
  }

  if ( ShouldDrawVector() ){
    if ( !HasObjectSelection() ){
      return;
    }
    faint::Image& active(m_images.Active());
    RunCommand( get_delete_objects_command( active.GetObjectSelection(), active ) );
  }
  else if ( HasRasterSelection() ){
    Command* cmd = get_delete_raster_selection_command(m_images.Active(),
      GetAppContext().GetToolSettings().Get(ts_BgCol),
      m_dwimDelete );
    RunCommand(cmd);
  }
  Refresh();
}

void CanvasScroller::DeselectObject( Object* obj ){
  DeselectObjects(as_list(obj));
}

void CanvasScroller::DeselectObjects(){
  m_images.Active().DeselectObjects();
  m_toolWrap.GetTool()->SelectionChange();
  GetAppContext().OnDocumentStateChange(m_canvasId);
}

void CanvasScroller::DeselectObjects( const objects_t& objects ){
  bool deselected = m_images.Active().Deselect(objects);
  if ( !deselected ){
    return;
  }
  m_toolWrap.GetTool()->SelectionChange();

  if ( !HasObjectSelection() ){
    GetAppContext().OnDocumentStateChange(m_canvasId);
  }
}

void CanvasScroller::DeselectRaster(){
  m_lastRefreshRect = union_of( m_lastRefreshRect, faint::mouse::image_to_view(GetImageSelection().GetRect(), m_geo)); // Fixme: Tricky stuff, add to command?
  RunCommand(get_deselect_raster_command(GetImageSelection()));
  Refresh();
}

CursorPositionInfo CanvasScroller::ExternalHitTest( const IntPoint& ptImage ){
  IntPoint ptView = faint::mouse::image_to_view(ptImage, m_geo);
  return HitTest(to_wx(ptView), get_tool_modifiers());
}

void CanvasScroller::ExternalRefresh(){
  Refresh();
}

IntPoint CanvasScroller::GetFaintScrollPos() const{
  return IntPoint(m_geo.x0, m_geo.y0);
}

faint::Bitmap CanvasScroller::GetSubBitmap( const IntRect& rect ) const{
  return sub_bitmap(m_images.Active().GetBitmap(), rect);
}

IntSize CanvasScroller::GetBitmapSize() const {
  return m_images.Active().GetSize();
}

CanvasId CanvasScroller::GetCanvasId() const{
  return m_canvasId;
}

Optional<faint::FilePath> CanvasScroller::GetFilePath() const{
  return m_filename;
}

Grid CanvasScroller::GetGrid() const{
  return m_grid;
}

faint::Image& CanvasScroller::GetImage(){
  return m_images.Active();
}

faint::ImageList& CanvasScroller::GetImageList(){
  return m_images;
}

const faint::ImageList& CanvasScroller::GetImageList() const{
  return m_images;
}

Point CanvasScroller::GetImageViewStart() const{
  const faint::coord zoom = m_geo.zoom.GetScaleFactor();
  return Point( std::max( ( m_geo.x0 - m_geo.left_border ) / zoom, LITCRD(0.0) ),
    std::max( ( m_geo.y0 - m_geo.top_border ) / zoom, LITCRD(0.0) ) );
}

RasterSelection& CanvasScroller::GetImageSelection(){
  return m_images.Active().GetRasterSelection();
}

const RasterSelection& CanvasScroller::GetImageSelection() const{
  return m_images.Active().GetRasterSelection();
}

CanvasInterface& CanvasScroller::GetInterface(){
  return *m_interface;
}

int CanvasScroller::GetMaxScrollRight() const{
  return floored(std::max( LITCRD(0.0), m_images.Active().GetSize().w * m_geo.zoom.GetScaleFactor() ));
}

int CanvasScroller::GetMaxScrollDown() const{
  return floored(std::max( LITCRD(0.0), m_images.Active().GetSize().h * m_geo.zoom.GetScaleFactor() ));
}

IntPoint CanvasScroller::GetMaxUsefulScroll() const{
  return IntPoint(GetMaxScrollRight(), GetMaxScrollDown()) -
    IntPoint(GetHorizontalPageSize(), GetVerticalPageSize());
}

faint::MenuFlags CanvasScroller::GetMenuFlags() const{
  const faint::Image& active(m_images.Active());
  faint::MenuFlags fl;
  fl.toolSelection = HasToolSelection();
  fl.rasterSelection = ShouldDrawRaster() && HasRasterSelection();
  fl.objectSelection = ShouldDrawVector() && HasObjectSelection();
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

const objects_t& CanvasScroller::GetObjects(){
  return m_images.Active().GetObjects();
}

RasterSelection& CanvasScroller::GetRasterSelection(){
  return GetImageSelection();
}

std::string CanvasScroller::GetRedoName() const{
  const Tool& tool = m_toolWrap.ToolRef();
  if ( tool.CanRedo() ){
    return tool.GetRedoName();
  }
  else if ( tool.PreventsGlobalRedo() ){
    return "";
  }
  return m_redoList.empty() ? "" : m_redoList.front().command->Name();
}

Point CanvasScroller::GetRelativeMousePos(){
  return faint::mouse::image_position( m_geo, *this );
}

index_t CanvasScroller::GetSelectedFrame() const{
  return m_images.GetSelectedIndex();
}

const objects_t& CanvasScroller::GetObjectSelection(){
  return m_images.Active().GetObjectSelection();
}

std::string CanvasScroller::GetUndoName() const{
  const Tool& tool = m_toolWrap.ToolRef();
  if ( tool.CanUndo() ){
    return tool.GetUndoName();
  }
  return m_undoList.empty() ? "" : m_undoList.back().command->Name();
}

faint::coord CanvasScroller::GetZoom() const{
  return m_geo.zoom.GetScaleFactor();
}

const ZoomLevel& CanvasScroller::GetZoomLevel() const{
  return m_geo.zoom;
}

bool CanvasScroller::Has( const ObjectId& id ){
  const objects_t& objects(m_images.Active().GetObjects());
  for ( const Object* obj : objects ){
    if ( is_or_has( obj, id ) ){
      return true;
    }
  }
  return false;
}

bool CanvasScroller::HasObjectSelection() const{
  return !m_images.Active().GetObjectSelection().empty();
}

bool CanvasScroller::HasRasterSelection() const{
  return GetImageSelection().Exists();
}

bool CanvasScroller::HasSelection() const{
  return HasRasterSelection() || HasObjectSelection();
}

bool CanvasScroller::HasToolSelection() const{
  return m_toolWrap.ToolRef().HasSelection();
}

bool CanvasScroller::InSelection( const Point& p ){
  return GetImageSelection().Contains( floored(p) );
}

bool CanvasScroller::IsDirty() const{
  const Optional<CommandId> lastModifying( get_last_modifying(m_undoList) );
  if ( lastModifying.NotSet() ){
    // No remaining undoable commands modified the image.
    // If the image was saved during this session, we've undone past
    // that point (and the image is dirty).
    // If the image has not been saved, we've undone back to the load
    // state, and the image is not dirty.
    return m_savedAfter.IsSet();
  }

  // There are undoable modifying commands, and the image hasn't been
  // saved during this session or was saved earlier or later.
  return m_savedAfter.NotSet() || ( lastModifying != m_savedAfter.Get() );
}

void CanvasScroller::MousePosRefresh(){
  MousePosRefresh( to_wx( faint::mouse::view_position(*this)), get_tool_modifiers() );
}

void CanvasScroller::OnCaptureLost( wxMouseCaptureLostEvent& ){
  Preempt(PreemptOption::ALLOW_COMMAND);
}

void CanvasScroller::OnChar( wxKeyEvent& event ){
  KeyInfo info = {
    &GetInterface(),
    &GetAppContext().GetStatusInfo(),
    GetAppContext().GetLayerType(),
    to_faint(event.GetUnicodeKey()),
    event.GetKeyCode(),
    event.GetModifiers()
  };

  if ( !key::alt_held(info.keyModifiers) ){
    if ( HandleToolResult( m_toolWrap.GetTool()->Char(info) ) ){
      // The tool ate the character.
      return;
    }
  }
  // Key ignored by tool, or Alt held. Allow normal key handling.
  event.Skip();
}

void CanvasScroller::OnDoubleClick( wxMouseEvent& evt ){
  CursorPositionInfo info = HitTest( evt.GetPosition(), mouse_modifiers(evt) );
  HandleToolResult(m_toolWrap.GetTool()->LeftDoubleClick( info ));
}

void CanvasScroller::OnEraseBackground( wxEraseEvent& ){
  // This function intentionally left blank
}

void CanvasScroller::OnKeyDown( wxKeyEvent& event ){
  int keycode = event.GetKeyCode();
  if ( is_tool_modifier( keycode ) ){
    // Update the tool state if a modifier is pressed (e.g. for cursor update)
    // Note: This repeats while the key is held.
    MousePosRefresh();
  }
  else {
    event.Skip();
  }
}

void CanvasScroller::OnKeyUp( wxKeyEvent& event ){
  int keycode = event.GetKeyCode();
  if ( is_tool_modifier( keycode ) ){
    // Update the tool state if a modifier is released (e.g. for cursor update)
    // Note: This repeates while the key is held.
    MousePosRefresh();
    return;
  }
  event.Skip();
}

void CanvasScroller::OnLeftDown( wxMouseEvent& evt ){
  SetFocus();
  wxPoint wx_mousePos( evt.GetPosition() );
  CursorPositionInfo info( HitTest( wx_mousePos, mouse_modifiers(evt) ) );
  CaptureMouse();

  if ( !IgnoreCanvasHandle(info) ){
    // Check if a canvas handle was clicked, and if so switch to the
    // canvas resize tool
    IntSize imageSize( m_images.Active().GetSize() );
    Optional<CanvasResizeHandle> canvasHandle = canvas_handle_hit_test( to_faint(wx_mousePos), imageSize, m_geo );
    if ( canvasHandle.IsSet()){
      Tool* resizeTool = new ResizeCanvas( canvasHandle.Get(),
        evt.GetButton() == wxMOUSE_BTN_LEFT ?
          ResizeCanvas::Resize : ResizeCanvas::Rescale );
      resizeTool->ToolSettingUpdate(GetAppContext().GetToolSettings());
      m_toolWrap.SetSwitched( resizeTool );
      SetFaintCursor( canvasHandle.Get().GetCursor() );
      return;
    }
  }
  HandleToolResult( m_toolWrap.GetTool()->LeftDown( info ) );
  SetFaintCursor( m_toolWrap.GetTool()->GetCursor( info ) );
  GetAppContext().OnDocumentStateChange(m_canvasId); // Fixme: For tool-undo
}

void CanvasScroller::OnLeftUp( wxMouseEvent& evt ){
  if ( HasCapture() ){
    ReleaseMouse();
  }
  SetFocus();
  CursorPositionInfo info = HitTest( evt.GetPosition(), mouse_modifiers( evt ) );
  HandleToolResult( m_toolWrap.GetTool()->LeftUp( info ) );
}

void CanvasScroller::OnMotion( wxMouseEvent& evt ){
  MousePosRefresh( evt.GetPosition(), mouse_modifiers( evt ) );
}

void CanvasScroller::OnMouseOut( wxMouseEvent& ){
  // The cursor has left the canvas (the drawing area + the border)
  // onto a different control or outside the window. Tools may need to
  // be informed so they don't draw for example a brush cursor.
  if ( m_toolWrap.GetTool()->RefreshOnMouseOut() ){
    RefreshToolRect();
  }
  GetAppContext().GetStatusInfo().Clear();
}

void CanvasScroller::OnMouseWheel( wxMouseEvent& evt ){
  int rot = evt.GetWheelRotation();
  if ( rot > 0 ){
    if ( evt.ControlDown() ){
      ScrollLineLeft();
    }
    else {
      ScrollLineUp();
    }
  }
  else if ( rot < 0 ){
    if ( evt.ControlDown() ){
      ScrollLineRight();
    }
    else {
      ScrollLineDown();
    }
  }
}

void CanvasScroller::OnPaint( wxPaintEvent& ){
  wxPaintDC paintDC( this );
  Rect rView = floated(to_faint( GetUpdateRegion().GetBox() ));
  faint::Image& active(m_images.Active());

  // Use the bitmap from the active frame, or the chimera if it is set (feedback from some operation, e.g.
  // the brightness/contrast dialog)
  const faint::Bitmap& faint_bmp = m_bitmapChimera == nullptr ?
    m_images.Active().GetBitmap() : *m_bitmapChimera;
  IntSize bmpSize( faint_bmp.GetSize() );
  IntRect imageRegion = Paint_GetImageRegion( rView, bmpSize, m_geo );
  Rect imageCoordRect = Paint_InImageCoordinates( rView, m_geo );
  if ( empty( imageRegion ) ){
    // Fixme: rView should be IntRect?
    Paint_EarlyReturn( paintDC, floored(rView), m_geo, active.GetSize() );
    return;
  }

  const Point mouseImagePos = faint::mouse::image_position(m_geo, *this);
  RasterSelection& rasterSelection( m_rasterSelectionChimera == nullptr ?
    GetImageSelection() : *m_rasterSelectionChimera );

  faint::Bitmap subBitmap(sub_bitmap(faint_bmp, imageRegion ));
  Overlays overlays;

  const objects_t& tempSel = active.GetObjectSelection();
  if ( tempSel.size() == 1 && is_raster_object(tempSel.front()) ){
    ObjRaster* rasterObj = as_ObjRaster(tempSel.front());
    rasterObj->Reset();
    faint::Bitmap& objBitmap = rasterObj->GetBitmap();
    FaintDC dc( objBitmap, origin_t(-rasterObj->GetRect().TopLeft()));
    m_toolWrap.GetTool()->Draw( dc, overlays, mouseImagePos  );
    rasterObj->Update();
  }
  else{
    bool toolBeforeZoom = m_toolWrap.DrawBeforeZoom();
    if ( toolBeforeZoom || rasterSelection.Floating() ){
      FaintDC dc ( subBitmap,
        origin_t( -floated( imageRegion.TopLeft() )) );
      if ( toolBeforeZoom ){
        m_toolWrap.GetTool()->Draw( dc, overlays, mouseImagePos );
      }
      if ( rasterSelection.Floating() ){
        rasterSelection.DrawFloating(dc);
      }
    }
  }

  const faint::coord zoom = m_geo.zoom.GetScaleFactor();
  faint::Bitmap scaled = m_geo.zoom.At100() ?
    subBitmap : ( zoom > LITCRD(1.0) ?
      scale_nearest( subBitmap, rounded(zoom) ):
      scale_bilinear( subBitmap, Scale(zoom) ) );

  if ( scaled.m_w <= 0 || scaled.m_h <= 0 ){
    Paint_EarlyReturn( paintDC, floored(rView), m_geo, active.GetSize() );
    return;
  }

  // Top left of image in view-coordinates
  IntPoint topLeft( faint::mouse::image_to_view_tr( floated(imageRegion.TopLeft()), m_geo ) );

  FaintDC dc( scaled );
  dc.SetOrigin( -imageRegion.TopLeft() * zoom );
  dc.SetScale( zoom );
  const objects_t& objects(active.GetObjects());
  for ( Object* object : objects ){
    if ( object->Visible() ){
      object->Draw( dc );
    }
  }

  if ( !m_toolWrap.DrawBeforeZoom() ){
    m_toolWrap.GetTool()->Draw( dc, overlays, mouseImagePos );
  }

  wxBitmap& backBuffer = get_back_buffer( truncated( rView.GetSize() ) );

  // Draw the image and any overlays to the back-buffer to avoid
  // flickering
  wxMemoryDC memDC(backBuffer);
  // Fixme: Consider drawing filled rectangles with canvasbg instead,
  // to clear less.
  memDC.SetBackground(wxBrush( g_canvasBg ));
  memDC.Clear();

  IntRect clipRect( floored(Point(topLeft.x - rView.x, topLeft.y - rView.y)), scaled.GetSize() );
  memDC.SetClippingRegion( to_wx(clipRect) );

  TransparencyStyle trStyle = GetAppContext().GetTransparencyStyle();
  if ( trStyle.IsCheckered() ){
    Paint_CheckeredBackground( memDC, floored(Point( topLeft.x - rView.x,
    topLeft.y - rView.y)), imageRegion.TopLeft(), m_geo, scaled.m_w, scaled.m_h );
  }
  else{
    Paint_FillBackground( memDC, floored(Point( topLeft.x - rView.x, topLeft.y - rView.y )), scaled.GetSize(), to_wx(trStyle.GetColor()) );
  }
  memDC.DestroyClippingRegion();

  // Draw the scaled part of the bitmap to the back buffer
  memDC.DrawBitmap( to_wx_bmp( scaled ), floored(topLeft.x - rView.x), floored(topLeft.y - rView.y) );
  memDC.SetDeviceOrigin( floored(-rView.x), floored(-rView.y) );
  paint_canvas_handles( memDC, active.GetSize(), m_geo );

  // Offset the DC for the backbuffer so that the top left corner of
  // the image is the origin (0,0)
  faint::coord ori_x = m_geo.left_border - m_geo.x0 - rView.x;
  faint::coord ori_y = m_geo.top_border - m_geo.y0 - rView.y;
  memDC.SetDeviceOrigin( floored(ori_x), floored(ori_y) );

  if ( ShouldDrawRaster() && !rasterSelection.Empty() ) {
    rasterSelection.DrawOverlay(overlays);
  }

  OverlayDC_WX overlayDC( memDC, zoom, imageCoordRect, bmpSize);
  if ( m_grid.Visible() ){
    overlayDC.GridLines( m_grid, imageRegion.TopLeft(), scaled.GetSize());
  }

  if ( ShouldDrawVector() ){
    paint_object_handles( active.GetObjectSelection(), overlayDC );
  }

  if ( m_pointOverlay.IsSet() ){
    overlays.Pixel(m_pointOverlay.Get());
  }

  overlays.Paint( overlayDC );
  memDC.SetDeviceOrigin( 0, 0 );

  // Blit it all to the window
  paintDC.Blit( floored(rView.x), floored(rView.y), floored(rView.w), floored(rView.h), &memDC, 0, 0 );
}

void CanvasScroller::OnScrollDrag( wxScrollWinEvent& event ){
  // This is a recurring event while a scrollbar-thumb is dragged
  event.Skip();
  int orientation = event.GetOrientation();
  int pos = event.GetPosition();
  if ( orientation == wxHORIZONTAL ){
    m_geo.x0 = pos + m_scroll.startX;
  }
  else if ( orientation == wxVERTICAL ){
    m_geo.y0 = pos + m_scroll.startY;
  }
  Refresh();
}

void CanvasScroller::OnScrollDragEnd( wxScrollWinEvent& event ){
  event.Skip();
  if ( event.GetOrientation() == wxHORIZONTAL ){
    AdjustHorizontalScrollbar( m_geo.x0 );
  }
  else {
    AdjustVerticalScrollbar( m_geo.y0 );
  }
}

void CanvasScroller::OnScrollLineDown( wxScrollWinEvent& event ){
  event.Skip();
  const int orientation = event.GetOrientation();
  if ( orientation == wxHORIZONTAL ){
    ScrollLineRight();
  }
  else if ( orientation == wxVERTICAL ){
    ScrollLineDown();
  }
  Refresh();
}

void CanvasScroller::OnScrollLineUp( wxScrollWinEvent& event ){
  event.Skip();
  const int orientation = event.GetOrientation();
  if ( orientation == wxHORIZONTAL ){
    ScrollLineLeft();
  }
  else if ( orientation == wxVERTICAL ){
    ScrollLineUp();
  }
  Refresh();
}

void CanvasScroller::OnScrollPageDown( wxScrollWinEvent& event ){
  event.Skip();
  if ( event.GetOrientation() == wxVERTICAL ){
    ScrollPageDown();
  }
  else if ( event.GetOrientation() == wxHORIZONTAL ){
    ScrollPageRight();
  }
}

void CanvasScroller::OnScrollPageUp( wxScrollWinEvent& event ){
  event.Skip();
  if ( event.GetOrientation() == wxVERTICAL ){
    ScrollPageUp();
  }
  else if ( event.GetOrientation() == wxHORIZONTAL ){
    ScrollPageLeft();
  }
}

void CanvasScroller::OnSize( wxSizeEvent& event ){
  AdjustScrollbars( IntPoint(m_geo.x0, m_geo.y0) );
  RefreshToolRect();
  event.Skip();
}

void CanvasScroller::OnIdle( wxIdleEvent& ) {
  // Update the scrollbars in the idle handler to prevent various
  // crashes and mouse capture problems
  // (e.g. issue 78)
  if ( m_scroll.updateHorizontal ){
    AdjustHorizontalScrollbar( m_geo.x0 );
  }
  if ( m_scroll.updateVertical ){
    AdjustVerticalScrollbar( m_geo.y0 );
  }
  if ( m_scroll.updateHorizontal || m_scroll.updateVertical ){
    Refresh();
    m_scroll.updateHorizontal = m_scroll.updateVertical = false;
  }
}

void CanvasScroller::Paste(){
  faint::Clipboard clipboard;
  if ( !clipboard.Good() ){
    return;
  }
  if ( m_toolWrap.GetTool()->AcceptsPastedText() ){
    faint::utf8_string text;
    if ( !clipboard.GetText( text ) ){
      // Stop further pasting when text paste failed. It would be
      // surprising if a bitmap was pasted while editing text.
      return;
    }
    else {
      m_toolWrap.GetTool()->Paste( text );
      Refresh();
    }
    return;
  }

  const bool rasterPaste = GetAppContext().GetLayerType() == Layer::RASTER;
  const bool objectPaste = !rasterPaste;
  if ( objectPaste ){
    objects_t objects;
    if ( clipboard.GetObjects(objects) ){
      PasteObjects(objects);
      return;
    }
  }

  faint::Bitmap bitmap;
  if ( clipboard.GetBitmap( bitmap ) ){
    if ( rasterPaste ){
      PasteBitmap( bitmap );
    }
    else {
      PasteBitmapAsObject( bitmap );
    }
    GetAppContext().OnDocumentStateChange(m_canvasId);
    return;
  }

  // No suitable format on clipboard
  return;
}

void CanvasScroller::PasteBitmap( const faint::Bitmap& bmp ){
  const RasterSelection& oldSelection(GetImageSelection());
  commands_t commands;
  if ( oldSelection.Floating() ){
    commands.push_back(oldSelection.StampFloatingSelection());
  }
  Settings settings(GetAppContext().GetToolSettings());
  commands.push_back( get_paste_raster_bitmap_command( bmp,
      floored(GetImageViewStart()), oldSelection, settings) );
  RunCommand( perhaps_bunch( CommandType::HYBRID, bunch_name(commands.back()->Name()), commands ) );
  GetAppContext().SelectTool( ToolId::RECTANGLE_SELECTION );
}

void CanvasScroller::PasteBitmapAsObject( const faint::Bitmap& bmp ){
  const Point pt = GetImageViewStart();
  Settings s( default_raster_settings() );
  s.Update( GetAppContext().GetToolSettings() );
  Object* rasterObj = new ObjRaster( Rect(pt, floated(bmp.GetSize())), bmp, s );
  RunCommand( get_add_object_command( rasterObj, select_added(false), "Paste") );
  SelectObject( rasterObj, deselect_old(true) );
  GetAppContext().SelectTool( ToolId::OBJECT_SELECTION );
}

void CanvasScroller::PasteObjects( objects_t& objects ){
  // Find the correct offset to place the pasted objects
  // with relative positions intact and anchored at the view-start
  Point minObj = bounding_rect( objects ).TopLeft();
  const Point viewStart = GetImageViewStart();
  offset_by( objects, viewStart - minObj );

  RunCommand( get_add_objects_command( objects, select_added(false), "Paste" ) );
  GetAppContext().SelectTool( ToolId::OBJECT_SELECTION );
  SelectObjects( objects, deselect_old(true) );
}

PreemptResult CanvasScroller::Preempt( PreemptOption option ){
  Tool* tool = m_toolWrap.GetTool();

  CursorPositionInfo info( HitTest(to_wx(faint::mouse::view_position(*this)), get_tool_modifiers() ) );
  ToolResult r = tool->Preempt(info);
  if ( r == TOOL_COMMIT ){
    if ( option == PreemptOption::ALLOW_COMMAND ){
      CommitTool(tool);
    }
    return PreemptResult::COMMIT;
  }
  else if ( r == TOOL_CANCEL ){
    Refresh();
    return PreemptResult::CANCEL;
  }
  else if ( r == TOOL_CHANGE ){
    if ( option == PreemptOption::ALLOW_COMMAND ){
      CommitTool( tool );
    }
    m_toolWrap.SetSwitched( 0 );
    return PreemptResult::COMMIT;
  }
  else {
    Refresh();
    return PreemptResult::NONE;
  }
}

void CanvasScroller::PreviousFrame(){
  const index_t maxIndex = m_images.GetNumImages();
  const index_t oldIndex = m_images.GetSelectedIndex();
  const index_t index( oldIndex.Get() == 0 ? maxIndex - 1 : oldIndex - 1 );
  SelectFrame(index);
  GetAppContext().OnDocumentStateChange(m_canvasId); // Fixme: For animation control
}

void CanvasScroller::NextFrame(){
  const index_t maxIndex = m_images.GetNumImages();
  const index_t oldIndex = m_images.GetSelectedIndex();
  const index_t index((oldIndex.Get() + 1) % maxIndex.Get());
  SelectFrame(index);
  GetAppContext().OnDocumentStateChange(m_canvasId); // Fixme: For animation control
}

void CanvasScroller::NotifySaved( const faint::FilePath& path ){
  m_filename.Set(path);
  m_savedAfter = get_last_modifying( m_undoList );
  GetAppContext().OnDocumentStateChange(m_canvasId); // Fixme: Typical chain MainFrame->Canvas::NotifySaved-MainFrame_refresh_stuff yo
}

void CanvasScroller::Redo(){
  Tool* tool = m_toolWrap.GetTool();
  if ( tool->CanRedo() ){
    tool->Redo();
    MousePosRefresh();
    GetAppContext().OnDocumentStateChange(m_canvasId); // Fixme: For undo/redo-state
    return;
  }

  if ( m_redoList.empty() || tool->PreventsGlobalRedo() ){
    return;
  }
  // Do not allow the preempted tool to commit, as this would
  // mess up undo/redo.
  Preempt(PreemptOption::DISCARD_COMMAND);

  OldCommand redone = m_redoList.front();
  m_redoList.pop_front();
  RunCommand( redone.command, clear_redo(false), redone.targetFrame );
}

void CanvasScroller::RemoveFrame( faint::Image* image ){
  m_images.Remove(image);
}

void CanvasScroller::RemoveFrame( const index_t& index ){
  m_images.Remove(index);
}

void CanvasScroller::ReorderFrame( const new_index_t& newIndex, const old_index_t& oldIndex ){
  m_images.Reorder(newIndex, oldIndex);
}

void CanvasScroller::RunCommand( Command* cmd ){
  // When a command is run, any commands in the redo list must be
  // cleared (See exception in CanvasScroller::Redo).
  RunCommand(cmd, clear_redo(true));
}

void CanvasScroller::RunCommand( Command* cmd, const FrameId& frameId ){
  // When a command is run, any commands in the redo list must be
  // cleared (See exception in CanvasScroller::Redo).
  RunCommand(cmd, clear_redo(true), &m_images.GetImageById(frameId));
}

void CanvasScroller::RunDWIM(){
  if ( !m_undoList.empty() && m_undoList.back().command->HasDWIM() ){
    Command* dwim = m_undoList.back().command->GetDWIM();
    Undo();
    RunCommand(dwim);
    Refresh();
  }
}

void CanvasScroller::ScrollMaxDown(){
  m_geo.y0 = std::max( 0, GetMaxScrollDown() - GetVerticalPageSize() );
  AdjustScrollbars( IntPoint(m_geo.x0, m_geo.y0) );
  Refresh();
}

void CanvasScroller::ScrollMaxLeft(){
  m_geo.x0 = 0;
  AdjustScrollbars( IntPoint(m_geo.x0, m_geo.y0) );
  Refresh();
}

void CanvasScroller::ScrollMaxRight(){
  m_geo.x0 = std::max( 0, GetMaxScrollRight() - GetHorizontalPageSize() );
  AdjustScrollbars( IntPoint(m_geo.x0, m_geo.y0) );
  Refresh();
}

void CanvasScroller::ScrollMaxUp(){
  m_geo.y0 = 0;
  AdjustScrollbars( IntPoint(m_geo.x0, m_geo.y0) );
  Refresh();
}

void CanvasScroller::ScrollPageDown(){
  m_geo.y0 = m_geo.y0 + (int) GetVerticalPageSize();
  m_geo.y0 = std::max( 0, std::min( m_geo.y0, GetMaxScrollDown() - GetVerticalPageSize() ) );
  m_scroll.updateVertical = true;
}

void CanvasScroller::ScrollPageLeft(){
  m_geo.x0 = std::max( 0, m_geo.x0 - (int) GetHorizontalPageSize() );
  m_scroll.updateHorizontal = true;
}

void CanvasScroller::ScrollPageRight(){
  m_geo.x0 = m_geo.x0 + (int) GetHorizontalPageSize();
  m_geo.x0 = std::max( 0, std::min( m_geo.x0, GetMaxScrollRight() - GetHorizontalPageSize() ) );
  m_scroll.updateHorizontal = true;
}

void CanvasScroller::ScrollPageUp(){
  m_geo.y0 = std::max( 0, m_geo.y0 - (int)GetVerticalPageSize() );
  m_scroll.updateVertical = true;
}

void CanvasScroller::SelectObject( Object* object, const deselect_old& deselectOld ){
  SelectObjects(as_list(object), deselectOld);
}

void CanvasScroller::SelectObjects( const objects_t& objects, const deselect_old& deselectOld ){
  if ( deselectOld.Get() ){
    m_images.Active().DeselectObjects();
  }
  m_images.Active().SelectObjects(objects);
  m_toolWrap.GetTool()->SelectionChange();
  GetAppContext().OnDocumentStateChange(m_canvasId);
  Refresh();
}

void CanvasScroller::SetChimera( RasterSelection* selectionChimera ){
  m_rasterSelectionChimera = selectionChimera;
}

void CanvasScroller::SetFaintScrollPos( const IntPoint& pos ){
  m_geo.x0 = pos.x;
  m_geo.y0 = pos.y;
  m_scroll.updateHorizontal = true;
  m_scroll.updateVertical = true;
}

void CanvasScroller::SetBitmapChimera( faint::Bitmap* bitmapChimera ){
  m_bitmapChimera = bitmapChimera;
  Refresh();
}

void CanvasScroller::SetGrid( const Grid& g ){
  m_grid = g;
  GetAppContext().OnDocumentStateChange(m_canvasId);
}

void CanvasScroller::SelectFrame( const index_t& index ){
  assert( index < m_images.GetNumImages() );
  if ( index != m_images.GetSelectedIndex() ){
    Preempt(PreemptOption::ALLOW_COMMAND);
    m_images.SetSelected( index );
    m_cmdContext->Reset();
    m_cmdContext->SetFrame(&m_images.Active());
    m_toolWrap.GetTool()->SelectionChange();
    Refresh();
  }
}

void CanvasScroller::SetPointOverlay( const IntPoint& p ){
  m_pointOverlay.Set(p);
}

void CanvasScroller::SetRasterSelection( const IntRect& r ){
  GetImageSelection().SetRect(intersection( r, image_rect( m_images.Active() )));
  Settings s(GetAppContext().GetToolSettings());
  update_mask( masked_background(s),
    s.Get(ts_BgCol),
    s.Get(ts_AlphaBlending),
    GetImageSelection() );
  m_toolWrap.GetTool()->SelectionChange();
  GetAppContext().OnDocumentStateChange(m_canvasId);
  MousePosRefresh();
  m_dwimDelete.Clear();
}

void CanvasScroller::SetZoomLevel( const ZoomLevel& zoom ){
  m_geo.zoom = zoom;
  GetAppContext().OnDocumentStateChange(m_canvasId);
  MousePosRefresh();
}

bool CanvasScroller::ShouldDrawRaster() const{
  ToolId tool = m_toolWrap.GetToolId();
  Layer selectedLayer = GetAppContext().GetLayerType();
  return get_tool_layer(tool, selectedLayer) == Layer::RASTER;
}

bool CanvasScroller::ShouldDrawVector() const{
  return !ShouldDrawRaster();
}

void CanvasScroller::Undo(){
  if ( m_toolWrap.GetTool()->CanUndo() ){
    m_toolWrap.GetTool()->Undo();
    MousePosRefresh();
    GetAppContext().OnDocumentStateChange(m_canvasId); // Fixme: For undo/redo-state
    return;
  }

  // Preempt the current tool, but allow it to commit.
  // if committing, whatever it did will be undone.
  PreemptResult res = Preempt(PreemptOption::ALLOW_COMMAND);
  if ( res == PreemptResult::CANCEL ){
    // The tool was aborted by the undo
    return;
  }

  // The tool either committed on the preempt (and the command should
  // now be undone) or there was no effect. Proceed with normal
  // undo-behavior.
  if ( m_undoList.empty() ){
    return;
  }

  OldCommand undone = m_undoList.back();
  CommandType undoType( undone.command->Type() );
  faint::Image* activeImage = undone.targetFrame;
  m_cmdContext->SetFrame(activeImage);
  if ( somewhat_reversible(undoType) ){
    // Reverse undoable changes
    undone.command->Undo( *m_cmdContext );
  }
  if ( !fully_reversible(undoType) ){
    SetRasterSelection(translated(activeImage->GetRasterSelection().GetRect(),
      -undone.command->SelectionOffset())); // Fixme: This will cause hybrid commands to undo selection offsetting twice!
    // Reset the image and reapply the raster steps of all commands to
    // undo the irreversible changes of the undone command.
    activeImage->Revert();
    for ( auto item : but_last(m_undoList) ){
      if ( item.targetFrame == activeImage ){
        Command* reapplied = item.command;
        reapplied->DoRaster( *m_cmdContext );
      }
    }
  }
  m_undoList.pop_back();
  assert(!has_command(m_redoList, undone.command));
  m_redoList.push_front( undone );

  GetAppContext().OnDocumentStateChange(m_canvasId);

  // Update the selection settings etc (originally added to make the
  // ObjSelectTool change the controls if a setting change was
  // undone)
  m_toolWrap.GetTool()->SelectionChange();
}

void CanvasScroller::UnwrapLast(){
  assert(!m_undoList.empty());
  Command* list = m_undoList.back().command;

  // Retrieve the single unwrapped command from a unary list, since
  // there's no use in keeping them in a list, and this makes the
  // undo-info less meaningful.
  Command* unwrapped = unwrap_list(list);

  if ( unwrapped != nullptr ){
    // Replace the unwrapped with its contained command
    m_undoList.back().command = unwrapped;
    delete list;

    if ( m_undoList.size() > 1 ){
      // Check if the last two commands should be merged. This must be
      // done after the unwrap, since merging won't happen when a command
      // is hidden in an open list.
      bool merged = penultimate(m_undoList).Merge(ultimate(m_undoList));
      if ( merged ){
	m_undoList.pop_back();
      }
    }
    GetAppContext().OnDocumentStateChange(m_canvasId);
  }
}

void CanvasScroller::ZoomFit(){
  // Use the complete size rather than the client size, so that
  // scrollbars don't limit the available size
  const IntSize windowSize(to_faint(GetSize()));
  const IntSize borders(m_geo.left_border * 2, m_geo.top_border * 2);
  const Size availableSpace(floated(max_coords(windowSize - borders, IntSize(1,1))));
  const Size imageSize(floated(m_images.Active().GetSize()));
  Scale rel(NewSize(availableSpace), imageSize);
  m_geo.zoom.SetApproximate(std::min(rel.x, rel.y));
  m_geo.x0 = m_geo.y0 = 0;
  AdjustScrollbars(IntPoint(m_geo.x0, m_geo.y0));
  MousePosRefresh();
  GetAppContext().OnZoomChange();
  Refresh();
}

int CanvasScroller::GetHorizontalPageSize() const {
  return GetClientSize().GetWidth() - m_geo.left_border * 2;
}

int CanvasScroller::GetVerticalPageSize() const {
  return GetClientSize().GetHeight() - m_geo.top_border * 2;
}

bool CanvasScroller::IgnoreCanvasHandle( const CursorPositionInfo& info ){
  // Prefer a clicked object handle with object selection tool
  // over canvas resize-handles.
  return (info.handleIndex != -1) && (m_toolWrap.GetToolId() == ToolId::OBJECT_SELECTION);
}

void CanvasScroller::MousePosRefresh( const wxPoint& mousePos, int modifiers ){
  CursorPositionInfo info = HitTest( mousePos, modifiers );
  if ( !HasCapture() && !IgnoreCanvasHandle(info) ){
    Optional<CanvasResizeHandle> canvasHandle = canvas_handle_hit_test( to_faint(mousePos), m_images.Active().GetSize(), m_geo );
    if ( canvasHandle.IsSet() ){
      SetFaintCursor( canvasHandle.Get().GetCursor() );
      StatusInterface& status = GetAppContext().GetStatusInfo();
      status.SetMainText( "Left click to resize, right click to rescale." );
      return;
    }
  }
  HandleToolResult( m_toolWrap.GetTool()->Motion( info ) );
  SetFaintCursor( m_toolWrap.GetTool()->GetCursor( info ) );
}

void CanvasScroller::ScrollLineUp(){
  // Fixme: Use m_scroll.startX et al.
  m_geo.y0 -= 10;
  m_geo.y0 = std::max( 0, std::min( m_geo.y0, GetMaxScrollDown() - GetVerticalPageSize()) );
  AdjustVerticalScrollbar( m_geo.y0 );
  Refresh();
}

void CanvasScroller::ScrollLineDown(){
  m_geo.y0 += g_lineSize;
  m_geo.y0 = std::max( 0, std::min( m_geo.y0, GetMaxScrollDown() - GetVerticalPageSize() ) );
  AdjustVerticalScrollbar( m_geo.y0 );
  Refresh();
}

void CanvasScroller::ScrollLineLeft(){
  m_geo.x0 = std::max( 0, m_geo.x0 - g_lineSize );
  AdjustHorizontalScrollbar( m_geo.x0 );
}

void CanvasScroller::ScrollLineRight(){
  m_geo.x0 = m_geo.x0 + g_lineSize;
  m_geo.x0 = std::max(0, std::min(m_geo.x0, GetMaxScrollRight() - GetHorizontalPageSize() ));
  AdjustHorizontalScrollbar( m_geo.x0 );
  Refresh();
}

IntRect CanvasScroller::GetVisibleImageRect() const{
  wxSize sz = GetSize();
  const faint::coord zoom = m_geo.zoom.GetScaleFactor();
  return IntRect( floored(Point((m_geo.x0 - m_geo.left_border) / zoom, (m_geo.y0 - m_geo.top_border) / zoom)),
    truncated(Size(sz.x / zoom + 100, sz.y / zoom + 100 )));
}

void CanvasScroller::RefreshToolRect(){
  Tool* tool = m_toolWrap.GetTool();
  IntRect toolRect(tool->GetRefreshRect(GetVisibleImageRect(), faint::mouse::image_position( m_geo, *this ) ) );
  InclusiveRefresh(faint::mouse::image_to_view( toolRect, m_geo ));
}

void CanvasScroller::CommitTool( Tool* tool, bool refresh ){
  Command* cmd = tool->GetCommand();
  if ( cmd == nullptr ){
    return;
  }

  if ( cmd->Type() == CommandType::RASTER ){
    const objects_t& objects = m_images.Active().GetObjectSelection();
    if ( objects.size() == 1 && is_raster_object(objects.front()) ){
      delete cmd;
      ObjRaster* obj = as_ObjRaster(objects.front());
      faint::Bitmap bmp = obj->GetBitmap();
      obj->Reset();
      cmd = new SetObjectBitmapCommand(obj, bmp, obj->GetTri(), "Draw on ObjRaster");
    }
  }

  RunCommand( cmd );
  if ( refresh ){
    Refresh();
  }
}

bool CanvasScroller::SelectionToClipboard(bool clear){
  faint::Clipboard clipboard;
  if ( !clipboard.Good() ){
    return false;
  }

  faint::utf8_string text;
  if ( m_toolWrap.GetTool()->CopyText( text, clear ) ){
    clipboard.SetText(text);
    if ( clear ){
      RefreshToolRect();
    }
    return true;
  }

  if ( ShouldDrawRaster() && HasRasterSelection() ){
    RasterSelection& selection(GetImageSelection());
    faint::Color bgCol = get_color_default(GetAppContext().GetToolSettings().Get(ts_BgCol),
      faint::color_white());
    clipboard.SetBitmap( selection.Floating() ?
      selection.GetBitmap() : GetSubBitmap(GetImageSelection().GetRect()),
      strip_alpha(bgCol));
    if ( clear ){
      DeleteSelection();
    }
    return true;
  }
  else if ( ShouldDrawVector() && HasObjectSelection() ){
    faint::Image& active(m_images.Active());
    clipboard.SetObjects( active.GetObjectSelection() );
    if ( clear ){
      RunCommand( get_delete_objects_command( active.GetObjectSelection(), active, "Cut" ) );
      Refresh();
    }
    return true;
  }
  return false;
}

CursorPositionInfo CanvasScroller::HitTest( const wxPoint& p, int modifiers ){
  int handleIndex = -1;
  Hit hitStatus = Hit::NONE;
  Object* object = 0;
  bool selected = false;

  // Selected objects are checked first so that they are prioritized
  const objects_t& objectSelection(m_images.Active().GetObjectSelection());
  for ( Object* objTemp : objectSelection ){
    if ( point_edit_enabled(objTemp) ){
      handleIndex = point_hit_test( objTemp->GetMovablePoints(), p, m_geo );
      if ( handleIndex != - 1 ){
        hitStatus = Hit::MOVABLE_POINT;
        selected = true;
        object = objTemp;
        break;
      }

      handleIndex = point_hit_test( objTemp->GetExtensionPoints(), p, m_geo );
      if ( handleIndex != -1 ){
        hitStatus = Hit::EXTENSION_POINT;
        selected = true;
        object = objTemp;
        break;
      }
    }

    if ( resize_handles_enabled(objTemp) ){
      std::vector<Point> pts( object_aligned_resize(objTemp) ?
        points(objTemp->GetTri()) :
        corners(objTemp->GetRect()) );
      handleIndex = get_handle_index( pts, p, m_geo );
      if ( handleIndex != -1 ){
        hitStatus = Hit::RESIZE_POINT;
        selected = true;
        object = objTemp;
        break;
      }
    }
  }

  if ( handleIndex == -1 ){
    Point ptImg = faint::mouse::view_to_image( to_faint(p), m_geo );
    std::pair<Object*, Hit> hitObject = ObjectAt( ptImg );
    object = hitObject.first;
    hitStatus = hitObject.second;
    selected = (object == nullptr ? false :
      contains(m_images.Active().GetObjectSelection(), object ));
  }

  Point imagePos = faint::mouse::view_to_image(to_faint(p), m_geo);
  CursorPositionInfo info = {
    &GetInterface(),
    &GetAppContext().GetStatusInfo(),
    modifiers,
    imagePos,
    InSelection( imagePos ),
    GetAppContext().GetLayerType(),
    hitStatus,
    selected,
    object,
    handleIndex };
  return info;
}

std::pair<Object*, Hit> CanvasScroller::ObjectAt( const Point& p ){
  // Create a small scaled and adjusted DC where the clicked pixel is
  // anchored at 0,0 in DC coordinates for pixel object hit tests
  FaintDC dc( m_hitTestBuffer );
  const faint::coord zoom = m_geo.zoom.GetScaleFactor();
  dc.SetOrigin(-p * zoom );
  dc.SetScale( zoom );
  dc.Clear( mask_outside );

  // First consider the selected objects...
  Object* consider = 0;
  Hit considerType = Hit::NONE;
  const objects_t& objectSelection(m_images.Active().GetObjectSelection());
  for ( Object* object : top_to_bottom(objectSelection) ){
    if ( object->HitTest( p ) ){
      object->DrawMask( dc );
      faint::Color color = dc.GetPixel( p );
      if ( color == mask_edge ){ // Fixme: Consider fuzzy or disabling AA
        return std::make_pair(object, Hit::BOUNDARY);
      }
      else if ( color == mask_fill ){
        return std::make_pair(object, Hit::INSIDE);
      }
      else {
        consider = object;
        considerType = Hit::NEARBY;
      }
    }
  }

  // ...then the rest
  dc.Clear( mask_outside );

  const objects_t& objects( m_images.Active().GetObjects() );
  for ( Object* object : top_to_bottom(objects) ){
    if ( object->HitTest( p ) ){
      object->DrawMask( dc );
      faint::Color color =  dc.GetPixel( p );
      if ( color  == mask_edge ){
        return std::make_pair(object, Hit::BOUNDARY);
      }
      else if ( color == mask_fill ){
        return std::make_pair(object, Hit::INSIDE);
      }
      else if ( (consider == nullptr || considerType == Hit::NEARBY ) && color == mask_no_fill ){
        consider = object;
        considerType = Hit::INSIDE;
      }
      else if ( consider == nullptr ){
        consider = object;
        considerType = Hit::NEARBY;
      }
    }
  }
  return std::make_pair(consider, considerType);
}

void CanvasScroller::SetFaintCursor( Cursor cursor ){
  if ( cursor == Cursor::DONT_CARE ){
    return;
  }
  SetCursor(to_wx_cursor(cursor));
}

void CanvasScroller::RunCommand( Command* cmd, const clear_redo& clearRedo, faint::Image* activeImage ){
  if ( activeImage == nullptr ){
    activeImage = &(m_images.Active());
  }
  const bool targetCurrentFrame = (activeImage == &m_images.Active());
  m_dwimDelete.Clear();
  m_cmdContext->SetFrame(activeImage);
  if ( !activeImage->HasStoredOriginal() && affects_raster(cmd) ){
    try {
      // Store the bitmap data (for undo) on the first change.
      activeImage->StoreAsOriginal();
    }
    catch ( const std::bad_alloc& ){
      delete cmd;
      show_out_of_memory_cancelled_error(this);
      return;
    }
  }
  try {
    IntSize oldSize(activeImage->GetSize());
    cmd->Do( *m_cmdContext );
    if ( clearRedo.Get() ){
      clear_list( m_redoList );
    }
    assert(!has_command(m_undoList, cmd));

    OldCommand mappedCmd(cmd, activeImage);
    bool merged = !m_undoList.empty() &&
      m_undoList.back().Merge(mappedCmd);
    if ( !merged ){
      m_undoList.push_back( OldCommand(cmd, activeImage) );
    }

    GetAppContext().OnDocumentStateChange(m_canvasId);

    if ( oldSize != activeImage->GetSize() ){
      if ( targetCurrentFrame ){
        // If the image size changes it is quite likely that the current
        // position will be outside the drawing area in a sea of dark
        // grey, simple return to 0,0 whenever the size changes for now.
        Point pos(m_geo.x0, m_geo.y0);
        faint::coord zoom = m_geo.zoom.GetScaleFactor();
        pos = ( cmd->Translate(pos / zoom) * zoom );
        AdjustScrollbars( floored(pos) );
      }

      // Clip the current selection to the new image size
      activeImage->GetRasterSelection().Clip(image_rect(*activeImage)); // Fixme: Move selection to image, fixme: Undo?
    }
    m_toolWrap.GetTool()->SelectionChange();
  }
  catch ( const std::bad_alloc& ){
    delete cmd;
    show_out_of_memory_cancelled_error(this);
  }
}

// Refreshes the union of the rectangle parameter and the rectangle
// parameter from the previous call.
void CanvasScroller::InclusiveRefresh( const IntRect& r ){
  const faint::coord zoom = m_geo.zoom.GetScaleFactor();
  if ( zoom >= LITCRD(1.0) ){
    RefreshRect(to_wx( union_of( r, m_lastRefreshRect ) ).Inflate(floored(LITCRD(2.0) * zoom)));
    m_lastRefreshRect = r;
  }
  else {
    // Fixme - Workaround. RefreshRect does not work well when zoomed out,
    // leading to problems with offsets and other stuff.
    // Note: This is slow.
    Refresh();
  }
}

bool CanvasScroller::HandleToolResult( ToolResult ref ){
  if ( ref == TOOL_COMMIT ){
    CommitTool( m_toolWrap.GetTool() );
  }
  else if ( ref == TOOL_CHANGE ){
    CommitTool( m_toolWrap.GetTool(), false );
    m_toolWrap.ClearSwitched();

    // Update settings, e.g. when cloning objects from the
    // ObjSelectTool
    m_toolWrap.GetTool()->SelectionChange();
    Refresh();
  }
  else if ( ref == TOOL_DRAW ){
    RefreshToolRect();
  }
  else if ( ref == TOOL_CANCEL ){
    Refresh();
  }
  // TOOL_NONE might signify that the tool didn't care, which can in
  // some cases suggest further processing outside this function.
  // (e.g. DeleteSelection)
  return ref != TOOL_NONE;
}

void CanvasScroller::Initialize( const initially_dirty& startDirty ){
  m_scroll.startX = 0;
  m_scroll.startY = 0;
  m_scroll.updateHorizontal = false;
  m_scroll.updateVertical = false;
  m_geo.x0 = 0;
  m_geo.y0 = 0;
  m_geo.left_border = 20;
  m_geo.top_border = 20;
  m_rasterSelectionChimera = nullptr;
  m_bitmapChimera = nullptr;
  SetBackgroundColour( g_canvasBg );
  SetBackgroundStyle( wxBG_STYLE_PAINT );
  m_interface = new CanvasContext(this);
  AdjustScrollbars(IntPoint(0,0));
  SetDropTarget( new CanvasFileDropTarget() );
  m_cmdContext = new CommandContextImpl(*this);
  if ( startDirty.Get() ){
    // Set a unique command id so that the image remains dirty until
    // the first save.
    m_savedAfter.Set(CommandId());
  }
}

BEGIN_EVENT_TABLE(CanvasScroller, wxPanel)
EVT_MOUSE_CAPTURE_LOST( CanvasScroller::OnCaptureLost )
EVT_CHAR(CanvasScroller::OnChar )
EVT_LEFT_DCLICK( CanvasScroller::OnDoubleClick )
EVT_ERASE_BACKGROUND( CanvasScroller::OnEraseBackground )
EVT_IDLE( CanvasScroller::OnIdle )
EVT_KEY_DOWN(CanvasScroller::OnKeyDown )
EVT_KEY_UP(CanvasScroller::OnKeyUp )
EVT_LEFT_DOWN( CanvasScroller::OnLeftDown )
EVT_RIGHT_DOWN( CanvasScroller::OnLeftDown )
EVT_LEFT_UP( CanvasScroller::OnLeftUp )
EVT_RIGHT_UP( CanvasScroller::OnLeftUp )
EVT_MOTION( CanvasScroller::OnMotion )
EVT_LEAVE_WINDOW( CanvasScroller::OnMouseOut )
EVT_MOUSEWHEEL( CanvasScroller::OnMouseWheel )
EVT_PAINT( CanvasScroller::OnPaint )
EVT_SCROLLWIN_THUMBTRACK( CanvasScroller::OnScrollDrag )
EVT_SCROLLWIN_THUMBRELEASE( CanvasScroller::OnScrollDragEnd )
EVT_SCROLLWIN_LINEDOWN( CanvasScroller::OnScrollLineDown )
EVT_SCROLLWIN_LINEUP( CanvasScroller::OnScrollLineUp )
EVT_SCROLLWIN_PAGEDOWN( CanvasScroller::OnScrollPageDown )
EVT_SCROLLWIN_PAGEUP( CanvasScroller::OnScrollPageUp )
EVT_SIZE( CanvasScroller::OnSize )
END_EVENT_TABLE()

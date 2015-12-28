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
#include "wx/filename.h"
#include "app/getappcontext.hh"
#include "canvasscroller.hh"
#include "commands/command-bunch.hh"
#include "commands/group-objects-cmd.hh"
#include "commands/flip-rotate-cmd.hh"
#include "gui/resizedialog.hh"
#include "gui/rotatedlg.hh"
#include "objects/objraster.hh"
#include "rendering/cairocontext.hh"
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
#include "util/mouse.hh"
#include "util/objutil.hh"
#include "util/settingutil.hh"
#include "util/toolutil.hh"
#include "util/util.hh"

const wxColour g_canvasBg( 160, 140, 160 );
const int g_objectHandleWidth = 8;
const IntSize g_canvasHandleSz(6,6);
const int g_lineSize = 10;

class CommandContextImpl : public CommandContext{
public:
  CommandContextImpl( CanvasScroller& canvas )
    : m_canvas(canvas),
      m_dc(0),
      m_frame(0)
  {}

  ~CommandContextImpl(){
    delete m_dc;
  }

  void SetFrame(faint::Image* frame){ // Non-virtual
    Reset();
    m_frame = frame;
  }

  void Add( Object* obj, const select_added& select, const deselect_old& deselect ){
    m_frame->Add(obj);
    if ( select.Get() ){
      m_canvas.SelectObject( obj, deselect );
    }
  }

  void Add( Object* obj, size_t z, const select_added& select, const deselect_old& deselect ){
    m_frame->Add(obj, z);
    if ( select.Get() ){
      m_canvas.SelectObject( obj, deselect );
    }
  }

  void AddFrame( faint::Image* image ){
    m_canvas.AddFrame(image);
  }

  void AddFrame( faint::Image* image, const index_t& index ){
    m_canvas.AddFrame(image, index);
  }

  const faint::Bitmap& GetBitmap() const {
    return m_frame->GetBitmap();
  }

  FaintDC& GetDC(){
    if ( m_dc == 0 ){
      assert(m_frame != 0);
      m_dc = new FaintDC(m_frame->GetBitmap());
    }
    return *m_dc; // Oops
  }

  faint::Image& GetFrame(const index_t& index){
    return m_canvas.GetImageList().GetImage(index);
  }

  IntSize GetImageSize() const {
    return m_frame->GetSize();
  }

  const objects_t& GetObjects(){
    return m_frame->GetObjects();
  }

  size_t GetObjectZ( Object* obj ){
    return m_frame->GetObjectZ(obj);
  }

  faint::Bitmap& GetRawBitmap(){
    return m_frame->GetBitmap();
  }

  bool HasObjects() const{
    return m_frame->GetNumObjects() != 0;
  }

  void OffsetRasterSelection( const IntPoint& delta ){
    RasterSelection& selection( m_canvas.GetImageSelection() );
    if ( selection.Exists() ){
      selection.Move( selection.TopLeft() + delta );
    }
  }

  void Remove( Object* obj ){
    m_canvas.DeselectObject(obj);
    m_frame->Remove(obj);
  }

  void RemoveFrame( const index_t& index ){
    m_canvas.RemoveFrame(index);
  }

  void RemoveFrame( faint::Image* image ){
    m_canvas.RemoveFrame(image);
  }

  void ReorderFrame(const new_index_t& newIndex, const old_index_t& oldIndex){
    m_canvas.ReorderFrame(newIndex, oldIndex);
  }

  void SetBitmap( const faint::Bitmap& bmp ){
    m_frame->SetBitmap(bmp);
  }

  void SetFrameDelay( const index_t& index, const delay_t& delay ){
    faint::ImageList& images(m_canvas.GetImageList());
    assert(index < images.GetNumImages() );
    images.GetImage(index).SetDelay(delay);
  }

  void SetObjectZ( Object* obj, size_t z ){
    m_frame->SetObjectZ(obj, z);
  }

  void SetRasterSelection( const RasterSelectionState& state ){
    m_frame->GetRasterSelection().SetState(state);
  }

  void SetRasterSelectionOptions( const RasterSelectionOptions& options ){
    m_frame->GetRasterSelection().SetOptions(options);
  }

  void Reset(){
    delete m_dc;
    m_dc = 0;
  }

private:
  CanvasScroller& m_canvas;
  FaintDC* m_dc;
  faint::Image* m_frame;
};

class CanvasContext : public CanvasInterface {
public:
  CanvasContext( CanvasScroller* canvas ):
    m_canvas( canvas )
  {}

  void BundleUndo( size_t count, const std::string& name ){
    m_canvas->BundleUndo(count, name);
  }

  void CenterView( const Point& pt ){
    m_canvas->CenterViewImage( pt );
  }

  void ClearDirty(){
    m_canvas->ClearDirty();
  }

  void ClearPointOverlay(){
    m_canvas->ClearPointOverlay();
  }

  void ClearRasterSelectionChimera(){
    m_canvas->ClearRasterSelectionChimera();
  }

  void ContextCrop(){
    m_canvas->Crop();
  }

  void ContextDelete(){
    m_canvas->DeleteSelection();
  }
  void ContextDeselect(){
    m_canvas->Deselect();
  }

  void ContextFlatten(){
    m_canvas->Flatten();
  }

   void ContextFlipHorizontal(){
     m_canvas->Apply(OperationFlip(Axis::HORIZONTAL));
  }

  void ContextFlipVertical(){
    m_canvas->Apply(OperationFlip(Axis::VERTICAL));
  }

  void ContextRotate90CW(){
    m_canvas->Rotate90CW();
  }

  void ContextSelectAll(){
    m_canvas->SelectAll();
  }

  void DeselectObject( Object* obj ){
    m_canvas->DeselectObject( obj );
  }

  void DeselectObjects(){
    m_canvas->DeselectObjects();
  };

  void DeselectObjects(const objects_t& objs){
    m_canvas->DeselectObjects(objs);
  }

  const faint::Bitmap& GetBitmap(){
    return m_canvas->GetImage().GetBitmap();
  }

  std::string GetFilename() const {
    return m_canvas->GetFilename();
  }

  const faint::Image& GetFrame(const index_t& index){
    return m_canvas->GetImageList().GetImage(index);
  }

  const faint::ImageList& GetFrames() const{
    return m_canvas->GetImageList();
  }

  Grid GetGrid() const{
    return m_canvas->GetGrid();
  }

  CanvasId GetId() const {
    return m_canvas->GetCanvasId();
  }

  const faint::Image& GetImage(){
    return m_canvas->GetImage();
  }

  index_t GetNumFrames() const{
    return m_canvas->GetImageList().GetNumImages();
  }

  const objects_t& GetObjects(){
    return m_canvas->GetObjects();
  }

  CursorPositionInfo GetPosInfo( const IntPoint& pos ){
    return m_canvas->ExternalHitTest(pos);
  }

  const RasterSelection& GetRasterSelection(){
    return m_canvas->GetRasterSelection();
  }

  Point GetRelativeMousePos(){
    return m_canvas->GetRelativeMousePos();
  }

  index_t GetSelectedFrame() const{
    return m_canvas->GetSelectedFrame();
  }

  const objects_t& GetObjectSelection(){
    return m_canvas->GetObjectSelection();
  }

  IntSize GetSize() const {
    return m_canvas->GetBitmapSize();
  }

  faint::Bitmap GetSubBitmap( const IntRect& r ) const{
    return m_canvas->GetSubBitmap( r );
  }

  faint::coord GetZoom() const {
    return m_canvas->GetZoom();
  }

  ZoomLevel GetZoomLevel() const{
    return m_canvas->GetZoomLevel();
  }

  bool Has( const ObjectId& id ) const {
    return m_canvas->Has( id );
  }

  bool IsSelected( const Object* obj ) const {
    return contains(m_canvas->GetObjectSelection(), obj);
  }

  void NextFrame(){
    m_canvas->NextFrame();
  }

  void PreviousFrame(){
    m_canvas->PreviousFrame();
  }

  void Redo(){
    m_canvas->Redo();
  }

  void Refresh(){
    m_canvas->Refresh();
  }

  void RunCommand( Command* cmd ){
    if ( cmd != 0 ){
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

  void RunDWIM(){
    m_canvas->RunDWIM();
  }

  void ScrollMaxDown(){
    m_canvas->ScrollMaxDown();
  }

  void ScrollMaxLeft(){
    m_canvas->ScrollMaxLeft();
  }

  void ScrollMaxRight(){
    m_canvas->ScrollMaxRight();
  }

  void ScrollMaxUp(){
    m_canvas->ScrollMaxUp();
  }

  void ScrollPageDown(){
    m_canvas->ScrollPageDown();
  }

  void ScrollPageLeft(){
    m_canvas->ScrollPageLeft();
  }

  void ScrollPageRight(){
    m_canvas->ScrollPageRight();
  }

  void ScrollPageUp(){
    m_canvas->ScrollPageUp();
  }

  void SelectFrame( const index_t& index ){
    m_canvas->SelectFrame(index);
  }

  void SelectObject( Object* obj, const deselect_old& deselectOld ){
    m_canvas->SelectObject( obj, deselectOld );
  }

  void SelectObjects( const objects_t& objs, const deselect_old& deselectOld ){
    m_canvas->SelectObjects( objs, deselectOld );
  }

  void SetChimera( RasterSelection* selection){
    m_canvas->SetChimera(selection);
  }

  void SetGrid( const Grid& g ){
    m_canvas->SetGrid(g);
  }

  void SetFilename( const std::string& filename ){
    m_canvas->SetFilename( filename );
  }

  void SetPointOverlay( const IntPoint& p ){
    m_canvas->SetPointOverlay(p);
  }

  void SetRasterSelection( const IntRect& r ){
    m_canvas->SetRasterSelection( r );
  }

  void SetZoom( const ZoomLevel& zoom ){
    m_canvas->SetZoomLevel( zoom );
  }

  void Undo(){
    m_canvas->Undo();
  }

  void ZoomDefault(){
    m_canvas->ChangeZoom( ZoomLevel::DEFAULT );
  }

  void ZoomFit(){
    m_canvas->ZoomFit();
  }

  void ZoomIn(){
    m_canvas->ChangeZoom( ZoomLevel::NEXT );
  }

  void ZoomOut(){
    m_canvas->ChangeZoom( ZoomLevel::PREVIOUS );
  }
private:
  CanvasScroller* m_canvas;
};

bool crop_objects( CanvasInterface& canvas, objects_t selectedObjects ){
  Command* cmd = get_crop_command( selectedObjects );
  if ( cmd == 0 ){
    return false;
  }
  canvas.RunCommand( cmd );
  return true;
}

wxRect canvas_handle_rect( size_t handle, const IntSize& canvasSize, const Geometry& geo ){
  const int x_off = geo.left_border / 2 - g_canvasHandleSz.w / 2;
  const int y_off = geo.top_border / 2 - g_canvasHandleSz.h / 2;

  const int left( -geo.x0 + x_off );
  const int top( -geo.y0 + y_off );

  const faint::coord zoom = geo.zoom.GetScaleFactor();
  const int width = rounded(canvasSize.w * zoom);
  const int height = rounded(canvasSize.h * zoom);
  const int right( geo.left_border + width - geo.x0 + x_off );
  const int bottom( geo.top_border + height - geo.y0 + y_off );

  if ( handle == 0 ){
    return to_wx(IntRect(IntPoint(left,top), g_canvasHandleSz));
  }
  else if ( handle == 1 ){
    return to_wx(IntRect(IntPoint(right, top), g_canvasHandleSz));
  }
  else if ( handle == 2 ){
    return to_wx(IntRect(IntPoint(left, bottom), g_canvasHandleSz));
  }
  else if ( handle == 3 ){
    return to_wx(IntRect(IntPoint(right, bottom), g_canvasHandleSz));
  }
  else if ( handle == 4 ){
    const int ch_w = g_canvasHandleSz.w;
    return to_wx(IntRect(IntPoint((int)geo.left_border + width / 2 - geo.x0 - ch_w / 2, top),
	g_canvasHandleSz));
  }
  else if ( handle == 5 ){
    const int ch_w = g_canvasHandleSz.w;
    return to_wx(IntRect(IntPoint(geo.left_border + width / 2 - geo.x0 - ch_w / 2, bottom),
	g_canvasHandleSz));

  }
  else if (handle == 6 ){
    const int ch_h = g_canvasHandleSz.h;
    return to_wx(IntRect(IntPoint( right, geo.top_border + height / 2 - geo.y0 - ch_h / 2), g_canvasHandleSz));
  }
  else if (handle == 7 ){
    const int ch_h = g_canvasHandleSz.h;
    return to_wx(IntRect(IntPoint(left,geo.top_border + height / 2 - geo.y0 - ch_h / 2),
	g_canvasHandleSz));
  }
  else {
    assert( false );
    return wxRect(0,0,0,0);
  }
}

int canvas_handle_hit_test( wxPoint pos, const IntSize& canvasSize, const Geometry& geo ){
  for ( size_t i = 0; i != 8; i++ ){
    if ( canvas_handle_rect( i, canvasSize, geo ).Contains( pos ) ){
      return i;
    }
  }
  return -1;
}

wxBitmap& get_back_buffer( const IntSize& minSize ){
  static wxBitmap backBuffer( minSize.w,minSize.h );
  wxSize oldSize = backBuffer.GetSize();
  if ( oldSize.GetWidth() < minSize.w || oldSize.GetHeight() < minSize.h ){
    backBuffer = wxBitmap( std::max( minSize.w, oldSize.GetWidth() ),
      std::max( minSize.h, oldSize.GetHeight() ) );
  }
  return backBuffer;
}

void paint_object_handles( const objects_t& objects, OverlayDC& dc ){
  for ( size_t i= 0; i != objects.size(); i++ ){
    const Object* obj(objects[i]);
    // Draw handles for objects, excluding currently adjusted objects
    // so the handles don't get in the way
    if ( obj->Inactive() && resize_handles_enabled(obj) ){
      dc.Handles( obj->GetRect() );
    }
  }
}

int point_hit_test( const std::vector<Point>& points, const wxPoint& pos, const Geometry& g ){
  for ( size_t i = 0; i != points.size(); i++ ){
    const Point& ptHandle = points[i];
    const faint::coord zoom = g.zoom.GetScaleFactor();
    Point topLeft( ptHandle.x * zoom - g_objectHandleWidth / 2, ptHandle.y * zoom - g_objectHandleWidth/ 2 );
    Rect r( topLeft, Size(g_objectHandleWidth, g_objectHandleWidth ) );
    if ( r.Contains( Point( pos.x - g.left_border + g.x0, pos.y - g.top_border + g.y0 ) ) ){
      return i;
    }
  }
  return -1;
}

int get_handle_index( const std::vector<Point>& points, const wxPoint& pos, const Geometry& g ){
  for ( size_t i = 0; i != points.size(); i++ ){
    const Point& ptHandle = points[i];
    const faint::coord zoom = g.zoom.GetScaleFactor();

    Point topLeft( ptHandle.x * zoom - g_objectHandleWidth / 2, ptHandle.y * zoom - g_objectHandleWidth/ 2 );
    Rect r( topLeft, Size(g_objectHandleWidth, g_objectHandleWidth ) );
    if ( r.Contains( Point( pos.x - g.left_border + g.x0, pos.y - g.top_border + g.y0 ) ) ){
      return i;
    }
  }

  Point p2( pos.x - g.left_border + g.x0, pos.y - g.top_border + g.y0 );
  // Sides
  struct {
    int in;
    int out;
  } off = {1, g_objectHandleWidth / 2};

  const faint::coord zoom = g.zoom.GetScaleFactor();
  if ( points.size() == 4 ){
    if ( p2.x > points[0].x * zoom - off.out && p2.x < points[0].x * zoom + off.in
      && p2.y > points[0].y * zoom && p2.y < points[3].y * zoom ){
      return 4;
    }
    else if ( p2.x > points[1].x * zoom - off.in && p2.x < points[1].x * zoom + off.out
      && p2.y > points[0].y * zoom && p2.y < points[3].y * zoom ){
      return 5;
    }
    else if ( points[0].x * zoom < p2.x && p2.x < points[1].x * zoom
      && points[0].y * zoom - off.out < p2.y && p2.y < points[0].y * zoom + off.in  ) {
      return 6;
    }

    else if ( points[0].x * zoom < p2.x && p2.x < points[1].x * zoom
      && points[2].y * zoom - off.in < p2.y && p2.y < points[2].y * zoom + off.out  ) {
      return 7;
    }
}
  return -1;
}

void clear_list( std::deque<old_command_t>& list ){
  for ( std::deque<old_command_t>::iterator it = list.begin(); it != list.end(); ++it ){
    delete (it->first);
  }
  list.clear();
}

void paint_canvas_handles( wxDC& dc, const IntSize& canvasSize, const Geometry& geo ){
  dc.SetPen( wxPen(wxColour(128, 128, 128), 1 ) );
  dc.SetBrush( wxBrush(wxColour(181, 165, 213) ) );

  for ( size_t i = 0; i != 8; i++ ){
    dc.DrawRectangle( canvas_handle_rect( i, canvasSize, geo ) );
  }
}

Rect Paint_InImageCoordinates( const Rect& rView, const Geometry& geo ){
  Rect rImageCoord( Point(rView.x + geo.x0 - geo.left_border, rView.y + geo.y0 - geo.top_border), Size(rView.w + geo.left_border, rView.h + geo.top_border ));
  const faint::coord zoom = geo.zoom.GetScaleFactor();
  rImageCoord.x /= zoom;
  rImageCoord.y /= zoom;
  rImageCoord.w = rImageCoord.w / zoom;
  rImageCoord.h = rImageCoord.h / zoom;
  return rImageCoord;
}

void Paint_EarlyReturn( wxDC& dc, const IntRect& rView, const Geometry& geo, const IntSize& imageSize ){
  dc.SetBrush( wxBrush( g_canvasBg ) );
  dc.SetPen( wxPen( g_canvasBg ) );
  dc.DrawRectangle( rView.x, rView.y, rView.w, rView.h );
  paint_canvas_handles( dc, imageSize, geo );
}

void Paint_CheckeredBackground( wxDC& dc, const IntPoint& topLeft, const IntPoint& imageRegionTopLeft, const Geometry& geo, int w, int h  ){
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

IntRect Paint_GetImageRegion( Rect rView, IntSize bmpSize, const Geometry& geo ){
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

wxCursor CanvasHandleCursor( int handle ){
  if ( handle == 0 || handle == 3 ){
    return wxCursor( wxCURSOR_SIZENWSE );
  }
  if ( handle == 1 || handle == 2 ){
    return wxCursor( wxCURSOR_SIZENESW );
  }
  if ( handle == 4 || handle == 5 ){
    return wxCursor( wxCURSOR_SIZENS );
  }
  if ( handle == 6 || handle == 7 ){
    return wxCursor( wxCURSOR_SIZEWE );
  }
  assert( false );
  return wxCursor( wxCURSOR_SIZENESW );
}

IntPoint CanvasHandlePos( int handle, const IntSize& imgSize ){
  if ( handle == 0 ){
    return IntPoint(0,0);
  }
  if ( handle == 1 ){
    return IntPoint( imgSize.w, 0 );
  }
  if ( handle == 2 ){
    return IntPoint( 0, imgSize.h );
  }
  if ( handle == 3 ){
    return IntPoint( imgSize.w, imgSize.h );
  }
  if ( handle == 4 ){
    return IntPoint( ( imgSize.w - 1 ) / 2, 0 );
  }
  if ( handle == 5 ){
    return IntPoint( ( imgSize.w - 1 ) / 2, ( imgSize.h - 1 ) );
  }
  if ( handle == 6 ){
    return IntPoint( imgSize.w - 1 , (imgSize.h - 1 ) / 2 );
  }
  if ( handle == 7 ){
    return IntPoint( 0, ( imgSize.h - 1 ) / 2 );
  }
  assert( false );
  return IntPoint(0,0);
}

IntPoint CanvasHandleOppositePos( int handle, const IntSize& imgSize ){
  if ( handle == 0 ){
    return IntPoint(imgSize.w - 1, imgSize.h -1 );
  }
  if ( handle == 1 ){
    return IntPoint( 0, imgSize.h - 1 );
  }
  if ( handle == 2 ){
    return IntPoint( imgSize.w - 1, 0 );
  }
  if ( handle == 3 ){
    return IntPoint( 0, 0 );
  }
  if ( handle == 4 ){
    return IntPoint( ( imgSize.w - 1 ) / 2, imgSize.h - 1 );
  }
  if ( handle == 5 ){
    return IntPoint( ( imgSize.w - 1 ) / 2, 0 );
  }
  if ( handle == 6 ){
    return IntPoint( 0, (imgSize.h - 1) / 2 );
  }
  if ( handle == 7 ){
    return IntPoint( imgSize.w - 1, (imgSize.h - 1) / 2 );
  }
  assert( false );
  return IntPoint(0,0);
}

ResizeCanvas::Direction CanvasHandleDirection( int handle ){
  if ( handle < 4 ){
    return ResizeCanvas::DIAGONAL;
  }
  else if ( handle == 4 ){
    return ResizeCanvas::UP_DOWN;
  }
  else if ( handle == 5 ){
    return ResizeCanvas::UP_DOWN;
  }
  else if ( handle == 6 ){
    return ResizeCanvas::LEFT_RIGHT;
  }
  else if ( handle == 7 ){
    return ResizeCanvas::LEFT_RIGHT;
  }
  assert( false );
  return ResizeCanvas::DIAGONAL;
}

class CanvasFileDropTarget : public wxFileDropTarget {
public:
  bool OnDropFiles( wxCoord /*x*/, wxCoord /*y*/, const wxArrayString& files){
    GetAppContext().Load(to_vector(files));
    return true;
  }
};

CanvasScroller::CanvasScroller( wxWindow* parent, ImageProps& props )
  : wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL|wxHSCROLL | wxWANTS_CHARS ),
    m_hitTestBuffer( faint::cairo_compatible_bitmap(IntSize(20, 20)) ),
    m_cmdContext(0),
    m_images(props)
{
  Initialize();
  m_grid = GetAppContext().GetDefaultGrid();
}

CanvasScroller::CanvasScroller( wxWindow* parent, std::vector<ImageProps>& props )
  : wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL|wxHSCROLL | wxWANTS_CHARS ),
    m_hitTestBuffer( faint::cairo_compatible_bitmap(IntSize(20, 20)) ),
    m_cmdContext(0),
    m_images(props)
{
  Initialize();
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
  GetAppContext().OnDocumentStateChange(m_canvasId); // Fixme: Queue!
}

void CanvasScroller::AddFrame( faint::Image* image, const index_t& index ){
  m_images.Insert(image, index);
  GetAppContext().OnDocumentStateChange(m_canvasId); // Fixme: Queue!
}

void CanvasScroller::Apply( const Operation& op ){
  ApplyTarget target = GetApplyTarget();
  if ( target == APPLY_OBJECT_SELECTION  ){
    RunCommand( op.DoObjects(m_images.Active().GetObjectSelection()) );
  }
  else if ( target == APPLY_RASTER_SELECTION ){
    RunCommand(op.DoRasterSelection( m_images.Active()));
  }
  else if ( target == APPLY_IMAGE ){
    RunCommand( op.DoImage() );
  }
  else {
    assert(false);
  }
  Refresh();
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

void CanvasScroller::BundleUndo( size_t count, const std::string& name ){
  assert( m_undoList.size() >= count );
  std::vector<old_command_t> bundled(m_undoList.end() - count, m_undoList.end());
  std::vector<Command*> cmds;
  for ( size_t i = 0; i != bundled.size(); i++ ){
    cmds.push_back(bundled[i].first);
  }
  assert(bundled.size() == count);
  m_undoList.erase(m_undoList.end() - count, m_undoList.end() );
  m_undoList.push_back( std::make_pair(
    command_bunch(get_command_type(cmds), bunch_name(name), cmds),
    bundled.front().second));
  GetAppContext().OnDocumentStateChange(m_canvasId);
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

void CanvasScroller::ClearDirty(){
  m_dirty = false;
  GetAppContext().OnDocumentStateChange(m_canvasId);
}

void CanvasScroller::ClearPointOverlay(){
  m_pointOverlay.Clear();
}

bool CanvasScroller::CopySelection(){
  bool cut = false;
  return SelectionToClipboard(cut);
}

void CanvasScroller::ClearRasterSelectionChimera(){
  m_rasterSelectionChimera = 0;
}

void CanvasScroller::Crop(){
  const bool rasterLayer = ShouldDrawRaster();
  if ( rasterLayer ){
    if ( HasRasterSelection() ){
      RasterSelection& selection(GetImageSelection());
      RunCommand( get_crop_to_selection_command( GetImageSelection() ) );
      selection.Deselect();
      Refresh();
      return;
    }
  }
  else {
    if ( crop_objects( GetInterface(), GetObjectSelection() ) ){
      Refresh();
      return;
    }
  }

  Command* command = get_auto_crop_command(m_images.Active());
  if ( command != 0 ){
    RunCommand(command);
    Refresh();
  }
}

void CanvasScroller::CutSelection(){
  bool cut = true;
  SelectionToClipboard(cut);
}

void CanvasScroller::DeleteSelection(){
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
    active.SetObjectSelection(objects_t()); // Fixme: Make part of command?
  }
  else if ( HasRasterSelection() ){
    Command* cmd = get_delete_raster_selection_command(m_images.Active(),
      GetAppContext().GetToolSettings().Get(ts_BgCol),
      m_dwimDelete );
    RunCommand(cmd);
  }
  Refresh();
}

void CanvasScroller::Deselect(){
  if ( m_toolWrap.GetTool()->HasSelection() ){
    HandleToolResult(m_toolWrap.GetTool()->Deselect());
    return;
  }

  if ( ShouldDrawVector() ){
    DeselectObjects();
  }
  else {
    DeselectRaster();
  }
  Refresh();
}

void CanvasScroller::DeselectObject( Object* object ){
  faint::Image& active(m_images.Active());
  objects_t objectSelection(active.GetObjectSelection());
  bool removed = remove(object, from(objectSelection));
  if ( !removed ){
    return;
  }
  active.SetObjectSelection(objectSelection);
  m_toolWrap.GetTool()->SelectionChange();

  if ( !HasObjectSelection() ){
    GetAppContext().OnDocumentStateChange(m_canvasId);
  }
}

void CanvasScroller::DeselectObjects(){
  SetObjectSelection( objects_t() );
}

void CanvasScroller::DeselectObjects( const objects_t& objects ){
  faint::Image& active(m_images.Active());
  objects_t objectSelection(active.GetObjectSelection());
  bool removed = remove(objects, from(objectSelection));
  if ( !removed ){
    return;
  }
  m_toolWrap.GetTool()->SelectionChange();
  if ( !HasObjectSelection() ){
    GetAppContext().OnDocumentStateChange(CanvasId());
  }
}

void CanvasScroller::DeselectRaster(){
  m_lastRefreshRect = union_of( m_lastRefreshRect, faint::mouse::image_to_view(GetImageSelection().GetRect(), m_geo));
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

void CanvasScroller::Flatten(){
  bool flattenSelected = HasObjectSelection();
  faint::Image& active(m_images.Active());
  const objects_t& objects = flattenSelected ?  active.GetObjectSelection() : active.GetObjects();
  if ( objects.empty() ){
    return;
  }
  Preempt(PreemptOption::ALLOW_COMMAND);
  RunCommand( get_flatten_command( objects, active ) );
  if ( flattenSelected ){
    active.SetObjectSelection(objects_t());
    m_toolWrap.GetTool()->SelectionChange();
  }
  Refresh();
}

ApplyTarget CanvasScroller::GetApplyTarget() {
  AppContext& app = GetAppContext();
  ToolId tool = m_toolWrap.GetToolId();
  if ( tool == T_RECT_SEL && HasRasterSelection() ){
    return APPLY_RASTER_SELECTION;
  }
  else if ( tool == T_OBJ_SEL && HasObjectSelection() ){
    return APPLY_OBJECT_SELECTION;
  }
  else if ( app.GetLayerType() == Layer::OBJECT && HasObjectSelection() ){
    return APPLY_OBJECT_SELECTION;
  }
  else if ( app.GetLayerType() == Layer::RASTER && HasRasterSelection() ){
    return APPLY_RASTER_SELECTION;
  }
  return APPLY_IMAGE;
}

Size CanvasScroller::GetApplyTargetSize(){
  ApplyTarget target = GetApplyTarget();
  if ( target == APPLY_OBJECT_SELECTION ){
    return bounding_rect(m_images.Active().GetObjectSelection()).GetSize();
  }
  else if ( target == APPLY_RASTER_SELECTION ){
    return floated(GetImageSelection().GetRect().GetSize());
  }
  else {
    return floated(m_images.Active().GetSize());
  }
}

faint::Bitmap CanvasScroller::GetSubBitmap( const IntRect& rect ) const{
  return cairo_compatible_sub_bitmap(m_images.Active().GetBitmap(), rect);
}

IntSize CanvasScroller::GetBitmapSize() const {
  return m_images.Active().GetSize();
}

CanvasId CanvasScroller::GetCanvasId() const{
  return m_canvasId;
}

std::string CanvasScroller::GetFilename() const{
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
  return std::max( LITCRD(0.0), m_images.Active().GetSize().w * m_geo.zoom.GetScaleFactor() );
}

int CanvasScroller::GetMaxScrollDown() const{
  return std::max( LITCRD(0.0), m_images.Active().GetSize().h * m_geo.zoom.GetScaleFactor() );
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
  return m_redoList.empty() ? "" : m_redoList.front().first->Name();
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
  return m_undoList.empty() ? "" : m_undoList.back().first->Name();
}

faint::coord CanvasScroller::GetZoom() const{
  return m_geo.zoom.GetScaleFactor();
}

const ZoomLevel& CanvasScroller::GetZoomLevel() const{
  return m_geo.zoom;
}

void CanvasScroller::GroupSelected(){
  const objects_t& objectSelection(m_images.Active().GetObjectSelection());
  if ( objectSelection.size() <= 1 ){
    return;
  }
  Preempt(PreemptOption::ALLOW_COMMAND);
  RunCommand( new GroupObjects( objectSelection, select_added(true) ) );
  m_toolWrap.GetTool()->SelectionChange();
  Refresh();
}

bool CanvasScroller::Has( const ObjectId& id ){
  const objects_t& objects(m_images.Active().GetObjects());
  for ( size_t i = 0; i != objects.size(); i++ ){
    if ( is_or_has( objects[i], id ) ){
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
  return GetImageSelection().Contains( truncated(p) );
}

bool CanvasScroller::IsDirty() const{
  return m_dirty;
}

void CanvasScroller::MousePosRefresh(){
  MousePosRefresh( to_wx( faint::mouse::view_position(*this)), get_tool_modifiers() );
}

void CanvasScroller::MoveObjectBackward(){
  assert( HasObjectSelection() );
  faint::Image& active(m_images.Active());
  Command* cmd = get_objects_backward_command(active.GetObjectSelection(), active);
  if ( cmd == 0 ){
    return;
  }
  RunCommand(cmd);
  Refresh();
}

void CanvasScroller::MoveObjectForward(){
  assert(HasObjectSelection());
  faint::Image& active(m_images.Active());
  Command* cmd = get_objects_forward_command(active.GetObjectSelection(), active);
  if ( cmd == 0 ){
    return;
  }
  RunCommand(cmd);
  Refresh();
}

void CanvasScroller::MoveObjectToBack(){
  assert(HasObjectSelection());
  faint::Image& active(m_images.Active());
  Command* cmd = get_objects_to_back_command(active.GetObjectSelection(), active);
  if ( cmd == 0 ){
    return;
  }
  RunCommand(cmd);
  Refresh();
}

void CanvasScroller::MoveObjectToFront(){
  assert(HasObjectSelection());
  faint::Image& active(m_images.Active());
  Command* cmd = get_objects_to_front_command(active.GetObjectSelection(), active); // Fixme: Remove need to pass objects to all get_objects_to-commands?
  if ( cmd == 0 ){
    return;
  }
  RunCommand(cmd);
  Refresh();
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
    // Note: This repeates while the key is held.
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
    int canvasHandle = canvas_handle_hit_test( wx_mousePos, imageSize, m_geo );
    if ( canvasHandle != - 1 ){
      IntPoint handlePos = CanvasHandlePos( canvasHandle, imageSize);
      IntPoint oppositePos = CanvasHandleOppositePos( canvasHandle, imageSize);
      Tool* resizeTool = new ResizeCanvas( handlePos, oppositePos, imageSize,
        evt.GetButton() == wxMOUSE_BTN_LEFT ? ResizeCanvas::Resize : ResizeCanvas::Rescale,
        CanvasHandleDirection( canvasHandle ) );
      resizeTool->ToolSettingUpdate(GetAppContext().GetToolSettings());
      m_toolWrap.SetSwitched( resizeTool );
      SetCursor( CanvasHandleCursor( canvasHandle ) );
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
  const faint::Bitmap& faint_bmp = active.GetBitmap();
  IntSize bmpSize( faint_bmp.GetSize() );
  IntRect imageRegion = Paint_GetImageRegion( rView, bmpSize, m_geo );
  Rect imageCoordRect = Paint_InImageCoordinates( rView, m_geo );
  if ( empty( imageRegion ) ){
    // Fixme: rView should be IntRect?
    Paint_EarlyReturn( paintDC, truncated(rView), m_geo, active.GetSize() );
    return;
  }

  const Point mouseImagePos = faint::mouse::image_position(m_geo, *this);
  RasterSelection& rasterSelection( m_rasterSelectionChimera == 0 ?
    GetImageSelection() : *m_rasterSelectionChimera );

  faint::Bitmap subBitmap(cairo_compatible_sub_bitmap( faint_bmp, imageRegion ));
  Overlays overlays;
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

  const faint::coord zoom = m_geo.zoom.GetScaleFactor();
  faint::Bitmap scaled = m_geo.zoom.At100() ?
    subBitmap : ( zoom > LITCRD(1.0) ?
      scale_nearest( subBitmap, rounded(zoom) ):
      scale_bilinear( subBitmap, Scale(zoom) ) );

  if ( scaled.m_w <= 0 || scaled.m_h <= 0 ){
    Paint_EarlyReturn( paintDC, truncated(rView), m_geo, active.GetSize() );
    return;
  }

  // Top left of image in view-coordinates
  IntPoint topLeft( faint::mouse::image_to_view_tr( floated(imageRegion.TopLeft()), m_geo ) );

  FaintDC dc( scaled );
  dc.SetOrigin( -imageRegion.TopLeft() * zoom );
  dc.SetScale( zoom );
  const objects_t& objects(active.GetObjects());
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* object = objects[i];
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

  IntRect clipRect( truncated(Point(topLeft.x - rView.x, topLeft.y - rView.y)), scaled.GetSize() );
  memDC.SetClippingRegion( to_wx(clipRect) );
  Paint_CheckeredBackground( memDC, truncated(Point( topLeft.x - rView.x,
    topLeft.y - rView.y)), imageRegion.TopLeft(), m_geo, scaled.m_w, scaled.m_h );
  memDC.DestroyClippingRegion();

  // Draw the scaled part of the bitmap to the back buffer
  memDC.DrawBitmap( to_wx( scaled ), topLeft.x - rView.x, topLeft.y - rView.y );
  memDC.SetDeviceOrigin( -rView.x, -rView.y );
  paint_canvas_handles( memDC, active.GetSize(), m_geo );

  // Offset the DC for the backbuffer so that the top left corner of
  // the image is the origin (0,0)
  faint::coord ori_x = m_geo.left_border - m_geo.x0 - rView.x;
  faint::coord ori_y = m_geo.top_border - m_geo.y0 - rView.y;
  memDC.SetDeviceOrigin( ori_x, ori_y );

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
  paintDC.Blit( rView.x, rView.y, rView.w, rView.h, &memDC, 0, 0 );
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
      truncated(GetImageViewStart()), oldSelection, settings) );
  RunCommand( perhaps_bunch( CMD_TYPE_HYBRID, bunch_name(commands.back()->Name()), commands ) );
  GetAppContext().SelectTool( T_RECT_SEL );
}

void CanvasScroller::PasteBitmapAsObject( const faint::Bitmap& bmp ){
  const Point pt = GetImageViewStart();
  Settings s( default_raster_settings() );
  s.Update( GetAppContext().GetToolSettings() );
  Object* rasterObj = new ObjRaster( Rect(pt, floated(bmp.GetSize())), bmp, s );
  RunCommand( get_add_object_command( rasterObj, select_added(false), "Paste") );
  SelectObject( rasterObj, deselect_old(true) );
  GetAppContext().SelectTool( T_OBJ_SEL );
}

void CanvasScroller::PasteObjects( objects_t& objects ){
  // Find the correct offset to place the pasted objects
  // with relative positions intact and anchored at the view-start
  Point minObj = bounding_rect( objects ).TopLeft();
  const Point viewStart = GetImageViewStart();
  offset_by( objects, viewStart - minObj );

  RunCommand( get_add_objects_command( objects, select_added(false), "Paste" ) );
  GetAppContext().SelectTool( T_OBJ_SEL );
  SetObjectSelection( objects );
  return;
}

PreemptResult CanvasScroller::Preempt( PreemptOption::type option ){
  Tool* tool = m_toolWrap.GetTool();

  CursorPositionInfo info( HitTest(to_wx(faint::mouse::view_position(*this)), get_tool_modifiers() ) );
  ToolResult r = tool->Preempt(info);
  if ( r == TOOL_COMMIT ){
    if ( option == PreemptOption::ALLOW_COMMAND ){
      CommitTool(tool);
    }
    return PREEMPT_COMMIT;
  }
  else if ( r == TOOL_CANCEL ){
    Refresh();
    return PREEMPT_CANCEL;
  }
  else if ( r == TOOL_CHANGE ){
    if ( option == PreemptOption::ALLOW_COMMAND ){
      CommitTool( tool );
    }
    m_toolWrap.SetSwitched( 0 );
    return PREEMPT_COMMIT;
  }
  else {
    Refresh();
    return PREEMPT_NONE;
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


  old_command_t redone = m_redoList.front();
  m_redoList.pop_front();
  RunCommand( redone.first, clear_redo(false), redone.second );
}

void CanvasScroller::RemoveFrame( faint::Image* image ){
  m_images.Remove(image);
  GetAppContext().OnDocumentStateChange(m_canvasId); // Fixme: Queue!
}

void CanvasScroller::RemoveFrame( const index_t& index ){
  m_images.Remove(index);
  GetAppContext().OnDocumentStateChange(m_canvasId); // Fixme: Queue!
}

void CanvasScroller::ReorderFrame( const new_index_t& newIndex, const old_index_t& oldIndex ){
  m_images.Reorder(newIndex, oldIndex);
  GetAppContext().OnDocumentStateChange(m_canvasId); // Fixme: Queue!
  Refresh();
}

void CanvasScroller::Rescale( const IntSize& sz ){
  RunCommand( get_rescale_command(sz, ScaleQuality::BILINEAR ) );
}

void CanvasScroller::Resize( const IntRect& r ){
  RunCommand( get_resize_command( m_images.Active().GetBitmap(), r, GetAppContext().Get(ts_BgCol) ) );
}

void CanvasScroller::Rotate90CW(){
  const ApplyTarget target = GetApplyTarget();
  if ( target == APPLY_IMAGE ){
    RunCommand( rotate_image_90cw_command() );
  }
  else if ( target == APPLY_OBJECT_SELECTION ){
    const objects_t& objectSelection(m_images.Active().GetObjectSelection());
    Point origin = bounding_rect(objectSelection).Center();
    RunCommand( get_rotate_command( objectSelection, faint::pi / 2, origin ) );
  }
  else if ( target == APPLY_RASTER_SELECTION ){
    RunCommand(get_rotate_selection_command(m_images.Active()));
    GetAppContext().OnDocumentStateChange(m_canvasId); // Fixme: For undo/redo
  }
  else {
    assert( false );
  }
  Refresh( true );
}

void CanvasScroller::RunCommand( Command* cmd ){
  // When a command is run, any commands in the redo list must be
  // cleared (See exception in CanvasScroller::Redo).
  RunCommand(cmd, clear_redo(true));
}

void CanvasScroller::RunDWIM(){
  if ( !m_undoList.empty() && m_undoList.back().first->HasDWIM() ){
    Command* dwim = m_undoList.back().first->GetDWIM();
    Undo();
    RunCommand(dwim);
    Refresh();
  }
}

void CanvasScroller::ScaleRasterSelection( const IntSize& size ){
  RunCommand( get_scale_raster_selection_command( m_images.Active(), size ));
  Refresh( true );
}

void CanvasScroller::ScaleSelectedObject( const Size& size ){
  assert( HasObjectSelection() );
  faint::Image& active(m_images.Active());
  const objects_t& objectSelection(active.GetObjectSelection());
  Rect oldRect = bounding_rect(objectSelection);
  Point origin = oldRect.TopLeft();
  Scale scale(New(size), oldRect.GetSize());
  RunCommand( get_scale_command( objectSelection, scale, origin ) );
  Refresh( true );
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

void CanvasScroller::SelectAll(){
  if ( m_toolWrap.GetTool()->HasSelection() ){
    if ( HandleToolResult(m_toolWrap.GetTool()->SelectAll() ) ){
      return;
    }
  }
  if ( ShouldDrawRaster() ){
    GetAppContext().SelectTool( T_RECT_SEL );
    SetRasterSelection( image_rect(m_images.Active()) );
  }
  else {
    GetAppContext().SelectTool( T_OBJ_SEL );
    SetObjectSelection( m_images.Active().GetObjects() );
  }
  Refresh();
}

void CanvasScroller::SelectObject( Object* object, const deselect_old& deselectOld ){
  faint::Image& active(m_images.Active());
  if ( deselectOld.Get() ){
    active.SetObjectSelection(objects_t()); // Fixme
  }
  else if ( contains(active.GetObjectSelection(), object) ){
    return;
  }

  objects_t objectSelection(m_images.Active().GetObjectSelection());
  size_t pos = get_sorted_insertion_pos( object, objectSelection, m_images.Active().GetObjects() );
  objectSelection.insert( objectSelection.begin() + pos, object );
  active.SetObjectSelection(objectSelection);
  m_toolWrap.GetTool()->SelectionChange();
  GetAppContext().OnDocumentStateChange(m_canvasId);
  Refresh();
}

void CanvasScroller::SelectObjects( const objects_t& objects, const deselect_old& deselectOld ){
  faint::Image& active(m_images.Active());
  if ( deselectOld.Get() ){
    active.SetObjectSelection(objects_t());
  }
  const objects_t& imageObjects(active.GetObjects());
  objects_t objectSelection(active.GetObjectSelection());
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* obj = objects[i];
    if ( lacks(objectSelection, obj) ){
      size_t pos = get_sorted_insertion_pos( obj, objectSelection, imageObjects );
      objectSelection.insert(objectSelection.begin() + pos, obj );
    }
  }
  active.SetObjectSelection(objectSelection);
  m_toolWrap.GetTool()->SelectionChange();
  GetAppContext().OnDocumentStateChange(m_canvasId);
  Refresh();
}

void CanvasScroller::SetChimera( RasterSelection* selectionChimera ){
  m_rasterSelectionChimera = selectionChimera;
}

void CanvasScroller::SetDirty(){
  m_dirty = true;
  GetAppContext().OnDocumentStateChange(m_canvasId);
}

void CanvasScroller::SetFilename(const std::string& strFilename ){
  if ( !strFilename.empty() ){
    wxFileName filename( strFilename );
    assert( filename.IsAbsolute() );
    assert( !filename.IsDir() );
  }
  m_filename = strFilename;
}

void CanvasScroller::SetGrid( const Grid& g ){
  m_grid = g;
  // Fixme: Refresh. Notify mainframe and so on.
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

void CanvasScroller::SetObjectSelection( const objects_t& objects ){
  m_images.Active().SetObjectSelection(objects);
  m_toolWrap.GetTool()->SelectionChange();
  GetAppContext().OnDocumentStateChange(m_canvasId);
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
  ToolId id = m_toolWrap.GetToolId();
  return ( GetAppContext().GetLayerType() == Layer::RASTER && id != T_OBJ_SEL && id != T_OTHER ) ||
    (id == T_RECT_SEL || id == T_PEN || id == T_BRUSH );
}

bool CanvasScroller::ShouldDrawVector() const{
  return !ShouldDrawRaster();
}

void CanvasScroller::ShowRotateDialog(){
  wxString title = "";
  ApplyTarget target(GetApplyTarget());
  if ( target == APPLY_OBJECT_SELECTION ){
    title = wxString(space_sep("Flip/Rotate", get_collective_name(m_images.Active().GetObjectSelection())));
  }
  else if ( target == APPLY_RASTER_SELECTION ){
    title = "Flip/Rotate Selection";
  }
  else {
    assert( target == APPLY_IMAGE );
    title = "Flip/Rotate Image";
  }
  RotateDialog rotateDialog( this, title );
  if ( faint::show_modal(rotateDialog) != wxID_OK ){
    return;
  }

  RotateDialog::operation op = rotateDialog.GetOperation();
  if ( op.choice == RotateDialog::FLIP_HORIZONTAL ){
    Apply(OperationFlip(Axis::HORIZONTAL));
  }
  else if ( op.choice == RotateDialog::FLIP_VERTICAL ){
    Apply(OperationFlip(Axis::VERTICAL));
  }
  else if ( op.choice == RotateDialog::ROTATE ){
    Rotate90CW();
  }
}

void CanvasScroller::ShowResizeDialog(){
  // Fixme: Some of this should be updated to use IntSize etc. as early (code-wise) as possible
  // Note that only objects should support floating scaling
  bool scaleOnly = false;
  wxString title = "Resize or Scale Image";
  ApplyTarget target(GetApplyTarget());
  if ( target == APPLY_RASTER_SELECTION ){
    scaleOnly = true;
    title = "Scale Selection";
  }
  else if ( target == APPLY_OBJECT_SELECTION ){
    scaleOnly = true;
    title = wxString(space_sep("Resize", get_collective_name(m_images.Active().GetObjectSelection())));
  }
  else {
    assert( target == APPLY_IMAGE );
  }

  Size originalSize(GetApplyTargetSize());
  // Fixme: Floating point size needed for objects
  AppContext& app = GetAppContext();
  ResizeDialog resizeDialog( this,
    title,
    ResizeDialogSettings( originalSize.w, originalSize.h, scaleOnly,
      app.GetDefaultResizeDialogSettings() ) );
  resizeDialog.SetBgColor( app.Get( ts_BgCol ) );
  if ( faint::show_modal( resizeDialog ) != wxID_OK ){
    return;
  }

  ResizeDialogSettings s = resizeDialog.GetSelection();
  if ( s.w <= 0 || s.h <= 0 ){
    // Invalid size specified, do nothing.
    return;
  }

  app.SetDefaultResizeDialogSettings(s);

  Size newSize( s.w, s.h );
  if ( target == APPLY_RASTER_SELECTION ){
    ScaleRasterSelection( truncated(newSize) );
    return;
  }
  else if ( target == APPLY_OBJECT_SELECTION ){
    ScaleSelectedObject( newSize );
    return;
  }
  else {
    // Scale or resize the entire image
    if ( s.defaultButton == ResizeDialogSettings::RESIZE_TOP_LEFT ){
      Resize(rect_from_size(truncated(newSize))); // Fixme: the truncating: should be IntSize already?
    }
    else if ( s.defaultButton == ResizeDialogSettings::RESIZE_CENTER ){
      Size oldSize = floated(GetBitmapSize()); // Fixme: Use the IntSize
      IntRect r( truncated(Point( 0 - ( newSize.w - oldSize.w ) / 2,
          0 - ( newSize.h - oldSize.h ) / 2 )),
        truncated( newSize ) );
      Resize( r );
    }
    else if ( s.defaultButton == ResizeDialogSettings::RESCALE ){
      // Fixme: Should already be int
      Rescale( truncated(newSize) ); // Fixme: the truncating: should be IntSize already?
    }
    Refresh();
  }
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
  if ( res == PREEMPT_CANCEL ){
    // The tool was aborted by the undo
    return;
  }

  // The tool either committed on the preempt (and the command should
  // now be undone) or there was no effect. Proceed with normal
  // undo-behavior.
  if ( m_undoList.empty() ){
    return;
  }

  old_command_t undone = m_undoList.back();
  CommandType undoType( undone.first->Type() );
  faint::Image* activeImage = undone.second;
  m_cmdContext->SetFrame(activeImage);
  if ( undoType == CMD_TYPE_OBJECT || undoType == CMD_TYPE_HYBRID ){
    // Hybrid and object commands must undo their changes to objects
    undone.first->Undo( *m_cmdContext );
  }
  if ( undoType == CMD_TYPE_RASTER || undoType == CMD_TYPE_HYBRID ){
    SetRasterSelection(translated(activeImage->GetRasterSelection().GetRect(), -undone.first->SelectionOffset()));
    // When a raster or hybrid command is undone, the image is reset
    // and the raster steps of all commands are reapplied.
    activeImage->Revert();
    for ( std::deque<old_command_t>::iterator it = m_undoList.begin(); it != m_undoList.end() - 1; ++it ){
      if ( it->second == activeImage ){
	Command* reapplied = it->first;
	reapplied->DoRaster( *m_cmdContext );
      }
    }
  }
  m_undoList.pop_back();
  assert(!has_command(m_redoList, undone.first));
  m_redoList.push_front( undone );

  objects_t objectSelection(activeImage->GetObjectSelection()); // Fixme: Move removal into faint::Image
  remove_missing_objects_from(objectSelection, *activeImage);
  activeImage->SetObjectSelection(objectSelection);
  if ( m_undoList.empty() ){
    ClearDirty();
  }

  GetAppContext().OnDocumentStateChange(m_canvasId);

  // Update the selection settings etc (originally added to make the
  // ObjSelectTool change the controls if a setting change was
  // undone)
  m_toolWrap.GetTool()->SelectionChange();
}

void CanvasScroller::UngroupSelected(){
  if ( !contains_group( m_images.Active().GetObjectSelection() ) ){
    return;
  }

  Preempt(PreemptOption::ALLOW_COMMAND);
  RunCommand( new UngroupObjects( m_images.Active().GetObjectSelection(), select_added(true) ) );
  m_toolWrap.GetTool()->SelectionChange();
  Refresh();
}

void CanvasScroller::ZoomFit(){
  Size image(floated(m_images.Active().GetSize()));
  Size client(floated(to_faint(GetClientSize()) - IntSize(m_geo.left_border * 2, m_geo.top_border * 2) ));
  Scale rel(NewSize(client), image);
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
  return (info.handleIndex != -1) && (m_toolWrap.GetToolId() == T_OBJ_SEL);
}

void CanvasScroller::MousePosRefresh( const wxPoint& mousePos, int modifiers ){
  CursorPositionInfo info = HitTest( mousePos, modifiers );
  if ( !HasCapture() && !IgnoreCanvasHandle(info) ){
    int canvasHandle = canvas_handle_hit_test( mousePos, m_images.Active().GetSize(), m_geo );
    if ( canvasHandle != - 1 ){
      SetCursor( CanvasHandleCursor( canvasHandle ) );
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
  return IntRect( truncated(Point((m_geo.x0 - m_geo.left_border) / zoom, (m_geo.y0 - m_geo.top_border) / zoom)),
    truncated(Size(sz.x / zoom + 100, sz.y / zoom + 100 )));
}

void CanvasScroller::RefreshToolRect(){
  Tool* tool = m_toolWrap.GetTool();
  IntRect toolRect(tool->GetRefreshRect(GetVisibleImageRect(), faint::mouse::image_position( m_geo, *this ) ) );
  InclusiveRefresh(faint::mouse::image_to_view( toolRect, m_geo ));
}

void CanvasScroller::CommitTool( Tool* tool, bool refresh ){
  Command* cmd = tool->GetCommand();
  if ( cmd != 0 ){
    RunCommand( cmd );
    if ( refresh ){
      Refresh();
    }
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
    clipboard.SetBitmap( selection.Floating() ?
      selection.GetBitmap() :
      GetSubBitmap(GetImageSelection().GetRect()), GetAppContext().GetToolSettings().Get(ts_BgCol));
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
      active.SetObjectSelection(objects_t());
      Refresh();
    }
    return true;
  }
  return false;
}

CursorPositionInfo CanvasScroller::HitTest( const wxPoint& p, int modifiers ){
  int handleIndex = -1;
  int hitStatus = HIT_NONE;
  Object* object = 0;
  bool selected = false;

  // Selected objects are checked first so that they are prioritized
  const objects_t& objectSelection(m_images.Active().GetObjectSelection());
  for ( size_t i = 0; i != objectSelection.size(); i++ ){
    Object* objTemp = objectSelection[i];

    if ( point_edit_enabled(objTemp) ){
      handleIndex = point_hit_test( objTemp->GetMovablePoints(), p, m_geo );
      if ( handleIndex != - 1 ){
	hitStatus = HIT_MOVABLE_POINT;
	selected = true;
	object = objTemp;
	break;
      }

      handleIndex = point_hit_test( objTemp->GetExtensionPoints(), p, m_geo );
      if ( handleIndex != -1 ){
	hitStatus = HIT_EXTENSION_POINT;
	selected = true;
	object = objTemp;
	break;
      }
    }

    if ( resize_handles_enabled(objTemp) ){
      handleIndex = get_handle_index( corners(objTemp->GetRect()), p, m_geo );
      if ( handleIndex != -1 ){
	hitStatus = HIT_RESIZE_POINT;
	selected = true;
	object = objTemp;
	break;
      }
    }
  }

  if ( handleIndex == -1 ){
    Point ptImg = faint::mouse::view_to_image( to_faint(p), m_geo );
    std::pair<Object*, HitInfo> hitObject = ObjectAt( ptImg );
    object = hitObject.first;
    hitStatus = hitObject.second;
    selected = object == 0 ? false : contains(m_images.Active().GetObjectSelection(), object );
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

std::pair<Object*, HitInfo> CanvasScroller::ObjectAt( const Point& p ){
  // Create a small scaled and adjusted DC where the clicked pixel is
  // anchored at 0,0 in DC coordinates for pixel object hit tests
  FaintDC dc( m_hitTestBuffer );
  const faint::coord zoom = m_geo.zoom.GetScaleFactor();
  dc.SetOrigin(-p * zoom );
  dc.SetScale( zoom );
  dc.Clear( mask_outside );

  // First consider the selected objects...
  Object* consider = 0;
  HitInfo considerType = HIT_NONE;
  const objects_t& objectSelection(m_images.Active().GetObjectSelection());
  for ( size_t i = 0; i != objectSelection.size(); i++ ){
    Object* object = objectSelection[i];
    if ( object->HitTest( p ) ){
      object->DrawMask( dc );
      faint::Color color = dc.GetPixel( p );
      if ( color  == mask_edge ){ // Fixme: Consider fuzzy or disabling AA
        return std::make_pair(object, HIT_BOUNDARY);
      }
      else if ( color == mask_fill ){
        return std::make_pair(object, HIT_INSIDE);
      }
      else {
        consider = object;
        considerType = HIT_NEAR;
      }
    }
  }

  // ...then the rest
  dc.Clear( mask_outside );

  const objects_t& objects( m_images.Active().GetObjects() );
  for ( int i = static_cast<int>(objects.size()) - 1; i != -1; i-- ){
    Object* object = objects[i];
    if ( object->HitTest( p ) ){
      object->DrawMask( dc );
      faint::Color color =  dc.GetPixel( p );
      if ( color  == mask_edge ){
        return std::make_pair(object, HIT_BOUNDARY);
      }
      else if ( color == mask_fill ){
        return std::make_pair(object, HIT_INSIDE);
      }
      else if ( (consider == 0 || considerType == HIT_NEAR ) && color == mask_no_fill ){
        consider = object;
        considerType = HIT_INSIDE;
      }
      else if ( consider == 0 ){
        consider = object;
        considerType = HIT_NEAR;
      }
    }
  }
  return std::make_pair(consider, considerType);
}

void CanvasScroller::SetFaintCursor( Cursor::type cursor ){
  if ( cursor == Cursor::DONT_CARE ){
    return;
  }
  SetCursor(to_wx_cursor(cursor));
}

void CanvasScroller::RunCommand( Command* cmd, const clear_redo& clearRedo, faint::Image* activeImage ){
  if ( activeImage == 0 ){
    activeImage = &(m_images.Active());
  }
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
    m_undoList.push_back( std::make_pair(cmd, activeImage) );
    m_dirty = true;
    GetAppContext().OnDocumentStateChange(m_canvasId);

    if ( oldSize != m_images.Active().GetSize() ){
      // If the image size changes it is quite likely that the current
      // position will be outside the drawing area in a sea of dark
      // grey, simple return to 0,0 whenever the size changes for now.
      Point pos(m_geo.x0, m_geo.y0);
      faint::coord zoom = m_geo.zoom.GetScaleFactor();
      pos = ( cmd->Translate(pos / zoom) * zoom );
      AdjustScrollbars( truncated(pos) );

      // Clip the current selection to the new image size
      activeImage->GetRasterSelection().Clip(image_rect(m_images.Active())); // Fixme: Move selection to image, fixme: Undo?
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
    RefreshRect( to_wx( union_of( r, m_lastRefreshRect ) ).Inflate(LITCRD(2.0) * zoom) );
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

void CanvasScroller::Initialize(){
  m_scroll.startX = 0;
  m_scroll.startY = 0;
  m_scroll.updateHorizontal = false;
  m_scroll.updateVertical = false;
  m_geo.x0 = 0;
  m_geo.y0 = 0;
  m_geo.left_border = 20;
  m_geo.top_border = 20;
  m_rasterSelectionChimera = 0;
  SetBackgroundColour( g_canvasBg );
  SetBackgroundStyle( wxBG_STYLE_PAINT );
  m_interface = new CanvasContext(this);
  m_dirty = false;

  AdjustScrollbars(IntPoint(0,0));
  SetDropTarget( new CanvasFileDropTarget() );
  m_cmdContext = new CommandContextImpl(*this);
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

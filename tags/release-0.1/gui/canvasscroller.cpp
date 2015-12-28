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
#include <functional>
#include "wx/filename.h"
#include "wx/dnd.h"
#include "bitmap/cairo_util.h"
#include "canvasscroller.hh"
#include "canvasinterface.hh"
#include "commands/cmdgroupobjects.hh"
#include "commands/deletecommand.hh"
#include "commands/delobjectcommand.hh"
#include "commands/orderobject.hh"
#include "commands/cmdfliprot.hh"
#include "commands/cmdsetbitmap.hh"
#include "commands/resizecommand.hh"
#include "commands/tricommand.hh"
#include "convertwx.hh"
#include "faintimage.hh"
#include "faintdc.hh"
#include "getappcontext.hh"
#include "objects/objraster.hh"
#include "tools/rectselbehavior.hh"
#include "tools/settingid.hh"
#include "tools/resizecanvas.hh"
#include "util/angle.hh"
#include "util/autocrop.hh"
#include "util/commandutil.hh"
#include "util/clipboard.hh"
#include "util/imageutil.hh"
#include "util/mouse.hh"
#include "util/objutil.hh"
#include "util/util.hh"
#include "gui/overlaydcwx.hh"

const wxColour canvasBg( 160, 140, 160 );
const int objectHandleWidth = 8;

template<int X, int Y, faint::Bitmap BmpFunc( const faint::Bitmap& bmp ), CmdFlipRotate::op OP>
struct OperationFlip {
  static Command* CommandObjects( std::vector<Object*>& objects ){
    Point origin = BoundingRect(objects).Center();
    std::vector<Command*> commands;
    for ( size_t i = 0; i != objects.size(); i++ ){
      Object* o = objects[i];
      const Tri& tri = o->GetTri();
      Tri newTri( Scaled( tri, X, Y, origin ) );
      commands.push_back( new TriCommand( o, newTri, tri ) );
    }
    return PerhapsBunch( CMD_TYPE_OBJECT, commands );
  }

  static Command* CommandRasterSelection( const IntRect& selection ){
    return new CmdFlipRotate( OP, selection );
  }

  static Command* CommandImage(){
    return new CmdFlipRotate( OP );
  }

  static void ApplyBitmap( faint::Bitmap& bmp ){
    bmp = BmpFunc( bmp );
  }
};

typedef OperationFlip<-1, 1, faint::FlipHorizontal, CmdFlipRotate::FLIP_HORIZONTAL> OperationFlipHorizontal;
typedef OperationFlip<1, -1, faint::FlipVertical, CmdFlipRotate::FLIP_VERTICAL> OperationFlipVertical;

class CanvasContext : public CanvasInterface {
public:
  CanvasContext( CanvasScroller* canvas ):
    m_canvas( canvas )
  {}

  void SelectObject( Object* obj, bool deselectOld ){
    m_canvas->SelectObject( obj, deselectOld );
  }

  void DeselectObject( Object* obj ){
    m_canvas->DeselectObject( obj );
  }

  void DeselectObjects(){
    m_canvas->DeselectObjects();
  };

  void DeselectRaster(){
    m_canvas->DeselectRaster();
  }

  faint::coord GetZoom() const {
    return m_canvas->GetZoom();
  }

  void ZoomIn(){
    m_canvas->ChangeZoom( ZoomLevel::NEXT );
  }

  void ZoomOut(){
    m_canvas->ChangeZoom( ZoomLevel::PREVIOUS );
  }

  void ZoomDefault(){
    m_canvas->ChangeZoom( ZoomLevel::DEFAULT );
  }

  #ifdef FAINT_RVALUE_REFERENCES
  faint::Bitmap GetBitmap( const IntRect& r ){
    return m_canvas->GetBitmap( r );
  }
  #else
  faint::Bitmap GetBitmap( const IntRect& r ){
    faint::Image& image = m_canvas->GetImage();
    faint::Bitmap* bmp = image.GetBitmap();
    return CairoCompatibleSubBitmap( *bmp, r.x, r.y, r.w, r.h );
  }
  #endif

  faint::Bitmap& GetBitmap(){
    return m_canvas->GetImage().GetBitmapRef();
  }

  IntSize GetSize() const {
    return m_canvas->GetBitmapSize();
  }

  std::vector<Object*>& GetSelectedObjects(){
    return m_canvas->GetSelectedObjects();
  }
  std::vector<Object*>& GetObjects(){
    return m_canvas->GetObjects();
  }

  Point GetRelativeMousePos(){
    return m_canvas->GetRelativeMousePos();
  }

  void Refresh(){
    m_canvas->Refresh();
  }

  void Undo(){
    m_canvas->Undo();
  }

  void Redo(){
    m_canvas->Redo();
  }

  void RunCommand( Command* cmd ){
    if ( cmd != 0 ){
      m_canvas->RunCommand( cmd );
    }
  }

  void ContextDeselect(){
    m_canvas->Deselect();
  }


  void ClearDirty(){
    m_canvas->ClearDirty();
  }

  IntRect GetRasterSelection(){
    return m_canvas->GetRasterSelection();
  }

  virtual faint::Image& GetImage(){
    return m_canvas->GetImage();
  }

  void SetRasterSelection( const IntRect& r ){
    m_canvas->SetRasterSelection( r );
  }

  std::string GetFilename() const {
    return m_canvas->GetFilename();
  }

  void SetFilename( const std::string& filename ){
    m_canvas->SetFilename( filename );
  }

  void CenterView( const Point& pt ){
    m_canvas->CenterViewImage( pt );
  }

  CanvasId GetId() const {
    return m_canvas->GetCanvasId();
  }

  bool Has( const ObjectId& id ) const {
    return m_canvas->Has( id );
  }

  void ContextCrop(){
    m_canvas->Crop();
  }

  void ContextFlipHorizontal(){
    m_canvas->ApplyOperation<OperationFlipHorizontal>();
  }

  void ContextFlipVertical(){
    m_canvas->ApplyOperation<OperationFlipVertical>();
  }

  void ContextRotate90CW(){
    m_canvas->Rotate90CW();
  }

  void ContextDelete(){
    m_canvas->DeleteSelection();
  }

  void ContextFlatten(){
    m_canvas->Flatten();
  }

  void SetGrid( const Grid& g ){
    m_canvas->SetGrid(g);
  }

  Grid GetGrid() const{
    return m_canvas->GetGrid();
  }

  virtual void ScrollPageUp(){
    m_canvas->ScrollPageUp();
  }

  virtual void ScrollPageDown(){
    m_canvas->ScrollPageDown();
  }
  virtual void ScrollPageLeft(){
    m_canvas->ScrollPageLeft();
  }

  virtual void ScrollPageRight(){
    m_canvas->ScrollPageRight();
  }

  virtual void ScrollMaxLeft(){
    m_canvas->ScrollMaxLeft();
  }

  virtual void ScrollMaxRight(){
    m_canvas->ScrollMaxRight();
  }

  virtual void ScrollMaxUp(){
    m_canvas->ScrollMaxUp();
  }

  virtual void ScrollMaxDown(){
    m_canvas->ScrollMaxDown();
  }
private:
  CanvasScroller* m_canvas;
};

wxBitmap& GetBackBuffer( const IntSize& minSize ){
  static wxBitmap backBuffer( minSize.w,minSize.h );
  wxSize oldSize = backBuffer.GetSize();
  if ( oldSize.GetWidth() < minSize.w || oldSize.GetHeight() < minSize.h ){
    backBuffer = wxBitmap( std::max( minSize.w, oldSize.GetWidth() ),
      std::max( minSize.h, oldSize.GetHeight() ) );
  }
  return backBuffer;
}

void PaintObjectHandles( std::vector<Object*>& objects, wxDC& dc, faint::coord scale ){
  OverlayPenAndBrush( dc );

  for ( size_t i= 0; i != objects.size(); i++ ){
    if ( objects[i]->Active() ){
      // Do not draw handles for objects currently adjusted so they
      // don't get in the way.
      continue;
    }
    std::vector<Point> points = objects[i]->GetResizePoints();
    std::vector<Point> mvPoints = objects[i]->GetMovablePoints();
    points.insert( points.end(), mvPoints.begin(), mvPoints.end() );

    for ( size_t pn = 0; pn != points.size(); pn++ ){
      Point& p = points[pn];
      dc.DrawRectangle( p.x * scale - objectHandleWidth / LITCRD(2.0) + LITCRD(0.5), p.y * scale - objectHandleWidth / LITCRD(2.0) + LITCRD(0.5), objectHandleWidth, objectHandleWidth );
    }
  }
}

void PaintSelection( const IntRect& selection, wxDC& dc ){
  wxBrush brush( wxColour(0,0,0));
  brush.SetStyle( wxTRANSPARENT );
  dc.SetBrush( brush );
  dc.SetPen( wxPen(wxColour(255,0,255), 1, wxPENSTYLE_DOT ) );
  dc.DrawRectangle( to_wx( selection ) );
}

IntRect ImageRect( const faint::Image& image ){
  return IntRect( IntPoint(0, 0), image.GetSize() );
}

int GetHandleIndex( const std::vector<Point>& points, const wxPoint& pos, const Geometry& g ){
  for ( size_t i = 0; i != points.size(); i++ ){
    const Point& ptHandle = points[i];
    const faint::coord zoom = g.zoom.GetScaleFactor();

    Point topLeft( ptHandle.x * zoom - objectHandleWidth / 2, ptHandle.y * zoom - objectHandleWidth/ 2 );
    Rect r( topLeft, Size(objectHandleWidth, objectHandleWidth ) );
    if ( r.Contains( Point( pos.x - g.left_border + g.x0, pos.y - g.top_border + g.y0 ) ) ){
      return i;
    }
  }

  Point p2( pos.x - g.left_border + g.x0, pos.y - g.top_border + g.y0 );
  // Sides
  struct {
    int in;
    int out;
  } off = {1, objectHandleWidth / 2};

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


// Find where in v_trg to insert object to keep relative sort order of v_src
int GetSortedInsertionPos( Object* obj, const std::vector<Object*>& v_trg, const std::vector<Object*>& v_src ){
  size_t i_src = 0;
  size_t i_trg = 0;
  for ( ; i_trg != v_trg.size(); i_trg++ ){
    const Object* o_trg = v_trg[i_trg];

    for ( ; i_src!= v_src.size(); i_src++ ){
      const Object* o_src = v_src[i_src];
      if ( o_src == obj ){
        // Object to be inserted found before reference object ->
        // this is the insertion point.
        return i_trg;
      }
      if ( o_src == o_trg ){
        // Reference object found in destination vector -
        // inserted object must be later in the source vector.
        break;
      }
    }
  }
  return i_trg;
}

void ClearList( std::deque<Command*>& list ){
  for ( std::deque<Command*>::iterator it = list.begin(); it != list.end(); ++it ){
    delete (*it);
  }
  list.clear();
}

class CanvasFileDropTarget : public wxFileDropTarget {
public:
  bool OnDropFiles( wxCoord /*x*/, wxCoord /*y*/, const wxArrayString& filenames ){
    AppContext& app = GetAppContext();
    for ( size_t i = 0; i != filenames.size(); i++ ){
      app.Load( filenames[i].c_str() );
    }
    return true;
  }
};

CanvasScroller::CanvasScroller( wxWindow* parent, const faint::Bitmap& bitmap )
  : wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL|wxHSCROLL | wxWANTS_CHARS ),
    m_hitTestBuffer( faint::CairoCompatibleBitmap( 20, 20 ) )
{
  Initialize( new faint::Image( bitmap ) );
}

CanvasScroller::CanvasScroller( wxWindow* parent, const CanvasInfo& info, const std::vector<Object*>& objects )
  : wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL|wxHSCROLL | wxWANTS_CHARS ),
    m_hitTestBuffer( faint::CairoCompatibleBitmap( 20, 20 ) )
{
  Initialize( new faint::Image( IntSize(info.width, info.height), objects ) );
}

CanvasScroller::CanvasScroller( wxWindow* parent, ImageProps& props )
  : wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL|wxHSCROLL | wxWANTS_CHARS ),
    m_hitTestBuffer( faint::CairoCompatibleBitmap( 20, 20 ) )
{
  Initialize( new faint::Image(props) );
}

CanvasId CanvasScroller::GetCanvasId() const{
  return m_canvasId;
}

ApplyTarget CanvasScroller::GetApplyTarget() {
  AppContext& app = GetAppContext();
  ToolId tool = m_toolWrap.GetToolId();
  if ( m_toolWrap.Tool()->HasBitmap() ){
    return APPLY_ACTIVE_TOOL;
  }
  else if ( tool == T_RECT_SEL && HasRasterSelection() ){
    return APPLY_RASTER_SELECTION;
  }
  else if ( tool == T_OBJ_SEL && HasObjectSelection() ){
    return APPLY_OBJECT_SELECTION;
  }
  else if ( app.GetLayerType() == LAYER_OBJECT && HasObjectSelection() ){
    return APPLY_OBJECT_SELECTION;
  }
  else if ( app.GetLayerType() == LAYER_RASTER && HasRasterSelection() ){
    return APPLY_RASTER_SELECTION;
  }
  return APPLY_IMAGE;
}

Size CanvasScroller::GetApplyTargetSize(){
  ApplyTarget target = GetApplyTarget();
  if ( target == APPLY_ACTIVE_TOOL ){
    return floated(m_toolWrap.Tool()->GetBitmap()->Size());
  }
  else if ( target == APPLY_OBJECT_SELECTION ){
    return BoundingRect(m_objectSelection).GetSize();
  }
  else if ( target == APPLY_RASTER_SELECTION ){
    return floated(m_rasterSelection.GetSize());
  }
  else {
    return floated(m_image->GetSize());
  }
}

bool CanvasScroller::AcceptsFocus() const{
  return true;
}

void CanvasScroller::Initialize( faint::Image* image ){
  m_updateHorizontal = false;
  m_updateVertical = false;
  m_geo.x0 = 0;
  m_geo.y0 = 0;
  m_geo.left_border = 20;
  m_geo.top_border = 20;

  m_image = image;
  SetBackgroundColour( canvasBg );
  SetBackgroundStyle( wxBG_STYLE_PAINT );
  m_interface = new CanvasContext(this);
  m_dirty = false;

  m_lastWasDelete = false;
  AdjustScrollbars();
  SetDropTarget( new CanvasFileDropTarget() );
}

CanvasScroller::~CanvasScroller(){
  if ( HasCapture() ){
    ReleaseMouse();
  }

  ClearList( m_redoList );
  delete m_image;
  ClearList( m_undoList );
  delete m_interface;
}

wxRect CanvasHandleRect( size_t handle, const IntSize& canvasSize, const Geometry& geo ){
  const int ch_w = 6;
  const int ch_h = 6;

  const int x_off = geo.left_border / 2 - ch_w / 2;
  const int y_off = geo.top_border / 2 - ch_h / 2;

  const int left( -geo.x0 + x_off );
  const int top( -geo.y0 + y_off );

  const faint::coord zoom = geo.zoom.GetScaleFactor();
  const int width = rounded(canvasSize.w * zoom);
  const int height = rounded(canvasSize.h * zoom);
  const int right( geo.left_border + width - geo.x0 + x_off );
  const int bottom( geo.top_border + height - geo.y0 + y_off );

  if ( handle == 0 ){
    return wxRect( left, top, ch_w, ch_h );
  }
  else if ( handle == 1 ){
    return wxRect( right, top, ch_w, ch_h );
  }
  else if ( handle == 2 ){
    return wxRect( left, bottom, ch_w, ch_h );
  }
  else if ( handle == 3 ){
    return wxRect( right, bottom, ch_w, ch_h );
  }
  else if ( handle == 4 ){
    return wxRect( geo.left_border + width / 2 - geo.x0 - ch_w / 2,
      top, ch_w, ch_h );
  }
  else if ( handle == 5 ){
    return wxRect( geo.left_border + width / 2 - geo.x0 - ch_w / 2,
      bottom, ch_w, ch_h );
  }
  else if (handle == 6 ){
    return wxRect( right,
      geo.top_border + height / 2 - geo.y0 - ch_h / 2,
      ch_w, ch_h );
  }
  else if (handle == 7 ){
    return wxRect( left,
      geo.top_border + height / 2 - geo.y0 - ch_h / 2,
      ch_w, ch_h );
  }
  else {
    assert( false );
    return wxRect(0,0,0,0);
  }
}

int CanvasHandleHitTest( wxPoint pos, const IntSize& canvasSize, const Geometry& geo ){
  for ( size_t i = 0; i != 8; i++ ){
    if ( CanvasHandleRect( i, canvasSize, geo ).Contains( pos ) ){
      return i;
    }
  }
  return -1;
}


void PaintCanvasHandles( wxDC& dc, const IntSize& canvasSize, const Geometry& geo ){
  dc.SetPen( wxPen(wxColour(128, 128, 128), 1 ) );
  dc.SetBrush( wxBrush(wxColour(181, 165, 213) ) );

  for ( size_t i = 0; i != 8; i++ ){
    dc.DrawRectangle( CanvasHandleRect( i, canvasSize, geo ) );
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

void Paint_EarlyReturn( wxDC& dc, Rect rView, const Geometry& geo, const IntSize& imageSize ){
  // Fixme: rView should be IntRect?
  dc.SetBrush( wxBrush( canvasBg ) );
  dc.SetPen( wxPen( canvasBg ) );
  dc.DrawRectangle( rView.x, rView.y, rView.w, rView.h );
  PaintCanvasHandles( dc, imageSize, geo );
}

void Paint_CheckeredBackground( wxDC& dc, const IntPoint& topLeft, const IntPoint& imageRegionTopLeft, const Geometry& geo, int w, int h  ){
  const faint::coord zoom = geo.zoom.GetScaleFactor();
  int xOff = truncated( imageRegionTopLeft.x * zoom ) % 40;
  int yOff = truncated( imageRegionTopLeft.y * zoom ) % 40;
  int j_max = h / 20 + 2;
  int i_max = w / 20 + 2;
  for ( int j = 0; j <= j_max ; j++ ){
    dc.SetBrush(wxBrush( wxColour(192, 192, 192 )));
    dc.SetPen(wxPen( wxColour(192,192,192)));
    int shift = ( j % 2 ) * 20;
    for ( int i = 0; i <= i_max; i++ ){
      dc.DrawRectangle( 0 - xOff + i * 40 + shift + topLeft.x, 0 - yOff + j * 20 + topLeft.y, 20, 20 );
    }
    dc.SetBrush( wxBrush( wxColour(255, 255, 255 ) ) );
    dc.SetPen(wxPen( wxColour(255,255,255)));
    for ( int i = 0; i != i_max; i++ ){
      dc.DrawRectangle( 0 - xOff + i * 40 - 20 + shift + topLeft.x, 0 - yOff + j * 20 + topLeft.y, 20, 20 );
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
  rImage = Intersection( rImage, Rect( Point(0, 0), floated(bmpSize) ) );
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

void CanvasScroller::OnPaint( wxPaintEvent& ){
  Rect rView = floated(to_faint( GetUpdateRegion().GetBox() ));
  faint::Bitmap* faint_bmp = m_image->GetBitmap();
  IntSize bmpSize( faint_bmp->m_w, faint_bmp->m_h );
  IntRect imageRegion = Paint_GetImageRegion( rView, bmpSize, m_geo );
  Rect imageCoordRect = Paint_InImageCoordinates( rView, m_geo );

  if ( Empty( imageRegion ) ){
    wxPaintDC paintDC( this );
    Paint_EarlyReturn( paintDC, rView, m_geo, m_image->GetSize() );
    return;
  }

  const Point mouseImagePos = faint::mouse::ImagePosition( m_geo, *this);

  faint::Bitmap subBitmap = CairoCompatibleSubBitmap( *faint_bmp, imageRegion );
  Overlays overlays;
  if ( m_toolWrap.DrawBeforeZoom() ){
    FaintDC dc ( subBitmap );
    dc.SetOrigin( -imageRegion.TopLeft().x, -imageRegion.TopLeft().y );
    m_toolWrap.Tool()->Draw( dc, overlays, mouseImagePos );
  }

  const faint::coord zoom = m_geo.zoom.GetScaleFactor();
  faint::Bitmap scaled = m_geo.zoom.At100() ?
    subBitmap : ( zoom > LITCRD(1.0) ?
      ScaleNearest( subBitmap, rounded(zoom) ):
      ScaleBilinear( subBitmap, zoom, zoom ) );

  if ( scaled.m_w <= 0 || scaled.m_h <= 0 ){
    wxPaintDC paintDC(this);
    Paint_EarlyReturn( paintDC, rView, m_geo, m_image->GetSize() );
    return;
  }

  // Top left of image in view-coordinates
  IntPoint topLeft( faint::mouse::ImageToView_tr( floated(imageRegion.TopLeft()), m_geo ) );

  FaintDC dc( scaled );
  dc.SetOrigin( -imageRegion.TopLeft().x * zoom, -imageRegion.TopLeft().y * zoom );
  dc.SetScale( zoom );
  std::vector<Object*>& objects = m_image->GetObjects();
  for ( size_t i = 0; i != objects.size(); i++ ){
    objects[i]->Draw( dc );
  }

  if ( !m_toolWrap.DrawBeforeZoom() ){
    m_toolWrap.Tool()->Draw( dc, overlays, mouseImagePos );
  }

  wxBitmap& backBuffer = GetBackBuffer( truncated( rView.GetSize() ) );

  // Draw the image and any overlays to the back-buffer to avoid
  // flickering
  wxMemoryDC memDC(backBuffer);
  // Fixme: Consider drawing filled rectangles with canvasbg instead,
  // to clear less.
  memDC.SetBackground(wxBrush( canvasBg ));
  memDC.Clear();

  // Fixme: Using a clipping region is ok, but the drawn checkered area seems _much_ too large when not
  // clipped
  IntRect clipRect( IntPoint(topLeft.x - rView.x, topLeft.y - rView.y), IntSize(scaled.m_w, scaled.m_h) );
  memDC.SetClippingRegion( to_wx(clipRect) );
  Paint_CheckeredBackground( memDC, IntPoint( topLeft.x - rView.x,
      topLeft.y - rView.y), imageRegion.TopLeft(), m_geo, scaled.m_w, scaled.m_h );
  memDC.DestroyClippingRegion();

  // Draw the scaled part of the bitmap to the back buffer
  memDC.DrawBitmap( to_wx( scaled ), topLeft.x - rView.x, topLeft.y - rView.y );
  memDC.SetDeviceOrigin( -rView.x, -rView.y );
  PaintCanvasHandles( memDC, m_image->GetSize(), m_geo );

  // Offset the DC for the backbuffer so that the top left corner of
  // the image is the origin (0,0)
  faint::coord ori_x = m_geo.left_border - m_geo.x0 - rView.x;
  faint::coord ori_y = m_geo.top_border - m_geo.y0 - rView.y;
  memDC.SetDeviceOrigin( ori_x, ori_y );

  if ( ShouldDrawRaster() && HasRasterSelection() ) {
    Rect scaledSelection( Point(m_rasterSelection.x * zoom, m_rasterSelection.y * zoom), Size(m_rasterSelection.w * zoom, m_rasterSelection.h * zoom) );
    PaintSelection( truncated(scaledSelection), memDC );
  }

  if ( ShouldDrawVector() ){
    PaintObjectHandles( m_objectSelection, memDC, zoom );
  }

  OverlayDC_WX overlayDC( memDC, zoom, imageCoordRect, bmpSize);
  if ( m_grid.Visible() ){
    overlayDC.GridLines( m_grid );
  }
  overlays.Paint( overlayDC );

  memDC.SetDeviceOrigin( 0, 0 );

  // Blit it all to the window
  wxPaintDC paintDC( this );
  paintDC.Blit( rView.x, rView.y, rView.w, rView.h, &memDC, 0, 0 );
}

int CanvasScroller::GetHorizontalPageSize() const {
  return GetClientSize().GetWidth() - m_geo.left_border * 2;
}

int CanvasScroller::GetVerticalPageSize() const {
  return GetClientSize().GetHeight() - m_geo.top_border * 2;
}

void CanvasScroller::AdjustScrollbars( int x, int y){
  AdjustHorizontalScrollbar( x );
  AdjustVerticalScrollbar( y );
}

void CanvasScroller::AdjustHorizontalScrollbar( int pos ){
  m_geo.x0 = pos;
  pos = std::max(0,pos);
  SetScrollbar( wxHORIZONTAL, // Orientation
    pos, // Position
    GetHorizontalPageSize(), // Thumb size
    std::max( GetMaxScrollRight(), pos + GetHorizontalPageSize() ), // Range
    true ); // Refresh
}

void CanvasScroller::AdjustVerticalScrollbar( int pos ){
  m_geo.y0 = pos;
  pos = std::max(0,pos);
  SetScrollbar( wxVERTICAL, // Orientation
    pos, // Position
    GetVerticalPageSize(), // Thumb size
    std::max( GetMaxScrollDown(), pos + GetVerticalPageSize() ) , // Range
    true ); // Refresh
}

void CanvasScroller::OnSize( wxSizeEvent& event ){
  AdjustScrollbars( m_geo.x0, m_geo.y0 );
  RefreshTool(m_toolWrap.Tool());
  event.Skip();
}

void CanvasScroller::OnScrollDrag( wxScrollWinEvent& event ){
  // This is a recurring event while a scrollbar-thumb is dragged
  event.Skip();
  int orientation = event.GetOrientation();
  int pos = event.GetPosition();
  if ( orientation == wxHORIZONTAL ){
    m_geo.x0 = std::max( 0, pos );
  }
  else if ( orientation == wxVERTICAL ){
    m_geo.y0 = std::max(0, pos );
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

void CanvasScroller::Paste(){
  if ( !wxTheClipboard->Open() ){
    // Failed opening clipboard
    return;
  }

  // Note: The faint::Get[...]-functions close the clipboard when
  // succesful (return true)

  if ( m_toolWrap.Tool()->AcceptsPastedText() ){
    std::string text;
    if ( !faint::GetText( wxTheClipboard, text ) ){
      // Stop further pasting when text paste failed. It would be
      // surprising if a bitmap was pasted while editing text.
      wxTheClipboard->Close();
      return;
    }
    else {
      m_toolWrap.Tool()->Paste( text );
      Refresh();
    }
    return;
  }

  const bool rasterPaste = GetAppContext().GetLayerType() == LAYER_RASTER;
  const bool objectPaste = !rasterPaste;
  if ( objectPaste ){
    std::vector<Object*> objects;
    if ( faint::GetObjects( wxTheClipboard, objects ) ){
      PasteObjects(objects);
      return;
    }
  }

  faint::Bitmap bitmap;
  if ( faint::GetBitmap( wxTheClipboard, bitmap ) ){
    if ( rasterPaste ){
      PasteBitmap( bitmap );
    }
    else {
      PasteBitmapAsObject( bitmap );
    }
    GetAppContext().OnDocumentStateChange();
    return;
  }

  // No suitable format on clipboard
  wxTheClipboard->Close();
  return;
}

void CanvasScroller::PasteObjects( std::vector<Object*>& objects ){
  // Find the correct offset to place the pasted objects
  // with relative positions intact and anchored at the view-start
  Point minObj = BoundingRect( objects ).TopLeft();
  const Point viewStart = GetImageViewStart();
  Offset( objects, viewStart - minObj );

  RunCommand( GetAddObjectsCommand( objects ) );
  SetObjectSelection( objects );
  GetAppContext().SelectTool( T_OBJ_SEL );
  return;
}

void CanvasScroller::PasteBitmap( const faint::Bitmap& bmp ){
  const Point pt = GetImageViewStart();
  DeselectRaster();
  GetAppContext().SelectTool( T_RECT_SEL );
  RectangleSelectBehavior* selectionTool = dynamic_cast<RectangleSelectBehavior*>(m_toolWrap.Tool());
  assert( selectionTool != 0 );
  selectionTool->InitializeFromPaste( bmp, truncated(pt) );
}

void CanvasScroller::PasteBitmapAsObject( const faint::Bitmap& bmp ){
  const Point pt = GetImageViewStart();
  FaintSettings s( GetRasterSettings() );
  s.Update( GetAppContext().GetToolSettings() );
  Object* rasterObj = new ObjRaster( Rect(pt, floated(bmp.Size())), bmp, s );
  RunCommand( GetAddObjectCommand( rasterObj ) );
  SelectObject( rasterObj, true );
  GetAppContext().SelectTool( T_OBJ_SEL );
}

void CanvasScroller::ScrollPageUp(){
  m_geo.y0 = std::max( 0, m_geo.y0 - (int)GetVerticalPageSize() );
  m_updateVertical = true;
}

void CanvasScroller::ScrollPageDown(){
  m_geo.y0 = m_geo.y0 + (int) GetVerticalPageSize();
  m_geo.y0 = std::max( 0, std::min( m_geo.y0, GetMaxScrollDown() - GetVerticalPageSize() ) );
  m_updateVertical = true;
}

void CanvasScroller::ScrollPageLeft(){
  m_geo.x0 = std::max( 0, m_geo.x0 - (int) GetHorizontalPageSize() );
  m_updateHorizontal = true;
}

void CanvasScroller::ScrollPageRight(){
  m_geo.x0 = m_geo.x0 + (int) GetHorizontalPageSize();
  m_geo.x0 = std::max( 0, std::min( m_geo.x0, GetMaxScrollRight() - GetHorizontalPageSize() ) );
  m_updateHorizontal = true;
}

void CanvasScroller::ScrollMaxLeft(){
  m_geo.x0 = 0;
  AdjustScrollbars( m_geo.x0, m_geo.y0 );
  Refresh();
}

void CanvasScroller::ScrollMaxRight(){
  m_geo.x0 = std::max( 0, GetMaxScrollRight() - GetHorizontalPageSize() );
  AdjustScrollbars( m_geo.x0, m_geo.y0 );
  Refresh();
}

void CanvasScroller::ScrollMaxUp(){
  m_geo.y0 = 0;
  AdjustScrollbars( m_geo.x0, m_geo.y0 );
  Refresh();
}

void CanvasScroller::ScrollMaxDown(){
  m_geo.y0 = std::max( 0, GetMaxScrollDown() - GetVerticalPageSize() );
  AdjustScrollbars( m_geo.x0, m_geo.y0 );
  Refresh();
}

int CanvasScroller::GetMaxScrollRight() const{
  return std::max( LITCRD(0.0), m_image->GetSize().w * m_geo.zoom.GetScaleFactor() );
}

int CanvasScroller::GetMaxScrollDown() const{
  return std::max( LITCRD(0.0), m_image->GetSize().h * m_geo.zoom.GetScaleFactor() );
}

void CanvasScroller::OnIdle( wxIdleEvent& ) {
  // Update the scrollbars in the idle handler to prevent various
  // crashes and mouse capture problems
  // (e.g. issue 78)
  if ( m_updateHorizontal ){
    AdjustHorizontalScrollbar( m_geo.x0 );
  }
  if ( m_updateVertical ){
    AdjustVerticalScrollbar( m_geo.y0 );
  }
  if ( m_updateHorizontal || m_updateVertical ){
    Refresh();
    m_updateHorizontal = m_updateVertical = false;
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

void CanvasScroller::OnScrollPageDown( wxScrollWinEvent& event ){
  event.Skip();
  if ( event.GetOrientation() == wxVERTICAL ){
    ScrollPageDown();
  }
  else if ( event.GetOrientation() == wxHORIZONTAL ){
    ScrollPageRight();
  }
}

void CanvasScroller::OnEraseBackground( wxEraseEvent& ){
  // This function intentionally left blank
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
    return IntPoint( ( imgSize.w - 1 ) / 2.0, ( imgSize.h - 1 ) );
  }
  if ( handle == 6 ){
    return IntPoint( imgSize.w - 1 , (imgSize.h - 1 ) / 2.0 );
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
    return IntPoint( ( imgSize.w - 1 ) / 2.0, 0 );
  }
  if ( handle == 6 ){
    return IntPoint( 0, (imgSize.h - 1) / 2.0 );
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

void CanvasScroller::OnLeftDown( wxMouseEvent& evt ){
  SetFocus();
  wxPoint wx_mousePos( evt.GetPosition() );
  int canvasHandle = CanvasHandleHitTest( wx_mousePos, m_image->GetSize(), m_geo );
  CaptureMouse();

  if ( canvasHandle != - 1 ){
    IntSize imageSize( m_image->GetSize() );
    IntPoint handlePos = CanvasHandlePos( canvasHandle, imageSize);
    IntPoint oppositePos = CanvasHandleOppositePos( canvasHandle, imageSize );
    m_toolWrap.SetSwitched( new ResizeCanvas(
      m_toolWrap.Tool(), handlePos, oppositePos,
      imageSize,
      evt.GetButton() == wxMOUSE_BTN_LEFT ? ResizeCanvas::Resize : ResizeCanvas::Rescale,
      CanvasHandleDirection( canvasHandle ) ) );
    SetCursor( CanvasHandleCursor( canvasHandle ) );
    return;
  }

  CursorPositionInfo info( HitTest( evt.GetPosition() ) );
  HandleToolResult( m_toolWrap.Tool()->LeftDown( info, MouseModifiers(evt) ) );
}

void CanvasScroller::OnLeftUp( wxMouseEvent& evt ){
  if ( HasCapture() ){
    ReleaseMouse();
  }
  SetFocus();
  const int modifiers = MouseModifiers( evt );
  CursorPositionInfo info = HitTest( evt.GetPosition() );

  HandleToolResult( m_toolWrap.Tool()->LeftUp( info, modifiers ) );
}

void CanvasScroller::OnDoubleClick( wxMouseEvent& evt ){
  const int modifiers = MouseModifiers( evt );
  CursorPositionInfo info = HitTest( evt.GetPosition() );
  HandleToolResult(m_toolWrap.Tool()->LeftDoubleClick( info, modifiers ));
}

void CanvasScroller::OnMotion( wxMouseEvent& evt ){
  MousePosRefresh( evt.GetPosition(), MouseModifiers( evt ) );
}

void CanvasScroller::MousePosRefresh( const wxPoint& mousePos, int modifiers ){
  if ( !HasCapture() ){
    int canvasHandle = CanvasHandleHitTest( mousePos, m_image->GetSize(), m_geo );
    if ( canvasHandle != - 1 ){
      SetCursor( CanvasHandleCursor( canvasHandle ) );
      StatusInterface& status = GetAppContext().GetStatusInfo();
      status.SetMainText( "Left click to resize, right click to rescale." );
      return;
    }
  }
  CursorPositionInfo info = HitTest( mousePos );
  HandleToolResult( m_toolWrap.Tool()->Motion( info, modifiers ) );
  SetFaintCursor( m_toolWrap.Tool()->GetCursor( info, modifiers ) );
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

void CanvasScroller::OnMouseOut( wxMouseEvent& ){
  // The cursor has left the canvas (the drawing area + the border)
  // onto a different control or outside the window. Tools may need to
  // be informed so they don't draw for example a brush cursor.
  if ( m_toolWrap.Tool()->MouseOut() ){
    RefreshTool(m_toolWrap.Tool());
  }
}

void CanvasScroller::ScrollLineUp(){
  m_geo.y0 = std::min( int( m_image->GetSize().h * m_geo.zoom.GetScaleFactor() ),
    std::max( m_geo.y0 - 10, 0 ) );
  AdjustVerticalScrollbar( m_geo.y0 );
  Refresh();
}

void CanvasScroller::ScrollLineDown(){
  m_geo.y0 = std::min( int( m_image->GetSize().h * m_geo.zoom.GetScaleFactor() ),
    std::max( m_geo.y0 + 10, 0 ) );
  AdjustVerticalScrollbar( m_geo.y0 );
  Refresh();
}

void CanvasScroller::ScrollLineLeft(){
  m_geo.x0 = std::min(
    int(m_image->GetSize().w * m_geo.zoom.GetScaleFactor()),
    std::max( m_geo.x0 - 10, 0 ) );
  AdjustHorizontalScrollbar( m_geo.x0 );
  Refresh();
}

void CanvasScroller::ScrollLineRight(){
  m_geo.x0 = std::min(
    int(m_image->GetSize().w * m_geo.zoom.GetScaleFactor()),
    std::max( m_geo.x0 + 10, 0 ) );
  AdjustHorizontalScrollbar( m_geo.x0 );
  Refresh();
}

void CanvasScroller::OnCaptureLost( wxMouseCaptureLostEvent& ){
  Preempt();
}

IntRect CanvasScroller::GetVisibleImageRect() const{
  wxSize sz = GetSize();
  const faint::coord zoom = m_geo.zoom.GetScaleFactor();
  return IntRect( IntPoint((m_geo.x0 - m_geo.left_border) / zoom, (m_geo.y0 - m_geo.top_border) / zoom), IntSize(sz.x / zoom + 100, sz.y / zoom + 100 ) );
}

void CanvasScroller::RefreshTool( ToolBehavior* tool ){
  IntRect toolRect(tool->GetRefreshRect(GetVisibleImageRect(), faint::mouse::ImagePosition( m_geo, *this ) ) );
  InclusiveRefresh(faint::mouse::ImageToView( toolRect, m_geo ));
}

void CanvasScroller::CommitTool( ToolBehavior* tool, bool refresh ){
  Command* cmd = tool->GetCommand();
  if ( cmd != 0 ){
    RunCommand( cmd );
    if ( refresh ){
      Refresh();
    }
  }
}

std::string CanvasScroller::GetFilename() const{
  return m_filename;
}

faint::coord CanvasScroller::GetZoom() const{
  return m_geo.zoom.GetScaleFactor();
}

const ZoomLevel& CanvasScroller::GetZoomLevel() const{
  return m_geo.zoom;
}

void CanvasScroller::ChangeZoom( ZoomLevel::ChangeType changeType ){
  faint::coord oldScale = m_geo.zoom.GetScaleFactor();
  m_geo.zoom.Change( changeType );
  faint::coord newScale = m_geo.zoom.GetScaleFactor();
  m_geo.x0 = m_geo.x0 / ( oldScale / newScale );
  m_geo.y0 = m_geo.y0 / ( oldScale / newScale );
  AdjustScrollbars( m_geo.x0, m_geo.y0 );
  MousePosRefresh( to_wx( faint::mouse::ViewPosition(*this)), GetToolModifiers() );
  GetAppContext().OnZoomChange();
  Refresh();
}

void CanvasScroller::SetFilename(const std::string& strFilename ){
  if ( !strFilename.empty() ){
    wxFileName filename( strFilename );
    assert( filename.IsAbsolute() );
    assert( !filename.IsDir() );
  }
  m_filename = strFilename;
}

bool CanvasScroller::IsDirty() const{
  return m_dirty;
}

bool CanvasScroller::CanRedo() const{
  return !m_redoList.empty();
}

bool CanvasScroller::CanUndo() const{
  return !m_undoList.empty();
}

void CanvasScroller::SetDirty(){
  m_dirty = true;
  GetAppContext().OnDocumentStateChange();
}

void CanvasScroller::ClearDirty(){
  m_dirty = false;
  GetAppContext().OnDocumentStateChange();
}

bool CropObjects( CanvasInterface& canvas, std::vector<Object*> selectedObjects ){
  Command* cmd = GetCropCommand( selectedObjects );
  if ( cmd == 0 ){
    return false;
  }
  canvas.RunCommand( cmd );
  return true;
}

void CanvasScroller::Crop(){
  const bool rasterLayer = ShouldDrawRaster();
  const bool didCropObjects =
    !rasterLayer &&
    CropObjects( GetInterface(), GetSelectedObjects() );

  if ( didCropObjects ){
    Refresh();
    return;
  }

  if ( m_toolWrap.Tool()->HasBitmap() ){
    faint::Bitmap bmp = *(m_toolWrap.Tool()->GetBitmap());
    Preempt( true );
    RunCommand( new SetBitmapCommand( bmp ) );
    Refresh();
    return;
  }

  IntRect cropRect(IntPoint(0,0),IntSize(0,0));
  if ( rasterLayer && HasRasterSelection() ){
    cropRect = m_rasterSelection;
    DeselectRaster();
  }
  else {
    const bool hasObjects = m_image->GetObjects().size() > 0;
    bool result = hasObjects ?
      GetAutoCropRect( faint::Flatten( *m_image ), cropRect ) :
      GetAutoCropRect( m_image->GetBitmapRef(), cropRect );
    if ( !result ){
      return;
    }
    cropRect = Intersection( cropRect, IntRect( IntPoint(0, 0), m_image->GetSize() ) ); // fixme: ImageRect
    if ( Empty( cropRect ) ){
      return;
    }
  }
  // Fixme: Use IntRect all along!
  IntRect i_cropRect( IntPoint(cropRect.x, cropRect.y), IntSize( cropRect.w, cropRect.h ) );
  RunCommand( new ResizeCommand( i_cropRect, faint::Color(255,255,255) ) );
  Refresh();
}

void CanvasScroller::CutSelection(){
  if ( CopySelection() ){
    DeleteSelection();
  }
}

bool CanvasScroller::CopySelection(){
  if ( !wxTheClipboard->Open() ){
    return false;
  }

  std::string text;
  if ( m_toolWrap.Tool()->CopyData( text, false ) ){
    faint::SetText( wxTheClipboard, text );
    return true;
  }

  faint::Bitmap bitmapFromTool;
  if ( m_toolWrap.Tool()->CopyData( bitmapFromTool ) ){
    faint::SetBitmap( wxTheClipboard, bitmapFromTool,
      GetAppContext().GetToolSettings().Get( ts_BgCol ));
    return true;
  }

  if ( ShouldDrawRaster() && HasRasterSelection() ){
    faint::SetBitmap( wxTheClipboard, GetBitmap( m_rasterSelection) ,
      GetAppContext().GetToolSettings().Get( ts_BgCol ));
    return true;
  }
  else if ( ShouldDrawVector() && !m_objectSelection.empty() ){
    faint::SetObjects( wxTheClipboard, m_objectSelection );
    return true;
  }
  wxTheClipboard->Close();
  return false;
}

void CanvasScroller::DeleteSelection(){
  if ( HandleToolResult( m_toolWrap.Tool()->Delete() ) ){
    // The tool handled the delete request
    return;
  }

  if ( ShouldDrawVector() ){
    if ( m_objectSelection.empty() ){
      return;
    }
    // Object delete
    RunCommand( GetDeleteObjectsCommand( m_objectSelection, *m_image ) );
    m_objectSelection.clear();
  }
  else if ( HasRasterSelection() ){
    // Raster delete
    if ( m_lastWasDelete ){
      // Alternate delete-scheme, use the determined alternate color
      // to erase with the surounding color instead of bg-color
      RunCommand( new DeleteCommand( m_rasterSelection, m_alternateColor ) );
    }
    else {
      faint::Color surroundingColor;
      bool gotColor = GetEdgeColor( m_image->GetBitmapRef(), m_rasterSelection, surroundingColor );
      const FaintSettings& s = GetAppContext().GetToolSettings();
      RunCommand( new DeleteCommand( m_rasterSelection, s.Get( ts_BgCol ) ) );
      if ( gotColor ){
        // Store the color on the edge of the selection rectangle to
        // allow a DWIM-delete with that color instead of current
        // background color
        m_lastWasDelete = true;
        m_alternateColor = surroundingColor;
      }
    }
  }
  Refresh();
}

void CanvasScroller::DeselectObject( Object* object ){
  std::vector<Object*>::iterator newEnd =
    std::remove_if( m_objectSelection.begin(),
      m_objectSelection.end(),
      bind2nd( std::equal_to<Object*>(), object ));
  m_objectSelection.erase(newEnd, m_objectSelection.end() );
  m_toolWrap.Tool()->SelectionChange();

  if ( m_objectSelection.empty() ){
    GetAppContext().OnDocumentStateChange();
  }
}

void CanvasScroller::DeselectObjects(){
  SetObjectSelection( std::vector<Object*>() );
}

void CanvasScroller::DeselectRaster(){
  m_lastRefreshRect = Union( m_lastRefreshRect, faint::mouse::ImageToView(m_rasterSelection, m_geo));
  SetRasterSelection( IntRect(IntPoint(0,0),IntSize(0,0)) );
}

void CanvasScroller::Deselect(){
  if ( HandleToolResult( m_toolWrap.Tool()->Deselect() ) ){
    // The tool handled the deselection
    return;
  }

  if ( ShouldDrawVector() ){
    DeselectObjects();
  }
  else {
    SetRasterSelection( IntRect(IntPoint(0,0),IntSize(0,0)) );
  }
  MousePosRefresh( to_wx( faint::mouse::ViewPosition(*this) ), GetToolModifiers() );
  Refresh();
}

bool CanvasScroller::HasRasterSelection() const{
  return !Empty(m_rasterSelection);
}

bool CanvasScroller::HasToolSelection(){
  return m_toolWrap.Tool()->HasSelection();
}

bool CanvasScroller::HasObjectSelection() const{
  return !m_objectSelection.empty();
}

bool CanvasScroller::HasSelection() const{
  return HasRasterSelection() || HasObjectSelection();
}

CursorPositionInfo CanvasScroller::HitTest( const wxPoint& p ){
  int handleIndex = -1;
  int hitStatus = HIT_NONE;
  Object* object = 0;
  bool selected = false;

  // Selected objects are checked first so that they are prioritized
  for ( size_t i = 0; i != m_objectSelection.size(); i++ ){
    Object* objTemp = m_objectSelection[i];
    handleIndex = GetHandleIndex( objTemp->GetResizePoints(), p, m_geo );
    if ( handleIndex != -1 ){
      hitStatus = HIT_RESIZE_POINT;
      selected = true;
      object = objTemp;
      break;
    }
    handleIndex = GetHandleIndex( objTemp->GetMovablePoints(), p, m_geo );
    if ( handleIndex != - 1 ){
      hitStatus = HIT_MOVABLE_POINT;
      selected = true;
      object = objTemp;
      break;
    }
  }

  if ( handleIndex == - 1 ){
    Point ptImg = faint::mouse::ViewToImage( to_faint(p), m_geo );
    std::pair<Object*, HitType> hitObject = ObjectAt( ptImg );
    object = hitObject.first;

    if ( object != 0 ){
      if ( hitObject.second == HT_EDGE ){
        hitStatus = HIT_BOUNDARY;
      }
      else if ( hitObject.second == HT_INSIDE ){
        hitStatus = HIT_INSIDE;
      }
      else {
        hitStatus = HIT_NEAR;
      }
      selected =
        std::find( m_objectSelection.begin(), m_objectSelection.end(), object ) != m_objectSelection.end();
    }
  }

  Point imagePos = faint::mouse::ViewToImage(to_faint(p), m_geo);
  CursorPositionInfo info = {
    &GetInterface(),
    imagePos,
    InSelection( imagePos ),
    GetAppContext().GetLayerType(),
    hitStatus,
    selected,
    object,
    handleIndex,
  };
  return info;
}

bool CanvasScroller::InSelection( const Point& p ){
  return HasRasterSelection() && m_rasterSelection.Contains( truncated(p) );
}

std::pair<Object*, CanvasScroller::HitType> CanvasScroller::ObjectAt( const Point& p ){
  // Create a small scaled and adjusted DC where the clicked pixel is
  // anchored at 0,0 in DC coordinates for pixel object hit tests
  FaintDC dc( m_hitTestBuffer );
  const faint::coord zoom = m_geo.zoom.GetScaleFactor();
  dc.SetOrigin(-p.x * zoom, -p.y * zoom );
  dc.SetScale( zoom );
  dc.Clear( mask_outside );

  // First consider the selected objects...
  Object* consider = 0;
  for ( size_t i = 0; i != m_objectSelection.size(); i++ ){
    Object* object = m_objectSelection[i];
    if ( object->HitTest( p ) ){
      consider = object;
      object->DrawMask( dc );
      faint::Color color = dc.GetPixel( p.x, p.y );
      if ( color  == mask_edge ){
        return std::make_pair(object, HT_EDGE);
      }
      else if ( color == mask_fill ){
        return std::make_pair(object, HT_INSIDE);
      }
    }
  }

  // ...then the rest
  dc.Clear( mask_outside );
  std::vector<Object*>& objects = m_image->GetObjects();
  for ( int i = objects.size() - 1; i!= -1; i-- ){
    Object* object = objects[i];
    if ( object->HitTest( p ) ){
      object->DrawMask( dc );
      faint::Color color =  dc.GetPixel( p.x, p.y );
      if ( color  == mask_edge ){
        return std::make_pair(object, HT_EDGE);
      }
      else if ( color == mask_fill ){
        return std::make_pair(object, HT_INSIDE);
      }
      else if ( consider == 0 ){
        consider = object;
      }
    }
  }
  return std::make_pair(consider, HT_NEAR);
}

void CanvasScroller::ExternalRefresh(){
  Refresh();
}

void CanvasScroller::SelectObject( Object* object, bool deselectOld ){
  if ( deselectOld ){
    m_objectSelection.clear();
  }
  else if ( std::find(m_objectSelection.begin(), m_objectSelection.end(), object ) != m_objectSelection.end() ){
    return;
  }
  int pos = GetSortedInsertionPos( object, m_objectSelection, m_image->GetObjects() );
  m_objectSelection.insert( m_objectSelection.begin() + pos, object );
  m_toolWrap.Tool()->SelectionChange();
  GetAppContext().OnDocumentStateChange();
  Refresh();
}

void CanvasScroller::SelectAll(){
  if ( ShouldDrawRaster() ){
    faint::Bitmap& bmp = m_image->GetBitmapRef();
    SetRasterSelection( IntRect( IntPoint(0, 0), IntSize(bmp.m_w, bmp.m_h) ) );
  }
  else {
    SetObjectSelection( m_image->GetObjects() );
  }
  Refresh();
}

void CanvasScroller::SetFaintCursor( int cursorId ){
  SetCursor(to_wx_cursor(cursorId));
}

void CanvasScroller::MoveObjectUp(){
  if ( m_objectSelection.empty() ){
    return;
  }

  std::vector<Command*> commands;
  unsigned int zLimit = m_image->GetObjects().size();
  for ( int i = m_objectSelection.size() -1; i >= 0; i-- ){
    Object* object = m_objectSelection[i];
    size_t Z = m_image->GetObjectZ( object );

    // Prevent objects from being moved on top of other selected objects that
    // could not be moved further up because of reaching top.
    if ( Z + 1 >= zLimit ){
      zLimit = Z;
    }
    else {
      zLimit = Z + 1;
      commands.push_back( new OrderObjectCommand( object, Z + 1, Z ) );
    }
  }

  if ( commands.empty() ){
    // No object could be moved further up
    return;
  }

  RunCommand( PerhapsBunch( CMD_TYPE_OBJECT, commands ) );
  Refresh();
}

void CanvasScroller::MoveObjectDown(){
  if ( m_objectSelection.empty() ){
    return;
  }

  size_t zLimit = 0;
  std::vector<Command*> commands;
  for ( size_t i = 0; i != m_objectSelection.size(); i++ ){
    Object* object = m_objectSelection[i];
    size_t Z = m_image->GetObjectZ( object );

    if ( Z == 0 || Z - 1 < zLimit ){
      // Prevent the current object from moving below Z-depth 0, and
      // from moving below an object that was previously prevented to
      // move below Z=0 (etc.), so that the relative order of all
      // selected objects is retained.
      zLimit = Z + 1;
    }
    else {
      zLimit = Z - 1;
      commands.push_back( new OrderObjectCommand( object, Z - 1, Z ) );
    }
  }

  if ( commands.empty() ){
    // No object could be moved further back
    return;
  }

  RunCommand( PerhapsBunch( CMD_TYPE_OBJECT, commands ) );
  Refresh();
}

void CanvasScroller::MoveObjectBack(){
  assert( !m_objectSelection.empty() );
  std::vector<Command*> commands;
  for ( size_t i = 0; i != m_objectSelection.size(); i++ ){
    Object* object = m_objectSelection[i];
    size_t currentPos = m_image->GetObjectZ( object );
    if ( i != currentPos ){
      commands.push_back( new OrderObjectCommand( object, i, currentPos ) );
    }
  }
  if ( commands.empty() ){
    // No object changed position
    return;
  }
  RunCommand( PerhapsBunch( CMD_TYPE_OBJECT, commands ) );
  Refresh();
}

void CanvasScroller::MoveObjectFront(){
  assert( !m_objectSelection.empty() );
  std::vector<Command*> commands;
  const size_t numObjects = m_image->GetObjects().size();
  for ( size_t i = 0; i != m_objectSelection.size(); i++ ){
    Object* object = m_objectSelection[i];
    size_t currentPos = m_image->GetObjectZ( object );
    if ( numObjects != currentPos ){
      commands.push_back( new OrderObjectCommand( object, numObjects, currentPos ) );
    }
  }
  if ( commands.empty() ){
    // No object changed position
    return;
  }
  RunCommand( PerhapsBunch( CMD_TYPE_OBJECT, commands ) );
  Refresh();
}

void CanvasScroller::SetFocus(){
  wxWindow::SetFocus();
}

void CanvasScroller::RunCommand( Command* cmd, bool clearRedo ){
  m_lastWasDelete = false;
  try {
    IntSize oldSize(m_image->GetSize());
    cmd->Do( *m_image );
    if ( clearRedo ){
      ClearList( m_redoList );
    }
    m_undoList.push_back( cmd );
    m_dirty = true;
    GetAppContext().OnDocumentStateChange();

    if ( oldSize != m_image->GetSize() ){
      // If the image size changes it is quite likely that the current
      // position will be outside the drawing area in a sea of dark
      // grey, simple return to 0,0 whenever the size changes for now.
      AdjustScrollbars( 0, 0 );
      SetRasterSelection( m_rasterSelection );
    }
  }
  catch ( const std::bad_alloc& ){
    delete cmd;
    wxMessageDialog( this, "An action required too much memory and was cancelled.", "Out of memory" ).ShowModal();
  }
}

void CanvasScroller::GroupSelected(){
  if ( m_objectSelection.size() <= 1 ){
    return;
  }
  m_toolWrap.Tool()->Preempt();
  RunCommand( new GroupObjects( m_interface, m_objectSelection ) );
  m_toolWrap.Tool()->SelectionChange();
  Refresh();
}

bool CanUngroup( const std::vector<Object*>& objects ){
  for ( size_t i = 0; i!= objects.size(); i++ ){
    if ( objects[i]->GetObjectCount() > 0 ){
      return true;
    }
  }
  return false;
}

void CanvasScroller::UngroupSelected(){
  if ( !CanUngroup( m_objectSelection ) ){
    return;
  }

  m_toolWrap.Tool()->Preempt();
  RunCommand( new UngroupObjects( m_interface, m_objectSelection ) );
  m_toolWrap.Tool()->SelectionChange();
  Refresh();
}

struct ObjExists {
  ObjExists( faint::Image& img )
    : m_image(img)
  {}
  bool operator()(Object* obj) const{
    return m_image.HasObject( obj );
  }
  faint::Image& m_image;
  typedef Object* argument_type;
};

void CanvasScroller::Undo(){
  PreemptResult res = Preempt();
  if ( res == PREEMPT_CANCEL ){
    // The tool was aborted by the undo
    return;
  }

  // The tool either committed on the preempt (and the command should
  // now be undone) or there was no effect.
  // Proceed with normal undo-behavior.
  if ( m_undoList.empty() ){
    return;
  }

  Command* undone = m_undoList.back();
  CommandType undoType = undone->GetType();
  if ( undoType == CMD_TYPE_OBJECT || undoType == CMD_TYPE_HYBRID ){
    // Hybrid and object commands must undo their changes to objects
    undone->Undo( *m_image );
  }
  if ( undoType == CMD_TYPE_RASTER || undoType == CMD_TYPE_HYBRID ){
    // When a raster or hybrid command is undone, the image is reset and all raster operations are
    // reapplied.
    m_image->Revert();
    for ( std::deque<Command*>::iterator it = m_undoList.begin(); it != m_undoList.end() - 1; ++it ){
      Command* reapplied = *it;
      reapplied->DoRaster( *m_image );
    }
  }
  m_undoList.pop_back();
  m_redoList.push_front( undone );

  std::vector<Object*>::iterator newEnd = std::remove_if( m_objectSelection.begin(), m_objectSelection.end(), std::not1( ObjExists( *m_image ) ));
  m_objectSelection.erase(newEnd, m_objectSelection.end() );

  if ( m_undoList.empty() ){
    ClearDirty();
  }

  GetAppContext().OnDocumentStateChange();
  Refresh();
}

void CanvasScroller::Redo(){
  if ( m_redoList.empty() ){
    return;
  }

  Command* redone = m_redoList.front();
  m_redoList.pop_front();
  RunCommand( redone, false );
  Refresh();
}

PreemptResult CanvasScroller::Preempt( bool discard ){
  ToolBehavior* tool = m_toolWrap.Tool();
  ToolRefresh r = tool->Preempt();
  if ( r == TOOL_COMMIT ){
    if ( !discard ){
      CommitTool(tool);
    }
    return PREEMPT_COMMIT;
  }
  else if ( r == TOOL_CANCEL ){
    Refresh();
    return PREEMPT_CANCEL;
  }
  else if ( r == TOOL_CHANGE ){
    if ( !discard ){
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

CanvasInterface& CanvasScroller::GetInterface(){
  return *m_interface;
}

std::vector<Object*>& CanvasScroller::GetSelectedObjects(){
  return m_objectSelection;
}

IntRect CanvasScroller::GetRasterSelection(){
  return m_rasterSelection;
}

Point CanvasScroller::GetRelativeMousePos(){
  return faint::mouse::ImagePosition( m_geo, *this );
}

void CanvasScroller::SetObjectSelection( const std::vector<Object*>& objects ){
  m_objectSelection = objects;
  m_toolWrap.Tool()->SelectionChange();
  GetAppContext().OnDocumentStateChange();
}

void CanvasScroller::SetRasterSelection( const IntRect& r ){
  m_rasterSelection = Intersection( r, ImageRect( *m_image ));
  m_toolWrap.Tool()->SelectionChange();
  GetAppContext().OnDocumentStateChange();
  MousePosRefresh( to_wx( faint::mouse::ViewPosition(*this) ), GetToolModifiers() );
  m_lastWasDelete = false; // Fixme
}

std::vector<Object*>& CanvasScroller::GetObjects(){
  return m_image->GetObjects();
}

faint::Image& CanvasScroller::GetImage(){
  return *m_image;
}

faint::Bitmap CanvasScroller::GetBitmap( const IntRect& r ) const{
  faint::Bitmap* bmp = m_image->GetBitmap();
  return CairoCompatibleSubBitmap( *bmp, r.x, r.y, r.w, r.h );
}

IntSize CanvasScroller::GetBitmapSize() const {
  return m_image->GetSize();
}

Point CanvasScroller::GetImageViewStart() const{
  const faint::coord zoom = m_geo.zoom.GetScaleFactor();
  return Point( std::max( ( m_geo.x0 - m_geo.left_border ) / zoom, LITCRD(0.0) ),
    std::max( ( m_geo.y0 - m_geo.top_border ) / zoom, LITCRD(0.0) ) );
}

void CanvasScroller::Resize( const IntSize& sz ){
  Resize( sz, GetAppContext().GetToolSettings().Get( ts_BgCol ) );
}

void CanvasScroller::Resize( const IntRect& r ){
  RunCommand( new ResizeCommand( r, GetAppContext().Get( ts_BgCol ) ) );
}

void CanvasScroller::Resize( const IntSize& sz, const faint::Color& expandColor ){
  RunCommand( new ResizeCommand( sz, expandColor ) );
}

void CanvasScroller::Rescale( const IntSize& sz ){
  RunCommand( new RescaleCommand( sz, RescaleCommand::Bilinear ) );
}

void CanvasScroller::Flatten(){
  bool flattenSelected = !m_objectSelection.empty();
  std::vector<Object*>& objects = flattenSelected ?
    m_objectSelection :
    m_image->GetObjects();

  if ( objects.empty() ){
    return;
  }

  RunCommand( GetFlattenCommand( objects, *m_image ) );
  if ( flattenSelected ){
    m_objectSelection.clear();
    m_toolWrap.Tool()->SelectionChange();
  }
  Refresh();
}

void CanvasScroller::CenterView( const IntPoint& ptView ){
  wxSize sz = GetClientSize();
  m_geo.x0 = ptView.x + m_geo.x0 - sz.GetWidth() / 2;
  m_geo.y0 = ptView.y + m_geo.y0 - sz.GetHeight() / 2;
  AdjustScrollbars( m_geo.x0, m_geo.y0 );
  WarpPointer( sz.GetWidth() / 2, sz.GetHeight() / 2 );
  Refresh();
}

void CanvasScroller::CenterViewImage( const Point& ptImage ){
  CenterView( faint::mouse::ImageToView_tr( ptImage, m_geo ) );
}

void CanvasScroller::OnChar( wxKeyEvent& event ){
  int keycode = event.GetKeyCode();
  if ( HandleToolResult( m_toolWrap.Tool()->Char( event.GetUnicodeKey(), keycode, event.GetModifiers() ) ) ){
    // The tool ate the character.
    return;
  }
  event.Skip();
}

void CanvasScroller::OnKeyDown( wxKeyEvent& event ){
  int keycode = event.GetKeyCode();

  if ( IsToolModifier( keycode ) ){
    // Update the tool state if a modifier is pressed (e.g. for cursor update)
    // Note: This repeates while the key is held.
    MousePosRefresh( to_wx( faint::mouse::ViewPosition(*this) ), GetToolModifiers() );
  }
  else {
    event.Skip();
  }
}

void CanvasScroller::OnKeyUp( wxKeyEvent& event ){
  int keycode = event.GetKeyCode();
  if ( IsToolModifier( keycode ) ){
    // Update the tool state if a modifier is released (e.g. for cursor update)
    // Note: This repeates while the key is held.
    MousePosRefresh( to_wx( faint::mouse::ViewPosition(*this) ), GetToolModifiers() );
    return;
  }
  event.Skip();
}

bool IsOrHas( const Object* object, const ObjectId& id ){
  if ( object->GetId() == id ){
    return true;
  }
  for ( size_t i = 0; i != object->GetObjectCount(); i++ ){
    if ( IsOrHas( object->GetObject( i ), id ) ){
      return true;
    }
  }
  return false;
}

bool CanvasScroller::Has( const ObjectId& id ){
  std::vector<Object*>& objects = m_image->GetObjects();
  for ( size_t i = 0; i != objects.size(); i++ ){
    if ( IsOrHas( objects[i], id ) ){
      return true;
    }
  }
  return false;
}

template<typename Operation>
void CanvasScroller::ApplyOperation(){
  ApplyTarget target = GetApplyTarget();

  if ( target == APPLY_ACTIVE_TOOL ){
    faint::Bitmap* bmp = m_toolWrap.Tool()->GetBitmap();
    Operation::ApplyBitmap(*bmp);
    m_toolWrap.Tool()->UpdateBitmap();
  }
  else if ( target == APPLY_OBJECT_SELECTION  ){
    RunCommand( Operation::CommandObjects( m_objectSelection ) );
  }
  else if ( target == APPLY_RASTER_SELECTION ){
    RunCommand( Operation::CommandRasterSelection( m_rasterSelection ) );
  }
  else {
    assert( target == APPLY_IMAGE );
    RunCommand( Operation::CommandImage() );
  }
  Refresh();
}

void CanvasScroller::RotateTool90CW(){
  faint::Bitmap* bmp = m_toolWrap.Tool()->GetBitmap();
  *bmp = faint::Rotate90CW( *bmp );
  m_toolWrap.Tool()->UpdateBitmap();
}

void CanvasScroller::Rotate90CW(){
  const ApplyTarget target = GetApplyTarget();
  if ( target == APPLY_IMAGE ){
    RunCommand( new CmdFlipRotate( CmdFlipRotate::ROTATE ) );
  }
  else if ( target == APPLY_OBJECT_SELECTION ){
    Point origin = BoundingRect(m_objectSelection).Center();
    RunCommand( GetRotateCommand( m_objectSelection, faint::pi / 2, origin ) );
  }
  else if ( target == APPLY_RASTER_SELECTION ){
    // Start a drag (selection tool) so that the raster selection is
    // rotated.
    StartDrag();
    RotateTool90CW();
  }
  else if ( target == APPLY_ACTIVE_TOOL ){
    RotateTool90CW();
  }
  else {
    assert( false );
  }
  Refresh( true );
}

void CanvasScroller::ScaleRasterSelection( const Size& size ){
  IntRect oldSelection = m_rasterSelection;
  ToolBehavior* tool = StartDrag();
  faint::Bitmap* bmp = tool->GetBitmap();
  *bmp = faint::ScaleBilinear( *bmp, size.w / oldSelection.w, size.h / oldSelection.h );
  tool->UpdateBitmap();
  Refresh( true );
}

void CanvasScroller::ScaleSelectedObject( const Size& size ){
  assert( !m_objectSelection.empty() );
  Rect oldRect = BoundingRect(m_objectSelection);
  Point origin = oldRect.TopLeft();
  faint::coord scale_x = size.w / oldRect.w;
  faint::coord scale_y = size.h / oldRect.h;

  RunCommand( GetScaleCommand( m_objectSelection, scale_x, scale_y, origin ) );
  Refresh( true );
}

void CanvasScroller::ScaleToolBitmap( const IntSize& size ){
  ToolBehavior* tool = m_toolWrap.Tool();
  assert( tool->HasBitmap() );
  faint::Bitmap* bmp = tool->GetBitmap();
  IntSize oldSize = bmp->Size();
  *bmp = faint::ScaleBilinear( *bmp, floated(size.w) / floated(oldSize.w), floated(size.h) / floated(oldSize.h) );
  tool->UpdateBitmap();
  Refresh(true);
}

bool CanvasScroller::ShouldDrawVector(){
  return !ShouldDrawRaster();
}

bool CanvasScroller::ShouldDrawRaster(){
  ToolId id = m_toolWrap.GetToolId();
  return ( GetAppContext().GetLayerType() == LAYER_RASTER && id != T_OBJ_SEL && id != T_OTHER ) ||
    (id == T_RECT_SEL || id == T_PEN || id == T_BRUSH );
}

/* Switches to the RectangleSelectBehavior and activates drag of the
   current selection */
ToolBehavior* CanvasScroller::StartDrag(){
  assert( HasRasterSelection() );
  RectangleSelectBehavior* rectSel = new RectangleSelectBehavior();
  rectSel->ToolSettingUpdate( GetAppContext().GetToolSettings() );
  rectSel->StartDrag();
  m_toolWrap.SetSwitched( rectSel );
  GetAppContext().OnDocumentStateChange();
  return m_toolWrap.Tool();
}

void CanvasScroller::InclusiveRefresh( const IntRect& r ){
  const faint::coord zoom = m_geo.zoom.GetScaleFactor();
  if ( zoom >= LITCRD(1.0) ){
    RefreshRect( to_wx( Union( r, m_lastRefreshRect ) ).Inflate(LITCRD(2.0) * zoom) );
    m_lastRefreshRect = r;
  }
  else {
    // Fixme - Workaround. RefreshRect does not work well when zoomed out,
    // leading to problems with offsets and other stuff.
    // Note: This is slow.
    Refresh();
  }
}

void CanvasScroller::SetGrid( const Grid& g ){
  m_grid = g;
}

Grid CanvasScroller::GetGrid() const{
  return m_grid;
}

bool CanvasScroller::HandleToolResult( ToolRefresh ref ){
  if ( ref == TOOL_COMMIT ){
    CommitTool( m_toolWrap.Tool() );
  }
  else if ( ref == TOOL_CHANGE ){
    CommitTool( m_toolWrap.Tool(), false );
    m_toolWrap.SetSwitched( m_toolWrap.Tool()->GetNewTool() );

    // Update settings, e.g. when cloning objects from the
    // ObjSelectBehavior
    m_toolWrap.Tool()->SelectionChange();
    Refresh();
  }
  else if ( ref == TOOL_OVERLAY ){
    RefreshTool( m_toolWrap.Tool() );
  }
  else if ( ref == TOOL_CANCEL ){
    Refresh();
  }
  // TOOL_NONE might signify that the tool didn't care, which can in
  // some cases suggest further processing outside this function.
  // (e.g. DeleteSelection)
  return ref != TOOL_NONE;
}

BEGIN_EVENT_TABLE(CanvasScroller, wxPanel)
EVT_SIZE( CanvasScroller::OnSize )
EVT_PAINT( CanvasScroller::OnPaint )
EVT_ERASE_BACKGROUND( CanvasScroller::OnEraseBackground )
EVT_MOUSE_CAPTURE_LOST( CanvasScroller::OnCaptureLost )
EVT_SCROLLWIN_THUMBTRACK( CanvasScroller::OnScrollDrag )
EVT_SCROLLWIN_THUMBRELEASE( CanvasScroller::OnScrollDragEnd )
EVT_SCROLLWIN_PAGEDOWN( CanvasScroller::OnScrollPageDown )
EVT_SCROLLWIN_PAGEUP( CanvasScroller::OnScrollPageUp )
EVT_SCROLLWIN_LINEUP( CanvasScroller::OnScrollLineUp )
EVT_SCROLLWIN_LINEDOWN( CanvasScroller::OnScrollLineDown )
EVT_LEFT_DOWN( CanvasScroller::OnLeftDown )
EVT_RIGHT_DOWN( CanvasScroller::OnLeftDown )
EVT_LEFT_UP( CanvasScroller::OnLeftUp )
EVT_RIGHT_UP( CanvasScroller::OnLeftUp )
EVT_LEFT_DCLICK( CanvasScroller::OnDoubleClick )
EVT_MOTION( CanvasScroller::OnMotion )
EVT_MOUSEWHEEL( CanvasScroller::OnMouseWheel )
EVT_KEY_UP(CanvasScroller::OnKeyUp )
EVT_KEY_DOWN(CanvasScroller::OnKeyDown )
EVT_CHAR(CanvasScroller::OnChar )
EVT_IDLE( CanvasScroller::OnIdle )
EVT_LEAVE_WINDOW( CanvasScroller::OnMouseOut )
END_EVENT_TABLE()

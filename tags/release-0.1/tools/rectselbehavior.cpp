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

#include "rectselbehavior.hh"
#include "canvasinterface.hh"
#include "cursors.hh"
#include "faintdc.hh"
#include "geo/geotypes.hh"
#include "getappcontext.hh"
#include "objects/objrectangle.hh"
#include "settingid.hh"
#include "util/util.hh"

// Creates a settings object for drawing active selection rectangles
FaintSettings Init_SettingsSelectionRect(){
  FaintSettings s = GetRectangleSettings();
  s.Set( ts_FgCol, faint::Color( 255, 0, 255 ) );
  s.Set( ts_BgCol, faint::Color( 0, 0, 0 ) );
  s.Set( ts_FillStyle, BORDER );
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, faint::LONG_DASH );
  s.Set( ts_AntiAlias, false );
  return s;
}

const FaintSettings& SettingsSelectionRect(){
  static FaintSettings selectionRectSettings = Init_SettingsSelectionRect();
  return selectionRectSettings;
}

RectangleSelectBehavior::RectangleSelectBehavior()
  : ToolBehavior( T_RECT_SEL, m_notifier )
{
  m_notifier.SetTarget( this );
  m_settings.Set( ts_Transparency, TRANSPARENT_BG );
  m_settings.Set( ts_BgCol, faint::Color( 0, 0, 0 ) );
  m_status = RSEL_NONE;
  m_dragBehavior = 0;
  m_maxDistance = 0.0f;
}

RectangleSelectBehavior::~RectangleSelectBehavior(){
  delete m_dragBehavior;
  m_dragBehavior = 0;
}

bool RectangleSelectBehavior::DrawBeforeZoom(Layer) const{
  return true;
}

ToolRefresh RectangleSelectBehavior::LeftDown( const CursorPositionInfo& info, int modifiers ){
  if ( fl( RIGHT_MOUSE, modifiers ) ){
    return HandleRightClick( info, modifiers );
  }
  else if ( info.inSelection ){
    // Not currently dragging, and clicking within selection
    return InitDragBehavior(info, modifiers);
  }
  else if ( m_dragBehavior != 0 && m_dragBehavior->GetStatus() == PENDING_FURTHER_DRAG ){
    // An uncommitted drag-operation is active - the user has released
    // the mouse but has not yet "stamped" the moved area on the
    // canvas.
    return UpdateDragStatus( info, modifiers );
  }
  else {
    return InitSelecting( info );
  }
}

ToolRefresh RectangleSelectBehavior::LeftUp( const CursorPositionInfo& pInfo, int modifiers ){
  Point p = pInfo.pos;
  if ( m_status == RSEL_SELECTING ){
    m_status = RSEL_NONE;
    IntRect newSelection( truncated(m_p1), truncated(m_p2) );
    if ( m_maxDistance < 1 ){
      // To prevent selecting a single pixel by mistake, require some
      // distance at some point during this selection
      GetAppContext().GetActiveCanvas().DeselectRaster();
      return TOOL_OVERLAY;
    }
    else {
      pInfo.canvas->SetRasterSelection( newSelection );
      return TOOL_OVERLAY;
    }
  }
  else if ( m_status == RSEL_DRAGGING ){
    assert ( m_dragBehavior != 0 );
    DragStatus dragStatus = m_dragBehavior->LeftUp( p, modifiers );

    if ( dragStatus == DONE_DRAGGING ){
      m_command.Set( m_dragBehavior->GetCommand() );
      m_status = RSEL_NONE;
      return TOOL_COMMIT;
    }
    else if ( dragStatus == PENDING_FURTHER_DRAG ){
      m_status = RSEL_DRAGGING;
      return TOOL_OVERLAY;
    }
  }

  return TOOL_NONE;
}

ToolRefresh RectangleSelectBehavior::Motion( const CursorPositionInfo& info, int modifiers ){
  if ( m_status == RSEL_SELECTING ){
    m_p2 = info.pos;

    if ( (modifiers & TOOLMODIFIER2) == TOOLMODIFIER2 ) {
      m_p2 = AdjustTo(m_p1, m_p2, 90, 45 );
    }

    m_maxDistance = std::max( m_maxDistance, distance( m_p1, m_p2 ) );

    StatusInterface& status = GetAppContext().GetStatusInfo();
    status.SetText( StrPoint( truncated(m_p1) ) + "-" + StrPoint(truncated(m_p2)), 0 );
    return TOOL_OVERLAY;
  }
  else if ( m_status == RSEL_DRAGGING ){
    return m_dragBehavior->Motion( info.pos, modifiers );
  }
  StatusInterface& status = GetAppContext().GetStatusInfo();

  status.SetText( StrPoint( info.pos ),0);
  return TOOL_NONE;
}

ToolRefresh RectangleSelectBehavior::Preempt(){
  if ( m_dragBehavior != 0 && m_dragBehavior->Preempt() ) {
    m_command.Set( m_dragBehavior->GetCommand() );
    delete m_dragBehavior;
    m_dragBehavior = 0;
    m_status = RSEL_NONE;
    return TOOL_COMMIT;
  }
  return TOOL_NONE;
}

bool RectangleSelectBehavior::HasBitmap() const{
  return m_status != RSEL_SELECTING && m_dragBehavior != 0;
}

faint::Bitmap* RectangleSelectBehavior::GetBitmap(){
  if ( m_status != RSEL_SELECTING && m_dragBehavior != 0 ){
    return m_dragBehavior->GetBitmap();
  }
  return 0;
}

void RectangleSelectBehavior::UpdateBitmap(){
  if ( m_status != RSEL_SELECTING && m_dragBehavior != 0 ){
    m_dragBehavior->UpdateBitmap();
  }
}

bool RectangleSelectBehavior::Draw( FaintDC& dc, Overlays& overlays, const Point& currPos ){
  if ( m_status == RSEL_SELECTING ){
    // Draw the selection indicator for the ongoing selection
    FaintSettings s( SettingsSelectionRect() );
    dc.Rectangle( truncated(m_p1), truncated(m_p2), s );
    return true;
  }
  else if ( m_status == RSEL_DRAGGING || ( m_dragBehavior != 0 && m_dragBehavior->GetStatus() == PENDING_FURTHER_DRAG ) ){
    return m_dragBehavior->Draw( dc, overlays, currPos );
  }
  return false;
}

IntRect RectangleSelectBehavior::GetRefreshRect( const IntRect&, const Point& ){
  if ( m_status == RSEL_SELECTING || m_status == RSEL_NONE ){
    return IntRect( truncated(m_p1), truncated(m_p2) );
  }
  else if ( m_status == RSEL_DRAGGING ){
    return m_dragBehavior->GetRefreshRect();
  }
  else if ( m_status == RSEL_SELECTING_DRAG_COMMIT_PENDING ){
    return m_dragBehavior->GetRefreshRect();
  }
  assert ( false );
  return IntRect(IntPoint(0,0),IntSize(0,0));
}

Command* RectangleSelectBehavior::GetCommand(){
  if ( m_status == RSEL_SELECTING_DRAG_COMMIT_PENDING ){
    m_status = RSEL_SELECTING;
  }
  return m_command.Retrieve();
}

int RectangleSelectBehavior::GetCursor( const CursorPositionInfo& info ){
  if ( info.inSelection ){
    return CURSOR_MOVE;
  }
  if ( m_status == RSEL_DRAGGING ){
    // While dragging the cursor should be CURSOR_MOVE, however with
    // RSEL_DRAGGING, the drag-behavior may also be inactive, leaving
    // a floating selection, so we must check that the cursor is
    // inside the rectangle.
    if ( m_dragBehavior->GetRect().Contains( info.pos ) ){
      return CURSOR_MOVE;
    }
  }
  return CURSOR_CROSSHAIR;
}

void RectangleSelectBehavior::InitializeFromPaste( const faint::Bitmap& bitmap, const IntPoint& topLeft ){
  StartDrag( bitmap, topLeft );
}

void RectangleSelectBehavior::StartDrag( const faint::Bitmap& bmp, const IntPoint& topLeft ){
  delete m_dragBehavior;
  m_dragBehavior = new RectSelDragBehavior( IntRect(topLeft, IntSize(bmp.m_w, bmp.m_h ) ),
    bmp, IntPoint(0,0), false );
  m_dragBehavior->UpdateMask( m_settings.Get( ts_Transparency ) == TRANSPARENT_BG,
    m_settings.Get( ts_BgCol ) );
  m_status = RSEL_DRAGGING;
}

void RectangleSelectBehavior::StartDrag(){
  CanvasInterface& canvas = GetAppContext().GetActiveCanvas();
  // Create and initialize a dragbehavior

  const IntRect& selectionRect = canvas.GetRasterSelection();
  faint::Bitmap subBitmap = canvas.GetBitmap( selectionRect );
  delete m_dragBehavior;
  m_dragBehavior = new RectSelDragBehavior( selectionRect, subBitmap, IntPoint(0,0), true );
  bool transparent = m_settings.Get( ts_Transparency ) == TRANSPARENT_BG;
  faint::Color bgColor = m_settings.Get( ts_BgCol );
  m_dragBehavior->UpdateMask( transparent, bgColor );
  canvas.DeselectRaster();
  m_dragBehavior->LeftUp( Point(0,0), 0 );
  m_status = RSEL_DRAGGING;
}

bool RectangleSelectBehavior::CopyData( faint::Bitmap& bmp ){
  if ( m_dragBehavior != 0 ){
    return m_dragBehavior->CopyData( bmp );
  }
  return false;
}

ToolRefresh RectangleSelectBehavior::Delete(){
  if ( m_dragBehavior != 0 ){
    m_command.Set( m_dragBehavior->Delete() );
    delete m_dragBehavior;
    m_dragBehavior = 0;
    m_status = RSEL_NONE;
    return TOOL_COMMIT;
  }
  return TOOL_NONE;
}

bool RectangleSelectBehavior::HasSelection() const{
  return m_dragBehavior != 0;
}

ToolRefresh RectangleSelectBehavior::Deselect(){
  if ( m_dragBehavior != 0 ){
    if ( m_dragBehavior->Preempt() ){
      m_command.Set( m_dragBehavior->GetCommand() );
      delete m_dragBehavior;
      m_dragBehavior = 0;
      m_status = RSEL_NONE;
      return TOOL_COMMIT;
    }
  }
  return TOOL_NONE;
}

unsigned int RectangleSelectBehavior::GetStatusFieldCount(){
  return 1;
}

ToolRefresh RectangleSelectBehavior::InitDragBehavior( const CursorPositionInfo& info, int modifiers ){
  assert( m_status != RSEL_DRAGGING );
  assert( m_dragBehavior == 0 || m_dragBehavior->GetStatus() == DONE_DRAGGING );
  // Create and initialize a dragbehavior
  const IntRect selectionRect = info.canvas->GetRasterSelection();
  faint::Bitmap subBitmap = info.canvas->GetBitmap( selectionRect );
  delete m_dragBehavior;
  m_dragBehavior = new RectSelDragBehavior( selectionRect, subBitmap, truncated(info.pos) - selectionRect.TopLeft());
  bool transparent = m_settings.Get( ts_Transparency ) == TRANSPARENT_BG;
  faint::Color bgColor = m_settings.Get( ts_BgCol );
  m_dragBehavior->UpdateMask( transparent, bgColor );
  m_dragBehavior->LeftDown( info.pos, modifiers );
  info.canvas->DeselectRaster();
  m_status = RSEL_DRAGGING;
  return TOOL_OVERLAY;
}

ToolRefresh RectangleSelectBehavior::InitSelecting( const CursorPositionInfo& info ){
  m_p1 = info.pos;
  m_p2 = m_p1;
  m_maxDistance = 0;
  m_status = RSEL_SELECTING;
  info.canvas->DeselectRaster();
  return TOOL_OVERLAY;
}

ToolRefresh RectangleSelectBehavior::UpdateDragStatus( const CursorPositionInfo& info, int modifiers ){
  DragStatus dragStatus = m_dragBehavior->LeftDown(info.pos, modifiers ) ;
  if ( dragStatus == IS_DRAGGING ){
    m_status = RSEL_DRAGGING;
    return TOOL_NONE;
  }
  else if ( dragStatus == DONE_DRAGGING ){
    m_p1 = m_p2 = info.pos;
    m_maxDistance = 0;
    m_status = RSEL_SELECTING_DRAG_COMMIT_PENDING;
    m_command.Set( m_dragBehavior->GetCommand() );
    delete m_dragBehavior;
    m_dragBehavior = 0;
    return TOOL_COMMIT;
  }
  else if ( dragStatus == CURRENT_DRAG_DONE ){
    m_status = RSEL_DRAGGING;
    m_command.Set( m_dragBehavior->GetCommand() );
    return TOOL_COMMIT;
  }
  else if ( dragStatus == PENDING_FURTHER_DRAG ){
    m_status = RSEL_DRAGGING;
  }
  assert( false );
  return TOOL_NONE;
}

ToolRefresh RectangleSelectBehavior::UpdateTransparency( const Point& pos ){
  AppContext& app( GetAppContext() );
  const faint::Bitmap& bitmap = *(m_dragBehavior->GetBitmap());
  Rect r( m_dragBehavior->GetRect() );
  if ( r.Contains( pos ) ){
    // Point is inside - use the clicked pixel color as mask.
    app.Set( ts_BgCol, GetColor( bitmap, truncated(pos - r.TopLeft()) ) );
    app.Set( ts_Transparency, TRANSPARENT_BG );
  }
  else {
    // Point is outside - disable transparency.
    app.Set( ts_Transparency, OPAQUE_BG );
  }
  return TOOL_NONE;
}

ToolRefresh RectangleSelectBehavior::PreemptedDrag(){
  m_command.Set( m_dragBehavior->GetCommand() );
  m_status = RSEL_NONE;
  delete m_dragBehavior;
  m_dragBehavior = 0;
  return TOOL_COMMIT;
}

ToolRefresh RectangleSelectBehavior::HandleRightClick( const CursorPositionInfo& info, int modifiers ){
  if ( m_status == RSEL_DRAGGING ){
    if ( fl(TOOLMODIFIER1, modifiers ) ){
      // Ctrl+Right-Click
      return UpdateTransparency( info.pos );
    }
    else if ( m_dragBehavior->Preempt() ){
      return PreemptedDrag();
    }
    return TOOL_NONE;
  }
  info.canvas->DeselectRaster();
  return TOOL_OVERLAY;
}

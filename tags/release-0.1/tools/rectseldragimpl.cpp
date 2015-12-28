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

#include "toolbehavior.hh"
#include "rectseldragimpl.hh"
#include "getappcontext.hh"
#include "faintdc.hh"
#include "commands/deletecommand.hh"
#include "objects/objrectangle.hh"

FaintSettings SettingsEraseRect( const faint::Color& color ){
  FaintSettings s = GetRectangleSettings();
  s.Set( ts_FillStyle, FILL );
  s.Set( ts_AntiAlias, false );
  s.Set( ts_FgCol, color );
  s.Set( ts_BgCol, color );
  return s;
}

class RectDragCommand : public Command {
public:
  RectDragCommand( const faint::Bitmap&, const IntRect& newRect,
    const IntRect& oldRect, bool transparent, const faint::Color& bgCol, bool copy );
  void Do( faint::Image& );
private:
  bool m_copy;
  bool m_transparent;
  faint::Color m_bgColor;
  IntRect m_rect;
  IntRect m_oldRect;
  faint::Bitmap m_bitmap;
};

RectDragCommand::RectDragCommand( const faint::Bitmap& bitmap, const IntRect& rect, const IntRect& oldRect,
  bool transparent, const faint::Color& bgColor, bool copy )
  : Command( CMD_TYPE_RASTER ),
    m_copy( copy ),
    m_transparent( transparent ),
    m_bgColor( bgColor ),
    m_rect(rect),
    m_oldRect( oldRect ),
    m_bitmap( bitmap )
{}

void RectDragCommand::Do( faint::Image& img ){
  FaintDC dc( img.GetBitmapRef() );

  if ( ! m_copy ){
    dc.Rectangle( m_oldRect, SettingsEraseRect(m_bgColor) );
  }
  if ( m_transparent ){
    dc.Bitmap( m_bitmap, m_bgColor, floated(m_rect.TopLeft()) );
  }
  else {
    dc.Bitmap( m_bitmap, floated(m_rect.TopLeft()) );
  }
}

RectSelDragBehavior::RectSelDragBehavior( const IntRect& rect, const faint::Bitmap& bitmap, const IntPoint& offset, bool dragging )
  : m_status( dragging ? IS_DRAGGING : PENDING_FURTHER_DRAG ),
    m_bitmap( bitmap ),
    m_offset( offset ),
    m_bgColor( 0, 0, 0 ),
    m_maxConstrainDist( 0 )
{
  m_copying = false;
  m_lastOrigin = rect.TopLeft();

  if ( dragging ){
    m_status = IS_DRAGGING;
  }
  else {
    m_copying = true;
    m_status = PENDING_FURTHER_DRAG;
  }
  m_rect = m_originalRect = rect;
  m_constrainDir = constrain_dir::NONE;
}

bool RectSelDragBehavior::Preempt(){
  if ( m_status == PENDING_FURTHER_DRAG ){
    m_status = DONE_DRAGGING;
    m_command.Set( new RectDragCommand( m_bitmap, m_rect, m_originalRect, m_transparent, m_bgColor, m_copying ) );
    return true;
  }
  return false;
}

DragStatus RectSelDragBehavior::LeftDown( const Point& pFloat, int modifiers ){
  if ( fl( RIGHT_MOUSE, modifiers ) ){
    // Fixme: Doesn't happen
    m_status = DONE_DRAGGING;
    m_command.Set( new RectDragCommand( m_bitmap, m_rect, m_originalRect, m_transparent, m_bgColor, m_copying ) );
    return m_status;
  }

  IntPoint p(truncated(pFloat));
  if ( ! m_rect.Contains( p ) ){
    m_status = DONE_DRAGGING;
    m_command.Set( new RectDragCommand( m_bitmap, m_rect, m_originalRect, m_transparent, m_bgColor, m_copying ) );
    return m_status;
  }
  m_lastPos = p;
  m_lastOrigin = m_rect.TopLeft();
  const bool wantsCopy = ( modifiers & TOOLMODIFIER1 ) != 0;

  if ( m_status == PENDING_FURTHER_DRAG ){
    m_offset = p - m_rect.TopLeft();

    if ( wantsCopy ){
      m_status = CURRENT_DRAG_DONE;
      m_command.Set( new RectDragCommand( m_bitmap, m_rect, m_originalRect, m_transparent, m_bgColor, m_copying ) );
      m_copying = wantsCopy;
    }
    else {
      m_status = IS_DRAGGING;
    }
  }
  else {
    m_copying = wantsCopy;
    m_status = IS_DRAGGING;
  }
  return m_status;
}

DragStatus RectSelDragBehavior::GetStatus(){
  return m_status;
}

DragStatus RectSelDragBehavior::LeftUp( const Point&, int ){
  m_status = PENDING_FURTHER_DRAG;
  return m_status;
}

unsigned int change_constrain_distance( const IntSize& sz, ConstrainDir dir ){
  if ( dir == constrain_dir::VERTICAL ){
    return sz.h / 2;
  }
  else if ( dir == constrain_dir::HORIZONTAL ){
    return sz.w / 2;
  }
  return 0;
}

ToolRefresh RectSelDragBehavior::Motion( const Point& floatPos, int modifiers ){
  IntPoint p = truncated(floatPos);
  if ( m_status == IS_DRAGGING ){
    IntPoint adjPos = p - m_offset;


    if ( (modifiers & TOOLMODIFIER2 ) ){
      // Constrain the move along only one axis (ignore either the X or Y adjustment)
      IntPoint constrainedPos( adjPos );
      ConstrainDir dir = ConstrainPos( constrainedPos, m_lastOrigin );

      // Check the effects of constraining orthogonally:
      //
      // Require a certain distance to change direction
      // (horizontal/vertical), to avoid changing direction when close
      // to origo, so that the constraint can be used even when
      // fine-tuning, without unexpected switch
      if ( m_constrainDir == constrain_dir::NONE || dir == m_constrainDir || m_maxConstrainDist < 25 ){
        // First constrain or same direction - Accept the adjustment
        adjPos = constrainedPos;
        m_constrainDir = dir;
        m_maxConstrainDist = OrthoDistance( constrainedPos, m_lastOrigin, m_constrainDir );
      }
      else {
        unsigned int distance = OrthoDistance( constrainedPos, m_lastOrigin, dir );
        if ( distance > change_constrain_distance( IntSize( m_bitmap.m_w, m_bitmap.m_h ), m_constrainDir ) ){
          // Different constraint direction than last time and at a
          // certain distance - Accept the adjustment
          adjPos = constrainedPos;
          m_constrainDir = dir;
          m_maxConstrainDist = distance;
        }
        else {
          // Different constraint direction than last time but the distance
          // is too small - keep constraining in the old direction
          ConstrainPos( adjPos, m_lastOrigin, m_constrainDir );
        }
      }
    }
    else {
      m_constrainDir = constrain_dir::NONE;
      m_maxConstrainDist = 0;
    }

    m_rect.MoveTo( adjPos );
    m_lastPos = p;


    StatusInterface& status = GetAppContext().GetStatusInfo();
    status.SetText( StrFromTo( m_lastOrigin, adjPos ) );

    return TOOL_OVERLAY;
  }
  if ( m_status == PENDING_FURTHER_DRAG ){
    StatusInterface& status = GetAppContext().GetStatusInfo();
    status.SetText( StrPoint( p ), 0 );
    return TOOL_OVERLAY;
  }
  return TOOL_NONE;

}

bool RectSelDragBehavior::Draw( FaintDC& dc, Overlays& overlays, const Point& ){
  if ( !m_copying ){
    // Draw the erase-rectangle over the dragged-from region
    dc.Rectangle( m_originalRect, SettingsEraseRect(m_bgColor) );
  }

  if ( m_transparent ){
    dc.Bitmap( m_bitmap, m_bgColor, floated(m_rect.TopLeft()) );
  }
  else {
    dc.Bitmap( m_bitmap, floated(m_rect.TopLeft()) );
  }

  if ( m_status != IS_DRAGGING ){
    // Draw the selection-indicator only when not dragging, in order
    // to not obscure anything
    overlays.Rectangle( floated(m_rect) );
  }
  return true;
}

IntRect RectSelDragBehavior::GetRefreshRect() {
  if ( m_copying ){
    return m_rect;
  }
  else {
    // Fixme: This causes a ridiculously large refresh
    return Union( m_rect, m_originalRect );
  }
}

const Rect RectSelDragBehavior::GetRect(){
  return floated(m_rect);
}

Command* RectSelDragBehavior::GetCommand(){
  if ( m_status == CURRENT_DRAG_DONE ){
    m_status = IS_DRAGGING;
  }
  return m_command.Retrieve();
}

void RectSelDragBehavior::UpdateMask( bool transparent, const faint::Color& bgColor ){
  m_transparent = transparent;
  m_bgColor = bgColor;
}

bool RectSelDragBehavior::CopyData( faint::Bitmap& bmp ){
  bmp = m_bitmap;
  return true;
}

Command* RectSelDragBehavior::Delete(){
  if ( m_copying ){
    return 0;
  }
  return new DeleteCommand( m_originalRect, m_bgColor );
}

faint::Bitmap* RectSelDragBehavior::GetBitmap(){
  return &m_bitmap;
}

void RectSelDragBehavior::UpdateBitmap(){
  m_rect.w = m_bitmap.m_w;
  m_rect.h = m_bitmap.m_h;
}

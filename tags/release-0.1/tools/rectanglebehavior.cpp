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

#include "rectanglebehavior.hh"
#include "commands/addobjectcommand.hh"
#include "cursors.hh"
#include "faintdc.hh"
#include "getappcontext.hh"
#include "settings.hh"
#include "settingid.hh"
#include "util/util.hh"
#include "util/objutil.hh"

Point AdjustP1( const Point& p1, const CursorPositionInfo& info, int modifiers ){
  if ( fl(TOOLMODIFIER2, modifiers) ){
    return AdjustTo( p1, info.pos, 90, 45 );
  }
  else if ( fl(TOOLMODIFIER1, modifiers ) ){
    const Point snapped = Snap( info.pos, info.canvas->GetObjects(), info.canvas->GetGrid() );

    // Do not snap to points on the rectangle start point
    return (snapped == p1) ? info.pos : snapped;
  }
  else {
    return info.pos;
  }
}

void AdjustStatus( StatusInterface& status, int modifiers ){
  const bool usingModifier = fl(TOOLMODIFIER1, modifiers) || fl(TOOLMODIFIER2, modifiers );
  status.SetMainText( usingModifier ? "": "Shift=Square, Ctrl=Snap" );
}

RectangleCommand::RectangleCommand( const Point& p0, const Point& p1, const FaintSettings& s)
  : Command( CMD_TYPE_RASTER ),
    m_settings(s),
    m_p0(p0),
    m_p1(p1)
{}

void RectangleCommand::Do( faint::Image& img ){
  FaintDC dc(img.GetBitmapRef());
  dc.Rectangle( m_p0, m_p1, m_settings ); // Fixme: Not the same as dc::Rectangle( Tri )!
}

RectangleBehavior::RectangleBehavior()
  : ToolBehavior(T_RECTANGLE)
{
  m_active = false;
  m_settings = GetRectangleSettings();
}

bool RectangleBehavior::DrawBeforeZoom(Layer layer) const{
  return layer == LAYER_RASTER;
}

ToolRefresh RectangleBehavior::LeftDown( const CursorPositionInfo& info, int modifiers ){
  m_active = true;
  int mouseButton = mbtn( modifiers );
  m_settings.Set( ts_AntiAlias, info.layerType == LAYER_OBJECT );
  m_settings.Set( ts_SwapColors, mouseButton == RIGHT_MOUSE );
  m_p0 = fl( TOOLMODIFIER1, modifiers ) ?
    Snap( info.pos, info.canvas->GetObjects(), info.canvas->GetGrid() ) :
    info.pos;
  m_p1 = info.pos;
  return TOOL_NONE;
}

ToolRefresh RectangleBehavior::LeftUp( const CursorPositionInfo& info, int modifiers ){
  m_active = false;
  m_p1 = AdjustP1( m_p0, info, modifiers );

  if ( info.layerType == LAYER_OBJECT && m_p0 == m_p1 ){
    // Disallow 0-size object rectangles
    return TOOL_NONE;
  }

  if ( info.layerType == LAYER_RASTER ){
    m_command.Set( new RectangleCommand( m_p0, m_p1, m_settings ) );
  }
  else if ( info.layerType == LAYER_OBJECT ){
    Rect r( m_p0, m_p1 );
    m_command.Set( new AddObjectCommand( new ObjRectangle( TriFromRect( r ), m_settings ) ) );
  }
  return TOOL_COMMIT;
}

ToolRefresh RectangleBehavior::Motion( const CursorPositionInfo& info,  int modifiers ){
  StatusInterface& status = GetAppContext().GetStatusInfo();
  if ( !m_active ){
    status.SetText( StrPoint( info.pos ) );
    return TOOL_NONE;
  }
  m_settings.Set( ts_AntiAlias, info.layerType == LAYER_OBJECT );
  m_p1 = AdjustP1( m_p0, info, modifiers );
  AdjustStatus( status, modifiers );
  status.SetText( StrFromTo( m_p0, m_p1 ), 0 );
  return TOOL_OVERLAY;
}

ToolRefresh RectangleBehavior::Preempt(){
  if ( m_active ){
    m_active = false;
    return TOOL_CANCEL;
  }
  return TOOL_NONE;
}

bool RectangleBehavior::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( !m_active ){
    return false;
  }
  dc.Rectangle( m_p0, m_p1, m_settings );
  return true;
}

IntRect RectangleBehavior::GetRefreshRect( const IntRect&, const Point& ){
  return truncated(Inflated(Rect( m_p0, m_p1 ), m_settings.Get( ts_LineWidth ) ) );
}

int RectangleBehavior::GetCursor(const CursorPositionInfo&){
  return CURSOR_SQUARE_CROSS;
}

Command* RectangleBehavior::GetCommand(){
  return m_command.Retrieve();
}

unsigned int RectangleBehavior::GetStatusFieldCount(){
  return 1;
}

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

#include "brushbehavior.hh"
#include "settingid.hh"
#include "faintdc.hh"
#include "getappcontext.hh"

class BrushCommand : public Command{
public:
  BrushCommand( const std::vector<IntPoint>& points, const FaintSettings& settings )
    : Command( CMD_TYPE_RASTER ),
    m_settings( settings ),
    m_points( points ){}

  void Do( faint::Image& img ){
    FaintDC dc(img.GetBitmapRef());
    dc.Stroke( m_points, m_settings );
  }

private:
  FaintSettings m_settings;
  std::vector<IntPoint> m_points;
};

BrushBehavior::BrushBehavior()
  : ToolBehavior( T_BRUSH )
{
  m_constrain = false;
  m_settings.Set( ts_BrushSize, 1 );
  m_settings.Set( ts_BrushShape, BRUSH_SQUARE );
  m_settings.Set( ts_FgCol, faint::Color(0,0,0) );
  m_settings.Set( ts_BgCol, faint::Color(0,0,0) );
  m_settings.Set( ts_SwapColors, false );
  m_active = false;

  // Prevent cursor being drawn at 0,0 before motion.
  m_drawCursor = false;
}

bool BrushBehavior::DrawBeforeZoom(Layer) const{
  return true;
}

ToolRefresh BrushBehavior::LeftDown( const CursorPositionInfo& info, int modifiers ){
  m_active = true;
  if ( RIGHT_MOUSE == mbtn(modifiers) ){
    m_settings.Set( ts_SwapColors, true );
  }
  else{
    m_settings.Set( ts_SwapColors, false );
  }
  m_points.clear();
  m_points.push_back( truncated( info.pos ) );
  return TOOL_OVERLAY;
}

ToolRefresh BrushBehavior::LeftUp( const CursorPositionInfo&, int ){
  if ( !m_active ){
    return TOOL_NONE;
  }
  m_active = false;
  m_command.Set( new BrushCommand( m_points, m_settings ) );
  m_settings.Set( ts_SwapColors, false );
  m_points.clear();
  m_constrain = false;
  return TOOL_COMMIT;
}

ToolRefresh BrushBehavior::Motion( const CursorPositionInfo& info,  int modifiers ){
  StatusInterface& statusInfo = GetAppContext().GetStatusInfo();
  statusInfo.SetMainText("");
  statusInfo.SetText( StrPoint( info.pos ), 0 );

  IntPoint pos( truncated(info.pos ) );
  m_drawCursor = true;
  if ( m_active ) {
    const bool constrainHeld( fl(TOOLMODIFIER2, modifiers) );
    if ( constrainHeld ) {
      if ( m_constrain ){
        ConstrainPos( pos, m_origin );
      }
      else {
        m_origin = pos;
      }
    }
    m_constrain = constrainHeld;

    if ( m_points.back() != pos ){
      m_points.push_back(pos);
    }
    else {
      return TOOL_NONE;
    }
  }
  return TOOL_OVERLAY;
}

bool BrushBehavior::MouseOut(){
  // Prevent leaving a brush cursor dropping when mouse leaves the
  // window
  m_drawCursor = false;
  return true;
}

ToolRefresh BrushBehavior::Preempt(){
  if ( !m_active ){
    return TOOL_NONE;
  }

  m_active = false;
  m_command.Set( new BrushCommand( m_points, m_settings ) );
  m_settings.Set( ts_SwapColors, false );
  m_points.clear();
  m_constrain = false;
  return TOOL_COMMIT;
}

bool BrushBehavior::Draw( FaintDC& dc, Overlays&, const Point& p ){
  if ( m_active ){
    dc.Stroke( m_points, m_settings );
    return true;
  }
  else if ( m_drawCursor ){
    std::vector<IntPoint> v;
    v.push_back( truncated(p) );
    dc.Stroke( v, m_settings );
    return true;
  }
  return false;
}

IntRect BrushBehavior::GetRefreshRect( const IntRect& visible, const Point& mousePos){
  if ( !m_active ){
    const int size = m_settings.Get( ts_BrushSize );
    const IntPoint p( truncated(mousePos) );
    return IntRect( p - size, p + size );
  }
  return GetRefreshRect( visible );
}

IntRect BrushBehavior::GetRefreshRect( const IntRect&){
  const int size = m_settings.Get( ts_BrushSize );
  if ( m_points.size() == 1 ) {
    IntPoint p = m_points.back();
    return IntRect( p - size, p + size );
  }
  IntPoint p0 = m_points[m_points.size() - 1];
  IntPoint p1 = m_points[m_points.size() - 2];
  return Inflated( IntRect( p0, p1 ), size );
}

int BrushBehavior::GetCursor(const CursorPositionInfo&){
  return CURSOR_BRUSH;
}

unsigned int BrushBehavior::GetStatusFieldCount(){
  return 1;
}

Command* BrushBehavior::GetCommand(){
  return m_command.Retrieve();
}

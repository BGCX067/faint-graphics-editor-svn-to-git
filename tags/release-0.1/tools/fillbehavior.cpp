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

#include "fillbehavior.hh"
#include "cursors.hh"
#include "settingid.hh"
#include "faintdc.hh"
#include "getappcontext.hh"
#include "commands/cmdchangesetting.hh"

FillCommand::FillCommand( const Point& point, const faint::Color& color )
  : Command( CMD_TYPE_RASTER ), m_point(point), m_fillColor(color)
{}

void FillCommand::Do( faint::Image& image ){
  faint::Bitmap* bmp = image.GetBitmap();
  FloodFill( *bmp, static_cast<int>(m_point.x), static_cast<int>(m_point.y), m_fillColor );
}

FillBehavior::FillBehavior() :
  ToolBehavior(T_FLOODFILL)
{
  m_settings.Set( ts_FgCol, faint::Color(0,0,0) );
  m_settings.Set( ts_BgCol, faint::Color(0,0,0) );
}

bool FillBehavior::DrawBeforeZoom(Layer) const{
  return false;
}

ToolRefresh FillBehavior::LeftDown( const CursorPositionInfo& info, int modifiers ){
  IntSize sz = info.canvas->GetSize();
  if ( info.pos.x < 0 || info.pos.y < 0 ||
    info.pos.x >= sz.w || info.pos.y >= sz.h ){
    return TOOL_NONE;
  }

  faint::Color fillColor = m_settings.Get(
    LEFT_MOUSE == mbtn(modifiers) ?
    ts_FgCol :
    ts_BgCol
  );

  if ( info.object == 0 || info.layerType == LAYER_RASTER ||
    !( info.hitStatus == HIT_BOUNDARY || info.hitStatus == HIT_INSIDE ) ){

    m_command.Set( new FillCommand( info.pos, fillColor ) );
    return TOOL_COMMIT;
  }

  m_command.Set( new CmdChangeSetting<ColorSetting>( info.object, info.hitStatus == HIT_BOUNDARY ?
      ts_FgCol : ts_BgCol, fillColor ) );
  return TOOL_COMMIT;
}

ToolRefresh FillBehavior::LeftUp( const CursorPositionInfo&,  int ){
  return TOOL_NONE;
}

ToolRefresh FillBehavior::Motion( const CursorPositionInfo& info, int ){
  StatusInterface& status = GetAppContext().GetStatusInfo();
  faint::Bitmap& bmp = info.canvas->GetBitmap();
  if ( info.pos.x < 0 || info.pos.y < 0 || info.pos.x >= static_cast<int>(bmp.m_w) || info.pos.y >= static_cast<int>(bmp.m_h) ) {
    status.SetText( std::string(""));
    return TOOL_NONE;
  }


  faint::Color col = GetColor( bmp, static_cast<int>(info.pos.x), static_cast<int>(info.pos.y) );
  status.SetText( StrSmartRGBA( col ) + " (" + StrPoint( info.pos ) + ")" );
  return TOOL_NONE;
}

ToolRefresh FillBehavior::Preempt(){
  return TOOL_NONE;
}

bool FillBehavior::Draw( FaintDC&, Overlays&, const Point& ){
  return true;
}

Command* FillBehavior::GetCommand(){
  return m_command.Retrieve();
}

int FillBehavior::GetCursor( const CursorPositionInfo& ){
  return CURSOR_FILL;
}

IntRect FillBehavior::GetRefreshRect( const IntRect&, const Point& ){
  return IntRect( IntPoint(0,0),IntSize(0,0) );
}

unsigned int FillBehavior::GetStatusFieldCount(){
  return 1;
}

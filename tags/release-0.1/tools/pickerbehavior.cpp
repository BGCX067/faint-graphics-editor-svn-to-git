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

#include "settingid.hh"
#include "bitmap/bitmap.h"
#include "getappcontext.hh"
#include "canvasinterface.hh"
#include "pickerbehavior.hh"
#include <string>

PickerBehavior::PickerBehavior()
  : ToolBehavior( T_PICKER )
{}

bool PickerBehavior::DrawBeforeZoom(Layer) const{
  return false;
}

ToolRefresh PickerBehavior::LeftDown( const CursorPositionInfo& info, int modifiers ){
  faint::Bitmap& bmp = info.canvas->GetBitmap();

  // Only allow picking within the image
  IntRect r( IntPoint(0, 0), IntSize(bmp.m_w, bmp.m_h));
  if ( !r.Contains( truncated(info.pos) ) ){
    return TOOL_NONE;
  }

  const ColorSetting colorId = mbtn( modifiers ) == LEFT_MOUSE ?
    ts_FgCol : ts_BgCol;

  if ( info.object == 0 || info.layerType == LAYER_RASTER ||
    !( info.hitStatus == HIT_BOUNDARY || info.hitStatus == HIT_INSIDE ) ) {
      faint::Color color = GetColor( bmp, static_cast<int>(info.pos.x), static_cast<int>(info.pos.y) );
      GetAppContext().Set( colorId, color );
      return TOOL_NONE;
  }

  GetAppContext().Set( colorId, info.object->GetSettings().Get(
    info.hitStatus == HIT_BOUNDARY ? ts_FgCol : ts_BgCol ) );
  return TOOL_NONE;
}

ToolRefresh PickerBehavior::LeftUp( const CursorPositionInfo&, int ){
  return TOOL_NONE;
}

ToolRefresh PickerBehavior::Motion( const CursorPositionInfo& info,  int ){
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

ToolRefresh PickerBehavior::Preempt(){
  return TOOL_NONE;
}

bool PickerBehavior::Draw( FaintDC&, Overlays&, const Point& ){
  return false;
}

Command* PickerBehavior::GetCommand(){
  assert( false );
  return 0;
}

IntRect PickerBehavior::GetRefreshRect( const IntRect&, const Point& ){
  return IntRect(IntPoint(0, 0), IntSize(0, 0));
}

int PickerBehavior::GetCursor( const CursorPositionInfo& ){
  return CURSOR_PICKER;
}

unsigned int PickerBehavior::GetStatusFieldCount(){
  return 1;
}

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

#include "deletecommand.hh"
#include "faintdc.hh"
#include "faintimage.hh"
DeleteCommand::DeleteCommand( const IntRect& rect, const faint::Color& bgColor )
  : Command( CMD_TYPE_RASTER ), m_rect(rect), m_color(bgColor)
{}

void DeleteCommand::Do( faint::Image& img ){
  FaintSettings s;
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_BgCol, m_color );
  s.Set( ts_FgCol, m_color );
  s.Set( ts_FillStyle, FILL ); 
  s.Set( ts_AntiAlias, false );
  FaintDC dc( img.GetBitmapRef() );
  dc.Rectangle( m_rect, s );
}

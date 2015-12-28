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

#include "resizecommand.hh"
#include "bitmap/cairo_util.h"
#include "faintimage.hh"
#include "objects/object.hh"

ResizeCommand::ResizeCommand( const IntSize& size, const faint::Color& bgColor )
  : Command(CMD_TYPE_HYBRID),
    m_rect( IntPoint(0, 0), size ),
    m_bgColor( bgColor )
{}

ResizeCommand::ResizeCommand( const IntRect& rect, const faint::Color& bgColor )
  : Command(CMD_TYPE_HYBRID),
    m_rect( rect ),
    m_bgColor( bgColor )
{}

void ResizeCommand::Do( faint::Image& image ){
  DoRaster( image );
  std::vector<Object*>& objects = image.GetObjects();
  for ( size_t i = 0; i != objects.size(); i++ ){
    OffsetBy( objects[i], -m_rect.TopLeft() );
  }
}

void ResizeCommand::DoRaster( faint::Image& image ){
  faint::Bitmap bmp = faint::CairoCompatibleBitmap( static_cast<faint::uint>(m_rect.w), static_cast<faint::uint>(m_rect.h) );
  Clear( bmp, m_bgColor );
  faint::Bitmap& oldBmp = *( image.GetBitmap() );
  DrawBitmap( bmp, oldBmp, static_cast<int>(-m_rect.x), static_cast<int>(-m_rect.y) );
  image.SetBitmap( bmp );
}

void ResizeCommand::Undo( faint::Image& image ){
  std::vector<Object*>& objects = image.GetObjects();
  for ( size_t i = 0; i != objects.size(); i++ ){
    OffsetBy( objects[i], m_rect.TopLeft() );    
  }
}

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

#include "cmdfliprot.hh"
#include "bitmap/bitmap.h"
#include "faintimage.hh"
#include "faintdc.hh"
#include "bitmap/cairo_util.h"
#include "objects/object.hh"
#include "util/angle.hh"
CmdFlipRotate::CmdFlipRotate( op operation, int angle )
  : Command( CMD_TYPE_HYBRID ),
    m_rect(IntPoint(0,0),IntSize(0,0))
{
  m_op = operation;
  m_angle = angle;
}

CmdFlipRotate::CmdFlipRotate( op operation, IntRect region, int angle )
  : Command( CMD_TYPE_RASTER ),
    m_rect(region)
{
  m_op = operation;
  m_angle = angle;
}

void CmdFlipRotate::Do( faint::Image& img ){
  if ( Empty(m_rect) ){
    // Use the entire image and also flip objects
    const Size size( floated( img.GetSize() ) );

    if ( m_op == ROTATE ){
      std::vector<Object*>& objects = img.GetObjects();
      Point p0( size.w, size.h );
      for ( size_t i = 0; i!= objects.size(); i++ ){
        objects[i]->SetTri( Translated( Rotated( objects[i]->GetTri(), faint::pi / 2, p0 ),
            - size.w, size.w - size.h ) );
      }
    }
    else if ( m_op == FLIP_HORIZONTAL ){
      std::vector<Object*>& objects = img.GetObjects();
      const Point p0( size.w / 2.0f, size.h / 2.0f );
      for ( size_t i = 0; i!= objects.size(); i++ ){
        objects[i]->SetTri( Scaled( objects[i]->GetTri(), -1, 1, p0 ) );
      }
    }
    else if ( m_op == FLIP_VERTICAL ){
      std::vector<Object*>& objects = img.GetObjects();
      const Point p0( size.w / 2.0f, size.h / 2.0f );
      for ( size_t i = 0; i!= objects.size(); i++ ){
        objects[i]->SetTri( Scaled( objects[i]->GetTri(), 1, -1, p0 ) );
      }
    }
  }
  DoRaster(img);
}

void CmdFlipRotate::Undo( faint::Image& img ){
  if ( !Empty( m_rect ) ){
    // No object undo when a selected region was flipped/rotated.
    return;
  }
  const Size size( floated( img.GetSize() ) );
  if ( m_op == ROTATE ){
    std::vector<Object*>& objects = img.GetObjects();
    const Size size( floated(img.GetSize()) );
    Point p0( size.w, size.h );
    for ( size_t i = 0; i != objects.size(); i++ ){
      objects[i]->SetTri( Translated( Rotated( objects[i]->GetTri(), - faint::pi / 2, p0 ),
          - size.w + size.h, - size.h ) );
    }
  }
  else if ( m_op == FLIP_HORIZONTAL ){
    std::vector<Object*>& objects = img.GetObjects();
    const Point p0( size.w / 2.0f, size.h / 2.0f );
    for ( size_t i = 0; i!= objects.size(); i++ ){
      objects[i]->SetTri( Scaled( objects[i]->GetTri(), -1, 1, p0 ) );
    }
  }
  else if ( m_op == FLIP_VERTICAL ){
    std::vector<Object*>& objects = img.GetObjects();
    const Point p0( size.w / 2.0f, size.h / 2.0f );
    for ( size_t i = 0; i!= objects.size(); i++ ){
      objects[i]->SetTri( Scaled( objects[i]->GetTri(), 1, -1, p0 ) );
    }
  }
}

void CmdFlipRotate::DoRaster( faint::Image& img ){
  const faint::Bitmap& bmp = img.GetBitmapRef();
  Point p0( static_cast<faint::coord>(bmp.m_w) / 2.0, static_cast<faint::coord>(bmp.m_h) / 2.0 );
  if ( Empty( m_rect ) ){
    if ( m_op == FLIP_HORIZONTAL ){
      img.SetBitmap( faint::FlipHorizontal( bmp ) );
    }
    else if ( m_op == FLIP_VERTICAL ){
      img.SetBitmap( faint::FlipVertical( bmp ) );
    }
    else if ( m_op == ROTATE ){
      img.SetBitmap( faint::Rotate90CW( bmp ) );
    }
  }
  else {
    faint::Bitmap adjusted = CairoCompatibleSubBitmap( bmp, m_rect );
    if ( m_op == FLIP_HORIZONTAL ){
      adjusted = faint::FlipHorizontal( adjusted );
    }
    else if ( m_op == FLIP_VERTICAL){
      adjusted = faint::FlipVertical( adjusted );
    }
    else if ( m_op == ROTATE ){
      adjusted = faint::Rotate90CW( adjusted );
    }

    FaintDC dc( img.GetBitmapRef() );
    dc.Bitmap( adjusted, floated(m_rect.TopLeft()) );
  }
}

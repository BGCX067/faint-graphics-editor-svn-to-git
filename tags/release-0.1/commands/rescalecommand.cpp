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

#include "rescalecommand.hh"
#include "commandbunch.hh"
#include "faintimage.hh"
#include "objects/object.hh"
#include "tricommand.hh"
#include <cassert>

RescaleCommand::RescaleCommand( const IntSize& size, Quality quality )
  : Command( CMD_TYPE_HYBRID ),
    m_size(size),
    m_quality(quality),
    m_objectResize( 0 )
{}

void RescaleCommand::DoRaster( faint::Image& image ){
  const faint::Bitmap& bmpOld = image.GetBitmapRef();
  faint::coord old_w = static_cast<faint::coord>( bmpOld.m_w );
  faint::coord old_h = static_cast<faint::coord>( bmpOld.m_h );
  if ( m_quality == Bilinear ){
    faint::Bitmap bmpNew = ScaleBilinear( bmpOld, m_size.w / old_w, m_size.h / old_h );
    image.SetBitmap( bmpNew );
  }
  else if ( m_quality == NearestNeighbour ){
    faint::Bitmap bmpNew = ScaleNearest( bmpOld, m_size.w / old_w, m_size.h / old_h );
    image.SetBitmap( bmpNew );
  }
  else {
    assert( false );
  }
}

void RescaleCommand::Do( faint::Image& image ){
  Size oldSize( floated( image.GetSize() ) );
  DoRaster( image );

  if ( m_objectResize == 0 ){
    m_objectResize = new CommandBunch( CMD_TYPE_OBJECT );
    std::vector<Object*> objects = image.GetObjects();

    for ( size_t i = 0; i != objects.size(); i++ ){
      Object* o = objects[i];
      Tri oldTri( o->GetTri() );
      Tri newTri(Scaled( oldTri, m_size.w / oldSize.w, m_size.h / oldSize.h, Point( 0, 0 ) ) );
      m_objectResize->Add( new TriCommand( o, newTri, oldTri ) );
    }
  }
  m_objectResize->Do( image );
}

void RescaleCommand::Undo( faint::Image& image ){
  if ( m_objectResize != 0 ){
    m_objectResize->Undo(image);
  }
}

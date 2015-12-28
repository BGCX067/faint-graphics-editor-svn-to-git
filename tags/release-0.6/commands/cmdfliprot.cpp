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

#include <cassert>
#include "commands/cmdfliprot.hh"
#include "objects/object.hh"
#include "rendering/cairocontext.hh"
#include "rendering/faintdc.hh"
#include "util/angle.hh"
#include "util/settingutil.hh"

class CmdFlipRotate : public Command {
public:
  enum op{ FLIP_HORIZONTAL, FLIP_VERTICAL, ROTATE };
  CmdFlipRotate( op );
  CmdFlipRotate( op, const IntRect& region );
  std::string Name() const;
  void Do( CommandContext& );
  void Undo( CommandContext& );
  void DoRaster( CommandContext& );
private:
  op m_op;
  IntRect m_rect;
};

Axis::type operation_to_axis( CmdFlipRotate::op operation){
  if ( operation == CmdFlipRotate::FLIP_HORIZONTAL ){
    return Axis::HORIZONTAL;
  }
  else if ( operation == CmdFlipRotate::FLIP_VERTICAL ){
    return Axis::VERTICAL;
  }
  assert( false );
  return Axis::HORIZONTAL;
}

bool is_flip_operation( CmdFlipRotate::op operation ){
  return operation == CmdFlipRotate::FLIP_HORIZONTAL ||
    operation == CmdFlipRotate::FLIP_VERTICAL;
}

CmdFlipRotate::op to_flip( Axis::type axis ){
  return axis == Axis::HORIZONTAL ? CmdFlipRotate::FLIP_HORIZONTAL :
    CmdFlipRotate::FLIP_VERTICAL;
}

Command* rotate_image_90cw_command(){
  return new CmdFlipRotate(CmdFlipRotate::ROTATE);
}

Command* flip_image_command( Axis::type axis ){
  return new CmdFlipRotate(to_flip(axis));
}

Command* rotate_90cw_command( const IntRect& rect){
  return new CmdFlipRotate(CmdFlipRotate::ROTATE, rect);
}

Command* flip_command( Axis::type axis, const IntRect& rect ){
  return new CmdFlipRotate(to_flip(axis), rect);
}

CmdFlipRotate::CmdFlipRotate( op operation )
  : Command( CMD_TYPE_HYBRID ),
    m_op(operation),
    m_rect(IntPoint(0,0),IntSize(0,0))
{}

CmdFlipRotate::CmdFlipRotate( op operation, const IntRect& region )
  : Command( CMD_TYPE_RASTER ),
    m_op(operation),
    m_rect(region)
{}

std::string CmdFlipRotate::Name() const{
  bool fullImage = empty(m_rect);
  switch ( m_op ){
    case ROTATE:
      return fullImage ?
	"Rotate Image" :
	"Rotate Region";
    case FLIP_HORIZONTAL:
      return fullImage ?
	"Horizontal Image Flip" :
	"Horizontal Region Flip";
    case FLIP_VERTICAL:
      return fullImage ?
	"Vertical Image Flip" :
	"Vertical Region Flip";
    default:
      assert(false);
      return "";
  }
}

void CmdFlipRotate::Do( CommandContext& context ){
  if ( empty(m_rect) ){
    // Use the entire image and also flip objects
    const Size size( floated( context.GetImageSize() ) );
    const objects_t& objects = context.GetObjects();
    if ( m_op == ROTATE ){
      Point p0( size.w, size.h );
      for ( size_t i = 0; i!= objects.size(); i++ ){
        objects[i]->SetTri( translated( rotated( objects[i]->GetTri(), faint::pi / 2, p0 ),
	    -size.w, size.w - size.h ) );
      }
    }
    else if ( m_op == FLIP_HORIZONTAL ){
      const Point p0( size.w / 2.0f, size.h / 2.0f );
      for ( size_t i = 0; i!= objects.size(); i++ ){
        objects[i]->SetTri( scaled( objects[i]->GetTri(), invert_x_scale(), p0 ) );
      }
    }
    else if ( m_op == FLIP_VERTICAL ){
      const Point p0( size.w / 2.0f, size.h / 2.0f );
      for ( size_t i = 0; i!= objects.size(); i++ ){
        objects[i]->SetTri( scaled( objects[i]->GetTri(), invert_y_scale(), p0 ) );
      }
    }
  }
  DoRaster(context);
}

void CmdFlipRotate::Undo( CommandContext& context ){
  if ( !empty( m_rect ) ){
    // No object undo when a selected region was flipped/rotated.
    return;
  }

  const Size size( floated( context.GetImageSize() ) );
  const objects_t& objects = context.GetObjects();
  if ( m_op == ROTATE ){
    Point p0( size.w, size.h );
    for ( size_t i = 0; i != objects.size(); i++ ){
      objects[i]->SetTri( translated( rotated( objects[i]->GetTri(), - faint::pi / 2, p0 ),
          - size.w + size.h, - size.h ) );
    }
  }
  else if ( m_op == FLIP_HORIZONTAL ){
    const Point p0( size.w / 2.0f, size.h / 2.0f );
    for ( size_t i = 0; i!= objects.size(); i++ ){
      objects[i]->SetTri( scaled( objects[i]->GetTri(), invert_x_scale(), p0 ) );
    }
  }
  else if ( m_op == FLIP_VERTICAL ){
    const Point p0( size.w / 2.0f, size.h / 2.0f );
    for ( size_t i = 0; i!= objects.size(); i++ ){
      objects[i]->SetTri( scaled( objects[i]->GetTri(), invert_y_scale(), p0 ) );
    }
  }
}

BITMAPRETURN flip_or_rotate_bitmap( const faint::Bitmap& bmp, CmdFlipRotate::op operation ){
  if ( is_flip_operation(operation) ){
    return faint::flip(bmp, operation_to_axis(operation) );
  }
  else if ( operation == CmdFlipRotate::ROTATE ){
    return faint::rotate_90cw( bmp );
  }
  assert( false );
  return faint::get_null_bitmap();
}

void CmdFlipRotate::DoRaster( CommandContext& context ){
  const faint::Bitmap& bmp( context.GetBitmap() );
  Point p0( static_cast<faint::coord>(bmp.m_w) / 2.0, static_cast<faint::coord>(bmp.m_h) / 2.0 );
  if ( empty( m_rect ) ){
    // Adjust the entire image
    context.SetBitmap( flip_or_rotate_bitmap(bmp, m_op) );
  }
  else {
    faint::Bitmap adjusted = cairo_compatible_sub_bitmap( bmp, m_rect );
    adjusted = flip_or_rotate_bitmap(adjusted, m_op);
    FaintDC& dc( context.GetDC() );
    dc.Bitmap( adjusted, floated(m_rect.TopLeft()), default_bitmap_settings() ); // Fixme
  }
}

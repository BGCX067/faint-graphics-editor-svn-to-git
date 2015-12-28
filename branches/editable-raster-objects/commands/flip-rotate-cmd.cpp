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
#include "commands/flip-rotate-cmd.hh"
#include "objects/object.hh"
#include "rendering/faintdc.hh"
#include "util/angle.hh"
#include "util/settingutil.hh"
#include "util/commandutil.hh" // for get_rotate_command

class CmdFlipRotate : public Command {
public:
  enum op{ FLIP_HORIZONTAL, FLIP_VERTICAL, ROTATE };
  CmdFlipRotate( op );
  CmdFlipRotate( op, const IntRect& region );
  void Do( CommandContext& ) override;
  void DoRaster( CommandContext& ) override;
  std::string Name() const override;
  void Undo( CommandContext& ) override;
private:
  op m_op;
  IntRect m_rect;
};

Axis operation_to_axis( CmdFlipRotate::op operation){
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

CmdFlipRotate::op to_flip( Axis axis ){
  return axis == Axis::HORIZONTAL ? CmdFlipRotate::FLIP_HORIZONTAL :
    CmdFlipRotate::FLIP_VERTICAL;
}

static Point get_object_offset( const Rect& imageRect, faint::radian angle ){
  Tri tri = rotated(tri_from_rect(imageRect), angle, imageRect.Center());
  return bounding_rect(tri).TopLeft();
}

class RotateImageCommand : public Command {
public:
  RotateImageCommand(faint::coord angle, const faint::Color& bgColor)
    : Command(CommandType::HYBRID),
      m_angle(angle),
      m_bgColor(bgColor)
  {}

  void Do( CommandContext& ctx ) override{
    Rect rect = floated(rect_from_size(ctx.GetImageSize()));
    m_offset = get_object_offset(rect, m_angle);

    for ( Object* obj : ctx.GetObjects() ){
      obj->SetTri(translated(rotated(obj->GetTri(), m_angle, rect.Center()),
        -m_offset.x, -m_offset.y));
    }

    ctx.SetBitmap(rotate(ctx.GetBitmap(), m_angle, m_bgColor));
  }

  std::string Name() const override{
    return "Rotate Image"; // Fixme: Add measure
  }

  void Undo( CommandContext& ctx ) override{
    Size oldSize = floated(ctx.GetImageSize());
    const objects_t& objects = ctx.GetObjects();
    Point center = point_from_size(oldSize) / 2;
    for ( Object* obj : objects ){
      obj->SetTri(translated(rotated(obj->GetTri(), -m_angle, center),
          m_offset.x, m_offset.y));
    }
  }
private:
  faint::radian m_angle;
  faint::Color m_bgColor;
  Point m_offset;
};

Command* rotate_image_command(faint::radian angle, const faint::Color& bgColor){
  return new RotateImageCommand(angle, bgColor);
}

Command* rotate_image_90cw_command(){
  return new CmdFlipRotate(CmdFlipRotate::ROTATE);
}

Command* flip_image_command( Axis axis ){
  return new CmdFlipRotate(to_flip(axis));
}

Command* rotate_90cw_command( const IntRect& rect){
  return new CmdFlipRotate(CmdFlipRotate::ROTATE, rect);
}

Command* flip_command( Axis axis, const IntRect& rect ){
  return new CmdFlipRotate(to_flip(axis), rect);
}

CmdFlipRotate::CmdFlipRotate( op operation )
  : Command( CommandType::HYBRID ),
    m_op(operation),
    m_rect(IntPoint(0,0),IntSize(0,0))
{}

CmdFlipRotate::CmdFlipRotate( op operation, const IntRect& region )
  : Command( CommandType::RASTER ),
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
      for ( Object* obj : objects ){
        obj->SetTri( translated( rotated( obj->GetTri(), faint::pi / 2, p0 ),
       -size.w, size.w - size.h ) );
      }
    }
    else if ( m_op == FLIP_HORIZONTAL ){
      const Point p0( size.w / 2.0f, size.h / 2.0f );
      for ( Object* obj : objects ){
        obj->SetTri( scaled( obj->GetTri(), invert_x_scale(), p0 ) );
      }
    }
    else if ( m_op == FLIP_VERTICAL ){
      const Point p0( size.w / 2.0f, size.h / 2.0f );
      for ( Object* obj : objects ){
        obj->SetTri( scaled( obj->GetTri(), invert_y_scale(), p0 ) );
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
    for ( Object* obj : objects ){
      obj->SetTri( translated( rotated( obj->GetTri(), -faint::pi / 2, p0 ),
          - size.w + size.h, - size.h ) );
    }
  }
  else if ( m_op == FLIP_HORIZONTAL ){
    const Point p0( size.w / 2.0f, size.h / 2.0f );
    for ( Object* obj : objects ){
      obj->SetTri( scaled( obj->GetTri(), invert_x_scale(), p0 ) );
    }
  }
  else if ( m_op == FLIP_VERTICAL ){
    const Point p0( size.w / 2.0f, size.h / 2.0f );
    for ( Object* obj : objects ){
      obj->SetTri( scaled( obj->GetTri(), invert_y_scale(), p0 ) );
    }
  }
}

faint::Bitmap flip_or_rotate_bitmap( const faint::Bitmap& bmp, CmdFlipRotate::op operation ){
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
    faint::Bitmap adjusted(sub_bitmap( bmp, m_rect ));
    adjusted = flip_or_rotate_bitmap(adjusted, m_op);
    FaintDC& dc( context.GetDC() );
    dc.Bitmap( adjusted, floated(m_rect.TopLeft()), default_bitmap_settings() ); // Fixme
  }
}

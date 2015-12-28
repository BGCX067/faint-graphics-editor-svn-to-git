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
#include "commands/rescalecommand.hh"
#include "commands/tricommand.hh"
#include "objects/object.hh"
#include "util/commandutil.hh"

RescaleCommand::RescaleCommand( const IntSize& size, ScaleQuality::type quality )
  : Command( CMD_TYPE_HYBRID ),
    m_size(size),
    m_oldSize(0,0), // Proper values set later
    m_quality(quality),
    m_objectResize( 0 )
{}

void RescaleCommand::Do( CommandContext& context ){
  Size oldSize( floated( context.GetImageSize() ) );
  DoRaster( context );

  if ( m_objectResize == 0 && context.HasObjects()){
    const Point origin(0,0);
    m_objectResize = get_scale_command(context.GetObjects(), Scale(New(floated(m_size)), oldSize), origin);
  }
  if ( m_objectResize != 0 ){
    m_objectResize->Do( context );
  }
}

void RescaleCommand::DoRaster( CommandContext& context ){
  const faint::Bitmap& bmp(context.GetBitmap());
  m_oldSize = IntSize(bmp.GetSize());
  Scale scale(New(floated(m_size)), floated(m_oldSize));
  context.SetBitmap( faint::scale(bmp, scale, m_quality) );
}

std::string RescaleCommand::Name() const{
  return "Rescale Image";
}

Point RescaleCommand::Translate( const Point& p ) const{
  // Fixme: It'd probably be better if this adjustment was centered,
  // rather than aligned with the left/top edges
  Size ratio(floated(m_size) / floated(m_oldSize) );
  return p * Point(ratio.w, ratio.h);
}


void RescaleCommand::Undo( CommandContext& context ){
  if ( m_objectResize != 0 ){
    m_objectResize->Undo(context);
  }
}

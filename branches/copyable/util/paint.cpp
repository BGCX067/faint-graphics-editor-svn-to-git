// -*- coding: us-ascii-unix -*-
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

#include "bitmap/bitmap.hh"
#include "util/color.hh"
#include "util/gradient.hh"
#include "util/paint.hh"
#include "util/pattern.hh"

namespace faint{
class PaintImpl{
public:
  PaintImpl( const faint::Color& in_color )
    : color(in_color)
  {}
  PaintImpl( const faint::Pattern& in_pattern )
    : pattern(in_pattern)
  {}
  PaintImpl( const faint::Gradient& in_gradient )
    : gradient(in_gradient)
  {}
  Optional<Color> color;
  Optional<Pattern> pattern;
  Optional<Gradient> gradient;
};

Paint::Paint(){
  m_impl = new PaintImpl(faint::Color(0,0,0));
}

Paint::Paint( const faint::Color& color ){
  m_impl = new PaintImpl(color);
}

Paint::Paint( const Gradient& g ){
  m_impl = new PaintImpl(g);
}

Paint::Paint( const faint::Pattern& pattern ){
  m_impl = new PaintImpl(pattern);
}

Paint::Paint( const Paint& other ){
  m_impl = new PaintImpl(*other.m_impl);
}

Paint::Paint(Paint&& other){
  m_impl = other.m_impl;
  other.m_impl = nullptr;
}

Paint::~Paint(){
  delete m_impl;
}

bool Paint::IsColor() const{
  return m_impl->color.IsSet();
}

bool Paint::IsGradient() const{
  return m_impl->gradient.IsSet();
}

bool Paint::IsPattern() const{
  return m_impl->pattern.IsSet();
}

const faint::Color& Paint::GetColor() const {
  return m_impl->color.Get();
}

const faint::Gradient& Paint::GetGradient() const{
  return m_impl->gradient.Get();
}

const faint::Pattern& Paint::GetPattern() const {
  return m_impl->pattern.Get();
}

Paint& Paint::operator=( const Paint& other ){
  if ( this == &other ){
    return *this;
  }

  delete m_impl;
  m_impl = new PaintImpl(*other.m_impl);
  return *this;
}

bool operator==( const Paint& a, const Paint& b ){
  if ( a.IsColor() ){
    return b.IsColor() ?
      a.GetColor() == b.GetColor() :
      false;
  }
  else if ( a.IsGradient() ){
    return b.IsGradient() ?
      a.GetGradient() == b.GetGradient() :
      false;
  }
  else if ( a.IsPattern() ){
    return b.IsPattern() ?
      a.GetPattern() == b.GetPattern() :
      false;
  }
  assert(false);
  return false;
}

bool operator!=( const Paint& a, const Paint& b ){
  return !operator==(a,b);
}

bool operator==( const Paint& src, const Color& c ){
  return src.IsColor() && src.GetColor() == c;
}

bool operator==( const Color& c, const Paint& src ){
  return src.IsColor() && src.GetColor() == c;
}

bool operator!=( const Paint& src, const Color& c ){
  return !operator==( src, c );
}

bool operator!=( const Color& c, const Paint& src ){
  return !operator==( src, c );
}

Paint offsat( const Paint& src, const IntPoint& delta ){
  if ( src.IsPattern() ){
    Pattern p(src.GetPattern());
    p.SetAnchor(p.GetAnchor() + delta);
    return Paint(p);
  }
  return src;
}

Paint offsat( const Paint& src, const IntPoint& delta, const IntPoint& clickPos ){
  if ( src.IsPattern() ){
    Pattern p(src.GetPattern());
    bool useClickPos = p.GetObjectAligned();
    p.SetAnchor(p.GetAnchor() + delta - ( useClickPos ? clickPos : IntPoint(0,0) ) );
    return Paint(p);
  }
  return src;
}

faint::Color get_color_default( const faint::Paint& src, const faint::Color& defaultColor ){
  return src.IsColor() ? src.GetColor() :
    defaultColor;
}


bool is_opaque_color( const Paint& src ){
  return src.IsColor() && opaque(src.GetColor());
}

bool is_translucent_color( const Paint& src ){
  return src.IsColor() && translucent(src.GetColor());
}

}

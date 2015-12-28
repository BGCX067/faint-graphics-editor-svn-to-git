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
#include "util/drawsource.hh"
#include "util/gradient.hh"
#include "util/pattern.hh"

namespace faint{
class DrawSourceImpl{
public:
  DrawSourceImpl( const faint::Color& in_color )
    : color(in_color)
  {}
  DrawSourceImpl( const faint::Pattern& in_pattern )
    : pattern(in_pattern)
  {}
  DrawSourceImpl( const faint::Gradient& in_gradient )
    : gradient(in_gradient)
  {}
  Optional<Color> color;
  Optional<Pattern> pattern;
  Optional<Gradient> gradient;
};

DrawSource::DrawSource(){
  m_impl = new DrawSourceImpl(faint::Color(0,0,0));
}

DrawSource::DrawSource( const faint::Color& color ){
  m_impl = new DrawSourceImpl(color);
}

DrawSource::DrawSource( const Gradient& g ){
  m_impl = new DrawSourceImpl(g);
}

DrawSource::DrawSource( const faint::Pattern& pattern ){
  m_impl = new DrawSourceImpl(pattern);
}

DrawSource::DrawSource( const DrawSource& other ){
  m_impl = new DrawSourceImpl(*other.m_impl);
}

DrawSource::~DrawSource(){
  delete m_impl;
}

bool DrawSource::IsColor() const{
  return m_impl->color.IsSet();
}

bool DrawSource::IsGradient() const{
  return m_impl->gradient.IsSet();
}

bool DrawSource::IsPattern() const{
  return m_impl->pattern.IsSet();
}

const faint::Color& DrawSource::GetColor() const {
  return m_impl->color.Get();
}

const faint::Gradient& DrawSource::GetGradient() const{
  return m_impl->gradient.Get();
}

const faint::Pattern& DrawSource::GetPattern() const {
  return m_impl->pattern.Get();
}

DrawSource& DrawSource::operator=( const DrawSource& other ){
  if ( this == &other ){
    return *this;
  }

  delete m_impl;
  m_impl = new DrawSourceImpl(*other.m_impl);
  return *this;
}

bool operator==( const DrawSource& a, const DrawSource& b ){
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

bool operator!=( const DrawSource& a, const DrawSource& b ){
  return !operator==(a,b);
}

bool operator==( const DrawSource& src, const Color& c ){
  return src.IsColor() && src.GetColor() == c;
}

bool operator==( const Color& c, const DrawSource& src ){
  return src.IsColor() && src.GetColor() == c;
}

bool operator!=( const DrawSource& src, const Color& c ){
  return !operator==( src, c );
}

bool operator!=( const Color& c, const DrawSource& src ){
  return !operator==( src, c );
}

DrawSource offsat( const DrawSource& src, const IntPoint& delta ){
  if ( src.IsPattern() ){
    Pattern p(src.GetPattern());
    p.SetAnchor(p.GetAnchor() + delta);
    return DrawSource(p);
  }
  return src;
}

DrawSource offsat( const DrawSource& src, const IntPoint& delta, const IntPoint& clickPos ){
  if ( src.IsPattern() ){
    Pattern p(src.GetPattern());
    bool useClickPos = p.GetObjectAligned();
    p.SetAnchor(p.GetAnchor() + delta - ( useClickPos ? clickPos : IntPoint(0,0) ) );
    return DrawSource(p);
  }
  return src;
}

faint::Color get_color_default( const faint::DrawSource& src, const faint::Color& defaultColor ){
  return src.IsColor() ? src.GetColor() :
    defaultColor;
}


bool is_opaque_color( const DrawSource& src ){
  return src.IsColor() && opaque(src.GetColor());
}

bool is_translucent_color( const DrawSource& src ){
  return src.IsColor() && translucent(src.GetColor());
}

}

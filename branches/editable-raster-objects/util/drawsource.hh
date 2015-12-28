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

#ifndef FAINT_DRAWSOURCE_HH
#define FAINT_DRAWSOURCE_HH

#include "geo/intpoint.hh"

namespace faint{
class Color;
class Gradient;
class Pattern;
class DrawSourceImpl;
class DrawSource{
public:
  DrawSource();
  explicit DrawSource( const Color& );
  explicit DrawSource( const Gradient& );
  explicit DrawSource( const Pattern&);
  DrawSource( const DrawSource& );
  ~DrawSource();
  bool IsColor() const;
  bool IsGradient() const;
  bool IsPattern() const;
  const Color& GetColor() const;
  const Gradient& GetGradient() const;
  const Pattern& GetPattern() const;
  DrawSource& operator=( const DrawSource& );
private:
  DrawSourceImpl* m_impl;
};

DrawSource offsat( const DrawSource&, const IntPoint& delta );
DrawSource offsat( const DrawSource&, const IntPoint& delta, const IntPoint& clickPos );

bool operator==( const DrawSource&, const DrawSource& );
bool operator!=( const DrawSource&, const DrawSource& );
bool operator==( const DrawSource&, const Color& );
bool operator==( const Color&, const DrawSource& );
bool operator!=( const DrawSource&, const Color& );
bool operator!=( const Color&, const DrawSource& );

// Get a color from the DrawSource if it contains a color, otherwise get the defaultColor
faint::Color get_color_default( const faint::DrawSource& src, const faint::Color& defaultColor );

// True if the DrawSource contains a Color with alpha of 255
bool is_opaque_color( const DrawSource& );

// True if the DrawSource contains a Color with alpha less than 255
bool is_translucent_color( const DrawSource& );

} // namespace

template<typename CF, typename PF, typename GF>
auto dispatch(const faint::DrawSource& src, CF colorFunc, PF patternFunc, GF gradientFunc) -> typename std::result_of<CF(faint::Color)>::type {
  if ( src.IsColor() ){
    return colorFunc(src.GetColor());
  }
  else if ( src.IsPattern() ){
    return patternFunc(src.GetPattern());
  }
  else {
    assert( src.IsGradient() );
    return gradientFunc(src.GetGradient());
  }
}

#endif

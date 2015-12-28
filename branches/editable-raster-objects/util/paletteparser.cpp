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

#include <sstream>
#include <string>
#include "util/color.hh"
#include "util/drawsource.hh"
#include "util/paletteparser.hh"

namespace faint{

PaletteFileError::PaletteFileError( int line )
  : m_line(line)
{}

int PaletteFileError::GetLineNum() const{
  return m_line;
}

DrawSourceMap parse_palette_stream( std::istream& source){
  DrawSourceMap drawSourceMap;
  std::string buffer;
  int line = 0;
  while ( getline( source, buffer ) ) {
    line += 1;
    std::stringstream ss( buffer );
    int r, g, b;
    int a = 255;
    char comma;
    ss >> r;
    ss >> comma;
    ss >> g;
    ss >> comma;
    ss >> b;
    ss >> comma;
    if ( ss.good() ){
      ss >> a;
    }
    if ( !faint::valid_color( r, g, b, a ) ){
      throw PaletteFileError(line);
    }
    DrawSource src(color_from_ints(r,g,b,a));
    drawSourceMap.Append(src);
  }
  return drawSourceMap;
}

} // namespace faint

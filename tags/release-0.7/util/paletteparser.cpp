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

#include "paletteparser.hh"
#include <string>
#include <sstream>
std::vector<faint::Color> parse( std::istream& source){
  using faint::uchar;
  std::vector<faint::Color> colors;
  std::string buffer;

  while ( getline( source, buffer ) ) {
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
    
    if ( faint::valid_color( r, g, b, a ) ){
      colors.push_back( faint::Color( uchar(r), uchar(g), uchar(b), uchar(a) ) );
    }
  }
  return colors;
}

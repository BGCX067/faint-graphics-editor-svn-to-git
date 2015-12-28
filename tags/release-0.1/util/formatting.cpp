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

#include "formatting.hh"
#include <sstream>
#include <iomanip>
#include "angle.hh"

std::string Bracketed( const std::string& s ){
  return "(" + s + ")";
}

std::string StrPoint( const Point& p ){
  return StrPoint( truncated(p) );
}

std::string StrPoint( const IntPoint& p ){
  std::stringstream ss;
  ss << p.x << "," << p.y;
  return ss.str();
}

std::string StrScale( faint::coord scale_x, faint::coord scale_y ){
  std::stringstream ss;
  ss << int(scale_x*100) << "%, " << int(scale_y*100) << "%";
  return ss.str();
}

std::string StrRGB( const faint::Color& c ){
  std::stringstream ss;
  ss << (int)c.r << "," << (int)c.g << "," << (int)c.b;
  return ss.str();
}

std::string StrRGBA( const faint::Color& c ){
  std::stringstream ss;
  ss << (int)c.r << "," << (int)c.g << "," << (int)c.b << "," << (int)c.a;
  return ss.str();
}

std::string StrSmartRGBA(const faint::Color& col, bool prefix ){        
  return col.a == 255 ?
    "RGB: " + StrSmartRGBA( col ) :
    "RGBA: " + StrSmartRGBA( col );
}

std::string StrSmartRGBA(const faint::Color& col ){
  return col.a == 255 ? StrRGB( col ) : StrRGBA( col );
}

std::string StrHex( const faint::Color& col){
  std::stringstream ss;
  ss << "#";
  ss.fill('0');
  ss << std::uppercase << std::hex <<
    std::setw(2) << (int)col.r <<
    std::setw(2) << (int)col.g <<
    std::setw(2) << (int)col.b;
  return ss.str();
}

std::string StrLength( faint::coord len ){
  std::stringstream ss;
  ss << len;
  return ss.str();
}

std::string StrLengthInt( int len ){
  std::stringstream ss;
  ss << len;
  return ss.str();
}

std::string StrCenterRadius( const Point& c, faint::coord rx, faint::coord ry ){
  std::stringstream ss;
  ss << "c: " << c.x << "," << c.y << " r: " << rx << ", " << ry;
  return ss.str();
}

std::string StrCenterRadius( const Point& c, double r ){
  std::stringstream ss;
  ss << "c: " << c.x << "," << c.y << " r: " << r;
  return ss.str();
}

std::string StrFromTo( const IntPoint& p1, const IntPoint& p2 ){
  std::stringstream ss;
  ss << p1.x << "," << p1.y << "->" << p2.x << "," << p2.y;
  return ss.str();
}

std::string StrFromTo( const Point& p1, const Point& p2 ){
  return StrFromTo(truncated(p1), truncated(p2));
}

std::string StrAngle( double angle ){
  std::stringstream ss;
  ss << std::fixed << std::setprecision(1) << angle << "o";
  return ss.str();
}

std::string StrAngle( int angle ){
  std::stringstream ss;
  if ( angle == 360 ){
    angle = 0;
  }
  ss << angle << "o";
  return ss.str();
}

std::string Percentage( int numerator, int denominator ){
  int scale( static_cast<int>( ( 100 * numerator / (double) denominator ) + 0.5 ) );
  std::stringstream ss;
  ss << scale << "%";
  return ss.str();
}

std::string StrLineStatus( const Point& p1, const Point& p2 ){
  faint::coord lineRadius( radius( p1.x, p1.y, p2.x, p2.y )
    + 1 ); // p1 == p2 should mean length 1 Fixme: Doubtful - in raster, the distance in pixels yes..
  int lineAngle = static_cast<int>( angle360(p1.x, p1.y, p2.x, p2.y) + 0.5 );
  return StrPoint(p1) + "-" + StrPoint(p2) + " length: " + StrLengthInt(static_cast<int>(lineRadius)) + "angle: " + StrAngle(lineAngle);
}

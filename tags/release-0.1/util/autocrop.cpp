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

#include "geo/geotypes.hh"
#include "bitmap/bitmap.h"
#include "autocrop.hh"

IntRect DoGetAutoCropRect( const faint::Bitmap& bmp, const faint::Color& bgCol ){  
  const unsigned int width = bmp.m_w;
  const unsigned int height = bmp.m_h;
  int y0 = 0;
  int y1 = 0;
  bool upDone = false;
  bool dnDone = false;

  for ( unsigned int y = 0; y != height; y++ ){
    for ( unsigned int x = 0; x!= width; x++ ){
      if ( !upDone ){
        if ( GetColor( bmp, x, y ) != bgCol ){
          y0 = y;
          upDone = true;
          if ( dnDone ){
            goto yDone;
          }
        }
      }
      if ( !dnDone ){
        if ( GetColor( bmp, x, height - y - 1 ) != bgCol ){
          y1 = height - y;
          dnDone = true;
          if ( upDone ){
            goto yDone;
          }
        }
      }
    }
  }

  // Post-y computation-label
 yDone:
  unsigned int x0 = 0;
  unsigned int x1 = 0;
  bool leftDone = false;
  bool rightDone = false;

  for ( unsigned int x = 0; x!= width; x++ ){
    for ( unsigned int y = 0; y!= height; y++ ){
      if ( !leftDone ){
        if ( GetColor( bmp, x, y ) != bgCol ){
          x0 = x;
          leftDone = true;
          if ( rightDone ){
            goto xDone;
          }
        }
      }
      if ( !rightDone ){
        if ( GetColor( bmp, width - x - 1, y ) != bgCol ){
          x1 = width - x;
          rightDone = true;
          if ( leftDone ){
            goto xDone;
          }
        }
      }
    }
  }

  // Done
 xDone:
  return IntRect(IntPoint(x0, y0), IntSize(x1 - x0, y1 - y0));
}

bool DecideColor( const faint::Bitmap& bmp, faint::Color& result ){
  const faint::Color leftColor = GetColor( bmp, 0, 0 );
  const faint::Color upColor = leftColor;
  const unsigned int width = bmp.m_w;
  const unsigned int height = bmp.m_h;

  for ( unsigned int x = 0; x < width; x++ ) {
    if ( GetColor( bmp, x, 0 ) != upColor ){
      goto not_top_row;
    }
  }
  result = upColor;
  return true;
 not_top_row:
  for ( unsigned int y = 0; y != height; y++ ){
    if ( GetColor( bmp, 0, y ) != leftColor ){
      goto not_left;
    }
  }
  result = leftColor;
  return true;
 not_left:

  const faint::Color rightColor = GetColor( bmp, bmp.m_w - 1, bmp.m_h - 1);
  const faint::Color bottomColor = rightColor;

  // and then to top right
  for ( unsigned int y = 0; y < height; y++ ){
    if ( GetColor( bmp, width - 1, height - y - 1 ) != rightColor ) {
      goto not_right;
    }
  }
  result = rightColor;
  return true;

 not_right:
  for ( unsigned int x = 0; x != width; x++ ){
    if ( GetColor( bmp, width - 1 - x, height - 1 ) != bottomColor ){
      goto not_bottom;
    }
  }
  result = bottomColor;
  return true;
 not_bottom:
  return false;
}

bool GetAutoCropRect( const faint::Bitmap& bmp, IntRect& result ){
  // First determine background color. Background color must have
  // atleast one complete same-colored edge.
  faint::Color bgColor(0,0,0,0);
  bool gotColor = DecideColor( bmp, bgColor );
  if ( !gotColor ){
    return false;
  }
  result = DoGetAutoCropRect( bmp, bgColor );
  return !Empty( result );
}

bool GetEdgeColor( const faint::Bitmap& bmp, const IntRect& r, faint::Color& result ){
  if ( r.x < 0 || r.x + r.w > static_cast<int>(bmp.m_w) || r.y < 0 || r.y + r.h > static_cast<int>(bmp.m_h) ){
    return false;
  }

  result = GetColor( bmp, r.x, r.y );
  for ( int x = r.x; x != r.x + r.w; x++ ){
    if ( GetColor( bmp, x, r.y ) != result ||
      GetColor( bmp, x, r.y + r.h - 1 ) != result ) {
      return false;
    }
  }
  for ( int y = r.y; y != r.y + r.h; y++ ){
    if ( GetColor( bmp, r.x + r.w - 1, y ) != result ||
      GetColor( bmp, r.x, y ) != result ){
      return false;
    }
  }
  return true;
}

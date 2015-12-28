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

#include "settings.hh"
#include "settingid.hh"
#include <cassert>

const BoolSetting ts_AlphaBlending;
const BoolSetting ts_AntiAlias;
const ColorSetting ts_BgCol;
const IntSetting ts_BrushShape;
const IntSetting ts_BrushSize;
const IntSetting ts_FillStyle;
const ColorSetting ts_FgCol;
const BoolSetting ts_FontBold;
const StrSetting ts_FontFace;
const BoolSetting ts_FontItalic;
const IntSetting ts_FontSize;
const IntSetting ts_LayerStyle;
const IntSetting ts_LineArrowHead;
const IntSetting ts_LineCap;
const IntSetting ts_LineJoin;
const IntSetting ts_LineStyle;
const FloatSetting ts_LineWidth;
const BoolSetting ts_PolyLine;
const BoolSetting ts_SwapColors;
const BoolSetting ts_TextAutoRect;
const IntSetting ts_BackgroundStyle;
const BoolSetting ts_EditPoints;

// Fixme: Duplicates generated names etc. (set_and_get.py).
// this should be generated too.
std::string setting_name( const UntypedSetting& s ){
  if ( s == ts_AlphaBlending ){
    return "alphablending";
  }
  else if ( s == ts_AntiAlias ){
    return "antialias";
  }
  else if ( s == ts_BgCol ){
    return "bgcol";
  }
  else if ( s == ts_BrushShape ){
    return "brushshape";
  }
  else if ( s == ts_BrushSize ){
    return "brushsize";
  }
  else if ( s == ts_EditPoints ){
    return "editpoints";
  }

  else if ( s == ts_FillStyle ){
    return "fillstyle";
  }
  else if ( s == ts_FgCol ){
    return "fgcol";
  }
  else if ( s == ts_FontBold ){
    return "bold";
  }
  else if ( s == ts_FontFace ){
    return "fontface";
  }
  else if ( s == ts_FontItalic ){
    return "italic";
  }
  else if ( s == ts_LayerStyle ){
    return "layerstyle";
  }
  else if ( s == ts_LineArrowHead ){
    return "arrow";
  }
  else if ( s == ts_LineCap ){
    return "linecap";
  }
  else if ( s == ts_LineJoin ) {
    return "join";
  }
  else if ( s == ts_LineStyle ){
    return "linestyle";
  }
  else if ( s == ts_LineWidth ){
    return "linewidth";
  }
  else if ( s == ts_PolyLine ){
    return "polyline";
  }
  else if ( s == ts_SwapColors ){
    return "swapcolors";
  }
  else if ( s == ts_TextAutoRect ){
    return "textautorect";
  }
  else if ( s == ts_BackgroundStyle ){
    return "bgstyle";
  }
  return "undefined_setting_name";
}

LineCap::type to_cap(int v){
  if ( v == LineCap::ROUND ){
    return LineCap::ROUND;
  }
  else if ( v == LineCap::BUTT ){
    return LineCap::BUTT;
  }
  assert( false );
  return LineCap::DEFAULT;
}

LineJoin::type to_join(int v){
  if ( v == LineJoin::BEVEL ){
    return LineJoin::BEVEL;
  }
  else if ( v == LineJoin::MITER ){
    return LineJoin::MITER;
  }
  else if ( v == LineJoin::ROUND ){
    return LineJoin::ROUND;
  }
  assert( false );
  return LineJoin::DEFAULT;
}

Layer::type to_layer( int layer ){
  assert( valid_layer(layer) );
  return static_cast<Layer::type>(layer);
}

bool valid_layer( int layer ){
  return Layer::MIN <= layer && layer <= Layer::MAX;
}

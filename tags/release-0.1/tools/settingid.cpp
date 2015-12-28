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
#include <cassert>;

const FloatSetting ts_LineWidth;
const IntSetting ts_LineStyle;
const IntSetting ts_LineCap;
const IntSetting ts_LineJoin;
const IntSetting ts_LineArrowHead;
const ColorSetting ts_FgCol;
const ColorSetting ts_BgCol;
const BoolSetting ts_SwapColors;
const IntSetting ts_FillStyle;
const IntSetting ts_BrushSize;
const IntSetting ts_FontSize;
const StrSetting ts_FontFace;
const BoolSetting ts_FontBold;
const BoolSetting ts_FontItalic;
const IntSetting ts_Transparency;
const IntSetting ts_LayerStyle;
const IntSetting ts_BrushShape;
const BoolSetting ts_TextAutoRect;
const BoolSetting ts_AntiAlias;

std::string SettingName( const UntypedSetting& s ){
  if ( s == ts_LineCap ){
    return "linecap";
  }
  if ( s == ts_LineWidth ){
    return "linewidth";
  }
  if ( s == ts_LineStyle ){
    return "linestyle";
  }
  if ( s == ts_LineArrowHead ){
    return "arrow";
  }
  if ( s == ts_Transparency ){
    return "transparency";
  }
  if ( s == ts_LayerStyle ){
    return "layerstyle";
  }
  if ( s == ts_BrushShape ){
    return "brushshape";
  }
  if ( s == ts_SwapColors ){
    return "swapcolors";
  }
  if ( s == ts_FgCol ){
    return "fgcol";
  }
  if ( s == ts_BgCol ){
    return "bgcol";
  }  
  if ( s == ts_TextAutoRect ){
    return "textautorect";
  }
  if ( s == ts_FontBold ){
    return "bold";
  }
  if ( s == ts_FontItalic ){
    return "italic";
  }
  if ( s == ts_AntiAlias ){
    return "antialias";
  }
  return "undefined_setting_name";
}

Layer ToLayer( int layer ){
  assert( ValidLayer(layer) );
  return static_cast<Layer>(layer);
}

bool ValidLayer( int layer ){
  return LAYER_MIN <= layer && layer <= LAYER_MAX;  
}
  

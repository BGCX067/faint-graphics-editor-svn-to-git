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

#ifndef FAINT_SETTINGID_HH
#define FAINT_SETTINGID_HH
#include "settings.hh"
extern const FloatSetting ts_LineWidth;
extern const IntSetting ts_LineStyle;
extern const IntSetting ts_LineCap;
namespace faint {
  enum lineCaps{
    CAP_ROUND=0,
    CAP_BUTT=1
  };
}

extern const IntSetting ts_LineJoin;
namespace faint {
  enum lineJoins{
    JOIN_BEVEL=0,
    JOIN_MITER=1,
    JOIN_ROUND=2
  };
}

extern const IntSetting ts_LineArrowHead;
namespace faint{
  enum arrowhead{
    ARROW_NONE = 0,
    ARROW_FRONT,
    ARROW_BACK,
    ARROW_BOTH     
  };
}

extern const ColorSetting ts_FgCol;
extern const ColorSetting ts_BgCol;
extern const BoolSetting ts_SwapColors;

extern const IntSetting ts_FillStyle;
enum fillStyles{ 
  BORDER=20, 
  BORDER_AND_FILL=21, 
  FILL=22 
};

extern const IntSetting ts_LineStyle;
namespace faint {
  enum lineStyles{
    SOLID=0,
    LONG_DASH=1
  };
}

extern const IntSetting ts_BrushSize;
extern const IntSetting ts_FontSize;
extern const StrSetting ts_FontFace;
extern const BoolSetting ts_FontBold;
extern const BoolSetting ts_FontItalic;

extern const IntSetting ts_Transparency;
enum transparencies{
  TRANSPARENT_BG=23, 
  OPAQUE_BG=24 
};

extern const IntSetting ts_LayerStyle;
enum Layer{   
  LAYER_RASTER = 0,
  LAYER_MIN = LAYER_RASTER,
  LAYER_OBJECT = 1,
  LAYER_MAX = LAYER_OBJECT
};

Layer ToLayer( int );
bool ValidLayer( int );

extern const IntSetting ts_BrushShape;
enum brush_shapes{
  BRUSH_SQUARE,
  BRUSH_CIRCLE
};

extern const BoolSetting ts_TextAutoRect;
extern const BoolSetting ts_AntiAlias;

std::string SettingName( const UntypedSetting& );

#endif

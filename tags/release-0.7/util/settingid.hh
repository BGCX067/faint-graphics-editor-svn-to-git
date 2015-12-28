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

struct LineCap{
  enum type{
    ROUND=0,
    BUTT=1,
    DEFAULT=BUTT
  };
};

LineCap::type to_cap(int);

extern const IntSetting ts_LineJoin;
struct LineJoin{
  enum type{
    BEVEL=0,
    MITER=1,
    ROUND=2,
    DEFAULT=MITER
  };
};

LineJoin::type to_join(int);

extern const IntSetting ts_LineArrowHead;
struct LineArrowHead{
  enum type{
    NONE = 0,
    FRONT,
    BACK,
    BOTH
  };
};

extern const ColorSetting ts_FgCol;
extern const ColorSetting ts_BgCol;
extern const BoolSetting ts_PolyLine;
extern const BoolSetting ts_SwapColors;

extern const IntSetting ts_FillStyle;
struct FillStyle{
  enum type{
    BORDER=20,
    BORDER_AND_FILL=21,
    FILL=22
  };
};

extern const IntSetting ts_LineStyle;
struct LineStyle{
  enum type{
    SOLID=0,
    LONG_DASH=1,
    DEFAULT=SOLID
  };
};

extern const IntSetting ts_BrushSize;
extern const IntSetting ts_FontSize;
extern const StrSetting ts_FontFace;
extern const BoolSetting ts_FontBold;
extern const BoolSetting ts_FontItalic;
extern const IntSetting ts_BackgroundStyle;

struct BackgroundStyle{
  enum type{
    MASKED=23,
    SOLID=24 // I wanted to name this "OPAQUE" but it's apparently #defined by some jerk
  };
};

extern const IntSetting ts_LayerStyle;
struct Layer{
  enum type{
    RASTER = 0,
    MIN = RASTER,
    OBJECT = 1,
    MAX = OBJECT
  };
};

Layer::type to_layer( int );
bool valid_layer( int );

extern const IntSetting ts_BrushShape;
struct BrushShape{
  enum type{
    SQUARE,
    CIRCLE
  };
};

extern const BoolSetting ts_TextAutoRect;
extern const BoolSetting ts_AntiAlias;
extern const BoolSetting ts_AlphaBlending;
extern const BoolSetting ts_EditPoints;

std::string setting_name( const UntypedSetting& );

#endif

// -*- coding: us-ascii-unix -*-
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

#ifndef FAINT_COMMON_FWD_HH
#define FAINT_COMMON_FWD_HH
#include <string>
#include <vector>
#include "geo/geo-fwd.hh"
#include "geo/primitive.hh"
#include "util/distinct.hh"
#include "util/template-fwd.hh"

namespace faint{
  class AlphaMap;
  class AlphaMapRef;
  class AppContext;
  class Bitmap;
  class Brush;
  class Canvas;
  class ColRGB;
  class Color;
  class ColorStop;
  class Command;
  class DirPath;
  class FaintDC;
  class FileList;
  class FilePath;
  class Format;
  class Gradient;
  class Grid;
  class Image;
  class ImageInfo;
  class ImageList;
  class ImageProps;
  class KeyPress;
  class LinearGradient;
  class line_t;
  class Mod;
  class Object;
  class Overlays;
  class Paint;
  class PaintMap;
  class Pattern;
  class PosInfo;
  class RadialGradient;
  class RasterSelection;
  class Rotation;
  class SelectionOptions;
  class SelectionState;
  class Settings;
  class Task;
  class Tool;
  class ToolModifiers;
  class ZoomLevel;
  class index_t;
  class utf8_char;
  class utf8_string;
  enum class Axis{HORIZONTAL, VERTICAL};
  enum class Cursor;
  enum class Layer;
  enum class MouseButton;
  enum class ScaleQuality{NEAREST, BILINEAR};
  template<typename T1, typename T2> class Either;
  template<typename T> class Copyable;
  typedef std::vector<Command*> commands_t;
  typedef std::vector<Object*> objects_t;
  typedef std::vector<Tri> tris_t;
  typedef Distinct<int, ImageProps, 0> delay_t;
  typedef std::vector<std::string> strings_t;
}

namespace faint{ namespace sel{
  class Existing;
}}

#endif

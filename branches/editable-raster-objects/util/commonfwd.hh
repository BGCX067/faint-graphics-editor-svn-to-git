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
#ifndef FAINT_COMMONFWD_HH
#define FAINT_COMMONFWD_HH
#include <string>
#include <vector>
#include "util/unique.hh"

namespace faint{
  class Bitmap;
  class Color;
  class ColorStop;
  class ColRGB;
  class DrawSource;
  class DrawSourceMap;
  class Gradient;
  class Image;
  class ImageList;
  class LinearGradient;
  class Pattern;
  class RadialGradient;
  class utf8_char;
  class utf8_string;
  typedef double coord;
  typedef double degree;
  typedef double radian;
  typedef unsigned int uint;
  typedef unsigned char uchar;
  extern const coord coord_epsilon;
}
class AngleSpan;
class CanvasInterface;
class CursorPositionInfo;
class Command;
class FaintDC;
class Grid;
class ImageInfo;
class ImageProps;
class IntPoint;
class IntRect;
class IntSize;
class Line;
class Object;
class Overlays;
class PathPt;
class Point;
class Radii;
class RasterSelection;
class RasterSelectionOptions;
class RasterSelectionState;
class Rect;
class Scale;
class Settings;
class Size;
class Task;
class Tool;
class Tri;
class ZoomLevel;
typedef std::vector<Command*> commands_t;
typedef Unique<int, ImageProps, 0> delay_t;
typedef std::vector<Object*> objects_t;
typedef std::vector<std::string> strings_t;
typedef std::vector<Tri> tris_t;

enum class Axis{ HORIZONTAL, VERTICAL };
enum class ScaleQuality{ NEAREST, BILINEAR };

#endif

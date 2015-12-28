// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#ifndef FAINT_QUANTIZE_HH
#define FAINT_QUANTIZE_HH
#include <vector>
#include "util/color.hh"

namespace faint{

class AlphaMap;
class Bitmap;

class ColorMap{
  // Maps indexes to colors.
public:
  ColorMap();
  void AddColor(const Color&);
  Color GetColor(int index) const;
  int GetNumColors() const;
private:
  std::vector<Color> m_colors;
};

std::pair<AlphaMap, ColorMap> quantized(const Bitmap&);
void quantize(Bitmap&);

}

#endif

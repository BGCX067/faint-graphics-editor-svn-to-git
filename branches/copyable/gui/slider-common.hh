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

#ifndef FAINT_SLIDER_COMMON_HH
#define FAINT_SLIDER_COMMON_HH
#include "bitmap/bitmap-fwd.hh"
#include "geo/range.hh"

namespace faint{

enum class SliderDir{ HORIZONTAL, VERTICAL };

class SliderBackground{
public:
  virtual ~SliderBackground(){}
  virtual void Draw( faint::Bitmap&, const IntSize& ) = 0;
  virtual SliderBackground* Clone() const = 0;
};

class SliderRectangleBackground : public SliderBackground{
public:
  void Draw( faint::Bitmap&, const IntSize& ) override;
  SliderBackground* Clone() const override;
};

class SliderMidPointBackground : public SliderBackground{
public:
  void Draw( faint::Bitmap&, const IntSize& ) override;
  SliderBackground* Clone() const override;
};

double pos_to_value( const int pos, const int length, const ClosedIntRange& );
int value_to_pos( const double value, const int length, const ClosedIntRange& );

} // namespace

#endif

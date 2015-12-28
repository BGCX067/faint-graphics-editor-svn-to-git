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

#ifndef FAINT_GRADIENT_PANEL_HH
#define FAINT_GRADIENT_PANEL_HH
#include "util/gradient.hh"
#include "util/statusinterface.hh"

class ColorPanel_Gradient_Impl;
class wxWindow;

class ColorPanel_Gradient{
public:
  ColorPanel_Gradient( wxWindow* parent, StatusInterface& );
  ~ColorPanel_Gradient();
  wxWindow* AsWindow();
  faint::Gradient GetGradient() const;
  void SetGradient( const faint::Gradient& );
private:
  ColorPanel_Gradient( const ColorPanel_Gradient& );
  ColorPanel_Gradient& operator=( const ColorPanel_Gradient& );
  ColorPanel_Gradient_Impl* m_impl;
};

#endif

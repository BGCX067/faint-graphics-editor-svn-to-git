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

#ifndef FAINT_HSL_PANEL_HH
#define FAINT_HSL_PANEL_HH
#include "util/commonfwd.hh"

class ColorPanel_HSL_Impl;
class wxWindow;

class ColorPanel_HSL{
public:
  ColorPanel_HSL( wxWindow* parent );
  ~ColorPanel_HSL();
  wxWindow* AsWindow();
  faint::Color GetColor() const;
  void SetColor( const faint::Color& );
private:
  ColorPanel_HSL(const ColorPanel_HSL&);
  ColorPanel_HSL& operator=(const ColorPanel_HSL&);
  ColorPanel_HSL_Impl* m_impl;
};

#endif

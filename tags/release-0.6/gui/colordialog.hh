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

#ifndef FAINT_COLORDIALOG_HH
#define FAINT_COLORDIALOG_HH
#include "geo/color.hh"

class wxWindow;
class ColorDialogImpl;

class ColorDialogCallback {
public:
  virtual ~ColorDialogCallback();
  virtual void OnColor(const faint::Color&);
};

class ColorDialog{
public:
  ColorDialog( wxWindow* parent );
  ColorDialog( wxWindow* parent, ColorDialogCallback& );
  ~ColorDialog();
  faint::Color GetColor() const;
  bool ShowModal( const faint::Color& );
private:
  // Use an impl-method instead of inheriting from wx-classes
  // to minimize the interface
  ColorDialogImpl* m_impl;
  ColorDialogCallback m_defaultCallback;
};

#endif

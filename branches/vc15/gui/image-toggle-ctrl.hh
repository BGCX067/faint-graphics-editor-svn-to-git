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

#ifndef FAINT_IMAGE_TOGGLE_CTRL_HH
#define FAINT_IMAGE_TOGGLE_CTRL_HH
#include "wx/bitmap.h"
#include "geo/axis.hh"
#include "geo/int-size.hh"
#include "gui/gui-string-types.hh"
#include "gui/tool-setting-ctrl.hh"

namespace faint{

class StatusInterface;

class ToggleImage{
  // A choice for an image toggle control
public:
  ToggleImage(const wxBitmap& in_bmp,
    int in_value,
    const utf8_string& in_statusText)
    : bmp(in_bmp),
      value(in_value),
      statusText(in_statusText)
  {}
  wxBitmap bmp;
  int value;
  utf8_string statusText;
};

// Fixme: Take initializer list of images instead
IntSettingCtrl* new_image_toggle(wxWindow* parent,
  const IntSetting&,
  const IntSize& imageSize,
  StatusInterface&,
  const tooltip_t&,
  const ToggleImage&,
  const ToggleImage&,
  Axis dir=Axis::VERTICAL,
  IntSize spacing={10, 5});

IntSettingCtrl* new_image_toggle(wxWindow* parent,
  const IntSetting&,
  const IntSize& imageSize,
  StatusInterface&,
  const tooltip_t&,
  const ToggleImage&,
  const ToggleImage&,
  const ToggleImage&,
  Axis dir=Axis::VERTICAL,
  IntSize spacing={10,5});

BoolSettingControl* new_bool_image_toggle(wxWindow* parent,
  const BoolSetting&,
  const wxBitmap&,
  StatusInterface&,
  const tooltip_t&);

} // namespace

#endif

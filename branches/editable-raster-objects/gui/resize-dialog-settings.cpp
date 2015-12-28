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

#include "gui/resize-dialog-settings.hh"

ResizeDialogSettings::ResizeDialogSettings()
  : defaultButton(RESIZE_TOP_LEFT),
    proportional(false),
    scaleOnly(false),
    size(0,0)
{}

ResizeDialogSettings::ResizeDialogSettings( const IntSize& in_size, bool in_proportional, bool in_scaleOnly, ResizeType in_defaultButton )
  : defaultButton(in_defaultButton),
    proportional(in_proportional),
    scaleOnly(in_scaleOnly),
    size(in_size)
{}

ResizeDialogSettings modified( const ResizeDialogSettings& original, const IntSize& size, bool scaleOnly ){
  ResizeDialogSettings settings(original);
  settings.size = size;
  settings.scaleOnly = scaleOnly;
  return settings;
}

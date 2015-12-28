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

#include "gui/resize-dialog-options.hh"

namespace faint{

ResizeDialogOptions::ResizeDialogOptions()
  : bilinearOnly(false),
    defaultButton(RESIZE_TOP_LEFT),
    nearest(false),
    proportional(false),
    scaleOnly(false)
{}

ResizeDialogOptions::ResizeDialogOptions(bool in_nearest, bool in_proportional, bool in_scaleOnly, bool in_bilinearOnly, ResizeType in_defaultButton)
  : bilinearOnly(in_bilinearOnly),
    defaultButton(in_defaultButton),
    nearest(in_nearest),
    proportional(in_proportional),
    scaleOnly(in_scaleOnly)
{}

ResizeDialogOptions modified(const ResizeDialogOptions& original,
  bool scaleOnly, bool
  bilinearOnly)
{
  ResizeDialogOptions settings(original);
  settings.scaleOnly = scaleOnly;
  settings.bilinearOnly = bilinearOnly;
  return settings;
}

} // namespace

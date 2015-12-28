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

#include "app/get-app-context.hh"
#include "bitmap/bitmap.hh"
#include "bitmap/draw.hh"
#include "bitmap/filter.hh"
#include "python/py-function-error.hh"
#include "text/formatting.hh"
#include "util/clipboard.hh"

namespace faint{

  void do_copy_rect(const Optional<Bitmap>& srcBmp, const IntRect& rect){
  if (srcBmp.NotSet()){
    throw ValueError("Item has no bitmap"); // Fixme
  }
  if (!inside(rect, srcBmp.Get())){
    throw ValueError("Rectangle not fully inside image.");
  }

  Bitmap bmp(subbitmap(srcBmp.Get(), rect)); // Fixme: Handle OOM
  Clipboard clipboard;
  if (!clipboard.Good()){
    throw ValueError("Failed opening clipboard");
  }

  // If the bg-color is a color, use it as the background for
  // blending alpha when pasting outside Faint.
  Paint paint(get_app_context().GetToolSettings().Get(ts_Bg));
  if (paint.IsColor()){
    clipboard.SetBitmap(bmp, strip_alpha(paint.GetColor()));
  }
  else{
    clipboard.SetBitmap(bmp);
  }
}

}

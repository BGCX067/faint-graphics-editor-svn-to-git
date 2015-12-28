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

#ifndef FAINT_CLIPBOARD
#define FAINT_CLIPBOARD
#include "wx/clipbrd.h"
#include "bitmap/bitmap.h"
#include <vector>
#include <string>
class Object;
class wxClipboard;

namespace faint {

  // When the specified data is available in the opened clipboard,
  // these functions put it in the out-parameter and return true.
  //
  // The clipboard will be closed if the function is succesful
  // (i.e. on true return value), otherwise it is left open.
  bool GetBitmap( wxClipboard*, faint::Bitmap& out );
  bool GetObjects( wxClipboard*, std::vector<Object*>& out );
  bool GetText( wxClipboard*, std::string& out );

  // These functions set the passed in data to the opened clipboard
  // and then close it.
  void SetBitmap( wxClipboard*, const faint::Bitmap& out, const faint::Color& bgCol );
  void SetText( wxClipboard* clipboard, const std::string& );
  void SetObjects( wxClipboard*, const std::vector<Object*>& );
}

#endif

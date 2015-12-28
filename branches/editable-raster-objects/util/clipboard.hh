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
#include <vector>
#include "util/commonfwd.hh"

namespace faint {
  class Clipboard{
  public:
    Clipboard();
    ~Clipboard();
    static void Flush();
    bool GetBitmap( faint::Bitmap& out );
    bool GetObjects( objects_t& out );
    bool GetText( faint::utf8_string& out );
    bool Good() const;

    // Puts the bitmap in the clipboard.
    // When pasted outside Faint, pixels with alpha will be blended
    // onto bgCol.
    void SetBitmap( const faint::Bitmap&, const faint::ColRGB& bgCol );
    void SetBitmap( const faint::Bitmap& );
    void SetObjects( const objects_t& );
    void SetText( const faint::utf8_string& );
  private:
    bool m_ok;
  };
}
#endif

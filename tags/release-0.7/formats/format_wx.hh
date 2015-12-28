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

#ifndef FAINT_FORMATWX_HH
#define FAINT_FORMATWX_HH
#include "wx/wx.h"
#include "formats/format.hh"

namespace faint{
  void load_file_wx( const std::string& filename, wxBitmapType, ImageProps& );
}

class FormatWX : public Format {
public:
  FormatWX( const extension_t&, const label_t&, wxBitmapType );
  FormatWX( const std::vector<extension_t>&, const label_t&, wxBitmapType );
  void Load( const std::string& filename, ImageProps& );
  SaveResult Save( const std::string& filename, CanvasInterface& );
private:
  wxBitmapType m_bmpType;
};

#endif

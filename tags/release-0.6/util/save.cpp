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

#include "app/app.hh"
#include "formats/format.hh"
#include "gui/mainframe.hh"
#include "util/save.hh"
#include "wx/filename.h"

bool Save( CanvasInterface& canvas ){
  // Fixme: Move all saving out of MainFrame
  MainFrame* frame = wxGetApp().GetFrame();
  return frame->Save( canvas );
}

namespace faint {
  bool Save( CanvasInterface& canvas, const std::string& filename ){
    // Fixme: This function duplicates MainFrame::Save with
    // less error handling
    std::vector<Format*>& formats = wxGetApp().GetFileFormats();

    extension_t extension(filename.substr( filename.size() - 3, 3 ));
    bool saved = false;
    for ( unsigned int i = 0; i!= formats.size(); i++ ){
      Format* format = formats[i];
      if ( format->CanSave() && format->Match(extension) ){
	SaveResult result = format->Save( filename, canvas );
	saved = result.Successful();
        break;
      }
    }
    return saved;
  }

  std::string GetFilename( const std::string& path ){
    wxString name;
    wxString ext;
    wxFileName::SplitPath( path, 0, 0, &name, &ext );
    return std::string(name + "." + ext);
  }
}

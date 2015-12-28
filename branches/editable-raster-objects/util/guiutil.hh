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

#ifndef FAINT_GUIUTIL_HH
#define FAINT_GUIUTIL_HH
#include <string>
#include "formats/format.hh" // For extension_t
#include "gui/command-dialog.hh"
#include "util/unique.hh"
#include "generated/gen-resource-id.hh" // Fixme: Include a forwarding file
class ArtContainer;
class wxBitmap;
class wxColour;
class wxCursor;
class wxDialog;
class wxFileName;
class wxFrame;
class wxIcon;
class wxIconBundle;
class wxTextCtrl;
class wxWindow;

class gui_string_class{};
typedef Unique<std::string, gui_string_class, 0> Title;
typedef Unique<std::string, gui_string_class, 1> storage_name;
typedef Unique<std::string, gui_string_class, 3> tooltip_t;
typedef Unique<std::string, gui_string_class, 4> description_t;

namespace faint{
  wxString bracketed( const wxString& );
  void console_message( const wxString& );
  wxCursor cur_from_bmp( const wxBitmap&, const IntPoint& hotspot );
  // Retrieve the stored position and size etc. for the frame with the
  // specified name
  void restore_persisted_state(wxFrame*, const storage_name&);

  // All modal dialogs must be shown via this helper to ensure
  // correct application behavior.
  int show_modal( wxDialog& );
  bool show_modal( CommandDialog&, wxWindow*, DialogFeedback& );
  wxColour get_gui_selected_color();
  wxColour get_gui_deselected_color();
  wxIcon get_icon( ArtContainer&, Icon );
  wxIconBundle bundle_icons( const wxIcon&, const wxIcon& );

  int parse_int_value( wxTextCtrl*, int defaultValue );
} // namespace

bool ask_exit_unsaved_changes( wxWindow* parent );
void show_copy_color_error( wxWindow* parent );
void show_error( wxWindow* parent, const Title&, const wxString& message );
void show_file_not_found_error( wxWindow* parent, const faint::FilePath& );
void show_file_not_supported_error( wxWindow* parent, const faint::FilePath& );
void show_load_failed_error( wxWindow* parent, const faint::FilePath&, const std::string& message );
void show_load_warnings( wxWindow* parents, const ImageProps& );
void show_save_extension_error( wxWindow* parent, const extension_t& );
void show_out_of_memory_cancelled_error(wxWindow* parent);

inline wxWindow* null_parent(){
  return nullptr;
}

namespace faint{
  class Accelerators{
    // Helper for step-wise creation of a wxAccelerator-table (dialog
    // keyboard shortcuts)
  public:
    void Add( int flags, int keyCode, int cmd );
    wxAcceleratorTable Get() const;
private:
    std::vector<wxAcceleratorEntry> m_entries;
  };
}

#endif

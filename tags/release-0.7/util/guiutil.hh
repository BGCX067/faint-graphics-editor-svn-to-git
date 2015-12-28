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
#include "util/unique.hh"
class ArtContainer;
class wxBitmap;
class wxColour;
class wxCursor;
class wxDialog;
class wxIcon;
class wxIconBundle;
class wxWindow;
class wxFileName;
class wxFrame;

class gui_string_class{};
typedef Unique<std::string, gui_string_class, 0> Title;
typedef Unique<std::string, gui_string_class, 1> storage_name;
typedef Unique<std::string, gui_string_class, 3> tooltip_t;
typedef Unique<std::string, gui_string_class, 4> description_t;

namespace faint{
  void console_message( const std::string& );
  wxCursor cur_from_bmp( const wxBitmap&, int hotspot_x, int hotspot_y );
  // Retrieve the stored position and size etc. for the frame with the
  // specified name
  void restore_persisted_state(wxFrame*, const storage_name&);

  // All modal dialogs must be shown via this helper to ensure
  // correct application behavior.
  int show_modal( wxDialog& );
  wxColour get_gui_selected_color();
  wxColour get_gui_deselected_color();
  wxIcon get_icon( ArtContainer&, const std::string& label );
  wxIconBundle bundle_icons( const wxIcon&, const wxIcon& );

} // namespace

bool ask_exit_unsaved_changes( wxWindow* parent );
void show_copy_color_error( wxWindow* parent );
void show_error( wxWindow* parent, const Title&, const std::string& message );
void show_file_not_found_error( wxWindow* parent, const wxFileName& );
void show_file_not_supported_error( wxWindow* parent, const wxFileName& );
void show_load_failed_error( wxWindow* parent, const wxFileName&, const std::string& message );
void show_save_extension_error( wxWindow* parent, const extension_t& );
void show_out_of_memory_cancelled_error(wxWindow* parent);
wxWindow* null_parent();

#endif

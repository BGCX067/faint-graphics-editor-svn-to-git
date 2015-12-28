#include <sstream>
#include "wx/wx.h"
#include "wx/filename.h"
#include "wx/persist.h"
#include "wx/persist/toplevel.h"
#include "app/getappcontext.hh"
#include "util/artcontainer.hh"
#include "util/guiutil.hh"

namespace faint{

  void console_message( const std::string& text ){
    // Fixme: wxMessageOutput seems to truncate the next prompt in msw, in some situations
    wxMessageOutput* msgOut = wxMessageOutput::Get();
    assert(msgOut != 0);
    msgOut->Output(wxString(text));
  }

  wxCursor cur_from_bmp( const wxBitmap& bmp, int hotspot_x, int hotspot_y ){
    wxImage img( bmp.ConvertToImage() );
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotspot_x );
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hotspot_y );
    img.SetMaskColour( 255, 0, 255 );
    return wxCursor(img);
  }

  int show_modal( wxDialog& dlg ){
    // Application::FilterEvent catches keypresses even when
    // dialogs are shown. Using this function for all dialogs
    // shown by the frame allows handling it.
    GetAppContext().BeginModalDialog();
    int result = dlg.ShowModal();
    GetAppContext().EndModalDialog();
    return result;
  }

  wxColour get_gui_selected_color(){
    return wxColour(112,154,209);
  }

  wxColour get_gui_deselected_color(){
    return wxColour(255,255,255);
  }

  wxIcon get_icon( ArtContainer& art, const std::string& label ){
    wxIcon icon;
    icon.CopyFromBitmap(art.Get(label));
    return icon;
  }

  wxIconBundle bundle_icons( const wxIcon& icon1, const wxIcon& icon2 ){
    wxIconBundle bundle;
    bundle.AddIcon(icon1);
    bundle.AddIcon(icon2);
    return bundle;
  }

  void restore_persisted_state(wxFrame* frame, const storage_name& name ){
    frame->SetName(name.Get());
    wxPersistenceManager::Get().RegisterAndRestore(frame);
  }
} // namespace

bool ask_exit_unsaved_changes( wxWindow* parent ){
  wxMessageDialog dlg(parent,
    "One or more files have unsaved changes.\nExit anyway?",
    "Unsaved Changes", wxYES_NO | wxNO_DEFAULT );
  const int choice = faint::show_modal(dlg);
  return choice == wxID_YES;
}

void show_copy_color_error( wxWindow* parent ){
  wxMessageDialog dlg(parent,
    "Failed Copying the color.\n\nThe clipboard could not be opened.",
    "Copy Color Failed",
    wxOK|wxICON_ERROR );
  faint::show_modal(dlg);
}

void show_error( wxWindow* parent, const Title& title, const std::string& message ){
  wxMessageDialog dlg(parent,
    message,
    title.Get(),
    wxOK|wxICON_ERROR );
  faint::show_modal(dlg);
}

void show_file_not_found_error( wxWindow* parent, const wxFileName& file ){
  std::stringstream ss;
  ss << "File not found:\n" << file.GetFullPath();
  wxMessageDialog dlg(parent, ss.str(), "File not found", wxOK|wxICON_ERROR);
  faint::show_modal(dlg);
}

void show_file_not_supported_error( wxWindow* parent, const wxFileName& file ){
  std::stringstream ss;
  ss << "File type not supported:\n" << file.GetFullPath();
  wxMessageDialog dlg(parent, ss.str(), "Unsupported File", wxOK|wxICON_ERROR);
  faint::show_modal(dlg);
}

void show_load_failed_error( wxWindow* parent, const wxFileName& file, const std::string& message ){
  std::stringstream ss;
  ss << "Failed loading:" << std::endl << file.GetFullPath() << std::endl << std::endl << message;
  wxMessageDialog dlg(parent, ss.str(), "Failed Loading Image", wxOK|wxICON_ERROR);
  faint::show_modal(dlg);
}

void show_save_extension_error( wxWindow* parent, const extension_t& ext ){
  std::stringstream ss;
  ss << "Failed Saving: Faint doesn't recognize the extension " << ext.Get() << std::endl;
  show_error(parent, Title("Failed Saving"), ss.str());
}

void show_out_of_memory_cancelled_error( wxWindow* parent ){
  show_error(parent, Title("Out of memory"),
    "An action required too much memory and was cancelled.");
}

wxWindow* null_parent(){
  return 0;
}

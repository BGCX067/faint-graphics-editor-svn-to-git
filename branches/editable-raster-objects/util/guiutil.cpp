#include <sstream>
#include "wx/wx.h"
#include "wx/filename.h"
#include "wx/persist.h"
#include "wx/persist/toplevel.h"
#include "app/getappcontext.hh"
#include "util/artcontainer.hh"
#include "util/guiutil.hh"
#include "util/imageprops.hh"

namespace faint{
  wxString bracketed( const wxString& s ){
    return "(" + s + ")";
  }

  void console_message( const wxString& text ){
    wxMessageOutput* msgOut = wxMessageOutput::Get();
    assert(msgOut != nullptr);
    msgOut->Output(text);
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

  bool show_modal( CommandDialog& dlg, wxWindow* parent, DialogFeedback& feedback ){
    GetAppContext().BeginModalDialog();
    bool result = dlg.ShowModal(parent, feedback);
    GetAppContext().EndModalDialog();
    return result;
  }

  wxColour get_gui_selected_color(){
    return wxColour(112,154,209);
  }

  wxColour get_gui_deselected_color(){
    return wxColour(255,255,255);
  }

wxIcon get_icon( ArtContainer& art, Icon iconId ){
  wxIcon icon;
  icon.CopyFromBitmap(art.Get(iconId));
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

  int parse_int_value( wxTextCtrl* textCtrl, int defaultValue ){
    std::string s(textCtrl->GetValue());
    std::stringstream ss(s);
    int i = defaultValue;
    ss >> i;
    return i;
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

void show_error( wxWindow* parent, const Title& title, const wxString& message ){
  wxMessageDialog dlg(parent,
    message,
    title.Get(),
    wxOK|wxICON_ERROR );
  faint::show_modal(dlg);
}

void show_file_not_found_error( wxWindow* parent, const faint::FilePath& file ){
  wxString errStr("File not found:\n");
  errStr += file.ToWx().GetLongPath();
  wxMessageDialog dlg(parent, errStr, "File not found", wxOK|wxICON_ERROR);
  faint::show_modal(dlg);
}

void show_file_not_supported_error( wxWindow* parent, const faint::FilePath& file ){
  wxString errStr("File type not supported:\n");
  errStr += file.ToWx().GetLongPath();
  wxMessageDialog dlg(parent, errStr, "Unsupported File", wxOK|wxICON_ERROR);
  faint::show_modal(dlg);
}

void show_load_failed_error( wxWindow* parent, const faint::FilePath& file, const std::string& message ){
  wxString errStr("Failed loading:\n");
  errStr += file.ToWx().GetLongPath();
  errStr += "\n\n";
  errStr += wxString(message); // Fixme: Message is ascii, filename wide, risky/surprising.
  wxMessageDialog dlg(parent, errStr, "Failed Loading Image", wxOK|wxICON_ERROR);
  faint::show_modal(dlg);
}

void show_load_warnings( wxWindow* parent, const ImageProps& props ){
  std::string warnings;
  for ( size_t i = 0; i != props.GetNumWarnings() && i < 10; i++ ){
    warnings += props.GetWarning(i) + "\n";
  }
  show_error(parent, Title("Warning"),
    warnings);
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

namespace faint{

void Accelerators::Add( int flags, int keyCode, int cmd ){
  m_entries.push_back(wxAcceleratorEntry(flags, keyCode, cmd));
}

wxAcceleratorTable Accelerators::Get() const{
  if ( m_entries.empty() ){
    return wxAcceleratorTable();
  }
  return wxAcceleratorTable(m_entries.size(), m_entries.data());
}

} // namespace faint

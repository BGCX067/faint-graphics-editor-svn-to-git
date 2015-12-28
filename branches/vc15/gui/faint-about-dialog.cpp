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

#include <sstream>
#include "wx/button.h"
#include "wx/dialog.h"
#include "wx/hyperlink.h"
#include "wx/notebook.h"
#include "wx/panel.h"
#include "wx/sizer.h"
#include "wx/statline.h"
#include "wx/stattext.h"
#include "app/build-info.hh"
#include "gui/dialog-context.hh"
#include "gui/faint-about-dialog.hh"
#include "gui/layout.hh"
#include "python/py-interface.hh" // For get_python_version
#include "rendering/cairo-context.hh" // For get_cairo_version
#include "text/formatting.hh"
#include "util-wx/gui-util.hh"
#include "util-wx/convert-wx.hh"
#include "util-wx/file-path-util.hh"
#include "util-wx/key-codes.hh"

namespace faint{

static wxStaticText* title_text(wxWindow* parent){
  wxStaticText* title = new wxStaticText(parent, wxID_ANY, "Faint Graphics Editor");
  title->SetFont(wxFontInfo(24).Bold());
  title->SetForegroundColour(wxColour(47,54,153));
  return title;
}

static wxStaticText* version_text(wxWindow* parent){
  wxStaticText* version = new wxStaticText(parent,
    wxID_ANY,
    "Version " + to_wx(faint_version()));
  version->SetForegroundColour(wxColour(47,54,153));
  version->SetFont(wxFontInfo().Bold());
  return version;
}

static wxStaticText* license_text(wxWindow* parent){
  return new wxStaticText(parent, wxID_ANY, "Copyright 2013 Lukas Kemmer\nLicensed under the Apache License 2.0");
}

static wxStaticLine* hline(wxWindow* parent){
  return new wxStaticLine(parent, wxID_ANY, wxDefaultPosition, wxSize(-1,1), wxLI_HORIZONTAL);
}

static wxHyperlinkCtrl* webpage_url(wxWindow* parent){
  const wxString url("http://code.google.com/p/faint-graphics-editor/");
  return new wxHyperlinkCtrl(parent, wxID_ANY, url, url);
}

static wxStaticText* details_text(wxWindow* parent){
  utf8_string faint_info =
    endline_sep(
      space_sep("Executable path:", get_faint_exe_path().Str()),
      space_sep("SVN path:",
        faint_svn_path() + ":" + faint_svn_revision()),
      space_sep("Build date:", faint_build_date()));

  std::stringstream ss;
  ss << "wxWidgets version: " <<
    wxMAJOR_VERSION << "." <<
    wxMINOR_VERSION << "." <<
    wxRELEASE_NUMBER << "." <<
    wxSUBRELEASE_NUMBER << std::endl;

  ss << "Python version: " << get_python_version() << std::endl;
  ss << "Cairo version: " << get_cairo_version() << std::endl;
  ss << "Pango version: " << get_pango_version() << std::endl;
  ss << std::endl;

  return new wxStaticText(parent, wxID_ANY, to_wx(faint_info) + "\n\n" + ss.str());
}

static wxPanel* about_panel(wxWindow* parent){
  wxPanel* panel = new wxPanel(parent, wxID_ANY);
  wxBoxSizer* rows = new wxBoxSizer(wxVERTICAL);
  rows->AddSpacer(panel_padding);
  rows->Add(title_text(panel),
    0, wxLEFT|wxRIGHT, panel_padding + 10);

  rows->Add(version_text(panel),
    0, wxLEFT|wxRIGHT, panel_padding + 10);

  rows->AddSpacer(item_spacing);
  rows->Add(hline(panel),
    0, wxLEFT|wxRIGHT|wxEXPAND, panel_padding + 10);

  rows->AddSpacer(item_spacing * 2);
  rows->Add(webpage_url(panel),
    0, wxLEFT|wxRIGHT, panel_padding + 10);
  rows->AddSpacer(item_spacing * 2);
  rows->Add(license_text(panel),
    0, wxLEFT|wxRIGHT, panel_padding + 10);
  rows->AddSpacer(item_spacing * 2);

  rows->AddSpacer(panel_padding);

  rows->AddSpacer(panel_padding);
  panel->SetSizerAndFit(rows);
  return panel;
}

static wxPanel* details_panel(wxWindow* parent){
  wxPanel* panel = new wxPanel(parent, wxID_ANY);
  wxBoxSizer* rows = new wxBoxSizer(wxVERTICAL);
  rows->Add(details_text(panel),
    0, wxLEFT|wxUP|wxRIGHT, panel_padding + 10);
  panel->SetSizerAndFit(rows);
  return panel;
}

class FaintAboutDialog : public wxDialog{
public:
  FaintAboutDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "About Faint")
  {
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxNotebook* tabs = new wxNotebook(this, wxID_ANY);
    tabs->AddPage(about_panel(tabs), "&About");
    tabs->AddPage(details_panel(tabs), "&Details");
    sizer->Add(tabs);

    wxButton* btnOk = new wxButton(this, wxID_OK);
    SetDefaultItem(btnOk);
    sizer->Add(btnOk, 0, wxALIGN_RIGHT);
    btnOk->SetFocus();

    auto select_about_tab = [=](){tabs->SetSelection(0);};
    auto select_details_tab = [=](){tabs->SetSelection(1);};

    set_accelerators(this,{
      // A)bout
      {key::A, select_about_tab},
      {Alt+key::A, select_about_tab},
      {key::Q, select_about_tab},

      {key::D, select_details_tab},
      {Alt+key::D, select_details_tab},
      {key::W, select_details_tab},

      {Alt+key::O, [&](){EndModal(wxID_OK);}}
    });

    SetSizerAndFit(sizer);

    // Center on parent
    Centre(wxBOTH);
  }
};

void show_faint_about_dialog(wxWindow* parent, DialogContext& dialogContext){
  FaintAboutDialog dlg(parent);
  dialogContext.ShowModal(dlg);
}

} // namespace

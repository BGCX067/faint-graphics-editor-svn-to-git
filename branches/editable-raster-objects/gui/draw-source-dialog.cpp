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

#include <functional>
#include <memory>
#include "wx/wx.h"
#include "wx/notebook.h"
#include "gui/draw-source-dialog.hh"
#include "gui/draw-source-dialog/gradient-panel.hh"
#include "gui/draw-source-dialog/hsl-panel.hh"
#include "gui/draw-source-dialog/pattern-panel.hh"
#include "util/guiutil.hh"
#include "util/pattern.hh"

static const int ID_COPY = 1;
static const int ID_PASTE = 2;
static const int ID_HSL = 3;
static const int ID_GRADIENT = 4;
static const int ID_PATTERN = 5;

static faint::Gradient gradient_from_color( const faint::Color& c ){
  faint::LinearGradient g;
  g.Add( faint::ColorStop(c, 0.0) );
  g.Add( faint::ColorStop(faint::color_white(), 1.0) );
  return faint::Gradient(g);
}

class DrawSourceDialog : public wxDialog {
public:
  DrawSourceDialog( wxWindow* parent, const wxString& title, StatusInterface& statusInfo )
    : wxDialog( parent, wxID_ANY, title )
  {
    m_tabs = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_NOPAGETHEME);
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_tabs);

    m_panelHSL.reset(new ColorPanel_HSL(m_tabs));
    m_tabs->AddPage(m_panelHSL->AsWindow(), "HSL");

    m_panelGradient.reset(new ColorPanel_Gradient(m_tabs, statusInfo));
    m_tabs->AddPage(m_panelGradient->AsWindow(), "Gradient");

    m_panelPattern.reset(new ColorPanel_Pattern(m_tabs));
    m_tabs->AddPage(m_panelPattern->AsWindow(), "Pattern");

    wxButton* okBtn = new wxButton(this, wxID_OK);
    wxButton* cancelButton = new wxButton( this, wxID_CANCEL );
    // Fixme: OS dependent!

    wxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL); // Fixme: Check wx default dialog button sizer
    buttonSizer->Add(okBtn);
    buttonSizer->Add(cancelButton);
    sizer->Add(buttonSizer, 0, wxALIGN_RIGHT|wxLEFT|wxRIGHT|wxBOTTOM, 10);
    SetDefaultItem(okBtn);
    SetSizerAndFit(sizer);
    Center(wxBOTH);
    wxAcceleratorEntry entries[5];
    entries[0].Set(wxACCEL_CTRL, (int) 'C', ID_COPY);
    entries[1].Set(wxACCEL_CTRL, (int) 'V', ID_PASTE);
    entries[2].Set(wxACCEL_NORMAL, (int) 'Q', ID_HSL );
    entries[3].Set(wxACCEL_NORMAL, (int) 'W', ID_GRADIENT);
    entries[4].Set(wxACCEL_NORMAL, (int) 'E', ID_PATTERN);
    wxAcceleratorTable accel(5, entries);
    SetAcceleratorTable(accel);
    Connect(ID_COPY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(DrawSourceDialog::OnCopy));
    Connect(ID_PASTE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(DrawSourceDialog::OnPaste));
    Connect(ID_HSL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(DrawSourceDialog::OnKeyHSL));
    Connect(ID_GRADIENT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(DrawSourceDialog::OnKeyGradient));
    Connect(ID_PATTERN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(DrawSourceDialog::OnKeyPattern));
  }
  faint::DrawSource GetDrawSource() const{
    wxWindow* selected = m_tabs->GetCurrentPage();
    if ( selected == m_panelHSL->AsWindow() ){
      return faint::DrawSource(m_panelHSL->GetColor());
    }
    else if ( selected == m_panelPattern->AsWindow() ){
      return faint::DrawSource(m_panelPattern->GetPattern());
    }
    else if ( selected == m_panelGradient->AsWindow() ){
      return faint::DrawSource(m_panelGradient->GetGradient());
    }
    else {
      assert(false);
      return faint::DrawSource(faint::Color(0,0,0));
    }
  }

  void SetDrawSource( const faint::DrawSource& src ){
    using namespace std::placeholders;
    dispatch(src,
      std::bind(&DrawSourceDialog::SetColor, this, _1),
      std::bind(&DrawSourceDialog::SetPattern, this, _1),
      std::bind(&DrawSourceDialog::SetGradient, this, _1));
  }

private:
  void SelectTab( wxWindow* window ){
    for ( size_t i = 0; i != m_tabs->GetPageCount(); i++ ){
      if ( m_tabs->GetPage(i) == window ){
        m_tabs->ChangeSelection(i);
        return;
      }
    }
    assert(false);
  }

  void SetColor( const faint::Color& color ){
    m_panelHSL->SetColor(color);
    m_panelGradient->SetGradient(gradient_from_color(color));
    SelectTab(m_panelHSL->AsWindow());
  }

  void SetGradient( const faint::Gradient& gradient ){
    m_panelGradient->SetGradient(gradient);
    SelectTab(m_panelGradient->AsWindow());
  }

  void SetPattern( const faint::Pattern& pattern ){
    m_panelPattern->SetPattern(pattern);
    SelectTab(m_panelPattern->AsWindow());
  }

  void OnCopy( wxCommandEvent& ){
    wxWindow* selected = m_tabs->GetCurrentPage();
    if ( selected == m_panelPattern->AsWindow() ){
      m_panelPattern->Copy();
    }
  }

  void OnKeyHSL( wxCommandEvent& ){
    SelectTab(m_panelHSL->AsWindow());
  }

  void OnKeyGradient( wxCommandEvent& ){
    SelectTab(m_panelGradient->AsWindow());
  }

  void OnKeyPattern( wxCommandEvent& ){
    SelectTab(m_panelPattern->AsWindow());
  }

  void OnPaste( wxCommandEvent& ){
    wxWindow* selected = m_tabs->GetCurrentPage();
    if ( selected == m_panelPattern->AsWindow() ){
      m_panelPattern->Paste();
    }
  }
  std::unique_ptr<ColorPanel_Gradient> m_panelGradient;
  std::unique_ptr<ColorPanel_HSL> m_panelHSL;
  std::unique_ptr<ColorPanel_Pattern> m_panelPattern;
  wxNotebook* m_tabs;
};

static ColorPanel_HSL* init_color_dialog_panel( wxPanel* bgPanel ){
  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  ColorPanel_HSL* colorPanel = new ColorPanel_HSL(bgPanel);
  sizer->Add(colorPanel->AsWindow());
  wxButton* okBtn = new wxButton(bgPanel, wxID_OK);
  wxButton* cancelButton = new wxButton(bgPanel, wxID_CANCEL );

  // Fixme: OS dependent!
  wxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL); // Fixme: Check wx default dialog button sizer
  buttonSizer->Add(okBtn);
  buttonSizer->Add(cancelButton);
  sizer->Add(buttonSizer, 0, wxALIGN_RIGHT|wxLEFT|wxRIGHT|wxBOTTOM, 10);
  bgPanel->SetSizer(sizer);
  return colorPanel;
}
class ColorDialog : public wxDialog {
public:
  ColorDialog( wxWindow* parent, const wxString& title, const faint::Color& initialColor )
    : wxDialog( parent, wxID_ANY, title )
  {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxPanel* bg = new wxPanel(this);
    sizer->Add(bg);
    m_panelHSL.reset(init_color_dialog_panel( bg ));
    m_panelHSL->SetColor(initialColor);
    SetSizerAndFit(sizer);
    Center(wxBOTH);
  }

  faint::Color GetColor() const{
    return m_panelHSL->GetColor();
  }
private:
  std::unique_ptr<ColorPanel_HSL> m_panelHSL;
};

Optional<faint::Color> show_color_only_dialog(wxWindow* parent, const wxString& title, const faint::Color& initial ){
  ColorDialog dlg( parent, title, initial );
  int result = faint::show_modal(dlg);
  if ( result == wxID_OK ){
    return option(dlg.GetColor());
  }
  return no_option();
}

Optional<faint::DrawSource> show_drawsource_dialog( wxWindow* parent, const wxString& title, const faint::DrawSource& initial, StatusInterface& statusInfo ){
  DrawSourceDialog dlg( parent, title, statusInfo );
  dlg.SetDrawSource(initial);
  int result = faint::show_modal(dlg);
  if ( result == wxID_OK ){
    return option(dlg.GetDrawSource());
  }
  return no_option();
}

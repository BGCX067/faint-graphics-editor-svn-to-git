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
#include "wx/wx.h"
#include "bitmap/filter.hh"
#include "gui/brightness-contrast-dialog.hh"
#include "gui/layout.hh"
#include "gui/slider.hh"
#include "util/commandutil.hh"

static bool enable_preview_default( const faint::Bitmap& bmp ){
  return ( bmp.m_w * bmp.m_h ) < (1024 * 1024);
}

class BrightnessContrastDialogImpl : public wxDialog {
public:
  BrightnessContrastDialogImpl( wxWindow* parent, DialogFeedback& feedback )
    : wxDialog( parent, wxID_ANY, "Brightness and Contrast", wxDefaultPosition, wxSize(320,200), wxDEFAULT_DIALOG_STYLE | wxWANTS_CHARS | wxRESIZE_BORDER ),
      m_bitmap(feedback.GetBitmap()),
      m_brightnessSlider(nullptr),
      m_contrastSlider(nullptr),
      m_enablePreviewCheck(nullptr),
      m_feedback(feedback)
  {
    // Create the member-controls in intended tab-order (placement follows)
    m_enablePreviewCheck = new wxCheckBox( this, wxID_ANY, "&Preview" );
    m_enablePreviewCheck->SetValue(enable_preview_default(m_bitmap));
    wxStaticText* lblBrightness = new wxStaticText( this, wxID_ANY, "&Brightness" );
    m_brightnessSlider = new Slider(this, ClosedIntRange(min_t(0), max_t(100)), 50, SliderDir::HORIZONTAL, SliderRectangleBackground() );
    m_brightnessSlider->SetInitialSize( wxSize(200, 20) );
    wxStaticText* lblContrast = new wxStaticText( this, wxID_ANY, "&Contrast" );
    m_contrastSlider = new Slider(this, ClosedIntRange(min_t(0),max_t(100)), 50, SliderDir::HORIZONTAL, SliderRectangleBackground() );
    m_contrastSlider->SetInitialSize( wxSize(200, 20) );
    wxButton* btnOk = new wxButton(this, wxID_OK);
    wxButton* btnCancel = new wxButton(this, wxID_CANCEL);

    // Outer-most sizer
    wxSizer* rows = new wxBoxSizer(wxVERTICAL);
    rows->AddSpacer(faint::panel_padding);

    wxSizer* previewRow = new wxBoxSizer(wxHORIZONTAL);
    previewRow->AddSpacer(faint::panel_padding);
    previewRow->Add(m_enablePreviewCheck);
    rows->Add( previewRow );
    rows->AddSpacer(faint::item_spacing);

    wxSizer* brightnessRow = new wxBoxSizer(wxHORIZONTAL);
    brightnessRow->AddSpacer(faint::panel_padding);

    brightnessRow->Add(lblBrightness, 0, wxALIGN_CENTER_VERTICAL );
    brightnessRow->AddSpacer(faint::item_spacing);
    brightnessRow->Add( m_brightnessSlider, 1, wxEXPAND );
    brightnessRow->AddSpacer(faint::panel_padding);
    rows->Add( brightnessRow, 0, wxEXPAND );
    rows->AddSpacer(faint::item_spacing);

    wxSizer* contrastRow = new wxBoxSizer(wxHORIZONTAL);
    contrastRow->AddSpacer( faint::panel_padding );
    contrastRow->Add( lblContrast, 0, wxALIGN_CENTER_VERTICAL );
    contrastRow->AddSpacer(faint::item_spacing);
    contrastRow->Add( m_contrastSlider, 1 );
    contrastRow->AddSpacer(faint::panel_padding);
    rows->Add( contrastRow, 0, wxEXPAND );
    rows->AddSpacer(faint::panel_padding);

    wxSize cntSz = lblContrast->GetSize();
    wxSize brSz = lblBrightness->GetSize();
    wxSize labelSize( std::max(brSz.GetWidth(), cntSz.GetWidth()) + 10, std::max(brSz.GetHeight(), cntSz.GetHeight()));
    lblContrast->SetInitialSize(labelSize);
    lblBrightness->SetInitialSize(labelSize);
    wxBoxSizer* btnSizer = new wxBoxSizer( wxHORIZONTAL );
    btnSizer->Add( btnOk );
    btnSizer->Add( btnCancel );
    rows->Add( btnSizer, 0, wxALIGN_CENTER );
    rows->AddSpacer(10);
    SetDefaultItem(btnOk);
    //SetSizer(rows);
    //Layout();
    SetSizerAndFit(rows);
    Centre(wxBOTH); // Center on parent

    wxAcceleratorEntry accelerators[1];
    accelerators[0].Set( wxACCEL_NORMAL, WXK_F5, wxID_REFRESH );
    SetAcceleratorTable(wxAcceleratorTable(1,accelerators));
    m_brightnessSlider->SetFocus();

    Bind(FAINT_SLIDER_CHANGE, SliderEventHandler(BrightnessContrastDialogImpl::OnSlider), this);

  }

  Command* GetCommand(){
    return get_brightness_and_contrast_command(GetValues());
  }

  bool ValuesModified() const{
    return m_contrastSlider->GetValue() != 50 || m_brightnessSlider->GetValue() != 50;
  }

private:
  faint::brightness_contrast_t GetValues() const{
    return faint::brightness_contrast_t( m_brightnessSlider->GetValue() / 50.0 - 1.0,
      m_contrastSlider->GetValue() / 50.0 );
  }

  void OnRefresh( wxCommandEvent& ){
    if ( !PreviewEnabled() ){
      UpdatePreview();
    }
  }

  void OnSlider( SliderEvent& ){
    if ( PreviewEnabled() ){
      UpdatePreview();
    }
  }

  void OnTogglePreview( wxCommandEvent& ){
    if ( !PreviewEnabled() ){
      ResetPreview();
    }
    else {
      UpdatePreview();
    }
  }

  bool PreviewEnabled() const{
    return m_enablePreviewCheck->GetValue();
  }

  void ResetPreview(){
    m_feedback.GetBitmap() = m_bitmap;
    m_feedback.Update();
  }

  void UpdatePreview(){
    faint::Bitmap bmp2(m_bitmap);
    faint::apply_brightness_and_contrast(bmp2, GetValues());
    m_feedback.GetBitmap() = bmp2;
    m_feedback.Update();
  }

  faint::Bitmap m_bitmap;
  Slider* m_brightnessSlider;
  Slider* m_contrastSlider;
  wxCheckBox* m_enablePreviewCheck;
  DialogFeedback& m_feedback;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(BrightnessContrastDialogImpl, wxDialog)
EVT_MENU(wxID_REFRESH, BrightnessContrastDialogImpl::OnRefresh)
EVT_CHECKBOX(-1, BrightnessContrastDialogImpl::OnTogglePreview)
END_EVENT_TABLE()

BrightnessContrastDialog::BrightnessContrastDialog(){
}

Command* BrightnessContrastDialog::GetCommand(){
  return m_command.Retrieve();
}

bool BrightnessContrastDialog::ShowModal(wxWindow* parent, DialogFeedback& feedback){
  BrightnessContrastDialogImpl impl(parent, feedback);
  if ( impl.ShowModal() == wxID_OK  && impl.ValuesModified() ){
    m_command.Set( impl.GetCommand() );
    return true;
  }
  // Cancelled or unchanged
  return false;
}

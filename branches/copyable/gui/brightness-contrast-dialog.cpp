// -*- coding: us-ascii-unix -*-
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

#include <algorithm>
#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/dialog.h"
#include "wx/stattext.h"
#include "bitmap/bitmap.hh"
#include "bitmap/filter.hh"
#include "gui/brightness-contrast-dialog.hh"
#include "gui/slider.hh"
#include "gui/wx-layout.hh"
#include "util/command-util.hh"
#include "util/gui-util.hh"

namespace faint{

static bool enable_preview_default(const Bitmap& bmp){
  return (bmp.m_w * bmp.m_h) < (1024 * 1024);
}

class BrightnessContrastDialog : public wxDialog {
public:
  BrightnessContrastDialog(wxWindow& parent, DialogFeedback& feedback)
    : wxDialog(&parent, wxID_ANY, "Brightness and Contrast", wxDefaultPosition, wxSize(320,200), wxDEFAULT_DIALOG_STYLE | wxWANTS_CHARS | wxRESIZE_BORDER),
      m_bitmap(copy(feedback.GetBitmap())),
      m_brightnessSlider(nullptr),
      m_contrastSlider(nullptr),
      m_enablePreviewCheck(nullptr),
      m_feedback(feedback)
  {
    using namespace layout;

    // Create the member-controls in intended tab-order (placement follows)
    m_enablePreviewCheck = new wxCheckBox(this, wxID_ANY, "&Preview");
    m_enablePreviewCheck->SetValue(enable_preview_default(m_bitmap));
    auto lblBrightness = label(this, "&Brightness");
    m_brightnessSlider = create_slider(this, BoundedInt::Mid(min_t(0), max_t(100)), SliderDir::HORIZONTAL, SliderRectangleBackground());
    m_brightnessSlider->SetInitialSize(wxSize(200, 20));

    auto lblContrast = label(this, "&Contrast");
    m_contrastSlider = create_slider(this, BoundedInt::Mid(min_t(0), max_t(100)), SliderDir::HORIZONTAL, SliderRectangleBackground());
    m_contrastSlider->SetInitialSize(wxSize(200, 20));

    wxSize cntSz = lblContrast->GetSize();
    wxSize brSz = lblBrightness->GetSize();
    wxSize labelSize(std::max(brSz.GetWidth(), cntSz.GetWidth()) + 10, std::max(brSz.GetHeight(), cntSz.GetHeight()));
    lblContrast->SetInitialSize(labelSize);
    lblBrightness->SetInitialSize(labelSize);

    SetSizerAndFit(create_column({
      create_row({m_enablePreviewCheck}),
      grow(create_row({
        {lblBrightness, 0, wxALIGN_CENTER_VERTICAL},
        {m_brightnessSlider, 1, wxEXPAND}})),
      grow(create_row({
        {lblContrast, 0, wxALIGN_CENTER_VERTICAL},
        {m_contrastSlider, 1, wxEXPAND}})),
      center(create_row_no_pad({
        make_default(this, new wxButton(this, wxID_OK)),
        new wxButton(this, wxID_CANCEL)}))
    }));

    Centre(wxBOTH); // Center on parent

    set_accelerators(this, {
      {key::F5, [=](){UpdatePreview();}},
      {key::P, [=](){
         m_enablePreviewCheck->SetValue(!m_enablePreviewCheck->GetValue());
         if (!PreviewEnabled()){
           ResetPreview();
         }
         else{
           UpdatePreview();
         }
        }
      }});

    m_brightnessSlider->SetFocus();

    Bind(EVT_FAINT_SLIDER_CHANGE,
      [&](SliderEvent&){
        if (PreviewEnabled()){
          UpdatePreview();
        }
      });

    Bind(wxEVT_CHECKBOX,
      [&](wxCommandEvent&){
        if (!PreviewEnabled()){
          ResetPreview();
        }
        else {
          UpdatePreview();
        }
      });
  }

  BitmapCommand* GetCommand(){
    return get_brightness_and_contrast_command(GetValues());
  }

  bool ValuesModified() const{
    return m_contrastSlider->GetValue() != 50 || m_brightnessSlider->GetValue() != 50;
  }

private:
  brightness_contrast_t GetValues() const{
    return brightness_contrast_t(m_brightnessSlider->GetValue() / 50.0 - 1.0,
      m_contrastSlider->GetValue() / 50.0);
  }

  bool PreviewEnabled() const{
    return m_enablePreviewCheck->GetValue();
  }

  void ResetPreview(){
    m_feedback.GetBitmap() = m_bitmap;
    m_feedback.Update();
  }

  void UpdatePreview(){
    Bitmap bmp2(copy(m_bitmap));
    apply_brightness_and_contrast(bmp2, GetValues());
    m_feedback.GetBitmap() = bmp2;
    m_feedback.Update();
  }

  Bitmap m_bitmap;
  Slider* m_brightnessSlider;
  Slider* m_contrastSlider;
  wxCheckBox* m_enablePreviewCheck;
  DialogFeedback& m_feedback;
};

Optional<BitmapCommand*> show_brightness_contrast_dialog(wxWindow& parent, DialogFeedback& feedback){
  BrightnessContrastDialog dlg(parent, feedback);
  if (dlg.ShowModal() == wxID_OK && dlg.ValuesModified()){
    return option(dlg.GetCommand());
  }
  return no_option();
}

}

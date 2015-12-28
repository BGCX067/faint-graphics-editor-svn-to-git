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

#include <algorithm>
#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/dialog.h"
#include "wx/sizer.h"
#include "bitmap/bitmap.hh"
#include "bitmap/filter.hh"
#include "commands/function-cmd.hh"
#include "gui/sharpness-dialog.hh"
#include "gui/slider.hh"
#include "gui/wx-layout.hh"
#include "util/command-util.hh"
#include "util/gui-util.hh"

namespace faint{

static bool enable_preview_default(const Bitmap& bmp){
  return (bmp.m_w * bmp.m_h) < (1024 * 1024);
}

class SharpnessDialog : public wxDialog {
public:
  SharpnessDialog(wxWindow& parent, DialogFeedback& feedback)
    : wxDialog(&parent, wxID_ANY, "Sharpness", wxDefaultPosition, wxSize(320,200), wxDEFAULT_DIALOG_STYLE | wxWANTS_CHARS | wxRESIZE_BORDER),
      m_bitmap(copy(feedback.GetBitmap())),
      m_sharpnessSlider(nullptr),
      m_enablePreviewCheck(nullptr),
      m_feedback(feedback)
  {
    // Create the member-controls in intended tab-order (placement follows)
    m_enablePreviewCheck = new wxCheckBox(this, wxID_ANY, "&Preview");
    m_enablePreviewCheck->SetValue(enable_preview_default(m_bitmap));
    m_sharpnessSlider = create_slider(this, BoundedInt::Mid(min_t(-100), max_t(100)), SliderDir::HORIZONTAL, SliderMidPointBackground());
    m_sharpnessSlider->SetInitialSize(wxSize(200, 20));

    using namespace layout;
    SetSizerAndFit(create_column({
      create_row({m_enablePreviewCheck}),
      create_row({
        {label(this, "&Sharpness"), 0, wxALIGN_CENTER_VERTICAL},
        grow(m_sharpnessSlider)}),
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
    coord sharpness = GetSharpness();
    if (sharpness < 0){
      return get_function_command("Blur",
        [=](Bitmap& bmp){gaussian_blur(bmp, -sharpness);});
    }
    else{
      return get_function_command("Sharpen",
        [=](Bitmap& bmp){unsharp_mask(bmp, sharpness);});
    }
  }

  bool ValidSharpness(){
    return m_sharpnessSlider->GetValue() != 0;
  }

private:
  coord GetSharpness() const{
    return m_sharpnessSlider->GetValue() / 10.0;
  }

  bool PreviewEnabled() const{
    return m_enablePreviewCheck->GetValue();
  }

  void ResetPreview(){
    m_feedback.GetBitmap() = m_bitmap;
    m_feedback.Update();
  }

  void UpdatePreview(){
    if (ValidSharpness()){
      Bitmap bmp2(copy(m_bitmap));
      double sharpness = GetSharpness();
      if (sharpness < 0){
        gaussian_blur(bmp2, -sharpness);
      }
      else {
        unsharp_mask(bmp2, sharpness);
      }
      m_feedback.GetBitmap() = bmp2;
    }
    else{
      m_feedback.GetBitmap() = m_bitmap;
    }
    m_feedback.Update();
  }

  Bitmap m_bitmap;
  Slider* m_sharpnessSlider;
  wxCheckBox* m_enablePreviewCheck;
  DialogFeedback& m_feedback;
};

Optional<BitmapCommand*> show_sharpness_dialog(wxWindow& parent, DialogFeedback& feedback){
  SharpnessDialog dlg(parent, feedback);
  if (dlg.ShowModal() == wxID_OK && dlg.ValidSharpness()){
    return option(dlg.GetCommand());
  }
  return no_option();
}

} // namespace

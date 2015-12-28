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

#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/dialog.h"
#include "bitmap/bitmap-templates.hh"
#include "bitmap/filter.hh"
#include "gui/dual-slider.hh"
#include "gui/slider-histogram-background.hh"
#include "gui/threshold-dialog.hh"
#include "gui/wx-layout.hh"
#include "util/command-util.hh"
#include "util/optional.hh"
#include "util-wx/convert-wx.hh"
#include "util-wx/gui-util.hh"
#include "util-wx/key-codes.hh"

namespace faint{

class ThresholdDialog : public wxDialog {
public:
  ThresholdDialog(wxWindow& parent, DialogFeedback& feedback, const Paint& inside, const Paint& outside)
    : wxDialog(&parent, wxID_ANY, "Threshold", wxDefaultPosition, wxSize(320,200), wxDEFAULT_DIALOG_STYLE | wxWANTS_CHARS | wxRESIZE_BORDER),
      m_bitmap(feedback.GetBitmap()),
      m_enablePreviewCheck(nullptr),
      m_feedback(feedback),
      m_inside(inside),
      m_outside(outside),
      m_slider(nullptr)
  {
    // Create the member-controls in intended tab-order (placement follows)
    m_enablePreviewCheck = new wxCheckBox(this, wxID_ANY, "&Preview");
    m_enablePreviewCheck->SetValue(true);
    m_slider = create_dual_slider(this,
      fractional_bounded_interval<threshold_range_t>(0.2, 0.8),
      SliderHistogramBackground(threshold_histogram(m_bitmap), ColRGB(200,200,200), ColRGB(0,0,0)));
    m_slider->SetInitialSize(wxSize(200, 50));

    using namespace layout;
    SetSizerAndFit(create_column({
      create_row({m_enablePreviewCheck}),
      grow(create_row({grow(m_slider)})),
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


    m_slider->SetFocus();
    UpdatePreview();

    Bind(EVT_FAINT_DUAL_SLIDER_CHANGE,
      [&](DualSliderEvent&){
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
    return get_threshold_command(GetCurrentRange(), m_inside, m_outside);
  }
private:
  threshold_range_t GetCurrentRange(){
    return threshold_range_t(m_slider->GetSelectedInterval());
  }

  bool PreviewEnabled() const{
    return m_enablePreviewCheck->GetValue();
  }

  void ResetPreview(){
    m_feedback.SetBitmap(m_bitmap);
  }

  void UpdatePreview(){
    m_feedback.SetBitmap(onto_new(threshold, m_bitmap,
      GetCurrentRange(), m_inside, m_outside));
  }

  Bitmap m_bitmap;
  wxCheckBox* m_enablePreviewCheck;
  DialogFeedback& m_feedback;
  Paint m_inside;
  Paint m_outside;
  DualSlider* m_slider;
};

Optional<BitmapCommand*> show_threshold_dialog(wxWindow& parent, DialogFeedback& feedback, const Paint& inside, const Paint& outside){
  ThresholdDialog dlg(parent, feedback, inside, outside);
  if (dlg.ShowModal() == wxID_OK){
    return option(dlg.GetCommand());
  }
  return no_option();
}

bmp_dialog_func bind_show_threshold_dialog(const Paint& inside, const Paint& outside){
  return [=](wxWindow& window, DialogFeedback& feedback){
    return show_threshold_dialog(window, feedback, inside, outside);
  };
}

} // namespace

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
#include "commands/function-cmd.hh"
#include "gui/color-balance-dialog.hh"
#include "gui/dual-slider.hh"
#include "gui/slider-histogram-background.hh"
#include "gui/wx-layout.hh"
#include "util/command-util.hh"
#include "util/convert-wx.hh"
#include "util/gui-util.hh"

namespace faint{

class ColorBalanceDialog : public wxDialog {
public:
  ColorBalanceDialog(wxWindow& parent, DialogFeedback& feedback)
    : wxDialog(&parent, wxID_ANY, "Color Balance", wxDefaultPosition, wxSize(320,200), wxDEFAULT_DIALOG_STYLE | wxWANTS_CHARS | wxRESIZE_BORDER),
      m_bitmap(copy(feedback.GetBitmap())),
      m_enablePreviewCheck(nullptr),
      m_feedback(feedback),
      m_redSlider(nullptr),
      m_greenSlider(nullptr),
      m_blueSlider(nullptr)
  {
    // Create the member-controls in intended tab-order (placement follows)
    m_enablePreviewCheck = new wxCheckBox(this, wxID_ANY, "&Preview");
    m_enablePreviewCheck->SetValue(true);

    m_redSlider = create_dual_slider(this,
      fractional_bounded_interval<color_range_t>(0.2, 0.8),
      SliderHistogramBackground(red_histogram(m_bitmap),
        ColRGB(181,0,0)));
    m_redSlider->SetInitialSize(wxSize(200, 20));

    m_greenSlider = create_dual_slider(this,
      fractional_bounded_interval<color_range_t>(0.2, 0.8),
      SliderHistogramBackground(green_histogram(m_bitmap),
        ColRGB(34,177,76)));
    m_greenSlider->SetInitialSize(wxSize(200, 20));

    m_blueSlider = create_dual_slider(this,
      fractional_bounded_interval<color_range_t>(0.2, 0.8),
      SliderHistogramBackground(blue_histogram(m_bitmap),
        ColRGB(47,54,153)));
    m_blueSlider->SetInitialSize(wxSize(200, 20));

    // Outer-most sizer
    using namespace layout;
    SetSizerAndFit(create_column({
      create_row({m_enablePreviewCheck}),
      grow(create_row({grow(m_redSlider)})),
      grow(create_row({grow(m_greenSlider)})),
      grow(create_row({grow(m_blueSlider)})),
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

    m_redSlider->SetFocus();
    UpdatePreview();

    Bind(EVT_FAINT_DUAL_SLIDER_CHANGE,
      [&](DualSliderEvent& event){
        if (event.Special()){
          Interval interval = event.GetSelectedInterval();
          m_redSlider->SetSelectedInterval(interval);
          m_greenSlider->SetSelectedInterval(interval);
          m_blueSlider->SetSelectedInterval(interval);
        }
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
    color_range_t red = GetRange(m_redSlider);
    color_range_t green = GetRange(m_greenSlider);
    color_range_t blue = GetRange(m_blueSlider);
    return get_function_command("Color balance",
      [=](Bitmap& bmp){color_balance(bmp, red, green, blue);});
  }

  ColorBalanceDialog(const ColorBalanceDialog&) = delete;
private:
  color_range_t GetRange(DualSlider* slider){
    return color_range_t(slider->GetSelectedInterval());
  }

  bool PreviewEnabled() const{
    return m_enablePreviewCheck->GetValue();
  }

  void ResetPreview(){
    m_feedback.GetBitmap() = m_bitmap;
    m_feedback.Update();
  }

  void UpdatePreview(){
    color_range_t red = GetRange(m_redSlider);
    color_range_t green = GetRange(m_greenSlider);
    color_range_t blue = GetRange(m_blueSlider);
    Bitmap bmp2(copy(m_bitmap));
    color_balance(bmp2, red, green, blue);
    m_feedback.GetBitmap() = bmp2;
    m_feedback.Update();
  }

  Bitmap m_bitmap;
  wxCheckBox* m_enablePreviewCheck;
  DialogFeedback& m_feedback;
  DualSlider* m_redSlider;
  DualSlider* m_greenSlider;
  DualSlider* m_blueSlider;
};

Optional<BitmapCommand*> show_color_balance_dialog(wxWindow& parent, DialogFeedback& feedback){
  ColorBalanceDialog dlg(parent, feedback);
  if (dlg.ShowModal() == wxID_OK){
    return option(dlg.GetCommand());
  }
  return no_option();
}

} // namespace

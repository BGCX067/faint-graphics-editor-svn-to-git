// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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
#include "bitmap/bitmap.hh"
#include "bitmap/bitmap-templates.hh"
#include "gui/alpha-dialog.hh"
#include "gui/slider.hh"
#include "gui/slider-alpha-background.hh"
#include "gui/wx-layout.hh"
#include "util/command-util.hh"
#include "util/optional.hh"
#include "util-wx/gui-util.hh"
#include "util-wx/key-codes.hh"

namespace faint{

class AlphaDialog : public wxDialog {
public:
  AlphaDialog(wxWindow& parent, DialogFeedback& feedback)
    : wxDialog(&parent, wxID_ANY, "Alpha", wxDefaultPosition, wxSize(320, 200),
      wxDEFAULT_DIALOG_STYLE | wxWANTS_CHARS | wxRESIZE_BORDER),
      m_alphaSlider(nullptr),
      m_bitmap(feedback.GetBitmap()),
      m_feedback(feedback)
  {
    // Create the member-controls in intended tab-order (placement follows)
    using namespace layout;
    auto lblAlpha = label(this, "&Alpha");
    const Bitmap& bmp(feedback.GetBitmap());
    int initialValue = bitmap_ok(bmp) ?
      static_cast<int>(get_color_raw(bmp, 0,0).a) :
      255;

    m_alphaSlider = create_slider(this,
      BoundedInt(min_t(0), initialValue, max_t(255)),
      SliderDir::HORIZONTAL,
      BorderedSliderMarker(),
      AlphaBackground(ColRGB(77,109,243)));
    m_alphaSlider->SetInitialSize(wxSize(200,20));

    SetSizerAndFit(create_column({
      grow(create_row({
        {lblAlpha, 0, wxALIGN_CENTER_VERTICAL},
        {m_alphaSlider, 1, wxEXPAND}})),
        center(create_row_no_pad({make_default(this, new wxButton(this, wxID_OK)),
          new wxButton(this, wxID_CANCEL)}))}));

    Centre(wxBOTH); // Center on parent

    Bind(EVT_FAINT_SLIDER_CHANGE,
      [&](SliderEvent&){
        UpdatePreview();
      });
  }

  BitmapCommand* GetCommand(){
    return get_set_alpha_command(GetAlpha());
  }

private:
  uchar GetAlpha() const{
    return static_cast<uchar>(m_alphaSlider->GetValue());
  }

  void UpdatePreview(){
    m_feedback.SetBitmap(onto_new(set_alpha, m_bitmap, GetAlpha()));
  }


  Slider* m_alphaSlider;
  Bitmap m_bitmap;
  DialogFeedback& m_feedback;
};

Optional<BitmapCommand*> show_alpha_dialog(wxWindow& parent,
  DialogFeedback& feedback)
{
  AlphaDialog dlg(parent, feedback);
  if (dlg.ShowModal() == wxID_OK){
    return option(dlg.GetCommand());
  }
  return no_option();
}

} // namespace

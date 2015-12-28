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

#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/dialog.h"
#include "wx/sizer.h"
#include "bitmap/bitmap-templates.hh"
#include "gui/pixelize-dialog.hh"
#include "gui/slider.hh"
#include "gui/wx-layout.hh"
#include "util/command-util.hh"
#include "util/optional.hh"
#include "util-wx/gui-util.hh"
#include "util-wx/key-codes.hh"

namespace faint{

static bool enable_preview_default(const Bitmap& bmp){
  return (bmp.m_w * bmp.m_h) < (1024 * 1024);
}

class PixelizeDialog : public wxDialog {
public:
  PixelizeDialog(wxWindow& parent, DialogFeedback& feedback)
    : wxDialog(&parent, wxID_ANY, "Pixelize", wxDefaultPosition, wxSize(320,200), wxDEFAULT_DIALOG_STYLE | wxWANTS_CHARS | wxRESIZE_BORDER),
      m_bitmap(feedback.GetBitmap()),
      m_pixelSizeSlider(nullptr),
      m_enablePreviewCheck(nullptr),
      m_feedback(feedback)
  {
    using namespace layout;

    // Create the member-controls in intended tab-order (placement follows)
    m_enablePreviewCheck = new wxCheckBox(this, wxID_ANY, "&Preview");
    m_enablePreviewCheck->SetValue(enable_preview_default(m_bitmap));
    m_pixelSizeSlider = create_slider(this, BoundedInt::Mid(min_t(1), max_t(100)), SliderDir::HORIZONTAL, SliderRectangleBackground());
    m_pixelSizeSlider->SetInitialSize(wxSize(200, 20));

    SetSizerAndFit(create_column({
      create_row({m_enablePreviewCheck}),
      grow(create_row({
        {label(this, "Pixel size"), 0, wxALIGN_CENTER_VERTICAL},
        grow(m_pixelSizeSlider)})),
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
        }}});

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

    m_pixelSizeSlider->SetFocus();

    if (PreviewEnabled()){
      UpdatePreview();
    }
  }

  BitmapCommand* GetCommand(){
    return get_pixelize_command(m_pixelSizeSlider->GetValue());
  }

  bool ValuesModified() const{
    return m_pixelSizeSlider->GetValue() != 1;
  }

private:
  bool PreviewEnabled() const{
    return m_enablePreviewCheck->GetValue();
  }

  void ResetPreview(){
    m_feedback.SetBitmap(m_bitmap);
  }

  void UpdatePreview(){
    m_feedback.SetBitmap(onto_new(pixelize, m_bitmap,
      m_pixelSizeSlider->GetValue()));
  }

  Bitmap m_bitmap;
  Slider* m_pixelSizeSlider;
  wxCheckBox* m_enablePreviewCheck;
  DialogFeedback& m_feedback;
};

Optional<BitmapCommand*> show_pixelize_dialog(wxWindow& parent, DialogFeedback& feedback){
  PixelizeDialog dlg(parent, feedback);
  if (dlg.ShowModal() == wxID_OK && dlg.ValuesModified()){
    return option(dlg.GetCommand());
  }
  return no_option();
}

} // namespace

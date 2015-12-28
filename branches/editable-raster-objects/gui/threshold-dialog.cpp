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
#include "gui/dual-slider.hh"
#include "gui/layout.hh"
#include "gui/threshold-dialog.hh"
#include "util/commandutil.hh"
#include "util/convertwx.hh"

class SliderHistogramBackground : public SliderBackground{
public:
  SliderHistogramBackground( const std::vector<int>& values )
    : m_bitmap(faint::get_null_bitmap()),
    m_values(values)
  {}

  void Draw( faint::Bitmap& bmp, const IntSize& size ) override{
    if ( !bitmap_ok(m_bitmap) || m_bitmap.GetSize() != size ){
      InitializeBitmap(size);
    }
    faint::blit(m_bitmap, onto(bmp), IntPoint(0,0));
  }

  SliderBackground* Clone() const override{
    return new SliderHistogramBackground(*this);
  }
private:
  void InitializeBitmap( const IntSize& size ){
    const bin_t bins( std::min(resigned(m_values.size()), size.w) );
    Histogram histogram(ClosedIntRange(min_t(0), max_t(m_values.size())), bins);
    for ( size_t i = 0; i != m_values.size(); i++ ){
      histogram.Insert(i, count_t(m_values[i]));
    }
    int max = histogram.Max();

    double binWidth = std::max(size.w / double(histogram.NumBins()), 1.0); // Pixels per bin
    double scale_h = size.h / double(max);
    m_bitmap = faint::Bitmap(size, faint::Color(200,200,200));
    for ( int bin = 0; bin < histogram.NumBins(); bin++ ){
      int x = static_cast<int>(bin * binWidth);
      int y = static_cast<int>(size.h - histogram.Count(bin_t(bin)) * scale_h);
      fill_rect_color( m_bitmap, IntRect(IntPoint(x,y), IntSize(int(binWidth) + 1, size.h ) ), faint::Color(0,0,0) );
    }
    faint::draw_rect_color( m_bitmap, IntRect(IntPoint(0,0), size), faint::Color(0, 0, 0), 1, false );
  }
  faint::Bitmap m_bitmap;
  std::vector<int> m_values;
};

class ThresholdDialogImpl : public wxDialog {
public:
  ThresholdDialogImpl( wxWindow* parent, DialogFeedback& feedback )
    : wxDialog( parent, wxID_ANY, "Threshold", wxDefaultPosition, wxSize(320,200), wxDEFAULT_DIALOG_STYLE | wxWANTS_CHARS | wxRESIZE_BORDER ),
      m_bitmap(feedback.GetBitmap()),
      m_enablePreviewCheck(nullptr),
      m_feedback(feedback),
      m_slider(nullptr)
  {
    // Create the member-controls in intended tab-order (placement follows)
    m_enablePreviewCheck = new wxCheckBox( this, wxID_ANY, "&Preview" );
    m_enablePreviewCheck->SetValue(true);
    m_slider = new DualSlider(this, as_closed_range<faint::threshold_range_t>(),
      fractional_interval<faint::threshold_range_t>(0.2, 0.8),
      SliderHistogramBackground(threshold_histogram(m_bitmap)));
    m_slider->SetInitialSize( wxSize(200, 50) );
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

    wxSizer* sliderRow = new wxBoxSizer(wxHORIZONTAL);
    sliderRow->AddSpacer(faint::panel_padding);
    sliderRow->Add( m_slider, 1, wxEXPAND );
    sliderRow->AddSpacer(faint::panel_padding);
    rows->Add( sliderRow, 1, wxEXPAND );
    rows->AddSpacer(faint::item_spacing);

    wxBoxSizer* btnSizer = new wxBoxSizer( wxHORIZONTAL );
    btnSizer->Add( btnOk );
    btnSizer->Add( btnCancel );
    rows->Add( btnSizer, 0, wxALIGN_CENTER );
    rows->AddSpacer(faint::panel_padding);
    SetDefaultItem(btnOk);
    SetSizerAndFit(rows);
    Centre(wxBOTH); // Center on parent

    wxAcceleratorEntry accelerators[1];
    accelerators[0].Set( wxACCEL_NORMAL, WXK_F5, wxID_REFRESH );
    SetAcceleratorTable(wxAcceleratorTable(1,accelerators));
    m_slider->SetFocus();
    UpdatePreview();
    Bind(FAINT_DUAL_SLIDER_CHANGE, DualSliderEventHandler(ThresholdDialogImpl::OnDualSlider), this);
  }

  Command* GetCommand(){
    return get_threshold_command(GetCurrentRange());
  }
private:
  faint::threshold_range_t GetCurrentRange(){
    return faint::threshold_range_t(m_slider->GetSelectedInterval());
  }

  void OnDualSlider( DualSliderEvent& ){
    if ( PreviewEnabled() ){
      UpdatePreview();
    }
  }

  void OnRefresh( wxCommandEvent& ){
    if ( !PreviewEnabled() ){
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
    faint::threshold(bmp2, GetCurrentRange());
    m_feedback.GetBitmap() = bmp2;
    m_feedback.Update();
  }

  faint::Bitmap m_bitmap;
  wxCheckBox* m_enablePreviewCheck;
  DialogFeedback& m_feedback;
  DualSlider* m_slider;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ThresholdDialogImpl, wxDialog)
EVT_MENU(wxID_REFRESH, ThresholdDialogImpl::OnRefresh)
EVT_CHECKBOX(-1, ThresholdDialogImpl::OnTogglePreview)
END_EVENT_TABLE()

ThresholdDialog::ThresholdDialog(){
}

Command* ThresholdDialog::GetCommand(){
  return m_command.Retrieve();
}

bool ThresholdDialog::ShowModal(wxWindow* parent, DialogFeedback& feedback){
  ThresholdDialogImpl impl(parent, feedback);
  if ( impl.ShowModal() == wxID_OK  ){
    m_command.Set( impl.GetCommand() );
    return true;
  }
  // Cancelled or unchanged
  return false;
}

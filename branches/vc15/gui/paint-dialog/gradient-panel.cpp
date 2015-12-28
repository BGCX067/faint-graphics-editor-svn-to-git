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
#include "wx/bitmap.h"
#include "wx/checkbox.h"
#include "wx/dcclient.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"
#include "app/app.hh" // Fixme: For get_art_container
#include "bitmap/bitmap.hh"
#include "bitmap/draw.hh"
#include "bitmap/pattern.hh"
#include "geo/adjust.hh"
#include "geo/geo-func.hh"
#include "geo/int-rect.hh"
#include "geo/int-size.hh"
#include "geo/line.hh"
#include "geo/range.hh" // for constrained
#include "gui/art-container.hh"
#include "gui/bitmap-list-ctrl.hh"
#include "gui/layout.hh"
#include "gui/mouse-capture.hh"
#include "gui/paint-dialog.hh"
#include "gui/paint-dialog/gradient-panel.hh"
#include "gui/placement.hh"
#include "text/formatting.hh"
#include "util/color-bitmap-util.hh"
#include "util/index.hh"
#include "util/iter.hh"
#include "util/optional.hh"
#include "util-wx/convert-wx.hh"
#include "util-wx/gui-util.hh"
#include "gui/paint-dialog/gradient-color-stop-slider.hh"
#include "gui/paint-dialog/linear-gradient-display.hh"
#include "gui/paint-dialog/radial-gradient-display.hh"

namespace faint{

static LinearGradient default_linear_gradient(){
  return LinearGradient(Angle::Zero(),
    {{{255,0,0}, 0.0}, {{0,0,0}, 1.0}});
}

template<typename T>
static Color get_stop_color(const index_t& index, const T& g){
  return g.GetStop(index).GetColor();
}

static color_stops_t icon_color_stops(){
  return {
    {color_black(), 0.0},
    {color_white(), 1.0}};
}

static wxBitmap linear_icon(const IntSize& size){
  LinearGradient g(Angle::Zero(), icon_color_stops());
  Bitmap bmp(gradient_bitmap(Gradient(g), size));
  return to_wx_bmp(bmp);
}

static wxBitmap radial_icon(const IntSize& size){
  Radii radii(floated(size.w), floated(size.h));
  RadialGradient g(Point(0,0), radii, icon_color_stops());
  Bitmap bmp(gradient_bitmap(Gradient(g), size));
  return to_wx_bmp(bmp);
}

class PaintPanel_Gradient::PaintPanel_Gradient_Impl : public wxPanel{
public:
  PaintPanel_Gradient_Impl(wxWindow* parent,
    StatusInterface& statusInfo,
    DialogContext& dialogContext)
    : wxPanel(parent, wxID_ANY)
  {
    const wxPoint displayTopLeft(panel_padding, panel_padding);

    IntSize bmpSize(28,23);
    m_gradientTypeCtrl = new BitmapListCtrl(this,
      bmpSize,
      statusInfo,
      Axis::VERTICAL);
    m_gradientTypeCtrl->Add(linear_icon(bmpSize), "Linear");
    m_gradientTypeCtrl->Add(radial_icon(bmpSize), "Radial");
    m_gradientTypeCtrl->SetPosition(displayTopLeft);

    const wxSize displaySize(420 - m_gradientTypeCtrl->GetSize().GetWidth() - panel_padding,100);

    m_linearDisplay = new LinearGradientDisplay(this, displaySize, dialogContext);
    m_linearDisplay->SetPosition(to_the_right_of(m_gradientTypeCtrl));
    m_linearDisplay->Hide();

    m_radialDisplay = new RadialGradientDisplay(this, displaySize, dialogContext);
    m_radialDisplay->SetPosition(to_the_right_of(m_gradientTypeCtrl));
    m_radialDisplay->Hide();

    m_angleTextLabel = new wxStaticText(this, wxID_ANY, "&Angle",
      wxPoint(panel_padding, bottom(m_linearDisplay) + 10));

    m_angleTextCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
      wxSize(50, -1));
    m_angleTextCtrl->SetPosition(to_the_right_middle_of(m_angleTextLabel,
        m_angleTextCtrl->GetSize()));

    m_angleTextCtrl->Bind(wxEVT_KILL_FOCUS,
      [this](wxFocusEvent& event){
        // Propagate event to clear selection indicator etc.
        event.Skip();
        UpdateAngleText();
      });

    m_objectAligned = new wxCheckBox(this, wxID_ANY, "&Object Aligned",
      below(m_angleTextLabel) + wxPoint(0,10));

    SetGradient(Gradient(default_linear_gradient()));

    Bind(EVT_FAINT_BITMAPLIST_SELECTION,
      [this](wxEvent&){
        if (m_gradientTypeCtrl->GetSelection() == 0){
          color_stops_t stops = m_radialDisplay->GetGradient().GetStops();
          m_linearDisplay->SetStops(stops);
          m_linearDisplay->Show();
          m_radialDisplay->Hide();
          m_angleTextLabel->Enable(true);
          m_angleTextCtrl->Enable(true);
        }
        else{
          color_stops_t stops = m_linearDisplay->GetGradient().GetStops();
          m_radialDisplay->SetStops(stops);
          m_linearDisplay->Hide();
          m_radialDisplay->Show();
          m_angleTextLabel->Enable(false);
          m_angleTextCtrl->Enable(false);
        }
      });

    Bind(EVT_GRADIENT_ANGLE_PICKED,
      [this](wxEvent&){
        UpdateAngleText();
      });

    Bind(wxEVT_COMMAND_TEXT_UPDATED,
      [this](wxCommandEvent& evt){
        wxTextCtrl* ctrl = dynamic_cast<wxTextCtrl*>(evt.GetEventObject());
        m_linearDisplay->SetGradient(with_angle(m_linearDisplay->GetGradient(),
            Angle::Deg(parse_coord_value(ctrl, 0.0))));
      });
  }

  Gradient GetGradient(){
    if (m_gradientTypeCtrl->GetSelection() == 0){
      LinearGradient linear(m_linearDisplay->GetGradient());
      linear.SetObjectAligned(m_objectAligned->GetValue());
      return Gradient(linear);
    }
    else{
      RadialGradient radial(m_radialDisplay->GetGradient());
      radial.SetObjectAligned(m_objectAligned->GetValue());
      return Gradient(radial);
    }
  }

  void SetGradient(const Gradient& g){
    if (g.IsLinear()){
      m_gradientTypeCtrl->SetSelection(index_t(0));
      const LinearGradient& lg(g.GetLinear());
      m_objectAligned->SetValue(lg.GetObjectAligned());
      m_linearDisplay->SetGradient(lg);
      m_linearDisplay->Show();
      m_radialDisplay->Hide();
      UpdateAngleText();
      m_angleTextLabel->Enable(true);
      m_angleTextCtrl->Enable(true);

    }
    else{
      m_gradientTypeCtrl->SetSelection(index_t(1));
      const RadialGradient& rg(g.GetRadial());
      m_objectAligned->SetValue(rg.GetObjectAligned());
      m_radialDisplay->SetGradient(rg);
      m_linearDisplay->Hide();
      m_radialDisplay->Show();
      m_angleTextLabel->Enable(false);
      m_angleTextCtrl->Enable(false);
    }
  }

  bool SetBackgroundColour(const wxColour& bgColor) override{
    wxPanel::SetBackgroundColour(bgColor);
    m_linearDisplay->SetBackgroundColour(bgColor);
    m_radialDisplay->SetBackgroundColour(bgColor);
    return true;
  }

private:
  void UpdateAngleText(){
    auto angle = m_linearDisplay->GetGradient().GetAngle();
    m_angleTextCtrl->ChangeValue(to_wx(str_degrees(angle)));
  }

  wxStaticText* m_angleTextLabel;
  wxTextCtrl* m_angleTextCtrl;
  LinearGradientDisplay* m_linearDisplay;
  RadialGradientDisplay* m_radialDisplay;
  wxCheckBox* m_objectAligned;
  BitmapListCtrl* m_gradientTypeCtrl;
};

PaintPanel_Gradient::PaintPanel_Gradient(wxWindow* parent,
  StatusInterface& statusInfo,
  DialogContext& dialogContext){
  m_impl = new PaintPanel_Gradient_Impl(parent, statusInfo, dialogContext);
}

PaintPanel_Gradient::~PaintPanel_Gradient(){
  m_impl = nullptr; // Deletion handled by wxWidgets
}

wxWindow* PaintPanel_Gradient::AsWindow(){
  return m_impl;
}

Gradient PaintPanel_Gradient::GetGradient() const{
  return m_impl->GetGradient();
}

void PaintPanel_Gradient::SetGradient(const Gradient& g){
  m_impl->SetGradient(g);
}

} // namespace

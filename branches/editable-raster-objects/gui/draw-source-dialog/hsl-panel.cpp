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

#include <sstream>
#include "wx/wx.h"
#include "gui/draw-source-dialog/hsl-panel.hh"
#include "gui/draw-source-dialog/placement.hh"
#include "gui/layout.hh"
#include "gui/slider.hh"
#include "util/color.hh"
#include "util/colorutil.hh"
#include "util/convertwx.hh"
#include "util/formatting.hh"
#include "util/guiutil.hh"

DECLARE_EVENT_TYPE(EVT_PICKED_HUE_SAT,-1)
DEFINE_EVENT_TYPE(EVT_PICKED_HUE_SAT)

class AlphaBackground : public SliderBackground{
public:
  explicit AlphaBackground( const faint::ColRGB& rgb )
    : m_rgb(rgb)
  {
    int cellW = 10;
    faint::Bitmap bg(IntSize(cellW*2,cellW*2), faint::color_white());
    const faint::Color alpha_gray(faint::grayscale_rgba(192));
    fill_rect_color( bg, IntRect(IntPoint(cellW,0), IntSize(cellW,cellW)), alpha_gray);
    fill_rect_color( bg, IntRect(IntPoint(0,cellW), IntSize(cellW,cellW)), alpha_gray);
    m_bgPattern = faint::DrawSource(faint::Pattern(bg));
  }
  void Draw( faint::Bitmap& bmp, const IntSize& size ) override{
    if ( !bitmap_ok(m_bitmap) || m_bitmap.GetSize() != size ){
      InitializeBitmap(size);
    }
    faint::clear(bmp, faint::Color(255,255,255));
    faint::fill_rect( bmp, IntRect(IntPoint(0,0), bmp.GetSize()), m_bgPattern );
    faint::blend( m_bitmap, onto(bmp), IntPoint(0,0));
  }
  SliderBackground* Clone() const override{
    return new AlphaBackground(*this);
  }
private:
  void InitializeBitmap( const IntSize& size ){
    m_bitmap = faint::alpha_gradient_bitmap(m_rgb, size);
  }
  faint::Bitmap m_bitmap;
  faint::DrawSource m_bgPattern;
  faint::ColRGB m_rgb;
};

class LightnessBackground : public SliderBackground{
public:
  explicit LightnessBackground( const faint::HS& hueSat )
    : m_hueSat(hueSat)
  {}

  void Draw( faint::Bitmap& bmp, const IntSize& size ) override{
    if ( !bitmap_ok(m_bitmap) || m_bitmap.GetSize() != size ){
      InitializeBitmap(size);
    }
    faint::blit( m_bitmap, onto(bmp), IntPoint(0,0) );
  }
  SliderBackground* Clone() const override{
    return new LightnessBackground(*this);
  }
private:
  void InitializeBitmap( const IntSize& size ){
    m_bitmap = lightness_gradient_bitmap(m_hueSat, size);
  }
  faint::Bitmap m_bitmap;
  faint::HS m_hueSat;
};

class HueSatPicker : public wxPanel {
public:
  HueSatPicker(wxWindow* parent) :
    wxPanel(parent, wxID_ANY),
    m_hslSize(241,241),
    m_hueSat(0.0,0.0)
  {
    SetBackgroundStyle( wxBG_STYLE_PAINT ); // Prevent flicker on full refresh
    SetInitialSize(m_hslSize);
    InitializeBitmap();
  }

  bool AcceptsFocus() const{
    return false;
  }

  bool AcceptsFocusFromKeyboard() const{
    return false;
  }

  faint::HS GetValue() const{
    return m_hueSat;
  }

  void Set( const faint::HS& hueSat ){
    m_hueSat = hueSat;
    Refresh();
  }

private:
  void InitializeBitmap(){
    m_bmp = to_wx_bmp(faint::hue_saturation_color_map(to_faint(m_hslSize) ));
  }

  void OnCaptureLost( wxMouseCaptureLostEvent& ){
    // Empty, but required on MSW.
  }

  void OnPaint( wxPaintEvent& ){
    wxPaintDC dc(this);
    dc.DrawBitmap(m_bmp, 0, 0);

    // Draw the marker
    dc.SetPen( wxPen(wxColour(0,0,0), 1, wxPENSTYLE_SOLID ) );
    int x = floored(( m_hueSat.h / 360.0 ) * 240.0);
    int y = floored(240 - m_hueSat.s * 240);
    dc.DrawLine( x, y - 10, x, y + 10 );
    dc.DrawLine( x - 10, y, x + 10, y );
  }

  void OnLeftDown( wxMouseEvent& event ){
    SetFromPos(event.GetPosition() );
    CaptureMouse();
  }

  void OnLeftUp( wxMouseEvent& ){
    if ( HasCapture() ){
      ReleaseMouse();
    }
  }

  void OnMotion( wxMouseEvent& event ){
    if ( HasCapture() ){
      SetFromPos(event.GetPosition());
    }
  }

  void SetFromPos( const wxPoint& pos ){
    int viewHue = std::max(0, std::min(pos.x, 240));
    m_hueSat.h = std::min((viewHue / 240.0) * 360.0, 359.0); // Fixme
    m_hueSat.s = 1.0 - std::max(0, std::min(pos.y, 240)) / 240.0;
    Refresh();
    wxCommandEvent newEvent( EVT_PICKED_HUE_SAT );
    newEvent.SetEventObject(this);
    ProcessEvent(newEvent);
  }

  wxBitmap m_bmp;
  wxSize m_hslSize;
  faint::HS m_hueSat;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(HueSatPicker, wxPanel)
EVT_MOUSE_CAPTURE_LOST(HueSatPicker::OnCaptureLost)
EVT_PAINT(HueSatPicker::OnPaint)
EVT_LEFT_DOWN(HueSatPicker::OnLeftDown)
EVT_LEFT_UP(HueSatPicker::OnLeftUp)
EVT_MOTION(HueSatPicker::OnMotion)
END_EVENT_TABLE()

class ColorPanel_HSL_Impl : public wxPanel{
public:
  ColorPanel_HSL_Impl( wxWindow* parent )
    : wxPanel(parent, wxID_ANY)
  {
    using faint::panel_padding;
    m_hueSatPicker = new HueSatPicker(this);
    m_hueSatPicker->SetPosition(wxPoint(panel_padding, panel_padding));
    wxStaticText* lblHue = new wxStaticText(this, wxID_ANY, "&Hue");
    m_hueTxt = ConnectKillFocus(CreateTextControl( IntRange(min_t(0),max_t(240)), below(m_hueSatPicker) ));
    PlaceLabel(lblHue, m_hueTxt, true);
    wxStaticText* lblSat = new wxStaticText(this, wxID_ANY, "&Sat");
    m_saturationTxt = ConnectKillFocus(CreateTextControl( IntRange(min_t(0),max_t(240)), below(m_hueTxt) ));
    PlaceLabel(lblSat, m_saturationTxt, false);
    m_lightnessSlider = new Slider(this,
      ClosedIntRange(min_t(0),max_t(240)),
      120,
      SliderDir::VERTICAL,
      LightnessBackground(m_hueSatPicker->GetValue()) );
    m_lightnessSlider->SetPosition(to_the_right_of(m_hueSatPicker));
    m_lightnessSlider->SetSize(wxSize(20,240));
    m_alphaSlider = new Slider(this, ClosedIntRange(min_t(0), max_t(255)),
      128,
      SliderDir::VERTICAL,
      AlphaBackground(faint::ColRGB(128,128,128)));
    m_alphaSlider->SetPosition(to_the_right_of(m_lightnessSlider));
    m_alphaSlider->SetSize(wxSize(20,255));
    m_colorBitmap = new wxStaticBitmap(this, wxID_ANY, wxBitmap(120,120));
    m_colorBitmap->SetPosition(to_the_right_of(m_alphaSlider));

    wxStaticText* lblLightness = new wxStaticText(this, wxID_ANY, "&Lightness");
    m_lightnessTxt = ConnectKillFocus(CreateTextControl(IntRange(min_t(0),max_t(240))));
    wxPoint lightnessTxtPos = m_lightnessSlider->GetPosition();
    wxSize lpSize = m_lightnessSlider->GetSize();
    lightnessTxtPos.x += lpSize.x - m_lightnessTxt->GetSize().x;
    lightnessTxtPos.y += m_lightnessSlider->GetSize().y + 5;
    m_lightnessTxt->SetPosition(lightnessTxtPos);
    PlaceLabel(lblLightness, m_lightnessTxt);

    // RGBA text-boxes
    wxStaticText* lblRed = new wxStaticText(this, wxID_ANY, "&R");
    m_redTxt = ConnectKillFocus(CreateTextControl(IntRange(min_t(0), max_t(255))));
    wxPoint redPos = m_colorBitmap->GetPosition();
    redPos.x = redPos.x + m_colorBitmap->GetSize().x - m_redTxt->GetSize().x;
    redPos.y += m_colorBitmap->GetSize().y + 5;
    m_redTxt->SetPosition(redPos);
    PlaceLabel(lblRed, m_redTxt);

    wxStaticText* lblGreen = new wxStaticText(this, wxID_ANY, "&G");
    wxPoint greenPos(redPos.x, redPos.y + m_redTxt->GetSize().y + 5 );
    m_greenTxt = ConnectKillFocus(CreateTextControl(IntRange(min_t(0), max_t(255)), greenPos));
    PlaceLabel(lblGreen, m_greenTxt);

    wxStaticText* lblBlue = new wxStaticText(this, wxID_ANY, "&B");
    wxPoint bluePos(greenPos.x, greenPos.y + m_greenTxt->GetSize().y + 5 );
    m_blueTxt = ConnectKillFocus(CreateTextControl(IntRange(min_t(0), max_t(255)), bluePos));
    PlaceLabel(lblBlue, m_blueTxt);

    wxStaticText* lblAlpha = new wxStaticText(this, wxID_ANY, "&A");
    wxPoint alphaPos(bluePos.x, bluePos.y + m_blueTxt->GetSize().y + 5 );
    m_alphaTxt = ConnectKillFocus(CreateTextControl(IntRange(min_t(0), max_t(255)), alphaPos));
    PlaceLabel(lblAlpha, m_alphaTxt);

    m_lightnessSlider->Connect(wxID_ANY, FAINT_SLIDER_CHANGE, SliderEventHandler(ColorPanel_HSL_Impl::OnPickedLightness), 0, this);
    m_alphaSlider->Connect(wxID_ANY, FAINT_SLIDER_CHANGE, SliderEventHandler(ColorPanel_HSL_Impl::OnPickedAlpha), 0, this);

    SetInitialSize(wxSize(right_side(m_colorBitmap) + panel_padding, bottom(m_saturationTxt) + panel_padding));
    SetColor(faint::Color(0,0,128,255));

    Connect(wxID_ANY, EVT_PICKED_HUE_SAT, wxCommandEventHandler(ColorPanel_HSL_Impl::OnPickedHueSat));

    Connect(wxID_ANY, wxEVT_COMMAND_TEXT_UPDATED, wxTextEventHandler(ColorPanel_HSL_Impl::OnTextEntry));
  }

  ~ColorPanel_HSL_Impl(){
    m_lightnessSlider->Disconnect(wxID_ANY, FAINT_SLIDER_CHANGE);
    m_alphaSlider->Disconnect(wxID_ANY, FAINT_SLIDER_CHANGE);
  }

  faint::Color GetColor() const{
    faint::Color c(faint::to_rgba( faint::HSL( m_hueSatPicker->GetValue(),
      m_lightnessSlider->GetValue() / 240.0 ),
      int(m_alphaSlider->GetValue()) ) );
    return c;
  }

  void SetColor( const faint::Color& color ){
    faint::HSL hsl( to_hsl(color.GetRGB()) );
    m_hueSatPicker->Set( hsl.GetHS());
    m_lightnessSlider->SetBackground( LightnessBackground( hsl.GetHS() ) );
    m_lightnessSlider->SetValue( floored(hsl.l * 240.0) ); // Fixme: Nasty conversion
    m_alphaSlider->SetBackground( AlphaBackground(color.GetRGB()) );
    m_alphaSlider->SetValue(color.a);
    UpdateColorBitmap();
    UpdateRGBA();
    UpdateHSL();
  }
private:
  wxTextCtrl* ConnectKillFocus(wxTextCtrl* ctrl){
    ctrl->Connect(wxID_ANY, wxEVT_KILL_FOCUS, wxFocusEventHandler(ColorPanel_HSL_Impl::OnTextCtrlKillFocus), 0, this);
    return ctrl;
  }

  wxTextCtrl* CreateTextControl( const IntRange& range, const wxPoint& pos=wxDefaultPosition ){
    wxTextCtrl* t = new wxTextCtrl(this, wxID_ANY, "", pos, wxSize(50,-1));
    t->SetMaxLength(3);
    m_ranges[t] = range;
    return t;
  }

  void OnPickedLightness(SliderEvent& event){
    int lightness = event.GetValue(); // Fixme: Range 0-240, HSL is 0-1.0
    faint::HSL hsl(m_hueSatPicker->GetValue(), lightness / 240.0 );
    m_alphaSlider->SetBackground( AlphaBackground(to_rgb(hsl)) );
    UpdateColorBitmap();
    m_lightnessTxt->ChangeValue(str_int(lightness));
    UpdateRGBA();
    Refresh();
  }

  void OnPickedAlpha(SliderEvent& event){
    UpdateColorBitmap();
    std::stringstream ss;
    ss << static_cast<int>(event.GetValue());
    m_alphaTxt->ChangeValue(ss.str());
    Refresh();
  }

  void OnPickedHueSat(wxCommandEvent&){
    faint::HS hueSat( m_hueSatPicker->GetValue() );
    m_lightnessSlider->SetBackground( LightnessBackground( hueSat ) );
    faint::ColRGB rgb( faint::to_rgb( faint::HSL( hueSat, m_lightnessSlider->GetValue() / 240.0 ) ) ); // Fixme: Nasty conversion
    m_alphaSlider->SetBackground( AlphaBackground(rgb));
    UpdateColorBitmap();
    std::stringstream ss;
    ss << int((hueSat.h / 360) * 240); // Fixme
    m_hueTxt->ChangeValue(ss.str());
    ss.str("");
    ss << int(hueSat.s * 240);
    m_saturationTxt->ChangeValue(ss.str());
    UpdateRGBA();
    // Fixme: Update RGBA text
  }

  void OnTextCtrlKillFocus(wxFocusEvent& e){
    // Propagate event to clear selection indicator etc.
    e.Skip();

    // Set the value to 0 if invalid
    wxTextCtrl* textCtrl( dynamic_cast<wxTextCtrl*>(e.GetEventObject()) );
    wxString text = textCtrl->GetValue();
    if ( text.empty() || !text.IsNumber() || text.Contains("-") ){
      textCtrl->SetValue("0");
      return;
    }

    int value = faint::parse_int_value(textCtrl, 0);
    IntRange& range(m_ranges[textCtrl]);
    if ( !range.Has(value) ){
      std::stringstream ss;
      ss << range.Constrain(value);
      textCtrl->SetValue(ss.str());
    }
  }

  void OnTextEntry(wxCommandEvent& evt){
    wxTextCtrl* ctrl = dynamic_cast<wxTextCtrl*>(evt.GetEventObject());
    IntRange range( m_ranges[ctrl] );

    int value = range.Constrain(faint::parse_int_value(ctrl,0));

    if ( ctrl == m_hueTxt ){
      faint::HS hs = m_hueSatPicker->GetValue();
      hs.h = std::min((value / 240.0 ) * 360.0, 359.0); // Fixme
      m_hueSatPicker->Set(hs);
      m_lightnessSlider->SetBackground( LightnessBackground( hs ) );
      UpdateColorBitmap();
    }
    else if ( ctrl == m_saturationTxt ){
      faint::HS hs = m_hueSatPicker->GetValue();
      hs.s = value / 240.0;
      m_hueSatPicker->Set(hs);
      m_lightnessSlider->SetBackground( LightnessBackground( hs ) );
      UpdateColorBitmap();
    }
    else if ( ctrl == m_lightnessTxt ){
      m_lightnessSlider->SetValue(value);
      m_alphaSlider->SetBackground(AlphaBackground(GetColor().GetRGB()));
      UpdateColorBitmap();
    }
    else if ( ctrl == m_alphaTxt ){
      m_alphaSlider->SetValue(value);
      UpdateColorBitmap();
    }
    else if ( ctrl == m_redTxt || ctrl == m_greenTxt || ctrl == m_blueTxt ){
      faint::ColRGB rgb(faint::rgb_from_ints(faint::parse_int_value(m_redTxt, 0),
        faint::parse_int_value(m_greenTxt, 0),
        faint::parse_int_value(m_blueTxt, 0)));
      faint::HSL hsl(faint::to_hsl(rgb));
      m_hueSatPicker->Set( hsl.GetHS() );
      m_lightnessSlider->SetBackground( LightnessBackground( hsl.GetHS() ) );
      m_lightnessSlider->SetValue( floored(hsl.l * 240.0) ); // Fixme: Conversion
      m_alphaSlider->SetBackground( AlphaBackground(rgb) );
      UpdateColorBitmap();
      UpdateHSL();
    }
  }

  void UpdateRGBA(){
    faint::Color c(GetColor());
    m_redTxt->ChangeValue(str_int(c.r));
    m_greenTxt->ChangeValue(str_int(c.g));
    m_blueTxt->ChangeValue(str_int(c.b));
    m_alphaTxt->ChangeValue(str_int(c.a));
  }

  void UpdateHSL(){
    faint::Color c(GetColor());
    faint::HSL hsl(faint::to_hsl(c.GetRGB()));
    m_hueTxt->ChangeValue( str_int(truncated((hsl.h / 360.0) * 240.0 ) ) ); // Fixme
    m_saturationTxt->ChangeValue( str_int(truncated(hsl.s * 240.0 ) ) ); // Fixme
    m_lightnessTxt->ChangeValue( str_int( truncated( hsl.l * 240.0 ) ) ); // Fixme
  }

  void UpdateColorBitmap(){
    faint::Color c(GetColor());
    faint::Bitmap bmp(color_bitmap(GetColor(), to_faint(m_colorBitmap->GetSize() )) );
    m_colorBitmap->SetBitmap(to_wx_bmp(bmp));
  }

  void PlaceLabel(wxStaticText* label, wxWindow* control, bool shift=false){
    // Place the label to the left of the control.
    // Note: I previously just had an AddLabel-function, but the label
    // would then end up after the (already-created) control in the
    // tab order. Hence, the label must be created before the
    // control. I tried wxWindows::MoveBeforeInTabOrder, but this did
    // not affect traversal with mnemonics, so e.g. Alt+H for the hue
    // label ("&Hue" ) would select the saturation text field.
    wxPoint ctrlPos(control->GetPosition());
    wxSize ctrlSize(control->GetSize());
    wxSize lblSize(label->GetSize());
    if ( shift ){
      control->SetPosition( wxPoint(ctrlPos.x + lblSize.x + 5, ctrlPos.y ) );
      label->SetPosition(wxPoint(ctrlPos.x,
        ctrlPos.y + ctrlSize.y / 2 - lblSize.y / 2 ));
    }
    else {
      wxPoint lblPos(ctrlPos.x - lblSize.x - 5,
        ctrlPos.y + ctrlSize.y / 2 - lblSize.y / 2 );
      label->SetPosition(lblPos);
    }
  }

  Slider* m_alphaSlider;
  wxTextCtrl* m_alphaTxt;
  wxTextCtrl* m_blueTxt;
  wxStaticBitmap* m_colorBitmap;
  wxTextCtrl* m_greenTxt;
  HueSatPicker* m_hueSatPicker;
  wxTextCtrl* m_hueTxt;
  Slider* m_lightnessSlider;
  wxTextCtrl* m_lightnessTxt;
  std::map<wxTextCtrl*, IntRange> m_ranges;
  wxTextCtrl* m_redTxt;
  wxTextCtrl* m_saturationTxt;
};

ColorPanel_HSL::ColorPanel_HSL(wxWindow* parent){
  m_impl = new ColorPanel_HSL_Impl(parent);
}

ColorPanel_HSL::~ColorPanel_HSL(){
  m_impl = nullptr; // Deletion handled by wxWidgets
}

faint::Color ColorPanel_HSL::GetColor() const{
  return m_impl->GetColor();
}

void ColorPanel_HSL::SetColor( const faint::Color& color ){
  m_impl->SetColor(color);
}

wxWindow* ColorPanel_HSL::AsWindow(){
  return m_impl;
}

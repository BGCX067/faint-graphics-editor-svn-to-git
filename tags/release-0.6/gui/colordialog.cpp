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
#include <map>
#include "colordialog.hh"
#include "geo/range.hh"
#include "util/colorutil.hh"
#include "util/convertwx.hh"
#include "util/guiutil.hh"
#include "wx/wx.h"

DECLARE_EVENT_TYPE(EVT_PICKED_HUE_SAT,-1)
DEFINE_EVENT_TYPE(EVT_PICKED_HUE_SAT)

DECLARE_EVENT_TYPE(EVT_PICKED_LIGHTNESS,-1)
DEFINE_EVENT_TYPE(EVT_PICKED_LIGHTNESS)

DECLARE_EVENT_TYPE(EVT_PICKED_ALPHA,-1)
DEFINE_EVENT_TYPE(EVT_PICKED_ALPHA)

enum ColorUpdateMode{ UPDATE_RGB=0, IGNORE_RGB=1 };

ColorDialogCallback::~ColorDialogCallback(){
  // Virtual destructor
}

void ColorDialogCallback::OnColor(const faint::Color&){
  // Default callback does nothing
}

unsigned char apply_sat( int c, int saturation ){
  return static_cast<unsigned char>(127 + saturation * ( c - 127 ) / 240.0); // fixme: Truncate?
}

wxPoint below( wxWindow* window){
  return wxPoint(window->GetPosition().x, window->GetPosition().y + window->GetSize().y + 5);
}

int bottom( wxWindow* window ){
  return window->GetPosition().y + window->GetSize().y;
}

int right_side( wxWindow* window ){
  return window->GetPosition().x + window->GetSize().x;
}

wxPoint to_the_right_of( wxWindow* window ){
  return wxPoint(window->GetPosition().x + window->GetSize().x + 5, window->GetPosition().y );
}

class AlphaPicker : public wxPanel {
public:
  AlphaPicker(wxWindow* parent, const wxColour& sourceColor )
    : wxPanel(parent, wxID_ANY),
      m_alpha(255),
      m_alphaSize(20,256),
      m_bitmap(m_alphaSize),
      m_sourceColor(sourceColor)
  {
    SetBackgroundStyle( wxBG_STYLE_PAINT ); // Prevent flicker on full refresh
    SetInitialSize(m_alphaSize);
    InitializeBitmap();
  }

  bool AcceptsFocus() const{
    return false;
  }

  bool AcceptsFocusFromKeyboard() const{
    return false;
  }

  faint::uchar GetAlpha() const{
    return static_cast<faint::uchar>(m_alpha);
  }

  void SetAlpha( int alpha, ColorUpdateMode mode ){
    m_alpha = std::min(255, std::max(0, alpha));
    wxCommandEvent newEvent( EVT_PICKED_ALPHA );
    newEvent.SetInt((int)mode);
    newEvent.SetEventObject(this);
    ProcessEvent(newEvent);
    Refresh();
  }

  void SetSourceColor( const wxColour& sourceColor ){
    m_sourceColor = sourceColor;
    InitializeBitmap();
  }
private:
  void InitializeBitmap(){
    wxMemoryDC dc(m_bitmap);
    wxColour bg = GetBackgroundColour();
    wxColour c = m_sourceColor;

    int rBg = bg.Red();
    int gBg = bg.Green();
    int bBg = bg.Blue();
    int rSrc = m_sourceColor.Red();
    int gSrc = m_sourceColor.Green();
    int bSrc = m_sourceColor.Blue();

    const int width = m_alphaSize.GetWidth();
    const int height = m_alphaSize.GetHeight();
    for ( int y = 0; y <= height; y++ ){
      wxColour color((rSrc * y + rBg * ( 255 - y )) / 255,
	(gSrc * y + gBg * ( 255 - y )) / 255,
	(bSrc * y + bBg * ( 255 - y )) / 255);
      dc.SetPen(wxPen(color));
      dc.DrawLine(0,255 - y, width, 255 - y );
    }

    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen( wxPen(wxColour(128,128,128) ) );
    dc.DrawRectangle(0,0, width, height);
  }

  void OnLeftDown(wxMouseEvent& event){
    CaptureMouse();
    SetAlpha( 255 - event.GetPosition().y, UPDATE_RGB );
  }

  void OnLeftUp(wxMouseEvent& ){
    if ( HasCapture() ){
      ReleaseMouse();
    }
  }

  void OnMotion(wxMouseEvent& event){
    if ( HasCapture() ){
      SetAlpha( 255 - event.GetPosition().y, UPDATE_RGB );
    }
  }

  void OnPaint( wxPaintEvent& ){
    wxPaintDC dc(this);
    dc.DrawBitmap(m_bitmap,0,0);
    dc.DrawLine( 0, 255 - m_alpha, m_alphaSize.x, 255 - m_alpha );
  }

  int m_alpha;
  wxSize m_alphaSize;
  wxBitmap m_bitmap;
  wxColour m_sourceColor;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(AlphaPicker, wxPanel)
EVT_PAINT(AlphaPicker::OnPaint)
EVT_LEFT_DOWN(AlphaPicker::OnLeftDown)
EVT_LEFT_UP(AlphaPicker::OnLeftUp)
EVT_MOTION(AlphaPicker::OnMotion)
END_EVENT_TABLE()

class LightnessPicker : public wxPanel {
public:
  LightnessPicker(wxWindow* parent, const wxColour& centerColor) :
    wxPanel(parent, wxID_ANY),
    m_centerColor(centerColor),
    m_lightness(120),
    m_lightnessSize(20,241)
  {
    SetBackgroundStyle( wxBG_STYLE_PAINT ); // Prevent flicker on full refresh
    SetInitialSize(m_lightnessSize);
    InitializeBitmap();
  }

  bool AcceptsFocus() const{
    return false;
  }
  bool AcceptsFocusFromKeyboard() const{
    return false;
  }

  wxColour GetColor() const{
    wxBitmap bmp2(m_bitmap);
    wxMemoryDC dc(bmp2);
    wxColour c;
    dc.GetPixel(0, 240 - m_lightness, &c);
    return c;
  }

  int GetLightness() const{
    return m_lightness;
  }

  void SetCenterColor( const wxColour& centerColor, ColorUpdateMode mode ){
    m_centerColor = centerColor;
    InitializeBitmap();
    wxCommandEvent newEvent( EVT_PICKED_LIGHTNESS );
    newEvent.SetInt((int)mode);
    newEvent.SetEventObject(this);
    ProcessEvent(newEvent);
    Refresh();
  }

  void SetLightness(int lightness, ColorUpdateMode mode) {
    m_lightness = lightness;
    wxCommandEvent newEvent( EVT_PICKED_LIGHTNESS );
    newEvent.SetInt((int)mode);
    newEvent.SetEventObject(this);
    ProcessEvent(newEvent);
    Refresh();
  }
private:
  void InitializeBitmap(){
    m_bitmap = wxBitmap(m_lightnessSize);
    wxMemoryDC dc(m_bitmap);
    for ( int L = 0; L <= 120; L++ ){
      wxColour c((m_centerColor.Red() / 120.0 ) * L,
	(m_centerColor.Green() / 120.0 ) * L,
	(m_centerColor.Blue() / 120.0 ) * L);
      dc.SetPen( wxPen( c, 1, wxPENSTYLE_SOLID ) );
      dc.DrawLine(0, m_lightnessSize.GetHeight() - L - 1, m_lightnessSize.GetWidth(), m_lightnessSize.GetHeight() - L - 1 );
    }

    for ( int L = 121; L <= 241; L++ ){
      wxColour c( ( (255 - m_centerColor.Red() ) / 120.0 ) * (L - 120) + m_centerColor.Red(),
	( (255 - m_centerColor.Green() ) / 120.0 ) * (L - 120 ) + m_centerColor.Green(),
	( ( 255 - m_centerColor.Blue() ) / 120.0 ) * (L - 120) + m_centerColor.Blue());
      dc.SetPen( wxPen( c, 1, wxPENSTYLE_SOLID ) );
      dc.DrawLine(0, m_lightnessSize.GetHeight() - L - 1, m_lightnessSize.GetWidth(), m_lightnessSize.GetHeight() - L - 1 );
    }
  }

  void OnLeftDown( wxMouseEvent& event ){
    SetFromPos(event.GetPosition() );
    CaptureMouse();
  }

  void OnLeftUp(wxMouseEvent&){
    if ( HasCapture() ){
      ReleaseMouse();
    }
  }

  void OnMotion( wxMouseEvent& event ){
    if ( HasCapture() ){
      SetFromPos(event.GetPosition());
    }
  }

  void OnPaint(wxPaintEvent&){
    wxPaintDC dc(this);
    dc.DrawBitmap(m_bitmap, 0, 0);
    int y = 240 - m_lightness;
    dc.DrawLine(0, y, m_lightnessSize.GetWidth(), y );
  }

  void SetFromPos( const wxPoint& pos ){
    m_lightness = std::min( 240, std::max(0, 240 - pos.y) );
    Refresh();
    wxCommandEvent newEvent( EVT_PICKED_LIGHTNESS );
    newEvent.SetEventObject(this);
    ProcessEvent(newEvent);
  }

  wxBitmap m_bitmap;
  wxColour m_centerColor;
  int m_lightness;
  wxSize m_lightnessSize;

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(LightnessPicker, wxPanel)
EVT_PAINT(LightnessPicker::OnPaint)
EVT_LEFT_DOWN(LightnessPicker::OnLeftDown)
EVT_LEFT_UP(LightnessPicker::OnLeftUp)
EVT_MOTION(LightnessPicker::OnMotion)
END_EVENT_TABLE()

class HueSatPicker : public wxPanel {
public:
  HueSatPicker(wxWindow* parent) :
    wxPanel(parent, wxID_ANY),
    m_hsvSize(241,241),
    m_hue(0),
    m_saturation(0)
  {
    SetBackgroundStyle( wxBG_STYLE_PAINT ); // Prevent flicker on full refresh
    SetInitialSize(m_hsvSize);
    InitializeBitmap();
  }

  bool AcceptsFocus() const{
    return false;
  }

  bool AcceptsFocusFromKeyboard() const{
    return false;
  }

  wxColour GetColor(){
    wxMemoryDC dc(m_bmp);
    wxColour c;
    dc.GetPixel(m_hue, 240 - m_saturation, &c);
    return c;
  }

  int GetHue() const {
    return m_hue;
  }

  int GetSaturation() const {
    return m_saturation;
  }

  void Set( int hue, int saturation, ColorUpdateMode mode ){
    m_hue = std::max(0, std::min(240, hue) );
    m_saturation = std::max(0, std::min(240, saturation));
    Refresh();
    wxCommandEvent newEvent( EVT_PICKED_HUE_SAT );
    newEvent.SetInt((int)mode);
    newEvent.SetEventObject(this);
    ProcessEvent(newEvent);
  }
private:
  void InitializeBitmap(){
    m_bmp = wxBitmap( m_hsvSize );
    // (At lightness 120)
    wxMemoryDC dc(m_bmp);
    int maxSaturation = 240;
    for ( int saturation = 240; saturation >= 0; saturation-- ){
      for ( int hue = 0; hue <= 40; hue++ ){
	const int red = 255;
	const int green = ( 255 / 40.0 ) * hue;
	const int blue = 0;
	int x = hue;
	int y = maxSaturation - saturation;

	wxColour color(apply_sat(red, saturation),
	  apply_sat(green, saturation),
	  apply_sat(blue, saturation));

	dc.SetPen( wxPen(color, 1, wxPENSTYLE_SOLID ) );
	dc.DrawPoint(x,y);
      }
      for ( int hue = 41; hue <= 80; hue++ ){
	const int red = 255 - ( 255 / 40.0 ) * (hue - 40);
	const int green = 255;
	const int blue = 0;
	int x = hue;
	int y = maxSaturation - saturation;
	wxColour color(apply_sat(red, saturation),
	  apply_sat(green, saturation),
	  apply_sat(blue, saturation));

	dc.SetPen( wxPen(color, 1, wxPENSTYLE_SOLID ) );
	dc.DrawPoint(x,y);
      }
      for ( int hue = 81; hue <= 120; hue++ ){
	const int red = 0;
	const int green = 255;
	const int blue = ( 255 / 40.0 ) * (hue - 80);
	int x = hue;
	int y = maxSaturation - saturation;

	wxColour color(apply_sat(red, saturation),
	  apply_sat(green, saturation),
	  apply_sat(blue, saturation));

	dc.SetPen( wxPen( color, 1, wxPENSTYLE_SOLID ) );
	dc.DrawPoint(x,y);
      }
      for ( int hue = 121; hue <= 160; hue++ ){
	const int red = 0;
	const int green = 255 - (255 / 40.0) * (hue - 120);
	const int blue = 255;
	int x = hue;
	int y = maxSaturation - saturation;
	wxColour color(apply_sat(red, saturation),
	  apply_sat(green, saturation),
	  apply_sat(blue, saturation));

	dc.SetPen( wxPen(color, 1, wxPENSTYLE_SOLID ) );
	dc.DrawPoint(x,y);
      }
      for ( int hue = 161; hue <= 200; hue++ ){
	const int red = (255 / 40) * (hue - 160);
	const int green = 0;
	const int blue = 255;
	int x = hue;
	int y = maxSaturation - saturation;
	wxColour color(apply_sat(red, saturation),
	  apply_sat(green, saturation),
	  apply_sat(blue, saturation));

	dc.SetPen( wxPen(color, 1, wxPENSTYLE_SOLID ) );
	dc.DrawPoint(x,y);
      }
      for ( int hue = 201; hue <= 240; hue++ ){
	const int red = 255;
	const int green = 0;
	const int blue = 255 - ( 255 / 40.0) * (hue - 200);
	int x = hue;
	int y = maxSaturation - saturation;
	wxColour color(apply_sat(red, saturation),
	  apply_sat(green, saturation),
	  apply_sat(blue, saturation));

	dc.SetPen( wxPen(color, 1, wxPENSTYLE_SOLID ) );
	dc.DrawPoint(x,y);
      }
    }
  }

  void OnPaint( wxPaintEvent& ){
    wxPaintDC dc(this);
    dc.DrawBitmap(m_bmp, 0, 0);
    dc.SetPen( wxPen(wxColour(0,0,0), 1, wxPENSTYLE_SOLID ) );
    dc.DrawLine(m_hue, 240 - m_saturation - 10, m_hue, 240 - m_saturation + 10 );   dc.DrawLine(m_hue - 10, 240 - m_saturation, m_hue + 10, 240 - m_saturation );
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
    m_saturation = std::min(240, std::max(240 - pos.y,0));
    m_hue = std::max(0, std::min(240, pos.x));
    Refresh();
    wxCommandEvent newEvent( EVT_PICKED_HUE_SAT );
    newEvent.SetInt(UPDATE_RGB);
    newEvent.SetEventObject(this);
    ProcessEvent(newEvent);
  }

  wxBitmap m_bmp;
  wxSize m_hsvSize;
  int m_hue;
  int m_saturation;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(HueSatPicker, wxPanel)
EVT_PAINT(HueSatPicker::OnPaint)
EVT_LEFT_DOWN(HueSatPicker::OnLeftDown)
EVT_LEFT_UP(HueSatPicker::OnLeftUp)
EVT_MOTION(HueSatPicker::OnMotion)
END_EVENT_TABLE()

bool ValidNumber( const wxString& wxStr ){
  std::string s(wxStr);
  for ( size_t i = 0; i != s.size(); i++ ){
    if ( !isdigit(s[i] ) ){
      return false;
    }
  }
  return true;
}

int ParseValue( const wxString& wxStr ){
  std::string s(wxStr);
  std::stringstream ss(s);
  int i = 0;
  ss >> i;
  return i;
}

class ColorDialogImpl : public wxDialog {
public:
  ColorDialogImpl( wxWindow* parent, const wxString& title, ColorDialogCallback& callback )
    : wxDialog( parent, wxID_ANY, title ),
      m_callback(callback)
  {
    m_hueSatPicker = new HueSatPicker(this);
    m_hueSatPicker->SetPosition(wxPoint(10,10));

    wxStaticText* lblHue = new wxStaticText(this, wxID_ANY, "&Hue");
    m_hueTxt = CreateTextControl( IntRange(min_t(0),max_t(240)), below(m_hueSatPicker) );
    PlaceLabel(lblHue, m_hueTxt, true);

    wxStaticText* lblSat = new wxStaticText(this, wxID_ANY, "&Sat");
    m_saturationTxt = CreateTextControl( IntRange(min_t(0),max_t(240)), below(m_hueTxt) );
    PlaceLabel(lblSat, m_saturationTxt, false);
    m_lightnessPicker = new LightnessPicker(this,wxColour(255,0,0));
    m_lightnessPicker->SetPosition(to_the_right_of(m_hueSatPicker));
    m_alphaPicker = new AlphaPicker(this, wxColor(255,0,0));
    m_alphaPicker->SetPosition(to_the_right_of(m_lightnessPicker));
    m_colorBitmap = new wxStaticBitmap(this, wxID_ANY, wxBitmap(120,120));
    m_colorBitmap->SetPosition(to_the_right_of(m_alphaPicker));

    wxStaticText* lblLightness = new wxStaticText(this, wxID_ANY, "&Lightness");
    m_lightnessTxt = CreateTextControl(IntRange(min_t(0),max_t(240)));
    wxPoint lightnessTxtPos = m_lightnessPicker->GetPosition();
    wxSize lpSize = m_lightnessPicker->GetSize();
    lightnessTxtPos.x += lpSize.x - m_lightnessTxt->GetSize().x;
    lightnessTxtPos.y += m_lightnessPicker->GetSize().y + 5;
    wxSize lpTextSize(m_lightnessPicker->GetSize());
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

    wxButton* okBtn = new wxButton(this, wxID_OK);
    wxButton* cancelButton = new wxButton( this, wxID_CANCEL );
    // Fixme: OS dependent!
    cancelButton->SetPosition(wxPoint(right_side(m_colorBitmap) - cancelButton->GetSize().x, bottom(m_lightnessTxt) + 5));
    okBtn->SetPosition(wxPoint(cancelButton->GetPosition().x - okBtn->GetSize().x, cancelButton->GetPosition().y));
    Centre(wxBOTH); // Center on parent

    SetClientSize(wxSize(right_side(m_colorBitmap) + 10, bottom(cancelButton ) + 10 ));
    SetDefaultItem(okBtn);
    m_hueSatPicker->Set( 100, 100, UPDATE_RGB );
  }

  faint::Color GetColor() const{
    wxColour c = m_lightnessPicker->GetColor();
    return faint::Color(c.Red(), c.Green(), c.Blue(), m_alphaPicker->GetAlpha()); // Fixme: This must exactly match R,G,B
  }

  void SetColor( const faint::Color& color, ColorUpdateMode mode ){
    m_hueSatPicker->Set( get_hue(color),get_saturation(color), mode);
    m_lightnessPicker->SetLightness( get_lightness(color), mode);
    m_alphaPicker->SetAlpha(color.a, mode);
  }
private:
  wxTextCtrl* ConnectKillFocus(wxTextCtrl* ctrl){
    ctrl->Connect(wxID_ANY, wxEVT_KILL_FOCUS, wxFocusEventHandler(ColorDialogImpl::OnTextCtrlKillFocus), 0, this);
    return ctrl;
  }

  wxTextCtrl* CreateTextControl( const IntRange& range, const wxPoint& pos=wxDefaultPosition ){
    wxTextCtrl* t = new wxTextCtrl(this, wxID_ANY, "", pos, wxSize(50,-1));
    t->SetMaxLength(3);
    m_ranges[t] = range;
    return t;
  }

  void OnPickedAlpha(wxCommandEvent&){
    m_colorBitmap->SetBitmap(color_bitmap(GetColor(), to_faint(m_colorBitmap->GetSize() ), false ) );
    std::stringstream ss;
    ss << static_cast<int>(m_alphaPicker->GetAlpha());
    m_alphaTxt->ChangeValue(ss.str());
    Refresh();
  }

  void OnPickedHueSat(wxCommandEvent& e){
    m_lightnessPicker->SetCenterColor(m_hueSatPicker->GetColor(), ColorUpdateMode(e.GetInt()));
    m_colorBitmap->SetBitmap(color_bitmap(GetColor(), to_faint(m_colorBitmap->GetSize() ), false ) );
    std::stringstream ss;
    ss << m_hueSatPicker->GetHue();
    m_hueTxt->ChangeValue(ss.str());
    ss.str("");
    ss << m_hueSatPicker->GetSaturation();
    m_saturationTxt->ChangeValue(ss.str());
  }

  void OnPickedLightness(wxCommandEvent& e){
    wxColour color = m_lightnessPicker->GetColor();
    m_alphaPicker->SetSourceColor(color);
    m_colorBitmap->SetBitmap(color_bitmap(GetColor(), to_faint(m_colorBitmap->GetSize() ), false ) );

    std::stringstream ss;
    ss << m_lightnessPicker->GetLightness();
    m_lightnessTxt->ChangeValue(ss.str());
    ss.str("");
    ColorUpdateMode mode(ColorUpdateMode(e.GetInt()));
    if ( mode == UPDATE_RGB ){
      ss << static_cast<int>(color.Red());
      m_redTxt->ChangeValue(ss.str());
      ss.str("");
      ss << static_cast<int>(color.Green());
      m_greenTxt->ChangeValue(ss.str());
      ss.str("");
      ss << static_cast<int>(color.Blue());
      m_blueTxt->ChangeValue(ss.str());

      ss.str("");
      ss << static_cast<int>(m_alphaPicker->GetAlpha());
      m_alphaTxt->ChangeValue(ss.str());
    }
    Refresh();
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

    int value = ParseValue(textCtrl->GetValue());
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

    wxString txt = ctrl->GetValue();
    int value = range.Constrain(ParseValue(txt));

    if ( ctrl == m_hueTxt ){
      m_hueSatPicker->Set(value, m_hueSatPicker->GetSaturation(), UPDATE_RGB);
    }
    else if ( ctrl == m_saturationTxt ){
      m_hueSatPicker->Set(m_hueSatPicker->GetHue(), value, UPDATE_RGB);
    }
    else if ( ctrl == m_lightnessTxt ){
      m_lightnessPicker->SetLightness(value, UPDATE_RGB);
    }
    else if ( ctrl == m_alphaTxt ){
      m_alphaPicker->SetAlpha(value, IGNORE_RGB);
    }
    else if ( ctrl == m_redTxt || ctrl == m_greenTxt || ctrl == m_blueTxt ){
      faint::Color color(faint::color_from_ints(ParseValue(m_redTxt->GetValue()),
      ParseValue(m_greenTxt->GetValue()),
      ParseValue(m_blueTxt->GetValue())));
      SetColor(color, IGNORE_RGB);
    }
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

  AlphaPicker* m_alphaPicker;
  wxTextCtrl* m_alphaTxt;
  wxTextCtrl* m_blueTxt;
  ColorDialogCallback& m_callback;
  wxStaticBitmap* m_colorBitmap;
  wxTextCtrl* m_greenTxt;
  HueSatPicker* m_hueSatPicker;
  wxTextCtrl* m_hueTxt;
  LightnessPicker* m_lightnessPicker;
  wxTextCtrl* m_lightnessTxt;
  std::map<wxTextCtrl*, IntRange> m_ranges;
  wxTextCtrl* m_redTxt;
  wxTextCtrl* m_saturationTxt;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ColorDialogImpl,wxDialog)
EVT_COMMAND(-1, EVT_PICKED_HUE_SAT, ColorDialogImpl::OnPickedHueSat)
EVT_COMMAND(-1, EVT_PICKED_LIGHTNESS, ColorDialogImpl::OnPickedLightness)
EVT_COMMAND(-1, EVT_PICKED_ALPHA, ColorDialogImpl::OnPickedAlpha)
EVT_TEXT(-1, ColorDialogImpl::OnTextEntry)
END_EVENT_TABLE()

ColorDialog::ColorDialog( wxWindow* parent ){
  m_impl = new ColorDialogImpl( parent, "Color", m_defaultCallback );
}

ColorDialog::ColorDialog( wxWindow* parent, ColorDialogCallback& callback ){
  m_impl = new ColorDialogImpl( parent, "Color", callback );
}

ColorDialog::~ColorDialog(){
  delete m_impl;
}

faint::Color ColorDialog::GetColor() const{
  return m_impl->GetColor();
}

bool ColorDialog::ShowModal( const faint::Color& color ){
  m_impl->SetColor(color, UPDATE_RGB);
  int result = faint::show_modal(*m_impl);
  return result == wxID_OK;
}

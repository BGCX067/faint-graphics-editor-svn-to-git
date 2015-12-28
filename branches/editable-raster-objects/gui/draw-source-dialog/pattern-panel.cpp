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
#include "app/getappcontext.hh" // For use image
#include "gui/draw-source-dialog/pattern-panel.hh"
#include "gui/draw-source-dialog/placement.hh"
#include "gui/layout.hh"
#include "util/clipboard.hh"
#include "util/color.hh"
#include "util/colorutil.hh"
#include "util/convertwx.hh"
#include "util/formatting.hh"
#include "util/guiutil.hh"
#include "util/pattern.hh"

DECLARE_EVENT_TYPE(EVT_CLICKED_ANCHOR, -1)
DEFINE_EVENT_TYPE(EVT_CLICKED_ANCHOR)

class PatternDisplay : public wxPanel{
public:
  PatternDisplay(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_pattern(faint::default_pattern(IntSize(240,240)))
  {
    SetInitialSize(to_wx(m_pattern.GetSize()));
    SetBackgroundStyle( wxBG_STYLE_PAINT ); // Avoid flicker on msw
    Connect(wxID_ANY, wxEVT_PAINT, wxPaintEventHandler(PatternDisplay::OnPaint));
    Connect(wxID_ANY, wxEVT_LEFT_DOWN, wxMouseEventHandler(PatternDisplay::OnLeftDown));
    Connect(wxID_ANY, wxEVT_LEFT_UP, wxMouseEventHandler(PatternDisplay::OnLeftUp));
    Connect(wxID_ANY, wxEVT_MOTION, wxMouseEventHandler(PatternDisplay::OnMotion));
  }

  IntPoint GetAnchor() const{
    return m_pattern.GetAnchor();
  }

  bool GetObjectAligned() const {
    return m_pattern.GetObjectAligned();
  }

  const faint::Pattern& GetPattern() const {
    return m_pattern;
  }

  void SetAnchor( const IntPoint& anchor ){
    m_pattern.SetAnchor(anchor);
  }

  void SetObjectAligned( bool objectAligned ){
    m_pattern.SetObjectAligned(objectAligned);
  }

  void SetPattern( const faint::Pattern& pattern ){
    m_pattern = pattern;
  }
private:
  void OnCaptureLost( wxMouseCaptureLostEvent& ){
    // Required on MSW
  }

  void OnLeftDown( wxMouseEvent& event ){
    CaptureMouse();
    SetAnchorFromPos(to_faint(event.GetPosition()));
  }

  void OnLeftUp( wxMouseEvent& ){
    if ( HasCapture() ){
      ReleaseMouse();
    }
  }

  void OnMotion( wxMouseEvent& event ){
    if ( HasCapture() ){
      SetAnchorFromPos(to_faint(event.GetPosition()));
    }
  }

  void SetAnchorFromPos( const IntPoint& pos ){
    IntSize sz(to_faint(GetSize()));
    IntPoint center(point_from_size(sz / 2));
    SetAnchor(GetAnchor() + pos - center); // Fixme: Notify text controls
    WarpPointer(center.x, center.y);
    wxCommandEvent newEvent(EVT_CLICKED_ANCHOR);
    ProcessEvent(newEvent);
    Refresh();
  }

  void OnPaint( wxPaintEvent& ){
    IntSize sz(to_faint(GetSize()));
    faint::Bitmap buffer(sz);
    clear(buffer, faint::Color(255,255,255));
    IntPoint center(point_from_size(sz / 2));
    faint::DrawSource src(offsat(faint::DrawSource(m_pattern), -center));
    fill_rect(buffer, rect_from_size(sz), src);
    draw_line_color(buffer, center - delta_x(5), center + delta_x(5), faint::color_black(), 1, false, LineCap::BUTT);
    draw_line_color(buffer, center - delta_y(5), center + delta_y(5), faint::color_black(), 1, false, LineCap::BUTT);
    wxPaintDC dc(this);
    dc.DrawBitmap( to_wx_bmp(buffer), 0, 0 );
  }
  faint::Pattern m_pattern;
};

class ColorPanel_Pattern_Impl : public wxPanel{
public:
  ColorPanel_Pattern_Impl( wxWindow* parent )
    : wxPanel(parent, wxID_ANY),
      m_anchor(0,0),
      m_objectAligned(nullptr),
      m_patternDisplay(nullptr)
  {
    using faint::panel_padding;
    m_patternDisplay = new PatternDisplay( this );
    m_patternDisplay->SetPosition(wxPoint(panel_padding, panel_padding));
    wxButton* btnUseImage = new wxButton(this, wxID_ANY, "Use Image", below( m_patternDisplay ));
    wxButton* btnPaste = new wxButton(this, wxID_ANY, "Paste Pattern", to_the_right_of(btnUseImage));
    wxButton* btnCopy = new wxButton(this, wxID_ANY, "Copy Pattern", to_the_right_of(btnPaste));
    m_objectAligned = new wxCheckBox( this, wxID_ANY, "&Object Aligned", to_the_right_of(m_patternDisplay) + wxPoint(10,0) );
    m_objectAligned->SetValue(m_patternDisplay->GetObjectAligned());
    m_anchorX = new wxTextCtrl(this, wxID_ANY, "0", below(m_objectAligned), wxSize(50, -1));
    m_anchorY = new wxTextCtrl(this, wxID_ANY, "0", to_the_right_of(m_anchorX), wxSize(50, -1));
    m_anchorX->Connect(wxID_ANY, wxEVT_KILL_FOCUS, wxFocusEventHandler(ColorPanel_Pattern_Impl::OnTextCtrlKillFocus), 0, this);
    m_anchorY->Connect(wxID_ANY, wxEVT_KILL_FOCUS, wxFocusEventHandler(ColorPanel_Pattern_Impl::OnTextCtrlKillFocus), 0, this);
    btnUseImage->Connect(wxID_ANY, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ColorPanel_Pattern_Impl::OnUseImage), 0, this);
    btnPaste->Connect(wxID_ANY, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ColorPanel_Pattern_Impl::OnPastePattern), 0, this);
    btnCopy->Connect(wxID_ANY, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ColorPanel_Pattern_Impl::OnCopyPattern), 0, this);
    m_objectAligned->Connect(wxID_ANY, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(ColorPanel_Pattern_Impl::OnObjectAligned), 0, this);
    Connect(wxID_ANY, EVT_CLICKED_ANCHOR, wxCommandEventHandler(ColorPanel_Pattern_Impl::OnClickedAnchor));
    UpdateDisplay();
  }

  void CopyPattern(){
    faint::Clipboard clip;
    if ( clip.Good() ){
      // Fixme: Add pattern-function for clip-board, to retain anchor, object aligned and so on
      clip.SetBitmap( m_patternDisplay->GetPattern().GetBitmap() );
    }
  }

  void SetPattern( const faint::Pattern& pattern){
    m_patternDisplay->SetPattern(pattern);
    m_objectAligned->SetValue(pattern.GetObjectAligned());

    IntPoint anchor(pattern.GetAnchor());
    m_anchorX->SetValue(str_int(anchor.x));
    m_anchorY->SetValue(str_int(anchor.y));
    UpdateDisplay();
  }

  const faint::Pattern& GetPattern() const{
    return m_patternDisplay->GetPattern();
  }

  void OnClickedAnchor( wxCommandEvent& ){
    IntPoint anchor(m_patternDisplay->GetAnchor());
    m_anchorX->SetValue(str_int(anchor.x));
    m_anchorY->SetValue(str_int(anchor.y));
  }

  void OnUseImage( wxCommandEvent& ){
    // Fixme: Reveals everything.
    AppContext& app = GetAppContext();
    m_patternDisplay->SetPattern( faint::Pattern(app.GetActiveCanvas().GetBitmap()) );
    UpdateDisplay();

  }
  void OnCopyPattern( wxCommandEvent& ){
    CopyPattern();
  }

  void OnObjectAligned( wxCommandEvent& ){
    m_patternDisplay->SetObjectAligned(m_objectAligned->GetValue());
  }

  void OnPastePattern( wxCommandEvent& ){
    PastePattern();
  }

  void OnTextCtrlKillFocus(wxFocusEvent& e){
    e.Skip();
    wxTextCtrl* textCtrl( dynamic_cast<wxTextCtrl*>(e.GetEventObject()) );
    wxString text = textCtrl->GetValue();
    IntPoint oldAnchor( m_patternDisplay->GetAnchor() );
    if ( text.empty() || !text.IsNumber() ){
      textCtrl->SetValue(str_int(textCtrl == m_anchorX ?
        oldAnchor.x :
        oldAnchor.y ) );
      return;
    }

    if ( textCtrl == m_anchorX ){
      m_patternDisplay->SetAnchor(IntPoint(faint::parse_int_value(textCtrl,oldAnchor.x),
        oldAnchor.y));
    }
    else if ( textCtrl == m_anchorY ){
      m_patternDisplay->SetAnchor(IntPoint(oldAnchor.x,
	faint::parse_int_value(textCtrl, oldAnchor.y)));
    }
    else{
      assert(false);
    }
    UpdateDisplay();

  }

  void PastePattern(){
    faint::Clipboard clip;
    if ( clip.Good() ){
      faint::Bitmap temp;
      if ( clip.GetBitmap( temp ) ){
        if ( bitmap_ok(temp) ){
          m_patternDisplay->SetPattern(faint::Pattern(temp));
        }
      }
    }
    UpdateDisplay();
  }
private:
  void UpdateDisplay(){
    m_patternDisplay->Refresh();
  }

  IntPoint m_anchor;
  wxCheckBox* m_objectAligned; // Fixme: Perhaps this is rather anchor than align
  PatternDisplay* m_patternDisplay;
  wxTextCtrl* m_anchorX;
  wxTextCtrl* m_anchorY;
};

ColorPanel_Pattern::ColorPanel_Pattern( wxWindow* parent ){
  m_impl = new ColorPanel_Pattern_Impl(parent);
}

ColorPanel_Pattern::~ColorPanel_Pattern(){
  m_impl = nullptr; // Deletion handled by wxWidgets
}

wxWindow* ColorPanel_Pattern::AsWindow(){
  return m_impl;
}

void ColorPanel_Pattern::Copy(){
  m_impl->CopyPattern();
}

faint::Pattern ColorPanel_Pattern::GetPattern() const{
  return m_impl->GetPattern();
}

void ColorPanel_Pattern::SetPattern( const faint::Pattern& pattern ){
  m_impl->SetPattern(pattern);
}

void ColorPanel_Pattern::Paste(){
  m_impl->PastePattern();
}

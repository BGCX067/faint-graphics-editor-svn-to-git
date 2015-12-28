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
#include "wx/dcclient.h"
#include "wx/event.h"
#include "wx/panel.h"
#include "wx/textctrl.h"
#include "app/get-app-context.hh" // For use image
#include "bitmap/bitmap.hh"
#include "bitmap/draw.hh"
#include "geo/geo-func.hh"
#include "geo/intrect.hh"
#include "geo/line.hh"
#include "gui/layout.hh"
#include "gui/mouse-capture.hh"
#include "gui/paint-dialog/pattern-panel.hh"
#include "gui/placement.hh"
#include "text/formatting.hh"
#include "util/canvas.hh"
#include "util/clipboard.hh"
#include "util/color.hh"
#include "util/color-bitmap-util.hh"
#include "util/color-span.hh"
#include "util/convert-wx.hh"
#include "util/either.hh"
#include "util/gui-util.hh"
#include "util/pattern.hh"

namespace faint{

DECLARE_EVENT_TYPE(EVT_CLICKED_ANCHOR, -1)
DEFINE_EVENT_TYPE(EVT_CLICKED_ANCHOR)

static void draw_cross(Bitmap& bmp, const IntPoint& center, int r, int lineWidth, const Color& c){
  draw_line(bmp, {center - delta_x(r), center + delta_x(r)},
    {c, lineWidth, false, LineCap::BUTT});
  draw_line(bmp, {center - delta_y(r), center + delta_y(r)},
    {c, lineWidth, false, LineCap::BUTT});
}

class PatternDisplay : public wxPanel{
public:
  PatternDisplay(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_hovered(false),
      m_mouse(this),
      m_pattern(default_pattern(IntSize(240,240)))
  {
    SetInitialSize(to_wx(m_pattern.GetSize()));
    SetBackgroundStyle(wxBG_STYLE_PAINT); // Avoid flicker on msw

    Bind(wxEVT_ENTER_WINDOW, [&](wxMouseEvent&){
      m_hovered = true;
      Refresh();
    });

    Bind(wxEVT_LEAVE_WINDOW, [&](wxMouseEvent&){
      m_hovered = false;
      Refresh();
    });

    Bind(wxEVT_LEFT_DOWN, &PatternDisplay::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &PatternDisplay::OnLeftUp, this);

    Bind(wxEVT_MOTION, &PatternDisplay::OnMotion, this);

    Bind(wxEVT_PAINT, &PatternDisplay::OnPaint, this);
  }

  IntPoint GetAnchor() const{
    return m_pattern.GetAnchor();
  }

  bool GetObjectAligned() const {
    return m_pattern.GetObjectAligned();
  }

  const Pattern& GetPattern() const {
    return m_pattern;
  }

  void SetAnchor(const IntPoint& anchor){
    m_pattern.SetAnchor(anchor);
  }

  void SetObjectAligned(bool objectAligned){
    m_pattern.SetObjectAligned(objectAligned);
  }

  void SetPattern(const Pattern& pattern){
    m_pattern = pattern;
  }
private:
  void OnLeftDown(wxMouseEvent& event){
    m_mouse.Capture();
    SetAnchorFromPos(to_faint(event.GetPosition()));
  }

  void OnLeftUp(wxMouseEvent&){
    m_mouse.Release();
  }

  void OnMotion(wxMouseEvent& event){
    m_hovered = true;
    if (m_mouse.HasCapture()){
      SetAnchorFromPos(to_faint(event.GetPosition()));
      SetCursor(to_wx_cursor(Cursor::BLANK));
    }
    else{
      SetCursor(to_wx_cursor(Cursor::CROSSHAIR));
    }
  }

  void SetAnchorFromPos(const IntPoint& pos){
    IntSize sz(to_faint(GetSize()));
    IntPoint center(point_from_size(sz / 2));
    SetAnchor(GetAnchor() + pos - center); // Fixme: Notify text controls
    WarpPointer(center.x, center.y);
    wxCommandEvent newEvent(EVT_CLICKED_ANCHOR);
    ProcessEvent(newEvent);
    Refresh();
  }

  void OnPaint(wxPaintEvent&){
    IntSize sz(to_faint(GetSize()));
    Bitmap buffer(sz);
    clear(buffer, Color(255,255,255));
    IntPoint center(point_from_size(sz / 2));
    Paint src(offsat(Paint(m_pattern), -center));
    fill_rect(buffer, rect_from_size(sz), src);
    if (!m_hovered){
      draw_cross(buffer, center, 11, 3, color_white());
    }
    draw_cross(buffer, center, 10, 1, color_black());
    wxPaintDC dc(this);
    dc.DrawBitmap(to_wx_bmp(buffer), 0, 0);
  }
  bool m_hovered;
  MouseCapture m_mouse;
  Pattern m_pattern;
};

class ColorPanel_Pattern_Impl : public wxPanel{
public:
  ColorPanel_Pattern_Impl(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_anchor(0,0),
      m_objectAligned(nullptr),
      m_patternDisplay(nullptr)
  {
    m_patternDisplay = new PatternDisplay(this);
    m_patternDisplay->SetPosition(wxPoint(panel_padding, panel_padding));
    wxButton* btnUseImage = new wxButton(this, wxID_ANY, "Use Image", below(m_patternDisplay));
    wxButton* btnPaste = new wxButton(this, wxID_ANY, "Paste Pattern", to_the_right_of(btnUseImage));
    wxButton* btnCopy = new wxButton(this, wxID_ANY, "Copy Pattern", to_the_right_of(btnPaste));
    m_objectAligned = new wxCheckBox(this, wxID_ANY, "&Object Aligned", to_the_right_of(m_patternDisplay) + wxPoint(10,0));
    m_objectAligned->SetValue(m_patternDisplay->GetObjectAligned());
    m_anchorX = new wxTextCtrl(this, wxID_ANY, "0", below(m_objectAligned), wxSize(50, -1));
    m_anchorY = new wxTextCtrl(this, wxID_ANY, "0", to_the_right_of(m_anchorX), wxSize(50, -1));

    m_anchorX->Bind(wxEVT_KILL_FOCUS, &ColorPanel_Pattern_Impl::OnTextCtrlKillFocus, this);
    m_anchorY->Bind(wxEVT_KILL_FOCUS, &ColorPanel_Pattern_Impl::OnTextCtrlKillFocus, this);
    btnUseImage->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &ColorPanel_Pattern_Impl::OnUseImage, this);
    btnPaste->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &ColorPanel_Pattern_Impl::OnPastePattern, this);
    btnCopy->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &ColorPanel_Pattern_Impl::OnCopyPattern, this);
    m_objectAligned->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &ColorPanel_Pattern_Impl::OnObjectAligned, this);

    Bind(EVT_CLICKED_ANCHOR, &ColorPanel_Pattern_Impl::OnClickedAnchor, this);
    UpdateDisplay();
  }

  void CopyPattern(){
    Clipboard clip;
    if (clip.Good()){
      // Fixme: Add pattern-function for clip-board, to retain anchor, object aligned and so on
      clip.SetBitmap(m_patternDisplay->GetPattern().GetBitmap());
    }
  }

  void SetPattern(const Pattern& pattern){
    m_patternDisplay->SetPattern(pattern);
    m_objectAligned->SetValue(pattern.GetObjectAligned());

    IntPoint anchor(pattern.GetAnchor());
    m_anchorX->SetValue(to_wx(str_int(anchor.x)));
    m_anchorY->SetValue(to_wx(str_int(anchor.y)));
    UpdateDisplay();
  }

  const Pattern& GetPattern() const{
    return m_patternDisplay->GetPattern();
  }

  void OnClickedAnchor(wxEvent&){
    IntPoint anchor(m_patternDisplay->GetAnchor());
    m_anchorX->SetValue(to_wx(str_int(anchor.x)));
    m_anchorY->SetValue(to_wx(str_int(anchor.y)));
  }

  void OnUseImage(wxCommandEvent&){
    // Fixme: Reveals everything.
    AppContext& app = get_app_context();
    const auto& background = app.GetActiveCanvas().GetBackground();
    background.Visit(
      [&](const Bitmap& bmp){
        m_patternDisplay->SetPattern(Pattern(bmp));
      },
      [&](const ColorSpan& colorSpan){
        // Not using the colorSpan size, since it might be huge, and
        // it would be pointless with only one color.
        const IntSize size(1,1);
        m_patternDisplay->SetPattern(Pattern(Bitmap(size, colorSpan.color)));
      });
    UpdateDisplay();
  }

  void OnCopyPattern(wxCommandEvent&){
    CopyPattern();
  }

  void OnObjectAligned(wxCommandEvent&){
    m_patternDisplay->SetObjectAligned(m_objectAligned->GetValue());
  }

  void OnPastePattern(wxCommandEvent&){
    PastePattern();
  }

  void OnTextCtrlKillFocus(wxFocusEvent& e){
    e.Skip();
    wxTextCtrl* textCtrl(dynamic_cast<wxTextCtrl*>(e.GetEventObject()));
    wxString text = textCtrl->GetValue();
    IntPoint oldAnchor(m_patternDisplay->GetAnchor());
    if (text.empty() || !text.IsNumber()){
      textCtrl->SetValue(to_wx(str_int(textCtrl == m_anchorX ?
        oldAnchor.x :
        oldAnchor.y)));
      return;
    }

    if (textCtrl == m_anchorX){
      m_patternDisplay->SetAnchor(IntPoint(parse_int_value(textCtrl,oldAnchor.x),
        oldAnchor.y));
    }
    else if (textCtrl == m_anchorY){
      m_patternDisplay->SetAnchor(IntPoint(oldAnchor.x,
        parse_int_value(textCtrl, oldAnchor.y)));
    }
    else{
      assert(false);
    }
    UpdateDisplay();

  }

  void PastePattern(){
    Clipboard clip;
    if (clip.Good()){
      auto maybeBmp(std::move(clip.GetBitmap()));
      if (maybeBmp){
        const Bitmap bmp(copy(maybeBmp.Take()));
        if (bitmap_ok(bmp)){
          IntPoint anchor(m_patternDisplay->GetAnchor());
          m_anchorX->SetValue(wxString::Format("%d", anchor.x));
          m_anchorY->SetValue(wxString::Format("%d", anchor.y));
          m_patternDisplay->SetPattern(Pattern(std::move(bmp)));
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

ColorPanel_Pattern::ColorPanel_Pattern(wxWindow* parent){
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

Pattern ColorPanel_Pattern::GetPattern() const{
  return m_impl->GetPattern();
}

void ColorPanel_Pattern::SetPattern(const Pattern& pattern){
  m_impl->SetPattern(pattern);
}

void ColorPanel_Pattern::Paste(){
  m_impl->PastePattern();
}

} // namespace

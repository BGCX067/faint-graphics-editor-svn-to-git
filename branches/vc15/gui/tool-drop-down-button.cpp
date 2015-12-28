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

#include "gui/tool-drop-down-button.hh"
#include "wx/popupwin.h"
#include "wx/bmpbuttn.h"
#include "wx/sizer.h"
#include "wx/tglbtn.h"
#include "bitmap/bitmap.hh"
#include "bitmap/color.hh"
#include "bitmap/draw.hh"
#include "geo/geo-func.hh"
#include "geo/line.hh"
#include "gui/events.hh"
#include "python/py-interface.hh"
#include "text/formatting.hh"
#include "util-wx/convert-wx.hh"

namespace faint{

ToolInfo::ToolInfo(const wxBitmap& inactive,
  const wxBitmap& active,
  const tooltip_t& tooltip,
  const description_t& description,
  const ToolId& toolId)
  : inactive(inactive),
    active(active),
    tooltip(tooltip),
    description(description),
    toolId(toolId)
{}

static wxBitmap drop_down_bitmap(const wxBitmap& src, const IntSize& outerSize){
  IntSize innerSize(to_faint(src.GetSize()));
  IntPoint offset(point_from_size(outerSize - innerSize)/ 2);
  Bitmap b(outerSize, color_transparent_black());

  blit_masked(offsat(to_faint(src), offset), onto(b),
    Color(1,0,0));
  int dx = 6;
  int dy = 3;
  int spacing = 2;
  IntPoint p1(b.m_w - dx - 1 - spacing, b.m_h - dy - 1 - spacing);
  IntPoint p2(p1 + delta_xy(dx / 2, dy));
  IntPoint p3(p1 + delta_x(dx));

  fill_triangle_color(b, p1, p2, p3,
    color_white());

  LineSettings s(color_black(), 1, LineStyle::SOLID, LineCap::BUTT);
  draw_line(b, {p1, p2}, s);
  draw_line(b, {p2, p3}, s);
  draw_line(b, {p1 - delta_y(1), p3 - delta_y(1)}, s);
  return to_wx_bmp(b);
}

class ToolPopup : public wxPopupWindow{
  // The popup-window shown when clicking the ToolDropDownButton
public:
  ToolPopup(wxBitmapToggleButton* parent, const std::vector<ToolInfo>& items, const IntSize& buttonSize)
    : wxPopupWindow(parent, wxBORDER_SIMPLE)
  {
    wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
    for (const auto& item : items){
      auto b = new wxBitmapToggleButton(this, wxID_ANY,
        item.inactive,
        wxDefaultPosition,
        wxSize(25,25),
        wxWANTS_CHARS); // TODO: wxSize(25, 25) == duplication
      m_buttons.push_back({b, item});
      boxSizer->Add(b);
    }

    wxBoxSizer* outerSizer = new wxBoxSizer(wxHORIZONTAL);
    outerSizer->Add(boxSizer,
      0, wxALL, 5);
    SetSizerAndFit(outerSizer);

    Bind(wxEVT_LEFT_DOWN,
      [parent,this,buttonSize](wxMouseEvent& e){
        wxPoint p(e.GetPosition());
        wxSize sz(GetSize());
        if (p.x < 0 || p.y < 0 || p.x >= sz.GetWidth() || p.y >= sz.GetHeight()){
          ReleaseMouse();
          Hide();
          return;
        }

        for (auto b : m_buttons){
          if (b.first->GetRect().Contains(e.GetPosition())){
            auto inactive(drop_down_bitmap(b.second.inactive, buttonSize));
            parent->SetBitmap(inactive);

            auto active(drop_down_bitmap(b.second.active, buttonSize));
            parent->SetBitmapFocus(active);
            parent->SetBitmapCurrent(active);
            parent->SetBitmapPressed(active);
            ReleaseMouse();
            Hide();
            ToolChangeEvent newEvent(b.second.toolId);
            parent->GetEventHandler()->ProcessEvent(newEvent);
          }
        }
      });

    Bind(wxEVT_RIGHT_DOWN,
      [this](wxMouseEvent&){
        ReleaseMouse();
        Hide();
      });

    Bind(wxEVT_MOUSE_CAPTURE_LOST,
      [&](wxMouseCaptureLostEvent&){
        Hide();
      });
  }
private:
  std::vector<std::pair<wxBitmapToggleButton*, ToolInfo>> m_buttons;
};

wxBitmapToggleButton* tool_drop_down_button(wxWindow* parent, const IntSize& size,
    const std::vector<ToolInfo>& items)
{
  const auto& first = items.front();
  auto* b = new wxBitmapToggleButton(parent,
    wxID_ANY, drop_down_bitmap(first.inactive, size),
    wxDefaultPosition,
    to_wx(size),
    wxWANTS_CHARS|wxBORDER_NONE);
  b->SetBitmapFocus(first.active);
  b->SetBitmapCurrent(first.active);
  b->SetBitmapPressed(first.active);

  ToolPopup* popup = new ToolPopup(b, items, size);
  b->Bind(wxEVT_TOGGLEBUTTON,
    [=](wxCommandEvent&){
      popup->Show();
      popup->CaptureMouse();
      popup->Position(b->GetScreenPosition() - popup->GetSize(),
        popup->GetSize());
    });
  return b;
}

} // namespace

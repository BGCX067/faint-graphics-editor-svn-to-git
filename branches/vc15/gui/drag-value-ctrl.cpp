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

#include "wx/dcclient.h"
#include "app/app.hh" // Fixme: For get_art_container. Pass it instead.
#include "app/resource-id.hh"
#include "geo/angle.hh"
#include "geo/geo-func.hh"
#include "geo/line.hh"
#include "geo/measure.hh"
#include "gui/art-container.hh"
#include "gui/drag-value-ctrl.hh"
#include "text/formatting.hh"
#include "util/status-interface.hh"
#include "util-wx/convert-wx.hh"

namespace faint{

const wxEventType FAINT_DRAG_VALUE_CHANGE = wxNewEventType();

DragValueChangeEvent::DragValueChangeEvent()
  : wxCommandEvent(FAINT_DRAG_VALUE_CHANGE)
{}

DragValueChangeEvent* DragValueChangeEvent::Clone() const{
  return new DragValueChangeEvent();
}

const wxEventTypeTag<DragValueChangeEvent> EVT_FAINT_DRAG_VALUE_CHANGE(FAINT_DRAG_VALUE_CHANGE);

const wxColour g_highlightColor(wxColour(0,0,0));
const wxColour g_originalColor(wxColour(40,40,40));

static int drag_value(const IntPoint& p0,
  const IntPoint& p1,
  int value_p0,
  const IntRange& range,
  coord dampening)
{
  assert(dampening > 0);
  coord d = truncated(distance(p0, p1));
  Angle a = angle360({floated(p0), floated(p1)});
  if (Angle::Deg(135) < a && a < Angle::Deg(315)){
    d *= -1;
  }
  return range.Constrain(value_p0 - truncated(d / dampening));
}

DragValueCtrl::DragValueCtrl(wxWindow* parent, const IntRange& range, const description_t& statusText, StatusInterface& statusInfo)
  : wxPanel(parent, wxID_ANY),
    m_currentValue(10),
    m_mouse(this, [&](){
        // On capture lost
        SetForegroundColour(g_originalColor);
        m_originValue = m_currentValue = GetDragValue();
        SendChangeEvent(m_originValue);
      },
      [&](){
        // On release
        SetCursor(get_art_container().Get(Cursor::MOVE_POINT));
      }),
    m_originValue(10),
    m_range(range),
    m_statusInfo(statusInfo),
    m_statusText(statusText.Get())
{
  SetForegroundColour(g_originalColor);
  SetCursor(get_art_container().Get(Cursor::MOVE_POINT));
  SetInitialSize(wxSize(50,40));

  Bind(wxEVT_LEFT_DOWN,
    [&](wxMouseEvent& evt){
      SetCursor(get_art_container().Get(Cursor::DRAG_SCALE));
      wxPoint pos(evt.GetPosition());
      m_origin = to_faint(pos);
      m_current = m_origin;
      m_originValue = m_currentValue;
      SetForegroundColour(g_highlightColor);
      m_mouse.Capture();
      m_statusInfo.SetMainText("");
    });

  Bind(wxEVT_LEFT_UP,
    [&](wxMouseEvent& evt){
      SetForegroundColour(g_originalColor);
      Refresh();
      m_mouse.Release();

      m_originValue = m_currentValue = GetDragValue();
      wxPoint pos = evt.GetPosition();
      wxSize size = GetSize();
      if (pos.x < size.GetWidth() && pos.y < size.GetHeight() && pos.x > 0 && pos.y > 0){
        m_statusInfo.SetMainText(m_statusText);
      }
      Refresh();
      SendChangeEvent(m_originValue);
    });

  Bind(wxEVT_MOTION,
    [&](wxMouseEvent& evt){
      if (m_mouse.HasCapture()){
        wxPoint pos(evt.GetPosition());
        m_current.x = pos.x;
        m_current.y = pos.y;

        int newValue = GetDragValue();
        if (newValue != m_currentValue){
          m_currentValue = newValue;
          Update(); // Required to avoid long delay in refreshing the number
          SendChangeEvent(m_currentValue);
        }
      }});

  Bind(wxEVT_ENTER_WINDOW,
    [&](wxMouseEvent&){
      if (!m_mouse.HasCapture()){
        m_statusInfo.SetMainText(m_statusText);
      }
    });

  Bind(wxEVT_LEAVE_WINDOW,
    [&](wxMouseEvent&){
      m_statusInfo.SetMainText("");
    });

  Bind(wxEVT_PAINT,
    [&](wxPaintEvent&){
      wxPaintDC dc(this);
      wxString str(to_wx(str_int(m_currentValue)));
      wxCoord textWidth, textHeight;
      dc.GetTextExtent(str, &textWidth, &textHeight);
      wxSize winSize(GetSize());
      dc.DrawText(str, winSize.GetWidth() / 2 - textWidth / 2 ,
        winSize.GetHeight() / 2 - textHeight / 2);
    });
}

int DragValueCtrl::GetDragValue() const{
  coord dampening = wxGetKeyState(WXK_CONTROL) ?
    10.0 : 3.0;
  return drag_value(m_current, m_origin, m_originValue, m_range, dampening);
}

void DragValueCtrl::SendChangeEvent(int value){
  DragValueChangeEvent event;
  event.SetEventObject(this);
  event.SetInt(value);
  ProcessEvent(event);
}

void DragValueCtrl::SetValue(int value){
  m_currentValue = value;
  Refresh();
}

}

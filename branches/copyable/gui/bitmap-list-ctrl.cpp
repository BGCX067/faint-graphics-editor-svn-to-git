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
#include "wx/dcclient.h"
#include "wx/dcbuffer.h"
#include "gui/bitmap-list-ctrl.hh"
#include "util/gui-util.hh"
#include "util/status-interface.hh"

namespace faint{

DEFINE_EVENT_TYPE(EVT_BITMAPLIST_SELECTION)

BitmapListCtrl::BitmapListCtrl(wxWindow* parent, const wxSize& bitmapSize, StatusInterface& status, int orientation, int hSpace, int vSpace)
: wxPanel(parent),
  m_imageSize(bitmapSize),
  m_vSpace(vSpace),
  m_hSpace(hSpace),
  m_orientation(orientation),
  m_selection(-1),
  m_selectionRect(0,0, bitmapSize.GetWidth() + 2 * hSpace, bitmapSize.GetHeight() + 2 * vSpace),
  m_status(status)
{
  SetBackgroundStyle(wxBG_STYLE_PAINT);
  if (m_orientation == wxVERTICAL){
    SetSize(bitmapSize.GetWidth() + 2 * m_hSpace, m_vSpace);
  }
  else if (orientation == wxHORIZONTAL){
    SetSize(m_hSpace, bitmapSize.GetHeight() + 2 * m_vSpace + 2);
  }
  else {
    assert(false);
  }

  Bind(wxEVT_PAINT, &BitmapListCtrl::OnPaint, this);
  Bind(wxEVT_LEFT_DOWN, &BitmapListCtrl::OnLeftDown, this);
  Bind(wxEVT_LEAVE_WINDOW, &BitmapListCtrl::OnLeaveWindow, this);
  Bind(wxEVT_MOTION, &BitmapListCtrl::OnMotion, this);
}

int BitmapListCtrl::Add(const wxBitmap& bmp, const utf8_string& statusText){
  assert(bmp.GetWidth() == m_imageSize.GetWidth());
  assert(bmp.GetHeight() == m_imageSize.GetHeight());

  m_images.push_back(bmp);
  m_statusTexts.push_back(statusText);
  wxSize size = GetSize();
  int width = size.GetWidth();
  int height = size.GetHeight();
  const int border = 2;

  if (m_orientation == wxVERTICAL){
    SetSize(width, resigned(m_images.size()) *
     (2 * m_vSpace + m_imageSize.GetHeight()) + border);
  }
  else{
    SetSize(resigned(m_images.size()) * (2 * m_hSpace + m_imageSize.GetWidth()) + border, height);
  }
  if (m_selection == -1){
    m_selection = 0;
  }
  return resigned(m_images.size() - 1);
}

int BitmapListCtrl::GetClickedIndex(const wxPoint& pos) const{
  assert(!m_images.empty());
  int maxIndex = resigned(m_images.size()) - 1;
  if (m_orientation == wxVERTICAL){
    int imageHeight = m_imageSize.GetHeight() + 2 * m_vSpace;
    return std::min(pos.y / imageHeight, maxIndex);
  }
  else {
    int imageWidth = m_imageSize.GetWidth() + 2 * m_hSpace;
    return std::min(pos.x / imageWidth, maxIndex);
  }
}

int BitmapListCtrl::GetSelection() const {
  return m_selection;
}

void BitmapListCtrl::OnLeftDown(wxMouseEvent& event){
  int oldSelection = m_selection;
  int selected = GetClickedIndex(event.GetPosition());

  SetSelection(selected);
  if (oldSelection != m_selection){
    wxCommandEvent event(EVT_BITMAPLIST_SELECTION, GetId());
    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent(event);
  }
}

void BitmapListCtrl::OnLeaveWindow(wxMouseEvent& event){
  m_status.SetMainText("");
  event.Skip();
}

void BitmapListCtrl::OnMotion(wxMouseEvent& event){
  int index = GetClickedIndex(event.GetPosition());
  m_status.SetMainText(m_statusTexts[to_size_t(index)]);
  event.Skip();
}

void BitmapListCtrl::OnPaint(wxPaintEvent&){
  wxAutoBufferedPaintDC paintDC(this);
  paintDC.SetBackground(get_gui_deselected_color());
  paintDC.Clear();
  if (m_orientation == wxVERTICAL){
    int yOffset = m_vSpace;
    int imageHeight = m_imageSize.GetHeight() + m_vSpace + m_vSpace;

    if (m_selection != -1){
      wxPen tPen(wxColour(0, 0, 0));
      tPen.SetStyle(wxPENSTYLE_TRANSPARENT);
      paintDC.SetPen(tPen);
      paintDC.SetBrush(wxBrush(get_gui_selected_color()));
      wxRect currentSel(m_selectionRect);
      currentSel.Offset(0, m_selection * imageHeight);
      paintDC.DrawRectangle(currentSel);
    }

    for (auto it = begin(m_images); it != end(m_images); ++it, yOffset += imageHeight){
      paintDC.DrawBitmap (*it, m_hSpace, yOffset);
    }
  }
  else {
    int xOffset = m_hSpace;
    int imageWidth = m_imageSize.GetWidth() + 2 * m_hSpace;

    if (m_selection != -1){
      wxPen tPen(wxColour(0, 0, 0));
      tPen.SetStyle(wxPENSTYLE_TRANSPARENT);
      paintDC.SetPen(tPen);
      paintDC.SetBrush(wxBrush(wxColour(112, 154, 209)));
      wxRect currentSel(m_selectionRect);
      currentSel.Offset(m_selection * imageWidth, 0);
      paintDC.DrawRectangle(currentSel);
    }

    for (auto it = begin(m_images); it != end(m_images); ++it, xOffset += imageWidth){
      paintDC.DrawBitmap(*it, xOffset, m_vSpace);
    }
  }
}

void BitmapListCtrl::SetSelection(int selection){
  assert(selection >= 0);
  assert (to_size_t(selection) < m_images.size());
  m_selection = selection;
  Refresh();
}

} // namespace

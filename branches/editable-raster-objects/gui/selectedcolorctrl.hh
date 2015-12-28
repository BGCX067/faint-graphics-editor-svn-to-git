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

#ifndef FAINT_SELECTEDCOLORCTRL_HH
#define FAINT_SELECTEDCOLORCTRL_HH
#include "wx/wx.h"
#include "gui/colordataobject.hh"
#include "util/settings.hh"
#include "util/statusinterface.hh"

DECLARE_EVENT_TYPE(EVT_SWAP_COLORS, -1)

class SelectedColorCtrl : public wxPanel, public ColorDropTarget { // Fixme: Interface should not inherit from wx-classes
public:
  SelectedColorCtrl( wxWindow* parent, SettingNotifier&, StatusInterface& );
  void UpdateColors( const faint::DrawSource& fg, const faint::DrawSource& bg );
  enum which{ HIT_FG, HIT_BG, HIT_NEITHER };
private:
  const faint::DrawSource& GetClickedDrawSource(which) const;
  which HitTest( const wxPoint& );
  wxDragResult OnDropColor( const IntPoint&, const faint::Color& ) override;
  void OnPaint( wxPaintEvent& );
  void OnEnter( wxMouseEvent& );
  void OnMotion( wxMouseEvent& );
  void OnLeave( wxMouseEvent& );
  void OnLeftDown( wxMouseEvent& );
  void OnRightDown( wxMouseEvent& );
  void OnMenuChoice( wxCommandEvent& );
  wxBitmap GetBitmap( const faint::Color& );
  const faint::DrawSource& MenuTargetColor() const;
  wxRect m_fgRect;
  wxRect m_bgRect;
  faint::DrawSource m_fg;
  faint::DrawSource m_bg;
  wxBitmap m_fgBmp;
  wxBitmap m_bgBmp;
  which m_menuEventColor;
  SettingNotifier& m_notifier;
  StatusInterface& m_statusInfo;
  DECLARE_EVENT_TABLE()
};

#endif

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

#ifndef FAINT_STATUSBUTTON_HH
#define FAINT_STATUSBUTTON_HH
#include <string>
#include "wx/tglbtn.h"
#include "wx/wx.h"
#include "util/guiutil.hh"
#include "util/statusinterface.hh"

class StatusButton : public wxButton{
public:
  // Button which uses a StatusInterface to display description text
  StatusButton( wxWindow* parent, const wxSize&, StatusInterface&, const std::string& label, const tooltip_t&, const description_t& );
  void UpdateText( const std::string& label, const tooltip_t&, const description_t& );
private:
  void OnLeave( wxMouseEvent& );
  void OnMotion( wxMouseEvent& );
  std::string m_description;
  StatusInterface& m_status;
  DECLARE_EVENT_TABLE()
};

class ToggleStatusButton : public wxBitmapToggleButton {
public:
  // An enable/disable bitmap button which uses a StatusInterface to
  // display description text
  ToggleStatusButton( wxWindow* parent, int id, const wxSize&, StatusInterface&, const wxBitmap&, const tooltip_t&, const description_t& );
private:
  void OnLeave( wxMouseEvent& );
  void OnMotion( wxMouseEvent& );
  std::string m_description;
  StatusInterface& m_status;
  DECLARE_EVENT_TABLE()
};

#endif

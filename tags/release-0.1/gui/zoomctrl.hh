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

#ifndef FAINT_ZOOMCTRL_HH
#define FAINT_ZOOMCTRL_HH

class ZoomLevel;

class ZoomCtrl : public wxPanel {
public:
  ZoomCtrl( wxWindow* parent );
  void Update( const ZoomLevel& );
private:
  void OnButton( wxCommandEvent& );
  wxStaticText* m_currentZoomText;
  wxButton* m_btnZoomIn;
  wxButton* m_btnZoomOut;
  wxButton* m_btnZoom100;
  DECLARE_EVENT_TABLE()
};

#endif

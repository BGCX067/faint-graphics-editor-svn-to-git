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

#ifndef FAINT_GRIDCTRL_HH
#define FAINT_GRIDCTRL_HH
#include <memory>
#include <vector>

class ArtContainer;
class SpinButton;
class Grid;
class DragValueCtrl;

typedef std::vector<wxWindow*> winvec_t;

class GridCtrl : public wxPanel {
public:
  GridCtrl( wxWindow* parent, const ArtContainer& );

  // Update the control to match external changes e.g. canvas change
  // or grid disabled via Python
  void Update();
private:
  void AdjustGrid( int );
  void EnableGrid( bool );
  void OnButton( wxCommandEvent& );
  void OnDragChange( wxCommandEvent& );
  void OnSpinDown( wxSpinEvent& );
  void OnSpinUp( wxSpinEvent& );
  bool m_enabled;
  const ArtContainer& m_art;
  DragValueCtrl* m_txtCurrentSize;
  wxButton* m_btnToggle;
  wxSizer* m_sizer;
  std::auto_ptr<SpinButton> m_spinButton;
  winvec_t m_showhide;
  DECLARE_EVENT_TABLE()
};
#endif

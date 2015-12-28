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
class DragValueCtrl;
class Grid;
class SpinButton;

typedef std::vector<wxWindow*> winvec_t;

class GridCtrl : public wxPanel {
public:
  GridCtrl( wxWindow* parent, const ArtContainer& );
  ~GridCtrl();
  // Update the control to match external changes e.g. canvas change
  // or grid disabled via Python
  void Update();
private:
  void AdjustGrid( int );
  void EnableGrid( bool );
  void OnButton( wxCommandEvent& );
  void OnDragChange( wxCommandEvent& );
  void OnRightClickDrag( wxMouseEvent& );
  void OnSpinDown( wxSpinEvent& );
  void OnSpinUp( wxSpinEvent& );
  const ArtContainer& m_art;
  wxButton* m_btnToggle;
  bool m_enabled;
  std::vector<wxWindow*> m_showhide;
  wxSizer* m_sizer;
  std::unique_ptr<SpinButton> m_spinButton;
  DragValueCtrl* m_txtCurrentSize;
  DECLARE_EVENT_TABLE()
};
#endif

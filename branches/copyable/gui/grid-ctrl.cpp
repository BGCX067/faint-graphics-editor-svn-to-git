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
#include <sstream>
#include "wx/sizer.h"
#include "app/get-app-context.hh"
#include "gui/drag-value-ctrl.hh"
#include "gui/events.hh"
#include "gui/grid-ctrl.hh"
#include "gui/spin-button.hh"
#include "util/art-container.hh"
#include "util/canvas.hh"
#include "util/grid.hh"

namespace faint{

#ifdef __WXMSW__
#define GRIDCONTROL_BORDER_STYLE wxBORDER_THEME
#else
#define GRIDCONTROL_BORDER_STYLE wxBORDER_NONE
#endif

void update_grid_toggle_button(const Grid&, wxButton*, const ArtContainer&);

Grid get_active_grid(int delta=0){
  Grid g(get_app_context().GetActiveCanvas().GetGrid());
  g.SetSpacing(std::max(g.Spacing() + delta, 1));
  return g;
}

void set_active_grid(const Grid& g){
  Canvas& canvas = get_app_context().GetActiveCanvas();
  canvas.SetGrid(g);
  canvas.Refresh();
}

static void set_active_grid_spacing(int spacing){
  Grid g(get_active_grid());
  g.SetSpacing(spacing);
  set_active_grid(g);
}

SpinButton* grid_spinbutton(wxWindow* parent, wxSizer* sizer, winvec_t& showhide){
  SpinButton* spinButton = new SpinButton(parent, IntSize(40,50), "Adjust Grid Spacing");
  spinButton->GetRaw()->Hide();
  sizer->Add(spinButton->GetRaw());
  showhide.push_back(spinButton->GetRaw());
  return spinButton;
}

DragValueCtrl* grid_text(wxWindow* parent, wxSizer* sizer, winvec_t& showhide, StatusInterface& statusInfo){
  DragValueCtrl* text = new DragValueCtrl(parent, IntRange(min_t(1)), description_t("Drag to adjust grid spacing. Right-Click to disable grid."), statusInfo);
  showhide.push_back(text);
  text->Hide();
  sizer->Add(text, 0, wxALIGN_CENTER_VERTICAL);
  return text;
}

wxButton* grid_toggle_button(wxWindow* parent, wxSizer* sizer, const ArtContainer& art){
  wxButton* button = noiseless_button(parent, "", tooltip_t(""),
    wxSize(60,50));
  update_grid_toggle_button(false, button, art);
  sizer->Add(button, 0, wxEXPAND);
  return button;
}

void update_grid_toggle_button(const Grid& g, wxButton* button, const ArtContainer& art){
  button->SetBitmap(art.Get(g.Enabled() ? Icon::GRID_OFF : Icon::GRID_ON));
  button->SetToolTip(g.Enabled() ? "Disable Grid" : "Enable Grid");
}

GridCtrl::GridCtrl(wxWindow* parent, const ArtContainer& art, StatusInterface& statusInfo)
  : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL|GRIDCONTROL_BORDER_STYLE),
    m_art(art),
    m_enabled(false)
{
  m_sizer = new wxBoxSizer(wxHORIZONTAL);
  m_txtCurrentSize = grid_text(this, m_sizer, m_showhide, statusInfo);
  m_spinButton.reset(grid_spinbutton(this, m_sizer, m_showhide));
  m_btnToggle = grid_toggle_button(this, m_sizer, art);
  m_txtCurrentSize->Bind(wxEVT_RIGHT_DOWN, &GridCtrl::OnRightClickDrag, this);
  SetSizerAndFit(m_sizer);

  Bind(wxEVT_BUTTON, &GridCtrl::OnButton, this);
  Bind(wxEVT_IDLE, &GridCtrl::OnIdle, this);
  Bind(wxEVT_SPIN_UP, &GridCtrl::OnSpinUp, this);
  Bind(wxEVT_SPIN_DOWN, &GridCtrl::OnSpinDown, this);
  Bind(EVT_FAINT_DRAG_VALUE_CHANGE, &GridCtrl::OnDragChange, this);
}

void GridCtrl::EnableGrid(bool enable){
  m_enabled = enable;

  // Show items if enabled, otherwise hide
  for (wxWindow* window : m_showhide){
    window->Show(m_enabled);
  }

  Grid g(get_active_grid());
  g.SetEnabled(m_enabled);
  g.SetVisible(m_enabled);
  m_txtCurrentSize->SetValue(g.Spacing());
  update_grid_toggle_button(g, m_btnToggle, m_art);
  set_active_grid(g);
  SetSizerAndFit(m_sizer);
  send_control_resized_event(this);
}

void GridCtrl::OnButton(wxCommandEvent&){
  EnableGrid(!m_enabled);
}

void GridCtrl::OnDragChange(wxCommandEvent& evt){
  m_newValue.Set(evt.GetInt());
}

void GridCtrl::OnIdle(wxIdleEvent&){
  if (m_newValue.IsSet()){
    set_active_grid_spacing(m_newValue.Take());
  }
}

void GridCtrl::OnRightClickDrag(wxMouseEvent&){
  EnableGrid(false);
}

void GridCtrl::OnSpinDown(wxSpinEvent&){
  m_newValue.Set(get_active_grid(-1).Spacing());
}

void GridCtrl::OnSpinUp(wxSpinEvent&){
  m_newValue.Set(get_active_grid(+1).Spacing());
}

void GridCtrl::Update(){
  Grid g(get_active_grid());
  m_txtCurrentSize->SetValue(g.Spacing());
  const bool enabled = g.Enabled();

  if (m_enabled != enabled){
    update_grid_toggle_button(g, m_btnToggle, m_art);
    m_enabled = enabled;
    for (wxWindow* window : m_showhide){
      window->Show(m_enabled);
    }
    SetSizerAndFit(m_sizer);
    send_control_resized_event(this);
  }
}

}

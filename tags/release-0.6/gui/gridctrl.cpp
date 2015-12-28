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

#include <sstream>
#include "wx/wx.h"
#include "app/getappcontext.hh"
#include "geo/grid.hh"
#include "gui/dragvaluectrl.hh"
#include "gui/events.hh"
#include "gui/gridctrl.hh"
#include "gui/spinbutton.hh"
#include "util/artcontainer.hh"
#include "util/derefiter.hh"

#ifdef __WXMSW__
#define GRIDCONTROL_BORDER_STYLE wxBORDER_THEME
#else
#define GRIDCONTROL_BORDER_STYLE wxBORDER_NONE
#endif

typedef faint::deref_iter<winvec_t> winvec_iter;
void update_grid_toggle_button( const Grid&, wxButton*, const ArtContainer& );

Grid get_active_grid(int delta=0){
  Grid g( GetAppContext().GetActiveCanvas().GetGrid() );
  g.SetSpacing( std::max(g.Spacing() + delta, 1 ) );
  return g;
}

SpinButton* grid_spinbutton( wxWindow* parent, wxSizer* sizer, winvec_t& showhide){
  SpinButton* spinButton = new SpinButton(parent, IntSize(40,50), "Adjust Grid Spacing");
  spinButton->GetRaw()->Hide();
  sizer->Add(spinButton->GetRaw());
  showhide.push_back(spinButton->GetRaw());
  return spinButton;
}

DragValueCtrl* grid_text( wxWindow* parent, wxSizer* sizer, winvec_t& showhide ){
  DragValueCtrl* text = new DragValueCtrl(parent, IntRange(min_t(1)), description_t("Click and drag to adjust grid spacing"));
  showhide.push_back(text);
  text->Hide();
  sizer->Add( text, 0, wxALIGN_CENTER_VERTICAL );
  return text;
}

wxButton* grid_toggle_button( wxWindow* parent, wxSizer* sizer, const ArtContainer& art){
  // wxWANTS_CHARS prevents sound on keypress when key has focus
  wxButton* button = new wxButton(parent, wxID_ANY, "", wxDefaultPosition, wxSize(60,50), wxWANTS_CHARS);
  button->SetInitialSize(wxSize(60,50));
  update_grid_toggle_button(false, button, art);
  sizer->Add( button, 0, wxEXPAND );
  return button;
}

void set_active_grid( const Grid& g ){
  CanvasInterface& canvas = GetAppContext().GetActiveCanvas();
  canvas.SetGrid(g);
  canvas.Refresh();
}

void update_grid_toggle_button( const Grid& g, wxButton* button, const ArtContainer& art ){
  button->SetBitmap(art.Get( g.Enabled() ? "grid_off" : "grid_on"));
  button->SetToolTip(g.Enabled() ? "Disable Grid" : "Enable Grid");
}

GridCtrl::GridCtrl( wxWindow* parent, const ArtContainer& art  )
  : wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL|GRIDCONTROL_BORDER_STYLE ),
    m_art(art),
    m_enabled(false),
    m_spinButton(0)
{
  m_sizer = new wxBoxSizer(wxHORIZONTAL);
  m_btnToggle = grid_toggle_button(this, m_sizer, art);
  m_spinButton.reset(grid_spinbutton(this, m_sizer, m_showhide));
  m_txtCurrentSize = grid_text(this, m_sizer, m_showhide);
  SetSizerAndFit( m_sizer );
}

void GridCtrl::AdjustGrid(int delta){
  Grid g(get_active_grid(delta));
  m_txtCurrentSize->SetValue(g.Spacing());
  set_active_grid(g);
}

void GridCtrl::EnableGrid( bool enable ){
  m_enabled = enable;

  // Show items if enabled, otherwise hide
  for ( winvec_iter it(m_showhide.begin()); it != m_showhide.end(); ++it ){
    it->Show(enable);
  }

  Grid g(get_active_grid());
  g.SetEnabled(m_enabled);
  g.SetVisible(m_enabled);
  m_txtCurrentSize->SetValue(g.Spacing());
  update_grid_toggle_button(g, m_btnToggle, m_art);
  set_active_grid(g);
  SetSizerAndFit(m_sizer);
  faint::send_control_resized_event(this);
}

void GridCtrl::OnButton( wxCommandEvent& ){
  EnableGrid( !m_enabled );
}

void GridCtrl::OnDragChange(wxCommandEvent& evt ){
  int value = evt.GetInt();
  Grid g( get_active_grid() );
  g.SetSpacing(value);
  set_active_grid(g);
}

void GridCtrl::OnSpinDown(wxSpinEvent& ){
  AdjustGrid(-1);
}

void GridCtrl::OnSpinUp( wxSpinEvent& ){
  AdjustGrid(1);
}

void GridCtrl::Update(){
  Grid g( get_active_grid() );
  m_txtCurrentSize->SetValue(g.Spacing());
  const bool enabled = g.Enabled();

  if ( m_enabled != enabled ){
    update_grid_toggle_button( g, m_btnToggle, m_art );
    m_enabled = enabled;
    for ( winvec_iter it(m_showhide.begin()); it != m_showhide.end(); ++it ){
      it->Show(m_enabled);
    }
    SetSizerAndFit(m_sizer);
  }
}

BEGIN_EVENT_TABLE( GridCtrl, wxPanel )
EVT_BUTTON( -1, GridCtrl::OnButton )
EVT_SPIN_UP(-1, GridCtrl::OnSpinUp )
EVT_SPIN_DOWN(-1, GridCtrl::OnSpinDown)
EVT_COMMAND(-1, EVT_DRAG_VALUE_CHANGE, GridCtrl::OnDragChange)
END_EVENT_TABLE()

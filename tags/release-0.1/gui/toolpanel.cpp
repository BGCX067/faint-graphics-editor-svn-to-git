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

#include "wx/wx.h"
#include "tools/settingid.hh"
#include "toolbar.hh"
#include "toolsettingpanel.hh"
#include "toolpanel.hh"

class ToolPanelImpl : public wxPanel{
public:
  ToolPanelImpl( wxWindow* parent, SettingNotifier& notifier, ArtContainer& art )
    : wxPanel( parent )
  {
    const int borderSize = 5;
    wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);

    m_toolbar = new Toolbar( this, art );
    outerSizer->Add( m_toolbar, 0, wxEXPAND | wxALL, borderSize );

    m_toolSettingPanel = new ToolSettingPanel( this, notifier, art );
    outerSizer->Add( m_toolSettingPanel, 1, wxEXPAND | wxALL, borderSize );
    SetSizerAndFit( outerSizer );
  }
  Toolbar* m_toolbar;
  ToolSettingPanel* m_toolSettingPanel;
};

ToolPanel::ToolPanel( wxWindow* parent, SettingNotifier& notifier, ArtContainer& art ){  
  m_impl = new ToolPanelImpl( parent, notifier, art );
}

ToolPanel::~ToolPanel(){
  // Note: deletion is handled by wxWidgets.
  m_impl = 0;
}

wxWindow* ToolPanel::AsWindow(){
  return m_impl;
}

bool ToolPanel::Visible() const{
  return m_impl->IsShown();  
}

void ToolPanel::Show( bool show ){
  m_impl->Show( show );
}

void ToolPanel::Hide(){
  Show( false );
}

void ToolPanel::ShowSettings( const FaintSettings& s ){
  m_impl->m_toolSettingPanel->ShowSettings(s);
}

void ToolPanel::SelectTool( ToolId id ){
  // Fixme: Weird, IIRC, used to put all handling in a MainFrame
  // On...-handler, and not duplicate button state...
  m_impl->m_toolbar->SendToolChoiceEvent( id ); 
}

void ToolPanel::SelectLayer( Layer layer ){
  // Fixme: See SelectTool note
  m_impl->m_toolbar->SendLayerChoiceEvent( layer );
}

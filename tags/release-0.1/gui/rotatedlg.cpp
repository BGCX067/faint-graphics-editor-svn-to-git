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

#include "rotatedlg.hh"
#include "wx/textctrl.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/radiobox.h"
#include "wx/arrstr.h"

RotateDialog::RotateDialog( wxWindow* parent, const wxString& title )
  : wxDialog( parent, wxID_ANY, title),
    m_choice(0)
{}

void RotateDialog::OnInitDialog( wxInitDialogEvent& ){
  wxBoxSizer* vSizer = new wxBoxSizer( wxVERTICAL );

  wxArrayString strs;
  strs.Add( "Flip &horizontal" );
  strs.Add( "Flip &vertical" );
  strs.Add( "&Rotate 90 CW"  );
  m_choice = new wxRadioBox( this, wxID_ANY, "Flip/Rotate", wxDefaultPosition, wxDefaultSize, strs, 1, wxRA_SPECIFY_COLS );
  vSizer->Add( m_choice );
  wxSizer* buttonSizer = CreateButtonSizer(wxOK|wxCANCEL);
  if ( buttonSizer != 0 ){
    // CreateButtonSizer returns 0 on some platforms
    vSizer->Add(buttonSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxDOWN, 10 );
  }
  SetSizerAndFit(vSizer);
  m_choice->SetFocus();
  // Center on parent
  Centre( wxBOTH );
}

RotateDialog::operation RotateDialog::GetOperation() const{
  operation op;
  op.angle = 0;
  int choice = m_choice->GetSelection();
  if ( choice == 0 ){
    op.choice = FLIP_HORIZONTAL;
  }
  else if ( choice == 1 ){
    op.choice = FLIP_VERTICAL;
  }
  else if ( choice == 2 ){
    op.choice = ROTATE;
    op.angle = 90;
  }
  else if ( choice == 3 ){
    op.choice = ROTATE;
    op.angle = -90;
  }
  return op;
}

BEGIN_EVENT_TABLE(RotateDialog, wxDialog)
EVT_INIT_DIALOG( RotateDialog::OnInitDialog )
END_EVENT_TABLE()

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

#include "helpwindow.hh"
#include "wx/wx.h"
#include "wx/html/helpctrl.h"

class HelpFrameImpl : public wxHtmlHelpFrame {
public:
  HelpFrameImpl( wxHtmlHelpData* data=0)
    : wxHtmlHelpFrame( data )
  {}

  void OnClose( wxCloseEvent& event ){
    if ( event.CanVeto() ){
      // Hide instead of closing
      event.Veto();
      Hide();
    }
    else {
      Destroy();
    }
  }
  DECLARE_EVENT_TABLE()
};


class HelpWindowImpl : public wxHtmlHelpController {
public:
  HelpWindowImpl( const std::string& helpBookPath )
    : wxHtmlHelpController(),
      m_initialized(false)
  {
    AddBook( helpBookPath );
  }

   bool HasFocus() const{
     return m_initialized && m_helpFrame->HasFocus();
   }

  void FaintCloseFrame(){
    // Put down the help frame for good
    m_helpFrame->Close( true );
  }

  void FaintShow(){
    if ( m_initialized ){
      m_helpFrame->Show();
      m_helpFrame->Raise();
    }
    else {
      DisplayContents();
      m_initialized = true;
    }
  }

protected:
  wxHtmlHelpFrame* CreateHelpFrame (wxHtmlHelpData *data) {
    wxHtmlHelpFrame* frame = new HelpFrameImpl( data );
    frame->SetController( this );
    frame->Create(0, -1, wxEmptyString, wxHF_DEFAULT_STYLE, 0, wxEmptyString );
    frame->SetTitleFormat( m_titleFormat );
    frame->SetShouldPreventAppExit( false );
    m_helpFrame = frame;
    return frame;
  }

private:
  const std::string m_helpPath;
  bool m_initialized;
};

HelpWindow::HelpWindow( const std::string& helpBookPath )
  : m_impl(0),
  m_helpPath( helpBookPath )    
{}

HelpWindow::~HelpWindow(){
  if ( m_impl != 0 ){
    m_impl->FaintCloseFrame();
  }
}

void HelpWindow::Show(){
  if ( m_impl == 0 ){
    m_impl = new HelpWindowImpl( m_helpPath );
  }
  m_impl->FaintShow();  
}

bool HelpWindow::HasFocus() const {
  return m_impl != 0 && m_impl->HasFocus();
}

BEGIN_EVENT_TABLE(HelpFrameImpl, wxHtmlHelpFrame)
EVT_CLOSE(HelpFrameImpl::OnClose)
END_EVENT_TABLE()


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
#include "wx/aui/auibook.h"
#include "wx/filename.h"
#include "app/getappcontext.hh"
#include "gui/canvasscroller.hh"
#include "gui/tabcontrol.hh"
#include "util/image.hh"
#include "util/save.hh"

wxString generate_title(){
  return "Untitled";
}

wxString get_filename( const wxString& path ){
  if ( path.size() == 0 ){
    return "Untitled";
  }
  return faint::GetFilename( std::string(path) ); // Fixme: what!
}

wxString name_from_canvas( CanvasScroller* canvas ){
  wxString filename = get_filename( canvas->GetFilename() );
  if ( canvas->IsDirty() ){
    filename += " *";
  }
  return filename;
}

class TabControlImpl : public wxAuiNotebook {
public:
  TabControlImpl( wxWindow* parent )
    : wxAuiNotebook( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE | wxWANTS_CHARS )
  {
    SetCanFocus( false );
    SetAcceleratorTable( wxNullAcceleratorTable );
  }

  void Close( size_t page ){
    // Create a close event so OnPageClose is called to avoid duplication
    // between calls to CloseActiveTab and built in tab close (the x-button).
    wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, GetId() );
    e.SetSelection( page );
    e.SetEventObject( this );
    GetEventHandler()->ProcessEvent(e);
  }

  void CloseActive(){
    if ( GetPageCount() == 0 ){
      return;
    }
    Close( GetSelection() );
  }

  CanvasScroller* GetCanvasPage( size_t i ){
    wxWindow* page = GetPage( i );
    assert( page != 0 );
    CanvasScroller* canvas( dynamic_cast<CanvasScroller*>(page) );
    assert( canvas != 0 );
    return canvas;
  }

  void OnPageChanged( wxAuiNotebookEvent& event ){
    event.Skip();
    GetAppContext().OnActiveCanvasChanged();
  }

  void OnPageChanging( wxAuiNotebookEvent& event ){
    int rawSelection = GetSelection();
    // Selection is -1 if the change was due to page close
    if ( GetPageCount() > 1 && rawSelection >= 0){
      size_t selection = static_cast<size_t>(rawSelection);
      assert( selection < GetPageCount() );
      CanvasScroller* canvas = GetCanvasPage(selection);
      canvas->Preempt(PreemptOption::ALLOW_COMMAND);
    }
    event.Skip();
  }

  void OnPageClose( wxAuiNotebookEvent& event ){
    event.Veto();
    size_t page = event.GetSelection();
    CanvasScroller* canvas = GetCanvasPage(page);

    if ( canvas->IsDirty() ){
      wxString filename = canvas->GetFilename().size() != 0 ? wxString("'") + canvas->GetFilename() + "'" : "untitled";
      wxMessageDialog msgDlg( this, wxString("Save changes to ") + filename + "?", "Unsaved Changes", wxYES_NO|wxCANCEL );
      int choice = msgDlg.ShowModal();
      if ( choice == wxID_YES  ){
        bool saved = Save( canvas->GetInterface() );
        if ( !saved ){
          return;
        }
      }
      else if ( choice == wxID_CANCEL ){
        return;
      }
    }

    if ( GetPageCount() == 1 ){
      // Remove annyoing flashing appearance on windows when closing
      // last tab. This freeze must only be done when the last tab is
      // closed, because it causes a refresh error if the tabcontrol
      // is split (see issue 86).
      Freeze();
      DeletePage(page);
      Thaw();
    }
    else {
      DeletePage(page);
    }
    if ( GetPageCount() == 0 ){
      Freeze(); // Avoid flicker the upper-left of the tab-bar from page creation
      ImageProps props(GetAppContext().GetDefaultImageInfo());
      AddPage( new CanvasScroller( this, props ),
        generate_title(),
        false );
      Thaw();
    }
  }

  void RefreshTabName( int index ){
    CanvasScroller* canvas = GetCanvasPage(index);
    SetPageText( index, name_from_canvas(canvas) );
  }

  void RefreshTabName(const CanvasId& id ){
    for ( size_t i = 0; i != GetPageCount(); i++ ){
      CanvasScroller* canvas = GetCanvasPage( i );
      if ( canvas->GetCanvasId() == id ){
        RefreshTabName(i);
        return;
      }
    }
  }
private:
  DECLARE_EVENT_TABLE()
};

TabControl::TabControl( wxWindow* parent ){
  m_impl = new TabControlImpl( parent );
}

wxWindow* TabControl::AsWindow(){
  return m_impl;
}

void TabControl::Close( size_t i ){
  assert( m_impl->GetPageCount() > i );
  m_impl->Close( i );
}

void TabControl::CloseActive(){
  m_impl->CloseActive();
}

CanvasScroller* TabControl::GetActiveCanvas(){
  assert(m_impl->GetPageCount() != 0 );
  size_t page = m_impl->GetSelection();
  CanvasScroller* canvas = m_impl->GetCanvasPage( page );
  return canvas;
}

CanvasScroller* TabControl::GetCanvas( size_t page ){
  assert( page < m_impl->GetPageCount() );
  CanvasScroller* cb = m_impl->GetCanvasPage( page );
  return cb;
}

size_t TabControl::GetCanvasCount(){
  return m_impl->GetPageCount();
}

bool TabControl::Has( const CanvasId& id ){
  for ( size_t num = 0; num != m_impl->GetPageCount(); num++ ){
    if ( id == m_impl->GetCanvasPage(num)->GetCanvasId() ){
      return true;
    }
  }
  return false;
}

CanvasScroller* TabControl::NewDocument( ImageProps& props, const change_tab& changeTab ){
  m_impl->Freeze(); // Avoid flicker the upper-left of the tab-bar from page creation
  CanvasScroller* canvas = new CanvasScroller( m_impl, props );
  m_impl->AddPage( canvas, generate_title(), changeTab.Get() );
  m_impl->Thaw();
  return canvas;
}

CanvasScroller* TabControl::NewDocument( std::vector<ImageProps>& props, const change_tab& changeTab ){
  m_impl->Freeze(); // Avoid flicker the upper-left of the tab-bar from page creation
  CanvasScroller* canvas = new CanvasScroller( m_impl, props );
  m_impl->AddPage( canvas, generate_title(), changeTab.Get() );
  m_impl->Thaw();
  return canvas;
}

void TabControl::RefreshTabName(const CanvasId& id ){
  assert(Has(id));
  m_impl->RefreshTabName( id );
}

void TabControl::Select( const CanvasId& id ){
  for ( size_t num = 0; num != m_impl->GetPageCount(); ++num ){
    if ( id == m_impl->GetCanvasPage(num)->GetCanvasId()) {
      m_impl->SetSelection(num);
      return;
    }
  }
  assert( false );
}

void TabControl::SelectNext(){
  m_impl->AdvanceSelection();
}

void TabControl::SelectPrevious(){
  m_impl->AdvanceSelection( false );
}

void TabControl::ShowTabs( bool show ){
  const int autoSize = -1;
  m_impl->SetTabCtrlHeight(show ? autoSize : 0 );
}

bool TabControl::UnsavedDocuments() const{
  for ( size_t i = 0; i != m_impl->GetPageCount(); i++ ){
    CanvasScroller* canvas = m_impl->GetCanvasPage(i);
    if ( canvas->IsDirty() ){
      return true;
    }
  }
  return false;
}

BEGIN_EVENT_TABLE(TabControlImpl, wxAuiNotebook)
EVT_AUINOTEBOOK_PAGE_CLOSE( -1, TabControlImpl::OnPageClose)
EVT_AUINOTEBOOK_PAGE_CHANGED( -1, TabControlImpl::OnPageChanged)
EVT_AUINOTEBOOK_PAGE_CHANGING( -1, TabControlImpl::OnPageChanging)
END_EVENT_TABLE()

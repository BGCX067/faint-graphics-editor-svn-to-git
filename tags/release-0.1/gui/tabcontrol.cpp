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
#include "tabcontrol.hh"
#include "canvasscroller.hh"
#include "getappcontext.hh"
#include "save.hh"

wxString GenerateTitle(){
  return "Untitled";
}

wxString GetFilename( const wxString& path ){
  if ( path.size() == 0 ){
    return "Untitled";
  }
  return faint::GetFilename( std::string(path) );
}

wxString nameFromCanvas( CanvasScroller* canvas ){
  wxString filename = GetFilename( canvas->GetFilename() );
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

  CanvasScroller* GetCanvasPage( size_t i ){
    wxWindow* page = GetPage( i );
    assert( page != 0 );
    CanvasScroller* canvas( dynamic_cast<CanvasScroller*>(page) );
    assert( canvas != 0 );
    return canvas;
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

    // Remove annyoing flashing appearance on windows when closing
    // last tab
    Freeze();
    DeletePage( page );
    Thaw();
    if ( GetPageCount() == 0 ){
      AddPage( new CanvasScroller( this, GetAppContext().GetDefaultCanvasInfo(), std::vector<Object*>() ),
        GenerateTitle(),
        false );
    }
  }

  void OnPageChanged( wxAuiNotebookEvent& event ){
    event.Skip();
    GetAppContext().OnDocumentChange();
  }

  void OnPageChanging( wxAuiNotebookEvent& event ){
    int rawSelection = GetSelection();
    // Selection is -1 if the change was due to page close
    if ( GetPageCount() > 1 && rawSelection >= 0){
      size_t selection = static_cast<size_t>(rawSelection);
      assert( selection < GetPageCount() );
      CanvasScroller* canvas = GetCanvasPage(selection);
      canvas->Preempt();
    }
    event.Skip();
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

  void RefreshTabName( int index ){
    CanvasScroller* canvas = GetCanvasPage(index);
    SetPageText( index, nameFromCanvas(canvas) );
  }

  void RefreshTabName(const CanvasScroller* const c ){
    for ( size_t i = 0; i != GetPageCount(); i++ ){
      CanvasScroller* canvas = GetCanvasPage( i );
      if ( canvas == c ){
        RefreshTabName(i);
        return;
      }
    }
  }

private:
  DECLARE_EVENT_TABLE()
};

size_t TabControl::GetCanvasCount(){
  return pImpl->GetPageCount();
}

bool TabControl::UnsavedDocuments() const{
  for ( size_t i = 0; i != pImpl->GetPageCount(); i++ ){
    CanvasScroller* canvas = pImpl->GetCanvasPage(i);
    if ( canvas->IsDirty() ){
      return true;
    }
  }
  return false;
}

TabControl::TabControl( wxWindow* parent ){
  pImpl = new TabControlImpl( parent );
}

CanvasScroller* TabControl::NewDocument( const CanvasInfo& info, bool changePage ){
  CanvasScroller* canvas = new CanvasScroller( pImpl, info, std::vector<Object*>() );
  pImpl->AddPage( canvas, GenerateTitle(), changePage );
  return canvas;
}

CanvasScroller* TabControl::NewDocument( ImageProps& props, bool changePage ){
  CanvasScroller* canvas = new CanvasScroller( pImpl, props );
  pImpl->AddPage( canvas, GenerateTitle(), changePage );
  return canvas;
}

CanvasScroller* TabControl::NewDocument( const CanvasInfo& info, const std::vector<Object*>& objects, bool changePage ){
  CanvasScroller* canvas = new CanvasScroller( pImpl, info, objects );
  pImpl->AddPage( canvas, GenerateTitle(), changePage );
  return canvas;
}

CanvasScroller* TabControl::NewDocument( wxString title, const faint::Bitmap& bitmap, bool changePage){
  CanvasScroller* canvas = new CanvasScroller( pImpl, bitmap );
  canvas->SetFilename( std::string(title) );
  pImpl->AddPage( canvas, title.size() == 0 ? GenerateTitle() : nameFromCanvas(canvas), changePage );
  return canvas;
}

void TabControl::SelectNext(){
  pImpl->AdvanceSelection();
}

void TabControl::SelectPrevious(){
  pImpl->AdvanceSelection( false );
}

void TabControl::CloseActive(){
  pImpl->CloseActive();
}

void TabControl::Close( size_t i ){
  if ( pImpl->GetPageCount() <= i ){
    return;
  }
  pImpl->Close( i );
}

CanvasScroller* TabControl::GetActiveCanvas(){
  if ( pImpl->GetPageCount() > 0 ){
    size_t page = pImpl->GetSelection();
    CanvasScroller* canvas = pImpl->GetCanvasPage( page );
    return canvas;
  }
  return 0;
}

CanvasScroller* TabControl::GetCanvas( size_t page ){
  assert( page < pImpl->GetPageCount() );
  CanvasScroller* cb = pImpl->GetCanvasPage( page );
  return cb;
}

wxWindow* TabControl::Window(){
  return pImpl;
}

void TabControl::RefreshActiveTabName(){
  size_t page = pImpl->GetSelection();
  pImpl->RefreshTabName( page );
}

void TabControl::RefreshTabName(const CanvasScroller* const c ){
  pImpl->RefreshTabName( c );
}

void TabControl::ShowWindowMenu(){
  pImpl->ShowWindowMenu(); // Garbage though!
}

void TabControl::ShowTabs( bool show ){
  const int autoSize = -1;
  pImpl->SetTabCtrlHeight(show ? autoSize : 0 );
}

BEGIN_EVENT_TABLE(TabControlImpl, wxAuiNotebook)
EVT_AUINOTEBOOK_PAGE_CLOSE( -1, TabControlImpl::OnPageClose)
EVT_AUINOTEBOOK_PAGE_CHANGED( -1, TabControlImpl::OnPageChanged)
EVT_AUINOTEBOOK_PAGE_CHANGING( -1, TabControlImpl::OnPageChanging)
END_EVENT_TABLE()

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

#include "wx/aui/auibook.h"
#include "wx/dnd.h"
#include "wx/filename.h"
#include "gui/canvas-panel.hh"
#include "gui/freezer.hh"
#include "gui/tab-ctrl.hh"
#include "text/formatting.hh"
#include "util/canvas.hh"
#include "util/convert-wx.hh"
#include "util/gui-util.hh"
#include "util/image-props.hh"
#include "util/save.hh"

namespace faint{

const wxEventType FAINT_ACTIVE_CANVAS_CHANGE = wxNewEventType();
const wxEventTypeTag<CanvasChangeEvent> EVT_FAINT_ACTIVE_CANVAS_CHANGE(FAINT_ACTIVE_CANVAS_CHANGE);

static wxString get_title(CanvasPanel* canvas ){
  const Optional<FilePath> filePath(canvas->GetFilePath());
  wxString title = filePath.IsSet() ?
    to_wx(filePath.Get().StripPath().Str()) :
    get_new_canvas_title();

  if ( canvas->IsDirty() ){
    title += " *";
  }
  return title;
}

class CanvasFileDropTarget : public wxFileDropTarget {
public:
  CanvasFileDropTarget(AppContext& app)
    : m_app(app)
  {}

  bool OnDropFiles(wxCoord, wxCoord, const wxArrayString& files) override{
    m_app.Load(to_FileList(files));
    return true;
  }
  AppContext& m_app;
};

const auto style = wxAUI_NB_DEFAULT_STYLE | wxWANTS_CHARS;

class TabCtrlImpl : public wxAuiNotebook {
public:
  TabCtrlImpl(wxWindow* parent, AppContext& app, StatusInterface& status)
    : wxAuiNotebook(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style),
      m_app(app),
      m_statusInfo(status)
  {
    SetCanFocus(false);
    SetAcceleratorTable( wxNullAcceleratorTable );
    typedef TabCtrlImpl Me;
    Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &Me::OnPageClose, this);
    Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &Me::OnPageChanged, this);
    Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGING, &Me::OnPageChanging, this);
    Bind(EVT_FAINT_CANVAS_CHANGE,
      [&](CanvasChangeEvent& event){
        RefreshTabName(event.GetCanvasId());
        event.Skip();
      });
  }

  void Close( int page ){
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
    Close(GetSelection());
  }

  CanvasPanel* GetCanvasPage( int i ){
    wxWindow* page = GetPage( to_size_t(i) );
    assert( page != nullptr );
    CanvasPanel* canvas( dynamic_cast<CanvasPanel*>(page) );
    assert( canvas != nullptr );
    return canvas;
  }

  CanvasPanel* NewDocument(ImageProps&& props, const change_tab& changeTab,
    const initially_dirty& startDirty)
  {
    // Avoid flicker in the upper-left of the tab-bar from page
    // creation
    auto freezer = freeze(this);

    CanvasPanel* canvas = new CanvasPanel(this, ImageList(std::move(props)), startDirty,
    CreateFileDropTarget(), m_app, m_statusInfo);
    AddPage( canvas, get_title(canvas), changeTab.Get() );
    return canvas;
  }

  CanvasPanel* NewDocument(std::vector<ImageProps>&& props,
    const change_tab& changeTab, const initially_dirty& startDirty)
  {
    // Avoid flicker in the upper-left of the tab-bar from page
    // creation
    auto freezer = freeze(this);

    CanvasPanel* canvas = new CanvasPanel(this, ImageList(std::move(props)), startDirty,
      CreateFileDropTarget(), m_app, m_statusInfo);
    AddPage(canvas, get_title(canvas), changeTab.Get());
    return canvas;
  }

  void OnPageChanged( wxAuiNotebookEvent&){
    const CanvasPanel* canvas = GetCanvasPage(GetSelection());
    CanvasChangeEvent newEvent(FAINT_ACTIVE_CANVAS_CHANGE,
      canvas->GetCanvasId());
    ProcessEvent(newEvent);
  }

  void OnPageChanging( wxAuiNotebookEvent& event ){
    int selection = GetSelection();

    // Selection is -1 if the change was due to page close
    if ( GetPageCount() > 1 && selection >= 0){
      assert( selection < resigned(GetPageCount()) );
      CanvasPanel* canvas = GetCanvasPage(selection);
      canvas->Preempt(PreemptOption::ALLOW_COMMAND);
    }
    event.Skip();
  }

  void OnPageClose( wxAuiNotebookEvent& event ){
    event.Veto();
    int page = event.GetSelection();
    CanvasPanel* canvas = GetCanvasPage(page);
    canvas->Preempt(PreemptOption::ALLOW_COMMAND);
    if ( canvas->IsDirty() ){
      SaveChoice choice = ask_close_unsaved_tab(this, canvas->GetFilePath());
      if ( choice == SaveChoice::CANCEL ){
        return;
      }

      if ( choice == SaveChoice::YES ){
        bool saved = m_app.Save( canvas->GetInterface() );
        if ( !saved ){
          return;
        }
      }
    }

    if ( GetPageCount() == 1 ){
      // Remove annyoing flashing appearance on windows when closing
      // last tab. This freeze must only be done when the last tab is
      // closed, because it causes a refresh error if the tabcontrol
      // is split (see issue 86).
      auto freezer = freeze(this);
      DeletePage(to_size_t(page));
    }
    else {
      DeletePage(to_size_t(page));
    }
    if ( GetPageCount() == 0 ){
      // Avoid flicker in the upper-left of the tab-bar from page
      // creation
      auto freezer = freeze(this);

      AddPage(new CanvasPanel(this,
          ImageList(ImageProps(m_app.GetDefaultImageInfo())),
          initially_dirty(false),
          CreateFileDropTarget(), m_app, m_statusInfo),
        get_new_canvas_title(),
        false );
    }
  }

  void RefreshTabName(int index){
    CanvasPanel* canvas = GetCanvasPage(index);
    SetPageText( to_size_t(index), get_title(canvas) );
  }

  void RefreshTabName(const CanvasId& id ){
    const int numPages = resigned(GetPageCount());
    for ( int i = 0; i != numPages; i++ ){
      CanvasPanel* canvas = GetCanvasPage(i);
      if ( canvas->GetCanvasId() == id ){
        RefreshTabName(i);
        return;
      }
    }
  }

  wxFileDropTarget* CreateFileDropTarget(){
    return new CanvasFileDropTarget(m_app);
  }
private:
  AppContext& m_app;
  StatusInterface& m_statusInfo;
};

TabCtrl::TabCtrl(wxWindow* parent, AppContext& app, StatusInterface& status){
  m_impl = new TabCtrlImpl( parent, app, status);
  m_defaultTabHeight = m_impl->GetTabCtrlHeight();
}

wxWindow* TabCtrl::AsWindow(){
  return m_impl;
}

void TabCtrl::Close( int i ){
  assert( i >= 0 );
  assert( to_size_t(i) < m_impl->GetPageCount() );
  m_impl->Close(i);
}

void TabCtrl::CloseActive(){
  m_impl->CloseActive();
}

CanvasPanel* TabCtrl::GetActiveCanvas(){
  assert(m_impl->GetPageCount() != 0 );
  int page = m_impl->GetSelection();
  CanvasPanel* canvas = m_impl->GetCanvasPage(page );
  return canvas;
}

CanvasPanel* TabCtrl::GetCanvas( int page ){
  assert(page >= 0);
  assert( to_size_t(page) < m_impl->GetPageCount() );
  CanvasPanel* cb = m_impl->GetCanvasPage( page );
  return cb;
}

int TabCtrl::GetCanvasCount(){
  return resigned(m_impl->GetPageCount());
}

bool TabCtrl::Has( const CanvasId& id ){
  const int numPages = resigned(m_impl->GetPageCount());
  for ( int num = 0; num != numPages; num++ ){
    if ( id == m_impl->GetCanvasPage(num)->GetCanvasId() ){
      return true;
    }
  }
  return false;
}

void TabCtrl::HideTabs(){
  m_defaultTabHeight = m_impl->GetTabCtrlHeight();
  m_impl->SetTabCtrlHeight(0);
}

CanvasPanel* TabCtrl::NewDocument(ImageProps&& props,
  const change_tab& changeTab, const initially_dirty& startDirty )
{
  return m_impl->NewDocument(std::move(props), changeTab, startDirty);
}

CanvasPanel* TabCtrl::NewDocument( std::vector<ImageProps>&& props,
  const change_tab& changeTab, const initially_dirty& startDirty )
{
  return m_impl->NewDocument(std::move(props), changeTab, startDirty);
}

void TabCtrl::Select( const CanvasId& id ){
  const int numPages = resigned(m_impl->GetPageCount());
  for (int num = 0; num != numPages; ++num){
    if ( id == m_impl->GetCanvasPage(num)->GetCanvasId()) {
      m_impl->SetSelection(to_size_t(num));
      return;
    }
  }
  assert( false );
}

void TabCtrl::SelectNext(){
  m_impl->AdvanceSelection();
}

void TabCtrl::SelectPrevious(){
  m_impl->AdvanceSelection( false );
}

void TabCtrl::ShowTabs(){
  m_impl->SetTabCtrlHeight(m_defaultTabHeight);
}

bool TabCtrl::UnsavedDocuments() const{
  const int numPages = resigned(m_impl->GetPageCount());
  for ( int i = 0; i != numPages; i++ ){
    CanvasPanel* canvas = m_impl->GetCanvasPage(i);
    if ( canvas->IsDirty() ){
      return true;
    }
  }
  return false;
}

} // namespace

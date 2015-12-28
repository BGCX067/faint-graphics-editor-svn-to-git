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
#include "gui/canvas-change-event.hh"
#include "gui/canvas-panel.hh"
#include "gui/freezer.hh"
#include "gui/tab-ctrl.hh"
#include "util/image-props.hh"
#include "util-wx/convert-wx.hh"
#include "util-wx/gui-util.hh"

namespace faint{

const wxEventType FAINT_ACTIVE_CANVAS_CHANGE = wxNewEventType();
const wxEventTypeTag<CanvasChangeEvent> EVT_FAINT_ACTIVE_CANVAS_CHANGE(FAINT_ACTIVE_CANVAS_CHANGE);

static wxString get_title(CanvasPanel* canvas){
  const Optional<FilePath> filePath(canvas->GetFilePath());
  wxString title = filePath.IsSet() ?
    to_wx(filePath.Get().StripPath().Str()) :
    get_new_canvas_title();

  if (canvas->IsDirty()){
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
    SetAcceleratorTable(wxNullAcceleratorTable);

    Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE,
      [this](wxAuiNotebookEvent& event){
        event.Veto();
        int page = event.GetSelection();
        CanvasPanel* canvas = GetCanvasPage(index_t(page));
        canvas->Preempt(PreemptOption::ALLOW_COMMAND);
        if (canvas->IsDirty()){
          SaveChoice choice = ask_close_unsaved_tab(this, canvas->GetFilePath());
          if (choice == SaveChoice::CANCEL){
            return;
          }

          if (choice == SaveChoice::YES){
            bool saved = m_app.Save(canvas->GetInterface());
            if (!saved){
              return;
            }
          }
        }

        if (GetPageCount() == 1){
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
        if (GetPageCount() == 0){
          // Avoid flicker in the upper-left of the tab-bar from page
          // creation
          auto freezer = freeze(this);

          AddPage(new CanvasPanel(this,
              ImageList(ImageProps(m_app.GetDefaultImageInfo())),
              initially_dirty(false),
              CreateFileDropTarget(), m_app, m_statusInfo),
            get_new_canvas_title(),
            false);
        }
      });

    Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED,
      [&](wxAuiNotebookEvent&){
        const CanvasPanel* canvas = GetCanvasPage(index_t(GetSelection()));
        CanvasChangeEvent newEvent(FAINT_ACTIVE_CANVAS_CHANGE,
          canvas->GetCanvasId());
        ProcessEvent(newEvent);
      });

    Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGING,
      [this](wxAuiNotebookEvent& event){
        int selection = GetSelection();

        // Selection is -1 if the change was due to page close
        if (GetPageCount() > 1 && selection >= 0){
          assert(selection < resigned(GetPageCount()));
          CanvasPanel* canvas = GetCanvasPage(index_t(selection));
          canvas->Preempt(PreemptOption::ALLOW_COMMAND);
        }
        event.Skip();
      });

    Bind(EVT_FAINT_CANVAS_CHANGE,
      [&](CanvasChangeEvent& event){
        RefreshTabName(event.GetCanvasId());
        event.Skip();
      });
  }

  void Close(const index_t& page){
    // Create a close event so that its handler is called to avoid
    // duplication between calls to CloseActiveTab and built in tab
    // close (the x-button).
    wxAuiNotebookEvent e(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, GetId());
    e.SetSelection(page.Get());
    e.SetEventObject(this);
    GetEventHandler()->ProcessEvent(e);
  }

  void CloseActive(){
    if (GetPageCount() == 0){
      return;
    }
    Close(index_t(GetSelection()));
  }

  CanvasPanel* GetCanvasPage(const index_t& i){
    wxWindow* page = GetPage(to_size_t(i));
    assert(page != nullptr);
    CanvasPanel* canvas(dynamic_cast<CanvasPanel*>(page));
    assert(canvas != nullptr);
    return canvas;
  }

  Optional<index_t> GetIndexForId(const CanvasId& id){
    const index_t numPages(resigned(GetPageCount()));
    for (index_t i(0); i != numPages; ++i){
      const CanvasPanel* canvas = GetCanvasPage(i);
      if (canvas->GetCanvasId() == id){
        return option(i);
      }
    }
    return {};
  }

  CanvasPanel* NewDocument(ImageProps&& props, const change_tab& changeTab,
    const initially_dirty& startDirty)
  {
    // Avoid flicker in the upper-left of the tab-bar from page
    // creation
    auto freezer = freeze(this);

    CanvasPanel* canvas = new CanvasPanel(this, ImageList(std::move(props)), startDirty,
    CreateFileDropTarget(), m_app, m_statusInfo);
    AddPage(canvas, get_title(canvas), changeTab.Get());
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

  void RefreshTabName(index_t i){
    CanvasPanel* canvas = GetCanvasPage(i);
    SetPageText(to_size_t(i), get_title(canvas));
  }

  void RefreshTabName(const CanvasId& id){
    RefreshTabName(GetIndexForId(id).Get());
  }

  wxFileDropTarget* CreateFileDropTarget(){
    return new CanvasFileDropTarget(m_app);
  }
private:
  AppContext& m_app;
  StatusInterface& m_statusInfo;
};

TabCtrl::TabCtrl(wxWindow* parent, AppContext& app, StatusInterface& status){
  m_impl = new TabCtrlImpl(parent, app, status);
  m_defaultTabHeight = m_impl->GetTabCtrlHeight();
}

wxWindow* TabCtrl::AsWindow(){
  return m_impl;
}

void TabCtrl::Close(const index_t& i){
  assert(i < GetCanvasCount());
  m_impl->Close(i);
}

void TabCtrl::CloseActive(){
  m_impl->CloseActive();
}

CanvasPanel* TabCtrl::GetActiveCanvas(){
  assert(m_impl->GetPageCount() != 0);
  index_t page(m_impl->GetSelection());
  CanvasPanel* canvas = m_impl->GetCanvasPage(page);
  return canvas;
}

CanvasPanel* TabCtrl::GetCanvas(const index_t& page){
  assert(page < GetCanvasCount());
  CanvasPanel* cb = m_impl->GetCanvasPage(page);
  return cb;
}

index_t TabCtrl::GetCanvasCount(){
  return index_t(resigned(m_impl->GetPageCount()));
}

bool TabCtrl::Has(const CanvasId& id){
  return m_impl->GetIndexForId(id).IsSet();
}

void TabCtrl::HideTabs(){
  m_defaultTabHeight = m_impl->GetTabCtrlHeight();
  m_impl->SetTabCtrlHeight(0);
}

CanvasPanel* TabCtrl::NewDocument(ImageProps&& props,
  const change_tab& changeTab, const initially_dirty& startDirty)
{
  return m_impl->NewDocument(std::move(props), changeTab, startDirty);
}

CanvasPanel* TabCtrl::NewDocument(std::vector<ImageProps>&& props,
  const change_tab& changeTab, const initially_dirty& startDirty)
{
  return m_impl->NewDocument(std::move(props), changeTab, startDirty);
}

void TabCtrl::Select(const CanvasId& id){
  auto index = m_impl->GetIndexForId(id);
  m_impl->SetSelection(to_size_t(index.Get()));
}

void TabCtrl::SelectNext(){
  m_impl->AdvanceSelection();
}

void TabCtrl::SelectPrevious(){
  m_impl->AdvanceSelection(false);
}

void TabCtrl::ShowTabs(){
  m_impl->SetTabCtrlHeight(m_defaultTabHeight);
}

bool TabCtrl::UnsavedDocuments() const{
  const index_t numPages(resigned(m_impl->GetPageCount()));
  for (index_t i(0); i != numPages; ++i){
    CanvasPanel* canvas = m_impl->GetCanvasPage(i);
    if (canvas->IsDirty()){
      return true;
    }
  }
  return false;
}

} // namespace

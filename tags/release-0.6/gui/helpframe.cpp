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

#include <map>
#include <fstream>
#include "helpframe.hh"
#include "util/guiutil.hh"
#include "util/unique.hh"
#include "wx/wx.h"
#include "wx/html/htmlwin.h"
#include "wx/splitter.h"
#include "wx/treectrl.h"
DECLARE_EVENT_TYPE(EVT_CLOSE_HELP, -1)
DEFINE_EVENT_TYPE(EVT_CLOSE_HELP)

DECLARE_EVENT_TYPE(EVT_MAXIMIZE_HELP, -1)
DEFINE_EVENT_TYPE(EVT_MAXIMIZE_HELP)

DECLARE_EVENT_TYPE(EVT_BACK_HELP, -1)
DEFINE_EVENT_TYPE(EVT_BACK_HELP)

DECLARE_EVENT_TYPE(EVT_SCROLL_HELP_END, -1)
DEFINE_EVENT_TYPE(EVT_SCROLL_HELP_END)

DECLARE_EVENT_TYPE(EVT_SCROLL_HELP_HOME, -1)
DEFINE_EVENT_TYPE(EVT_SCROLL_HELP_HOME)

DECLARE_EVENT_TYPE(EVT_SCROLL_HELP_LINE_UP, -1)
DEFINE_EVENT_TYPE(EVT_SCROLL_HELP_LINE_UP)

DECLARE_EVENT_TYPE(EVT_SCROLL_HELP_LINE_DOWN, -1)
DEFINE_EVENT_TYPE(EVT_SCROLL_HELP_LINE_DOWN)

DECLARE_EVENT_TYPE(EVT_SCROLL_HELP_PAGE_DOWN, -1)
DEFINE_EVENT_TYPE(EVT_SCROLL_HELP_PAGE_DOWN)

DECLARE_EVENT_TYPE(EVT_SCROLL_HELP_PAGE_UP, -1)
DEFINE_EVENT_TYPE(EVT_SCROLL_HELP_PAGE_UP)

bool send_event( wxWindow* window, int eventId){
  wxCommandEvent newEvent(eventId);
  window->GetEventHandler()->ProcessEvent(newEvent);
  return true;
}

bool common_help_key( wxWindow* window, const wxKeyEvent& event ){
  // Handle key-presses common to windows within the HelpFrame. The
  // window parameter is the window that received the event, and is used
  // for propagating new events to the frame
  if ( event.GetKeyCode() == WXK_F1 || event.GetKeyCode() == WXK_ESCAPE ){
    return send_event(window, EVT_CLOSE_HELP);
  }
  else if ( event.GetKeyCode() == WXK_RETURN && event.AltDown() ){
    return send_event(window, EVT_MAXIMIZE_HELP);
  }
  else if ( event.GetKeyCode() == WXK_BACK ){
    return send_event(window, EVT_BACK_HELP);
  }
  else if ( event.GetKeyCode() == WXK_SPACE || event.GetKeyCode() == WXK_PAGEDOWN ){
    return send_event(window, EVT_SCROLL_HELP_PAGE_DOWN);
  }
  else if ( event.GetKeyCode() == WXK_PAGEUP ){
    return send_event(window, EVT_SCROLL_HELP_PAGE_UP);
  }
  else if ( event.GetKeyCode() == WXK_UP && event.ControlDown() ){
    return send_event(window, EVT_SCROLL_HELP_LINE_UP);
  }
  else if ( event.GetKeyCode() == WXK_DOWN && event.ControlDown() ){
    return send_event(window, EVT_SCROLL_HELP_LINE_DOWN);
  }
  else if ( event.GetKeyCode() == WXK_HOME ){
    return send_event(window, EVT_SCROLL_HELP_HOME);
  }
  else if ( event.GetKeyCode() == WXK_END ){
    return send_event(window, EVT_SCROLL_HELP_END);
  }
  return false;
}

wxSplitterWindow* create_help_splitter(wxWindow* parent){
  wxSplitterWindow* splitter = new wxSplitterWindow(parent);

  // Prevent unsplitting
  splitter->SetMinimumPaneSize(20);

  // Only grow the right window when rescaling
  splitter->SetSashGravity(0.0);
  return splitter;
}

class HelpWindow : public wxHtmlWindow{
// The html area for the help-text. Uses the basic wxHtmlWindow to
// avoid heavier dependencies (like wxWebView), this should be enough
// for the help system.
public:
  HelpWindow(wxWindow* parent)
    : wxHtmlWindow(parent)
  {}

  void FaintLineDown(){
    wxPoint pos = GetViewStart();
    Scroll(pos.x, pos.y + 1);
  }

  void FaintLineUp(){
    wxPoint pos = GetViewStart();
    Scroll(pos.x, pos.y - 1);
  }

  void FaintPageDown(){
    int steps = GetScrollPageSize(wxVERTICAL);
    wxPoint pos = GetViewStart();
    Scroll(pos.x, pos.y + steps - m_keep);
  }

  void FaintPageUp(){
    int steps = GetScrollPageSize(wxVERTICAL);
    wxPoint pos = GetViewStart();
    Scroll(pos.x, pos.y - steps + m_keep);
  }

  void FaintHome(){
    wxPoint pos = GetViewStart();
    Scroll(pos.x, 0);
  }

  void FaintEnd(){
    wxPoint pos = GetViewStart();
    int x, y;
    GetVirtualSize(&x, &y);
    Scroll(pos.x, y);
  }
private:
  void OnKeyDown(wxKeyEvent& event){
    bool handled = common_help_key(this, event);
    if ( !handled ){
      event.Skip();
    }
  }
  void OnSize(wxSizeEvent& event){
    m_updateScroll.Set(GetViewStart());
    event.Skip();
  }

private:
  void OnIdle(wxIdleEvent&){
    // The wxHtmlWindow scrolls to 0,0 on resize, which is annoying.
    // this is an attempt at a workaround, but I guess it should adjust
    // for the changed size.
    if( m_updateScroll.IsSet() ){
      wxPoint pos(m_updateScroll.Clear());
      Scroll(pos.x, pos.y);
    }
  }
  // (Approximate-) lines to keep from the current page while
  // scrolling a page, to make it easier to follow scrolling with pgdn, pgup
  static const int m_keep = 2;
  Optional<wxPoint> m_updateScroll;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(HelpWindow, wxHtmlWindow)
EVT_KEY_DOWN(HelpWindow::OnKeyDown)
EVT_SIZE(HelpWindow::OnSize)
EVT_IDLE(HelpWindow::OnIdle)
END_EVENT_TABLE()

class HelpTree;
typedef Unique<std::string, HelpTree, 0> page_filename;
typedef std::map<wxTreeItemId, page_filename> page_map_t;

void insert_page(const wxTreeItemId& id, const page_filename& filename, page_map_t& pages ){
  pages.insert(std::make_pair(id, filename));
}

class HelpTree : public wxTreeCtrl{
// The tree-based table of contents for the HelpFrame.
public:
  HelpTree( wxWindow* parent, const std::string& contentsFile ) :
    wxTreeCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE|wxTR_HIDE_ROOT|wxTR_NO_LINES|wxTR_TWIST_BUTTONS)
  {
    SetInitialSize(wxSize(100,100));
    m_root = AddRoot("");
    CreateFromFile(contentsFile);
  }

  wxTreeItemId AddPage( const std::string& title, const page_filename& page ){
    return AddPage(m_root, title, page );
  }

  wxTreeItemId AddPage( const wxTreeItemId& parent, const std::string& title, const page_filename& page ){
    wxTreeItemId id = AppendItem(parent, title);
    m_idToPage.insert(std::make_pair(id, page));
    return id;
  }

  page_filename GetPage( const wxTreeItemId& id ){
    page_map_t::const_iterator it = m_idToPage.find(id);
    assert(it != m_idToPage.end());
    return it->second;
  }

  void Next(){
    wxTreeItemId selected = GetSelection();
    if ( !selected.IsOk() ){
      return;
    }
    wxTreeItemId next = GetNextSibling(selected);
    if ( !next.IsOk() ){
      return;
    }
    SelectItem(next);
  }

  void Prev(){
    wxTreeItemId selected = GetSelection();
    if ( !selected.IsOk() ){
      return;
    }
    wxTreeItemId prev = GetPrevSibling(selected);
    if ( !prev.IsOk() ){
      return;
    }
    SelectItem(prev);
  }

  void SelectPage( const page_filename& name ){
    for ( page_map_t::iterator it = m_idToPage.begin(); it != m_idToPage.end(); ++it ){
      if ( it->second == name ){
	SelectItem(it->first);
      }
    }
  }
private:
  void CreateFromFile( const std::string& contentsFile ){
    std::ifstream f(contentsFile.c_str());
    std::string s;
    wxTreeItemId currentParent = m_root;
    while ( std::getline(f,s) ){
      if ( s.empty() ){
	break;
      }
      size_t sep = s.find(";");
      assert(sep != std::string::npos);
      page_filename filename(s.substr(sep + 1));
      std::string name = s.substr(0, sep);
      bool child = name[0] == '>';
      if ( child ){
	name = name.substr(1);
      }
      if ( child ){
	AddPage(currentParent, name, filename);
      }
      else {
	currentParent = AddPage(name, filename);
      }
    }
  }

  void OnKeyDown(wxKeyEvent& event){
    bool handled = common_help_key(this, event);
    if ( !handled ){
      event.Skip();
    }
  }

  page_map_t m_idToPage;
  wxTreeItemId m_root;

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(HelpTree, wxTreeCtrl)
EVT_KEY_DOWN(HelpTree::OnKeyDown)
END_EVENT_TABLE()

const int help_toolbar_back=0;
const int help_toolbar_forward=1;

page_filename parse_page_filename( const wxString& str ){
  size_t pos = str.rfind("/");
  if ( pos == wxString::npos ){
    pos = str.rfind("\\");
  }
  if ( pos == wxString::npos ){
    return page_filename("");
  }
  return page_filename(std::string(str.substr(pos + 1)));
}

page_filename link_to_filename( const wxString& str ){
  size_t pos = str.rfind("#");
  if ( pos == wxString::npos){
    return page_filename(std::string(str));
  }
  return page_filename(std::string(str.substr(0, pos)));
}

class HelpFrameImpl : public wxFrame {
public:
  HelpFrameImpl(const std::string& rootDir)
    : wxFrame(null_parent(), wxID_ANY, "Faint Help"),
      m_html(0),
      m_tree(0),
      m_rootDir(rootDir),
      m_initialized(false)
  {
    wxSplitterWindow* splitter = create_help_splitter(this);
    m_tree = new HelpTree(splitter, m_rootDir + "/contents.dat");
    m_html = new HelpWindow(splitter);
    splitter->SplitVertically(m_tree, m_html);
    splitter->SetSashPosition(200);
    wxToolBar* toolbar = new wxToolBar(this, wxID_ANY);
    toolbar->AddTool(help_toolbar_back, "Back", wxBitmap(m_rootDir + "/images/back.png", wxBITMAP_TYPE_PNG));
    toolbar->AddTool(help_toolbar_forward, "Forward", wxBitmap(m_rootDir + "/images/forward.png", wxBITMAP_TYPE_PNG));
    toolbar->Realize();
    SetToolBar(toolbar);
  }

  bool FaintHasFocus(){
    return HasFocus() || m_html->HasFocus() || m_tree->HasFocus();
  }

  void FaintShow(){
    if ( !m_initialized ){
      m_html->LoadFile(wxFileName(m_rootDir + "/main.html"));
      m_initialized = true;
    }
    Show();
  }
private:
  void GoBack(){
    m_html->HistoryBack();
    UpdateTreeSelection();
  }
  void GoForward(){
    m_html->HistoryForward();
    UpdateTreeSelection();
  }

  void OnBackHelp(wxCommandEvent&){
    GoBack();
  }

  void OnClose(wxCloseEvent& event){
    if ( event.CanVeto() ){
      // Hide instead of close if possible
      event.Veto();
      Hide();
    }
    else {
      Destroy();
    }
  }

  // Handles custom EVT_CLOSE_HELP from contained windows.
  void OnCloseFaintHelp(wxCommandEvent&){
    // Non-forcing close
    Close();
  }

  void OnLink(wxHtmlLinkEvent& evt){
    m_tree->SelectPage(link_to_filename(evt.GetLinkInfo().GetHref()));
    evt.Skip();
  }

  void OnMaximizeHelp(wxCommandEvent&){
    Maximize(!IsMaximized());
  }

  void OnToolbar(wxCommandEvent& evt){
    if ( evt.GetId() == help_toolbar_back ){
      GoBack();
    }
    else if ( evt.GetId() == help_toolbar_forward ){
      GoForward();
    }
  }

  void OnScrollHelpHome(wxCommandEvent&){
    m_html->FaintHome();
  }


  void OnScrollHelpEnd(wxCommandEvent&){
    m_html->FaintEnd();
  }

  void OnScrollHelpLineDown(wxCommandEvent&){
    if ( m_tree->HasFocus() ){
      m_html->FaintLineDown();
    }
    else {
      m_tree->Next();
    }
  }

  void OnScrollHelpLineUp(wxCommandEvent&){
    if ( m_tree->HasFocus() ){
      m_html->FaintLineUp();
    }
    else {
      m_tree->Prev();
    }
  }

  void OnScrollHelpPageDown(wxCommandEvent&){
    m_html->FaintPageDown();
  }

  void OnScrollHelpPageUp(wxCommandEvent&){
    m_html->FaintPageUp();
  }

  void OnTreeSelection(wxTreeEvent& evt){
    evt.Skip();

    page_filename page(m_tree->GetPage(evt.GetItem()));
    wxFileName htmlFile( m_rootDir + "/" + page.Get() );
    m_html->LoadFile(htmlFile);
    m_tree->SetItemBold(evt.GetItem());
    wxTreeItemId old = evt.GetOldItem();
    if ( old.IsOk() ){
      m_tree->SetItemBold(evt.GetOldItem(), false);
    }
  }

  void UpdateTreeSelection(){
    m_tree->SelectPage(parse_page_filename(m_html->GetOpenedPage()));
  }
  HelpWindow* m_html;
  HelpTree* m_tree;
  const std::string m_rootDir;
  bool m_initialized;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(HelpFrameImpl, wxFrame)
EVT_CLOSE(HelpFrameImpl::OnClose)
EVT_COMMAND(-1, EVT_CLOSE_HELP, HelpFrameImpl::OnCloseFaintHelp)
EVT_COMMAND(-1, EVT_MAXIMIZE_HELP, HelpFrameImpl::OnMaximizeHelp)
EVT_COMMAND(-1, EVT_BACK_HELP, HelpFrameImpl::OnBackHelp)
EVT_HTML_LINK_CLICKED(-1, HelpFrameImpl::OnLink)
EVT_COMMAND(-1, EVT_SCROLL_HELP_END, HelpFrameImpl::OnScrollHelpEnd)
EVT_COMMAND(-1, EVT_SCROLL_HELP_HOME, HelpFrameImpl::OnScrollHelpHome)
EVT_COMMAND(-1, EVT_SCROLL_HELP_LINE_DOWN, HelpFrameImpl::OnScrollHelpLineDown)
EVT_COMMAND(-1, EVT_SCROLL_HELP_LINE_UP, HelpFrameImpl::OnScrollHelpLineUp)
EVT_COMMAND(-1, EVT_SCROLL_HELP_PAGE_DOWN, HelpFrameImpl::OnScrollHelpPageDown)
EVT_COMMAND(-1, EVT_SCROLL_HELP_PAGE_UP, HelpFrameImpl::OnScrollHelpPageUp)
EVT_TOOL(-1, HelpFrameImpl::OnToolbar)
EVT_TREE_SEL_CHANGED(-1, HelpFrameImpl::OnTreeSelection)
END_EVENT_TABLE()

HelpFrame::HelpFrame(const std::string& rootDir){
  m_impl = new HelpFrameImpl(rootDir);
}

HelpFrame::~HelpFrame(){
  if ( m_impl != 0 ){
    Close();
  }
  // Note: m_impl should not be deleted as wxFrame deletion is handled
  // by wxWidgets
}

void HelpFrame::Close(){
  m_impl->Close(true);
  // Note: m_impl should not be deleted as wxFrame deletion is handled
  // by wxWidgets
  m_impl = 0;
}

bool HelpFrame::FaintHasFocus() const{
  return m_impl->FaintHasFocus();
}

wxFrame* HelpFrame::GetRawFrame(){
  return m_impl;
}

bool HelpFrame::IsHidden() const{
  return !IsShown();
}

bool HelpFrame::IsIconized() const{
  return m_impl->IsIconized();
}

bool HelpFrame::IsShown() const{
  return m_impl->IsShown();
}

void HelpFrame::Raise(){
  m_impl->Raise();
}

void HelpFrame::Restore(){
  m_impl->Restore();
}

void HelpFrame::SetIcons( const wxIcon& icon16, const wxIcon& icon32 ){
  m_impl->SetIcons(faint::bundle_icons(icon16, icon32));
}

void HelpFrame::Show(){
  m_impl->FaintShow();
}

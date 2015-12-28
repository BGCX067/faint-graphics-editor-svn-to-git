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

#include "python/pyinterface.hh"
#include "wx/wx.h"
#include "menubar.hh"
#include "gui/menupredicate.hh"
#include "gui/canvasscroller.hh"
#include "gui/interpreterframe.hh"
#include "app.hh"

using faint::MenuPred;
using faint::BoundMenuPred;
const int ID_CROP = wxNewId();
// Fixme: wxNewId is deprecated. Figure out proper way to generate event ids
const int ID_MAXIMIZE = wxNewId();
const int ID_FULLSCREEN = wxNewId();
const int ID_PASTE_NEW = wxNewId();
const int ID_SELECT_ALL = wxNewId();
const int ID_SELECT_NONE = wxNewId();
const int ID_RESIZE_DIALOG = wxNewId();
const int ID_FLIP_ROTATE_DIALOG = wxNewId();
const int ID_TOGGLE_TOOLPANEL = wxNewId();
const int ID_TOGGLE_COLORBAR = wxNewId();
const int ID_TOGGLE_STATUSBAR = wxNewId();
const int ID_CONSOLE = wxNewId();
const int ID_OBJECT_BROWSER = wxNewId();
const int ID_GROUP = wxNewId();
const int ID_UNGROUP = wxNewId();
const int ID_MOVE_UP = wxNewId();
const int ID_MOVE_DOWN = wxNewId();
const int ID_MOVE_FRONT = wxNewId();
const int ID_MOVE_BACK = wxNewId();
const int ID_FLATTEN = wxNewId();
const int ID_CLEAR_RECENT = wxNewId();
const int ID_OPEN_ALL_RECENT = wxNewId();

class MenuEventHandler : public wxEvtHandler {
private:
  void OnCut( wxCommandEvent& ){
    GetActiveCanvas()->CutSelection();
  }

  void OnAbout( wxCommandEvent& ){
    GetFrame()->ShowAboutDialog();
  }

  void OnClearRecentFiles( wxCommandEvent& ){
    GetFrame()->ClearRecentFiles();
  }

  void OnCloseTab( wxCommandEvent& ){
    GetFrame()->CloseActiveTab();
  }

  void OnCopy( wxCommandEvent& ){
    GetActiveCanvas()->CopySelection();
  }

  void OnCrop( wxCommandEvent& ){
    GetActiveCanvas()->Crop();
  }

  void OnDelete( wxCommandEvent& ){
    GetActiveCanvas()->DeleteSelection();
  }

  void OnFlatten( wxCommandEvent& ){
    GetActiveCanvas()->Flatten();
  }

  void OnGroup( wxCommandEvent& ){
    GetActiveCanvas()->GroupSelected();
  }

  void OnMoveObjectDown( wxCommandEvent& ){
    GetActiveCanvas()->MoveObjectDown();
  }

  void OnMoveObjectUp( wxCommandEvent& ){
    GetActiveCanvas()->MoveObjectUp();
  }

  void OnMoveObjectToFront( wxCommandEvent& ){
    GetActiveCanvas()->MoveObjectFront(); // Fixme: Rename canvas function as well
  }

  void OnMoveObjectToBack( wxCommandEvent& ){
    GetActiveCanvas()->MoveObjectBack();
  }

  void OnNewDocument( wxCommandEvent& ){
    MainFrame* frame = GetFrame();
    frame->NewDocument( frame->GetDefaultCanvasInfo() ); // Fixme: Move GetDefaultCanvasInfo out of MainFrame
  }

  void OnNextTab( wxCommandEvent& ){
    GetFrame()->NextTab();
  }

  void OnOpen( wxCommandEvent& ){
    GetFrame()->ShowOpenFileDialog();
  }

  void OnOpenRecent( wxCommandEvent& event ){
    int index = event.GetId() - wxID_FILE1;
    GetFrame()->OpenRecent( index );
  }

  void OnOpenAllRecentFiles( wxCommandEvent& ){
    GetFrame()->OpenAllRecent();
  }

  void OnPaste( wxCommandEvent& ){
    GetActiveCanvas()->Paste();
  }

  void OnQuit( wxCommandEvent& ){
    GetFrame()->Quit();
  }

  void OnRedo( wxCommandEvent& ){
    GetActiveCanvas()->Redo();
  }

  void OnSave( wxCommandEvent& ){
    GetFrame()->Save( GetActiveCanvas()->GetInterface() );
  }

  void OnSaveAs( wxCommandEvent& ){
    GetFrame()->ShowSaveAsDialog( GetActiveCanvas()->GetInterface() );
  }

  void OnUndo( wxCommandEvent& ){
    GetActiveCanvas()->Undo();
  }

  void OnUngroup( wxCommandEvent& ){
    GetActiveCanvas()->UngroupSelected();
  }

  void OnHelp( wxCommandEvent& ){
    GetFrame()->ShowHelp();
  }

  void OnMaximize( wxCommandEvent& ){
    GetFrame()->ToggleMaximize();
  }

  void OnPythonConsole( wxCommandEvent& ){
    InterpreterFrame* interpreter = GetInterpreterFrame();
    if ( interpreter->IsShown() ) {
      if ( interpreter->IsIconized() ){
	interpreter->Restore();
      }
      interpreter->Raise();
    }
    else {
      interpreter->Enable();
      interpreter->Show();
    }
  }

  void OnPrevTab( wxCommandEvent& ){
    GetFrame()->PreviousTab(); // Fixme
  }

  void OnResizeDialog( wxCommandEvent& ){
    GetFrame()->ShowResizeDialog();
  }

  void OnSelectAll( wxCommandEvent& ){
    GetFrame()->SelectAll(); // Fixme
  }

  void OnSelectNone( wxCommandEvent& ){
    GetFrame()->SelectNone(); // Fixme
  }

  void OnToggleColorbar( wxCommandEvent& event ){
    GetFrame()->ToggleColorbar( event.IsChecked() );
  }

  void OnToggleFullScreen( wxCommandEvent& ){
    MainFrame* frame = GetFrame();
    frame->FaintFullScreen( !frame->IsFullScreen() );
  }

  void OnToggleToolPanel( wxCommandEvent& event ){
    GetFrame()->ToggleToolPanel( event.IsChecked() );
  }

  void OnToggleStatusbar( wxCommandEvent& event ){
    GetFrame()->ToggleStatusbar( event.IsChecked() );
  }

  void OnPasteNew( wxCommandEvent& ){
    GetFrame()->PasteNew();
  }

  void OnRotateDialog( wxCommandEvent& ){
    GetFrame()->ShowRotateDialog();
  }

  void OnZoomIn( wxCommandEvent& ){
    GetActiveCanvas()->ChangeZoom( ZoomLevel::NEXT );
  }

  void OnZoomOut( wxCommandEvent& ){
    GetActiveCanvas()->ChangeZoom( ZoomLevel::PREVIOUS );
  }

  void OnZoomDefault( wxCommandEvent& ){
    GetActiveCanvas()->ChangeZoom( ZoomLevel::DEFAULT );
  }

  MainFrame* GetFrame(){
    return wxGetApp().GetFrame();
  }

  InterpreterFrame* GetInterpreterFrame(){
    return wxGetApp().GetInterpreterFrame();
  }

  CanvasScroller* GetActiveCanvas(){
    return GetFrame()->GetActiveCanvas();
  }

  DECLARE_EVENT_TABLE()
};

// Fixme: Replace with Connect-calls in Add-steps
BEGIN_EVENT_TABLE(MenuEventHandler, wxEvtHandler )
EVT_MENU(ID_CLEAR_RECENT, MenuEventHandler::OnClearRecentFiles )
EVT_MENU(ID_CONSOLE, MenuEventHandler::OnPythonConsole )
EVT_MENU(ID_CROP, MenuEventHandler::OnCrop )
EVT_MENU(ID_FLATTEN, MenuEventHandler::OnFlatten )
EVT_MENU(ID_FLIP_ROTATE_DIALOG, MenuEventHandler::OnRotateDialog )
EVT_MENU(ID_FULLSCREEN, MenuEventHandler::OnToggleFullScreen )
EVT_MENU(ID_GROUP, MenuEventHandler::OnGroup )
EVT_MENU(ID_MAXIMIZE, MenuEventHandler::OnMaximize )
EVT_MENU(ID_MOVE_BACK, MenuEventHandler::OnMoveObjectToBack)
EVT_MENU(ID_MOVE_DOWN, MenuEventHandler::OnMoveObjectDown)
EVT_MENU(ID_MOVE_FRONT, MenuEventHandler::OnMoveObjectToFront)
EVT_MENU(ID_MOVE_UP, MenuEventHandler::OnMoveObjectUp)
EVT_MENU(ID_OPEN_ALL_RECENT, MenuEventHandler::OnOpenAllRecentFiles )
EVT_MENU(ID_PASTE_NEW, MenuEventHandler::OnPasteNew )
EVT_MENU(ID_RESIZE_DIALOG, MenuEventHandler::OnResizeDialog )
EVT_MENU(ID_SELECT_ALL, MenuEventHandler::OnSelectAll )
EVT_MENU(ID_SELECT_NONE, MenuEventHandler::OnSelectNone )
EVT_MENU(ID_TOGGLE_COLORBAR, MenuEventHandler::OnToggleColorbar )
EVT_MENU(ID_TOGGLE_STATUSBAR, MenuEventHandler::OnToggleStatusbar )
EVT_MENU(ID_TOGGLE_TOOLPANEL, MenuEventHandler::OnToggleToolPanel )
EVT_MENU(ID_UNGROUP, MenuEventHandler::OnUngroup )
EVT_MENU(wxID_ABOUT, MenuEventHandler::OnAbout)
EVT_MENU(wxID_BACKWARD, MenuEventHandler::OnPrevTab )
EVT_MENU(wxID_COPY, MenuEventHandler::OnCopy )
EVT_MENU(wxID_CUT, MenuEventHandler::OnCut )
EVT_MENU(wxID_DELETE, MenuEventHandler::OnDelete)
EVT_MENU(wxID_EXIT, MenuEventHandler::OnQuit )
EVT_MENU_RANGE( wxID_FILE1, wxID_FILE1 + 10, MenuEventHandler::OnOpenRecent )
EVT_MENU(wxID_FORWARD, MenuEventHandler::OnNextTab )
EVT_MENU(wxID_HELP, MenuEventHandler::OnHelp )
EVT_MENU(wxID_NEW, MenuEventHandler::OnNewDocument )
EVT_MENU(wxID_OPEN, MenuEventHandler::OnOpen)
EVT_MENU(wxID_PASTE, MenuEventHandler::OnPaste )
EVT_MENU(wxID_REDO, MenuEventHandler::OnRedo)
EVT_MENU(wxID_REMOVE, MenuEventHandler::OnCloseTab )
EVT_MENU(wxID_SAVE, MenuEventHandler::OnSave)
EVT_MENU(wxID_SAVEAS, MenuEventHandler::OnSaveAs)
EVT_MENU(wxID_UNDO, MenuEventHandler::OnUndo)
EVT_MENU(wxID_ZOOM_IN, MenuEventHandler::OnZoomIn )
EVT_MENU(wxID_ZOOM_OUT, MenuEventHandler::OnZoomOut )
EVT_MENU(wxID_ZOOM_100, MenuEventHandler::OnZoomDefault )
END_EVENT_TABLE()

void Menubar::Initialize(){
  m_wxMenubar = new wxMenuBar();
  wxMenu* fileMenu = new wxMenu();
  Add(fileMenu, wxID_NEW, Label("New\tCtrl+N", "Create a new image"));
  Add(fileMenu, wxID_OPEN, Label("&Open...\tCtrl+O", "Open an existing image"));

  wxMenu* recent = new wxMenu();
  m_recentFilesMenu = recent;
  Add(recent, ID_OPEN_ALL_RECENT, Label("Open &All", "Open all recently used files") );
  Add(recent, ID_CLEAR_RECENT, Label("&Clear", "Clear the recent files list")) ;

  fileMenu->AppendSubMenu( recent, "Open &Recent", "Recent files" );
  fileMenu->AppendSeparator();
  Add(fileMenu, wxID_SAVE, Label("&Save\tCtrl+S", "Save the active image"));
  Add(fileMenu, wxID_SAVEAS, Label("Save &As...\tCtrl+Shift+S", "Save the active image with a new name"));
  fileMenu->AppendSeparator();
  Add(fileMenu, wxID_EXIT, Label("E&xit\tAlt+F4", "Exit Faint") );
  m_wxMenubar->Append(fileMenu, "&File");

  wxMenu* editMenu = new wxMenu();
  Add( editMenu, wxID_UNDO, Label("&Undo\tCtrl+Z", "Undo the last action"));
  Add( editMenu, wxID_REDO, Label("&Redo\tCtrl+Y", "Redo the last undone action"));
  editMenu->AppendSeparator();
  Add( editMenu, wxID_CUT, MenuPred( MenuPred::ANY_SELECTION ) );
  Add( editMenu, wxID_COPY, MenuPred( MenuPred::ANY_SELECTION ) );
  Add( editMenu, wxID_PASTE );  // Fixme: use own id. wxWidgets says 'paste selection'
  Add( editMenu, ID_PASTE_NEW, Label("Paste New\tCtrl+Alt+V", "Paste to a new image") );
  editMenu->AppendSeparator();
  Add( editMenu, ID_CROP, Label("Crop\tCtrl+Alt+C"));
  Add( editMenu, wxID_DELETE, Label("Delete\tDel", TOGGLE_ALL), MenuPred( MenuPred::ANY_SELECTION ) );
  editMenu->AppendSeparator();
  Add( editMenu, ID_SELECT_ALL, Label("Select &All\tCtrl+A") );
  Add( editMenu, ID_SELECT_NONE, Label("Select &None\tCtrl+D"), MenuPred( MenuPred::ANY_SELECTION ) );
  editMenu->AppendSeparator();
  Add( editMenu, ID_CONSOLE, Label("Python &Interpreter...\tF8") );
  m_wxMenubar->Append(editMenu, "&Edit");

  wxMenu* viewMenu = new wxMenu();
  Add( viewMenu, wxID_ZOOM_IN, Label("Zoom In\t+", TOGGLE_ALPHA ) );
  Add( viewMenu, wxID_ZOOM_OUT, Label("Zoom Out\t-", TOGGLE_ALPHA ) );
  Add( viewMenu, wxID_ZOOM_100, Label("Default Zoom\t*", TOGGLE_ALPHA ) );
  viewMenu->AppendSeparator();
  AddCheck( viewMenu, ID_TOGGLE_TOOLPANEL, Label("&Tool Panel", "Show or hide the tool panel") );
  viewMenu->Check( ID_TOGGLE_TOOLPANEL, true ); // Fixme
  AddCheck( viewMenu, ID_TOGGLE_STATUSBAR, Label("&Status Bar", "Show or hide the status bar" ) );
  viewMenu->Check( ID_TOGGLE_STATUSBAR, true ); // Fixme
  AddCheck( viewMenu, ID_TOGGLE_COLORBAR, Label("&Color Panel", "Show or hide the color panel" ) );
  viewMenu->Check( ID_TOGGLE_COLORBAR, true ); // Fixme
  viewMenu->AppendSeparator();
  Add( viewMenu, ID_MAXIMIZE, Label("&Maximize\tAlt+Enter") );
  Add( viewMenu, ID_FULLSCREEN, Label("&Fullscreen\tF11") );
  m_wxMenubar->Append(viewMenu, "&View");

  wxMenu* objectMenu = new wxMenu();
  Add( objectMenu, ID_GROUP, Label("&Group Objects\tCtrl+G" ), MenuPred( MenuPred::OBJECT_SELECTION ) );
  Add( objectMenu, ID_UNGROUP, Label("&Unroup Objects\tCtrl+U" ), MenuPred( MenuPred::OBJECT_SELECTION ) );
  objectMenu->AppendSeparator();
  Add( objectMenu, ID_MOVE_UP, Label("Move &Forward\tF", TOGGLE_ALPHA ), MenuPred( MenuPred::OBJECT_SELECTION ) );
  Add( objectMenu, ID_MOVE_DOWN, Label("Move &Back\tB", TOGGLE_ALPHA ), MenuPred( MenuPred::OBJECT_SELECTION ) );
  Add( objectMenu, ID_MOVE_FRONT, Label("Bring to Front\tCtrl+F"), MenuPred( MenuPred::OBJECT_SELECTION ) );
  Add( objectMenu, ID_MOVE_BACK, Label("Move to Back\tCtrl+B"), MenuPred( MenuPred::OBJECT_SELECTION ) );
  objectMenu->AppendSeparator();
  Add( objectMenu, ID_FLATTEN, Label("Flatten (Rasterize)\tCtrl+Space"), MenuPred( MenuPred::HAS_OBJECTS ) );
  m_wxMenubar->Append(objectMenu, "&Objects");

  wxMenu* imageMenu = new wxMenu();
  Add( imageMenu, ID_FLIP_ROTATE_DIALOG, Label("&Flip/Rotate...\tCtrl+R", "Adjust the image or selection") );
  imageMenu->AppendSeparator();
  Add( imageMenu, ID_RESIZE_DIALOG, Label("&Resize...\tCtrl+E", "Scale or resize the image or selection") );
  m_wxMenubar->Append(imageMenu, "&Image");

  wxMenu* tabMenu = new wxMenu();
  Add( tabMenu, wxID_FORWARD, Label("&Next Tab\tCtrl+Tab") );
  Add( tabMenu, wxID_BACKWARD, Label("&Previous Tab\tCtrl+Shift+Tab") );
  Add( tabMenu, wxID_REMOVE, Label("&Close\tCtrl+W") );
  m_wxMenubar->Append(tabMenu, "&Tabs");

  wxMenu* helpMenu = new wxMenu();
  Add( helpMenu, wxID_HELP, Label("&Help Index\tF1") );
  helpMenu->AppendSeparator();
  Add( helpMenu, wxID_ABOUT, Label("&About", "Information about Faint") );
  m_wxMenubar->Append( helpMenu, "&Help" );
}

Menubar::Menubar(){
  Initialize();
  m_eventHandler = new MenuEventHandler();
}

Menubar::~Menubar(){
  delete m_eventHandler;
}

wxMenuBar* Menubar::GetRawMenubar(){
  return m_wxMenubar;
}

wxMenu* Menubar::GetRecentFilesMenu(){
  return m_recentFilesMenu;
}

wxEvtHandler* Menubar::GetEventHandler(){
  return m_eventHandler;
}

void Menubar::Add( wxMenu* menu, int id ){
  menu->Append( id );
}

void Menubar::AddShortcutDisable( int id, const Label& label ){
  ToggleShortcut toggle = label.GetToggle();
  bool disableOnEntry = ( toggle != TOGGLE_NEVER );
  bool disableNumeric = ( toggle == TOGGLE_ALL );
  if ( disableOnEntry ){
    m_notWhileTexting.push_back( ToggleLabel( id, label.GetLabelText(), disableNumeric ) );
  }
}

void Menubar::Add( wxMenu* menu, int id, const Label& label ){
  menu->Append( id, label.GetLabelText(), label.GetHelpText() );
  AddShortcutDisable( id, label );
}

void Menubar::Add( wxMenu* menu, int id, const Label& label, const MenuPred& predicate ){
  Add( menu, id, label );
  m_menuPredicates.push_back( predicate.GetBound(id) );
}

void Menubar::Add( wxMenu* menu, int id, const MenuPred& predicate ){
  menu->Append( id );
  m_menuPredicates.push_back( predicate.GetBound(id) );
}

void Menubar::AddCheck( wxMenu* menu, int id, const Label& label ){
  menu->AppendCheckItem( id, label.GetLabelText(), label.GetHelpText() );
  AddShortcutDisable( id, label );
}

void Menubar::Update( bool rasterSelection, bool objectSelection, bool hasObjects ){
  for ( std::vector<BoundMenuPred>::const_iterator it = m_menuPredicates.begin(); it != m_menuPredicates.end(); ++it ){
    it->Update( m_wxMenubar, rasterSelection, objectSelection, hasObjects );
  }
}

void Menubar::UpdateZoom( const ZoomLevel& zoom ){
  m_wxMenubar->Enable( wxID_ZOOM_IN, !zoom.AtMax() );
  m_wxMenubar->Enable( wxID_ZOOM_OUT, !zoom.AtMin() );
  m_wxMenubar->Enable( wxID_ZOOM_100, !zoom.At100() );
}

void Menubar::BeginTextEntry( bool numericOnly ){
  for ( std::vector<ToggleLabel>::const_iterator it = m_notWhileTexting.begin(); it != m_notWhileTexting.end(); ++it ){
    it->DisableShortcut( m_wxMenubar, numericOnly );
  }
}

void Menubar::EndTextEntry(){
  for ( std::vector<ToggleLabel>::const_iterator it = m_notWhileTexting.begin(); it != m_notWhileTexting.end(); ++it ){
    it->EnableShortcut( m_wxMenubar );
  }
}

Menubar::ToggleLabel::ToggleLabel( int id, const std::string& label, bool disableNumeric )
  : m_id(id),
    m_disableNumeric( disableNumeric )
{
  size_t tabPos = label.find("\t");
  if ( tabPos != std::string::npos ){
    m_label = label.substr(0,tabPos);
    m_shortcut = label.substr(tabPos);
  }
}

void Menubar::ToggleLabel::EnableShortcut( wxMenuBar* mb ) const{
  mb->SetLabel( m_id, m_label + m_shortcut );
}

void Menubar::ToggleLabel::DisableShortcut( wxMenuBar* mb, bool numeric ) const{
  if ( !numeric || m_disableNumeric ){
    mb->SetLabel( m_id, m_label );
  }
}

Menubar::Label::Label( const char* label, ToggleShortcut toggle )
  : m_label( label ),
    m_toggle( toggle )
{}

Menubar::Label::Label( const char* label, const char* help, ToggleShortcut toggle )
  : m_label( label ),
    m_help( help ),
    m_toggle( toggle )
{}

std::string Menubar::Label::GetLabelText() const{
  return m_label;
}

std::string Menubar::Label::GetHelpText() const{
  return m_help;
}


Menubar::ToggleShortcut Menubar::Label::GetToggle() const{
  return m_toggle;
}


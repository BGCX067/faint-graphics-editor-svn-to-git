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

#include "app/getappcontext.hh"
#include "wx/wx.h"
#include "wx/config.h" // For recent files
#include "wx/docview.h" // For recent files
#include "wx/filename.h" // For recent files
#include "gui/brightness-contrast-dialog.hh"
#include "gui/canvasscroller.hh"
#include "gui/events.hh"
#include "gui/mainframe.hh"
#include "gui/menubar.hh"
#include "gui/menupredicate.hh"
#include "gui/resize-dialog.hh"
#include "gui/rotate-dialog.hh"
#include "gui/threshold-dialog.hh"
#include "gui/interpreterframe.hh"
#include "util/context-commands.hh"
#include "util/image.hh"
using faint::MenuPred;
using faint::BoundMenuPred;

int ID_ZOOM_100_TOGGLE = wxNewId();

void harmonize_zoom( const CanvasInterface& source, AppContext& app ){
  const ZoomLevel zoom(source.GetZoomLevel());
  size_t num = app.GetCanvasCount();
  for ( size_t i = 0; i != num; i++ ){
    CanvasInterface& canvas = app.GetCanvas(i);
    if ( canvas.GetId() != source.GetId() ){
      canvas.SetZoom(zoom);
    }
  }
}

RecentFiles::~RecentFiles(){
}

class RecentFilesImpl : public RecentFiles {
  // Handles the storing, retrieving and menu-states for
  // recent files in the Menubar.
public:
  // The recent file menu receives the list of files,
  // the wxMenuBar is used to modify related items.
  // in that menu,
  RecentFilesImpl( wxMenu* recentFilesMenu, int openAllId, int clearId )
    : m_clearId(clearId),
      m_config("Faint"),
      m_fileHistory(),
      m_menu(recentFilesMenu),
      m_openAllId(openAllId)
  {
    m_fileHistory.UseMenu(recentFilesMenu);
    Load();
  }

  void Add( const faint::FilePath& path ) override{
    m_fileHistory.AddFileToHistory(path.ToWx().GetLongPath());
    UpdateMenu();
  }

  void Clear() override{
    m_cleared.clear();
    for ( size_t i = 0; i != m_fileHistory.GetCount(); i++ ){
      m_cleared.push_back(std::string(m_fileHistory.GetHistoryFile(i)));
    }
    for ( size_t i = m_fileHistory.GetCount(); i != 0; i-- ){
      m_fileHistory.RemoveFileFromHistory( i - 1 );
    }
    Save();
    UpdateMenu();
  }

  faint::FilePath Get(size_t i) const override{
    wxString strPath(m_fileHistory.GetHistoryFile(i));
    return faint::FilePath::FromAbsoluteWx(strPath);
  }

  void Save() override{
    m_fileHistory.Save(m_config);
  }

  size_t Size() const override{
    return m_fileHistory.GetCount();
  }

  void UndoClear() override{
    if ( m_fileHistory.GetCount() != 0 ){
      return;
    }
    for ( size_t i = 0; i != m_cleared.size(); i++ ){
      m_fileHistory.AddFileToHistory((std::string)m_cleared[i]);
    }
    UpdateMenu();
  }
private:
  void Load(){
    m_fileHistory.Load(m_config);
    m_cleared.clear();
    for ( int i = m_fileHistory.GetCount(); i != 0; i-- ){
      wxString str = m_fileHistory.GetHistoryFile(i-1);
      if ( !wxFileName(str).FileExists() ){
        m_fileHistory.RemoveFileFromHistory( i - 1 );
      }
    }
    UpdateMenu();
  }


  void UpdateMenu(){
    bool hasFiles = m_fileHistory.GetCount() != 0;

    if ( hasFiles ){
      m_menu->Enable( m_openAllId, true );
      m_menu->Enable( m_clearId, true );
      m_menu->SetLabel( m_clearId, "&Clear" );
      m_menu->SetHelpString( m_clearId, "Clear Recent Files");
    }
    else {
      m_menu->Enable( m_openAllId, false );
      if ( !m_cleared.empty() ){
	m_menu->Enable( m_clearId, true );
	m_menu->SetLabel( m_clearId, "Undo &Clear" );
	m_menu->SetHelpString( m_clearId, "Undo clearing Recent Files");
      }
      else {
	m_menu->Enable( m_clearId, false );
	m_menu->SetLabel( m_clearId, "&Clear" );
	m_menu->SetHelpString( m_clearId, "Clear Recent Files");
      }
    }
  }
  std::vector<std::string> m_cleared;
  const int m_clearId;
  wxConfig m_config;
  wxFileHistory m_fileHistory;
  wxMenu* m_menu;
  const int m_openAllId;
};

faint::FileList get_all( const RecentFiles& recent ){
  faint::FileList files;
  for ( size_t i = 0; i != recent.Size(); i++ ){
    files.push_back(recent.Get(i));
  }
  return files;
}

namespace faint {
  // Wrapper to allow the MenuPredicate to be unaware of wxMenu.
  class MenuRefWX : public MenuReference{
  public:
    MenuRefWX( wxMenuBar* menu )
      : m_menu(menu)
    {}

    void Append( wxMenu* menu, const wxString& title ){
      m_menu->Append(menu, title);
    }

    void Enable( int itemId, bool enable ) override{
      m_menu->Enable( itemId, enable );
    }

    wxMenuBar* m_menu;
  };
} // namespace faint

class MenuEventHandler : public wxEvtHandler {
public:
  MenuEventHandler( MainFrame& frame, Menubar& menu )
    : m_frame(frame),
      m_menu(menu),
      m_redoing(false),
      m_undoing(false)
  {
    m_frame.PushEventHandler(this);
  }

  ~MenuEventHandler(){
    m_frame.PopEventHandler();
  }

  void OnAbout( wxCommandEvent& ){
    m_frame.ShowAboutDialog();
  }

  void OnCut( wxCommandEvent& ){
    GetActiveCanvas().CutSelection();
  }

  void OnClearRecentFiles( wxCommandEvent& ){
    RecentFiles& recent = m_menu.GetRecentFiles();
    if ( recent.Size() != 0 ){
      recent.Clear();
    }
    else {
      recent.UndoClear();
    }
  }

  void OnCloseTab( wxCommandEvent& ){
    m_frame.CloseActiveTab();
  }

  void OnCopy( wxCommandEvent& ){
    GetActiveCanvas().CopySelection();
  }

  void OnCrop( wxCommandEvent& ){
    CanvasInterface& active = GetActiveCanvas();
    active.RunCommand(context_crop(active));
  }

  void OnDelete( wxCommandEvent& ){
    GetActiveCanvas().ContextDelete();
  }

  void OnDialogBrightnessContrast( wxCommandEvent& ){
    BrightnessContrastDialog dialog;
    GetAppContext().Show( dialog );
  }

  void OnDialogResize( wxCommandEvent& ){
    ResizeDialog dialog;
    GetAppContext().Show(dialog);
  }

  void OnDialogRotate( wxCommandEvent& ){
    RotateDialog dialog;
    GetAppContext().Show( dialog );
  }

  void OnDialogThreshold( wxCommandEvent& ){
    ThresholdDialog dialog;
    GetAppContext().Show( dialog );
  }

  void OnFlatten( wxCommandEvent& ){
    CanvasInterface& active = GetActiveCanvas();
    active.RunCommand(context_flatten(active));
  }

  void OnGroup( wxCommandEvent& ){
    CanvasInterface& active = GetActiveCanvas();
    active.RunCommand(group_selected_objects(active));
  }

  void OnMoveObjectBackward( wxCommandEvent& ){
    CanvasInterface& active = GetActiveCanvas();
    active.RunCommand(context_objects_backward(active));
  }

  void OnMoveObjectForward( wxCommandEvent& ){
    CanvasInterface& active = GetActiveCanvas();
    active.RunCommand(context_objects_forward(active));
  }

  void OnMoveObjectToFront( wxCommandEvent& ){
    CanvasInterface& active = GetActiveCanvas();
    active.RunCommand(context_objects_to_front(active));
  }

  void OnMoveObjectToBack( wxCommandEvent& ){
    CanvasInterface& active = GetActiveCanvas();
    active.RunCommand(context_objects_to_back(active));
  }

  void OnNewDocument( wxCommandEvent& ){
    m_frame.NewDocument( GetAppContext().GetDefaultImageInfo() );
  }

  void OnNextTab( wxCommandEvent& ){
    m_frame.NextTab();
  }

  void OnOpen( wxCommandEvent& ){
    m_frame.ShowOpenFileDialog();
  }

  void OnOpenRecent( wxCommandEvent& event ){
    int index = event.GetId() - wxID_FILE1;
    faint::FilePath filePath(m_menu.GetRecentFiles().Get(index));
    m_frame.Open( filePath, change_tab(true) );
  }

  void OnOpenAllRecentFiles( wxCommandEvent& ){
    m_frame.Open(get_all(m_menu.GetRecentFiles()));
  }

  void OnPaste( wxCommandEvent& ){
    GetActiveCanvas().Paste();
  }

  void OnQuit( wxCommandEvent& ){
    GetAppContext().Quit();
  }

  void OnRedo( wxCommandEvent& ){
    if ( m_redoing || m_undoing){
      return;
    }
    m_redoing = true;
    CanvasInterface& active = GetActiveCanvas();
    active.Redo();
    active.Refresh();
    m_redoing = false;
  }

  void OnSave( wxCommandEvent& ){
    m_frame.Save( GetActiveCanvas() );
  }

  void OnSaveAs( wxCommandEvent& ){
    m_frame.ShowSaveAsDialog( GetActiveCanvas() );
  }

  void OnUndo( wxCommandEvent& ){
    if ( m_undoing || m_redoing ){
      return;
    }
    m_undoing = true;
    CanvasInterface& active = GetActiveCanvas();
    active.Undo();
    active.Refresh();
    m_undoing = false;
  }

  void OnUngroup( wxCommandEvent& ){
    CanvasInterface& active = GetActiveCanvas();
    active.RunCommand(ungroup_selected_objects(active));
  }

  void OnHelp( wxCommandEvent& ){
    GetAppContext().ShowHelpFrame();
  }

  void OnMaximize( wxCommandEvent& ){
    m_frame.ToggleMaximize();
  }

  void OnPythonConsole( wxCommandEvent& ){
    GetAppContext().ShowPythonConsole();
  }

  void OnPrevTab( wxCommandEvent& ){
    m_frame.PreviousTab();
  }

  void OnSelectAll( wxCommandEvent& ){
    CanvasInterface& active = GetActiveCanvas();
    active.RunCommand(context_select_all(active));
  }

  void OnSelectNone( wxCommandEvent& ){
    CanvasInterface& active = GetActiveCanvas();
    active.RunCommand(context_deselect(active));
  }

  void OnToggleColorbar( wxCommandEvent& event ){
    m_frame.ToggleColorbar( event.IsChecked() );
  }

  void OnToggleFullScreen( wxCommandEvent& ){
    m_frame.FaintFullScreen( !m_frame.IsFullScreen() );
  }

  void OnToggleToolPanel( wxCommandEvent& event ){
    m_frame.ToggleToolPanel( event.IsChecked() );
  }

  void OnToggleStatusbar( wxCommandEvent& event ){
    m_frame.ToggleStatusbar( event.IsChecked() );
  }

  void OnPasteNew( wxCommandEvent& ){
    m_frame.PasteNew();
  }

  void OnZoom100( wxCommandEvent& ){
    GetActiveCanvas().ZoomDefault();
  }

  void OnZoom100Toggle( wxCommandEvent& ){
    CanvasInterface& active = GetActiveCanvas();
    if ( active.GetZoomLevel().At100() ){
      active.ZoomFit();
    }
    else {
      active.ZoomDefault();
    }
  }

  void OnZoom100All( wxCommandEvent& ){
    AppContext& app = GetAppContext();
    size_t num = app.GetCanvasCount();
    for ( size_t i = 0; i != num; i++ ){
      app.GetCanvas(i).ZoomDefault();
    }
  }

  void OnZoomFit( wxCommandEvent& ){
    GetActiveCanvas().ZoomFit();
  }

  void OnZoomFitAll( wxCommandEvent& ){
    AppContext& app = GetAppContext();
    size_t num = app.GetCanvasCount();
    for ( size_t i = 0; i != num; i++ ){
      app.GetCanvas(i).ZoomFit();
    }
  }

  void OnZoomIn( wxCommandEvent& ){
    GetActiveCanvas().ZoomIn();
  }

  void OnZoomInAll( wxCommandEvent& ){
    CanvasInterface& active = GetActiveCanvas();
    active.ZoomIn();
    harmonize_zoom( active, GetAppContext() );
  }

  void OnZoomOut( wxCommandEvent& ){
    GetActiveCanvas().ZoomOut();
  }

  void OnZoomOutAll( wxCommandEvent& ){
    CanvasInterface& active = GetActiveCanvas();
    active.ZoomOut();
    harmonize_zoom( active, GetAppContext() );
  }

  CanvasInterface& GetActiveCanvas(){
    return m_frame.GetActiveCanvas()->GetInterface();
  }
private:
  MainFrame& m_frame;
  Menubar& m_menu;
  bool m_redoing;
  bool m_undoing;
};

std::string undo_label( const std::string& cmd ){
  if ( cmd.empty() ){
    return "&Undo\tCtrl+Z";
  }
  return "&Undo " + cmd + "\tCtrl+Z";
}

std::string redo_label( const std::string& cmd ){
  if ( cmd.empty() ){
    return "&Redo\tCtrl+Y";
  }
  return "&Redo " + cmd + "\tCtrl+Y";
}

// Macro for indicating functions in the MenuEventHandler class in a
// more palatable way. The binding to a concrete MenuEventHandler has
// to be done separately, using the inherited wxEvtHandler::Connect.
#define HANDLER(a) wxCommandEventHandler(MenuEventHandler::a)

void Menubar::Initialize(MainFrame& frame){
  m_eventHandler = new MenuEventHandler(frame, *this);
  m_menuRef = new faint::MenuRefWX( new wxMenuBar() );
  wxMenu* fileMenu = new wxMenu();
  Add(fileMenu, wxID_NEW, Label("New\tCtrl+N", "Create a new image"), HANDLER(OnNewDocument));
  Add(fileMenu, wxID_OPEN, Label("&Open...\tCtrl+O", "Open an existing image"), HANDLER(OnOpen));

  wxMenu* recent = new wxMenu();
  m_recentFilesMenu = recent;

  const int openAllRecentId = NewId();
  const int clearRecentId = NewId();

  Add(recent, openAllRecentId, Label("Open &All", "Open all recently used files"), HANDLER(OnOpenAllRecentFiles));
  Add(recent, clearRecentId, Label("&Clear", "Clear the recent files list"), HANDLER(OnClearRecentFiles)) ;

  // Handle connecting of recent files as a special case
  m_eventHandler->Connect(wxID_FILE1, wxID_FILE1 + 10, wxEVT_COMMAND_MENU_SELECTED, HANDLER(OnOpenRecent));

  fileMenu->AppendSubMenu( recent, "Open &Recent", "Recent files" );
  fileMenu->AppendSeparator();
  Add(fileMenu, wxID_SAVE, Label("&Save\tCtrl+S", "Save the active image"), MenuPred(MenuPred::DIRTY), HANDLER(OnSave));
  Add(fileMenu, wxID_SAVEAS, Label("Save &As...\tCtrl+Shift+S", "Save the active image with a new name"), HANDLER(OnSaveAs));
  fileMenu->AppendSeparator();
  Add(fileMenu, wxID_EXIT, Label("E&xit\tAlt+F4", "Exit Faint"), HANDLER(OnQuit));
  m_menuRef->Append(fileMenu, "&File");

  wxMenu* editMenu = new wxMenu();
  Add( editMenu, wxID_UNDO, Label(undo_label(""), "Undo the last action"), MenuPred(MenuPred::CAN_UNDO), HANDLER(OnUndo));
  Add( editMenu, wxID_REDO, Label(redo_label(""), "Redo the last undone action"), MenuPred(MenuPred::CAN_REDO), HANDLER(OnRedo));
  editMenu->AppendSeparator();
  Add( editMenu, wxID_CUT, MenuPred( MenuPred::ANY_SELECTION ), HANDLER(OnCut));
  Add( editMenu, wxID_COPY, MenuPred( MenuPred::ANY_SELECTION ), HANDLER(OnCopy));
  Add( editMenu, wxID_PASTE, Label("&Paste\tCtrl+V", "Paste from clipboard"), HANDLER(OnPaste));
  Add( editMenu, NewId(), Label("Paste N&ew\tCtrl+Alt+V", "Paste to a new image"), HANDLER(OnPasteNew));
  editMenu->AppendSeparator();
  Add( editMenu, NewId(), Label("Cr&op\tCtrl+Alt+C"), HANDLER(OnCrop));
  Add( editMenu, wxID_DELETE, Label("&Delete\tDel"), MenuPred( MenuPred::ANY_SELECTION ), HANDLER(OnDelete)); // Fixme: Should be disabled on numeric entry, but this ruins the use of the delete key in the edit tool
  editMenu->AppendSeparator();
  Add( editMenu, NewId(), Label("Select &All\tCtrl+A"), HANDLER(OnSelectAll));
  Add( editMenu, NewId(), Label("Select &None\tCtrl+D"), MenuPred( MenuPred::ANY_SELECTION), HANDLER(OnSelectNone));
  editMenu->AppendSeparator();
  Add( editMenu, NewId(), Label("Python &Interpreter...\tF8"), HANDLER(OnPythonConsole));
  m_menuRef->Append(editMenu, "&Edit");

  wxMenu* viewMenu = new wxMenu();
  Add( viewMenu, ID_ZOOM_IN, Label("Zoom In\t+", "Zoom in one step", TOGGLE_ALPHABETIC ), HANDLER(OnZoomIn));
  Add( viewMenu, ID_ZOOM_OUT, Label("Zoom Out\t-", "Zoom out one step", TOGGLE_ALPHABETIC ), HANDLER(OnZoomOut));
  Add( viewMenu, ID_ZOOM_100_TOGGLE, Label("Zoom 1:1\t*", TOGGLE_ALPHABETIC ), HANDLER(OnZoom100Toggle));
  viewMenu->AppendSeparator();
  AddCheck( viewMenu, NewId(), Label("&Tool Panel", "Show or hide the tool panel"), HANDLER(OnToggleToolPanel), true);
  AddCheck( viewMenu, NewId(), Label("&Status Bar", "Show or hide the status bar" ), HANDLER(OnToggleStatusbar), true);
  AddCheck( viewMenu, NewId(), Label("&Color Panel", "Show or hide the color panel" ), HANDLER(OnToggleColorbar), true);
  viewMenu->AppendSeparator();
  Add( viewMenu, NewId(), Label("&Maximize\tAlt+Enter"), HANDLER(OnMaximize));
  Add( viewMenu, NewId(), Label("&Fullscreen\tF11"), HANDLER(OnToggleFullScreen));
  m_menuRef->Append(viewMenu, "&View");

  wxMenu* objectMenu = new wxMenu();
  Add( objectMenu, NewId(), Label("&Group Objects\tCtrl+G", "Combine the selected objects into a group" ), MenuPred(MenuPred::MULTIPLE_SELECTED), HANDLER(OnGroup));
  Add( objectMenu, NewId(), Label("&Unroup Objects\tCtrl+U", "Disband the selected group" ), MenuPred( MenuPred::GROUP_IS_SELECTED ), HANDLER(OnUngroup));
  objectMenu->AppendSeparator();
  Add( objectMenu, NewId(), Label("Move &Forward\tF", TOGGLE_ALPHABETIC ), MenuPred( MenuPred::CAN_MOVE_FORWARD ), HANDLER(OnMoveObjectForward));
  Add( objectMenu, NewId(), Label("Move &Backward\tB", TOGGLE_ALPHABETIC ), MenuPred( MenuPred::CAN_MOVE_BACKWARD ), HANDLER(OnMoveObjectBackward) );
  Add( objectMenu, NewId(), Label("Bring to Front\tCtrl+F"), MenuPred( MenuPred::CAN_MOVE_FORWARD ), HANDLER(OnMoveObjectToFront));
  Add( objectMenu, NewId(), Label("Move to Back\tCtrl+B"), MenuPred( MenuPred::CAN_MOVE_BACKWARD ), HANDLER(OnMoveObjectToBack));
  objectMenu->AppendSeparator();
  Add( objectMenu, NewId(), Label("Flatten (Rasterize)\tCtrl+Space", "Flatten the selected objects onto the background"), MenuPred( MenuPred::HAS_OBJECTS ), HANDLER(OnFlatten));
  m_menuRef->Append(objectMenu, "&Objects");

  wxMenu* imageMenu = new wxMenu();
  Add( imageMenu, NewId(), Label("&Brightness/Contrast...", "Adjust the image brightness and contrast"), HANDLER(OnDialogBrightnessContrast));
  Add( imageMenu, NewId(), Label("&Threshold...", "Threshold the image by intensity"), HANDLER(OnDialogThreshold));
  imageMenu->AppendSeparator();
  Add( imageMenu, NewId(), Label("&Flip/Rotate...\tCtrl+R", "Adjust the image or selection"), HANDLER(OnDialogRotate));
  Add( imageMenu, NewId(), Label("&Resize...\tCtrl+E", "Scale or resize the image or selection"), HANDLER(OnDialogResize));
  m_menuRef->Append(imageMenu, "&Image");

  wxMenu* tabMenu = new wxMenu();
  Add( tabMenu, wxID_FORWARD, Label("&Next Tab\tCtrl+Tab"), HANDLER(OnNextTab));
  Add( tabMenu, wxID_BACKWARD, Label("&Previous Tab\tCtrl+Shift+Tab"), HANDLER(OnPrevTab));
  Add( tabMenu, wxID_REMOVE, Label("&Close\tCtrl+W"), HANDLER(OnCloseTab));
  m_menuRef->Append(tabMenu, "&Tabs");

  wxMenu* helpMenu = new wxMenu();
  Add( helpMenu, wxID_HELP, Label("&Help Index\tF1", "Show the help for Faint"), HANDLER(OnHelp));
  helpMenu->AppendSeparator();
  Add( helpMenu, wxID_ABOUT, Label("&About", "Information about Faint"), HANDLER(OnAbout));
  m_menuRef->Append( helpMenu, "&Help" );
  frame.SetMenuBar(m_menuRef->m_menu);
  m_recentFiles = new RecentFilesImpl(m_recentFilesMenu, openAllRecentId, clearRecentId );

  // Events from the zoom control
  m_eventHandler->Connect( ID_ZOOM_100, wxEVT_COMMAND_MENU_SELECTED, HANDLER(OnZoom100));
  m_eventHandler->Connect( ID_ZOOM_IN_ALL, wxEVT_COMMAND_MENU_SELECTED, HANDLER(OnZoomInAll));
  m_eventHandler->Connect( ID_ZOOM_OUT_ALL, wxEVT_COMMAND_MENU_SELECTED, HANDLER(OnZoomOutAll));
  m_eventHandler->Connect( ID_ZOOM_100_ALL, wxEVT_COMMAND_MENU_SELECTED, HANDLER(OnZoom100All));
  m_eventHandler->Connect( ID_ZOOM_FIT, wxEVT_COMMAND_MENU_SELECTED, HANDLER(OnZoomFit));
  m_eventHandler->Connect( ID_ZOOM_FIT_ALL, wxEVT_COMMAND_MENU_SELECTED, HANDLER(OnZoomFitAll));

}

Menubar::Menubar(MainFrame& frame)
  : m_maxId(wxID_HIGHEST+1),
    m_recentFiles(nullptr),
    m_entryMode(EntryMode::NONE)
{
  Initialize(frame);
}

Menubar::~Menubar(){
  delete m_eventHandler;
  delete m_menuRef;
  delete m_recentFiles;
}

void Menubar::AddShortcutDisable( int id, const Label& label ){
  ToggleShortcut toggle = label.GetToggle();
  bool disableOnEntry = ( toggle != TOGGLE_NEVER );
  bool disableNumeric = ( toggle == TOGGLE_ALL );
  if ( disableOnEntry ){
    m_notWhileTexting.push_back( ToggleLabel( id, label.GetLabelText(), disableNumeric ) );
  }
}

void Menubar::Add( wxMenu* menu, int id, const Label& label, const wxObjectEventFunction& handler ){
  menu->Append( id, label.GetLabelText(), label.GetHelpText() );
  AddShortcutDisable( id, label );
  m_eventHandler->Connect(id, wxEVT_COMMAND_MENU_SELECTED, handler);
}

void Menubar::Add( wxMenu* menu, int id, const Label& label, const MenuPred& predicate, const wxObjectEventFunction& handler ){
  Add( menu, id, label, handler );
  m_menuPredicates.push_back( predicate.GetBound(id) );
}

void Menubar::Add( wxMenu* menu, int id, const MenuPred& predicate, const wxObjectEventFunction& handler ){
  menu->Append( id );
  m_menuPredicates.push_back( predicate.GetBound(id) );
  m_eventHandler->Connect(id, wxEVT_COMMAND_MENU_SELECTED, handler);
}

void Menubar::AddCheck( wxMenu* menu, int id, const Label& label, const wxObjectEventFunction& handler, bool checked ){
  menu->AppendCheckItem( id, label.GetLabelText(), label.GetHelpText() );
  AddShortcutDisable( id, label );
  m_eventHandler->Connect(id, wxEVT_COMMAND_MENU_SELECTED, handler);
  menu->Check( id, checked );
}

void Menubar::Update( const faint::MenuFlags& flags ){
  m_menuRef->m_menu->SetLabel( wxID_UNDO, undo_label(flags.undoLabel) );
  m_menuRef->m_menu->SetLabel( wxID_REDO, redo_label(flags.redoLabel) );
  for ( BoundMenuPred& predicate : m_menuPredicates ){
    predicate.Update( *m_menuRef, flags );
  }
}

void Menubar::UpdateZoom( const ZoomLevel& zoom ){
  m_menuRef->Enable( ID_ZOOM_IN, !zoom.AtMax() );
  m_menuRef->Enable( ID_ZOOM_OUT, !zoom.AtMin() );
  if ( zoom.At100() ){
    m_menuRef->m_menu->SetLabel( ID_ZOOM_100_TOGGLE, "Zoom Fit\t*" );
    m_menuRef->m_menu->SetHelpString( ID_ZOOM_100_TOGGLE, "Fit image in view");
  }
  else {
    m_menuRef->m_menu->SetLabel( ID_ZOOM_100_TOGGLE, "Zoom 1:1\t*" );
    m_menuRef->m_menu->SetHelpString( ID_ZOOM_100_TOGGLE, "Zoom to actual size");
  }
}

void Menubar::BeginTextEntry( bool numericOnly ){
  for ( ToggleLabel& label : m_notWhileTexting ){
    label.DisableShortcut( m_menuRef->m_menu, numericOnly );
  }
  m_entryMode = numericOnly ? EntryMode::NUMERIC : EntryMode::ALPHA_NUMERIC;
}

void Menubar::EndTextEntry(){
  for ( ToggleLabel& label : m_notWhileTexting ){
    label.EnableShortcut( m_menuRef->m_menu );
  }
  m_entryMode = EntryMode::NONE;
}

RecentFiles& Menubar::GetRecentFiles(){
  return *m_recentFiles;
}

EntryMode Menubar::GetTextEntryMode() const{
  return m_entryMode;
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

Menubar::Label::Label( const std::string& label, ToggleShortcut toggle )
  : m_label( label ),
    m_toggle( toggle )
{}

Menubar::Label::Label( const std::string& label, const std::string& help, ToggleShortcut toggle )
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

int Menubar::NewId(){
  return m_maxId++;
}

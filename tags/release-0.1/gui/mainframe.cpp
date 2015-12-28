// Copyright 2009 Lukas Kemmer
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You
// may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

#include <fstream>
#include <string>
#include <sstream>
#include "wx/wx.h"
#include "wx/aboutdlg.h"
#include "wx/config.h"
#include "gui/helpwindow.hh"
#include "wx/docview.h" // For recent files
#include "wx/filename.h"
#include "mainframe.hh"
#include "bitmap/cairo_version.h"
#include "gui/canvasscroller.hh"
#include "gui/interpreterframe.hh"
#include "gui/resizedlg/resizedialog.hh"
#include "gui/rotatedlg.hh"
#include "gui/toolbar.hh"
#include "gui/toolsettingpanel.hh"
#include "gui/tabcontrol.hh"
#include "gui/selectedcolorctrl.hh" // For color event identifiers
#include "formats/format_wx.hh"
#include "tools/toolbehavior.hh"
#include "tools/ellipsebehavior.hh"
#include "tools/linebehavior.hh"
#include "tools/objselectbehavior.hh"
#include "tools/rectanglebehavior.hh"
#include "tools/rectselbehavior.hh"
#include "tools/splinebehavior.hh"
#include "tools/fillbehavior.hh"
#include "tools/brushbehavior.hh"
#include "tools/penbehavior.hh"
#include "tools/pickerbehavior.hh"
#include "tools/polygonbehavior.hh"
#include "tools/textbehavior.hh"
#include "tools/settingid.hh"
#include "util/clipboard.hh"
#include "util/convertwx.hh"
#include "util/pathutil.hh"
#include "util/objutil.hh"
#include "wx/spinctrl.h" // Nasty, for cast
#include "gui/toolpanel.hh"
#include "gui/colorpanel.hh"

class FrameNotifier : public SettingNotifier {
public:
  FrameNotifier( MainFrame* frame ){
    m_frame = frame;
  }
  void Notify( const BoolSetting& s, bool value ){
    m_frame->Set( s, value );
  }
  void Notify( const IntSetting& s, int value ){
    m_frame->Set( s, value );
  }
  void Notify( const StrSetting& s, const std::string& value ){
    m_frame->Set( s, value );
  }
  void Notify( const ColorSetting& s, const faint::Color& value ){
    m_frame->Set( s, value );
  }
  void Notify( const FloatSetting& s, FloatSetting::ValueType value ){
    m_frame->Set( s, value );
  }
private:
  MainFrame* m_frame;
};

class SBInterface : public StatusInterface {
public:
  SBInterface( wxStatusBar* statusbar ) :
    m_statusbar( statusbar ) {}
  void SetMainText( const std::string& text ){
    m_statusbar->SetStatusText( wxString( text.c_str() ), 0 );
  }
  void SetText( const std::string& text, int field=0 ){
    m_statusbar->SetStatusText( wxString( text.c_str() ), field + 1 );
  }

private:
  wxStatusBar* m_statusbar;
};

class FrameContext : public AppContext {
public:
  FrameContext( MainFrame* frame, wxStatusBar* statusbar, InterpreterFrame* interpreterFrame )
    : m_frame( frame ),
      m_interpreterFrame( interpreterFrame ),
      m_statusbar( statusbar ),
      m_deleteOnClose(0)
  {}

  ~FrameContext(){
    delete m_deleteOnClose;
  }

  void DialogOpenFile(){
    m_frame->ShowOpenFileDialog();
  }

  CanvasInfo GetDefaultCanvasInfo(){
    return m_frame->GetDefaultCanvasInfo();
  }

  CanvasInterface& NewDocument( size_t w, size_t h){
    return NewDocument( CanvasInfo( w, h, faint::Color( 255, 255, 255 ) ) );
  }

  CanvasInterface& NewDocument( const CanvasInfo& info ){
    CanvasScroller* canvas = m_frame->NewDocument( info );
    return canvas->GetInterface();
  }

  CanvasInterface* Load( const char* path, bool changeTab ){
    return m_frame->Open( wxString( path ), changeTab  );
  };

  void Close( CanvasInterface& canvas ){
    m_frame->CloseDocument( canvas );
  }

  void UpdateShownSettings(){
    m_frame->UpdateShownSettings();
  }

  void UpdateToolSettings( const FaintSettings& s ){
    m_frame->UpdateToolSettings( s );
  }

  CanvasInterface& GetActiveCanvas(){
    return m_frame->GetActiveCanvas()->GetInterface();
  }

  void SelectTool( ToolId id ){
    m_frame->SelectTool( id );
  }

  ToolId GetToolId(){
    return m_frame->GetToolId();
  }

  ToolBehavior* GetActiveTool(){
    return m_frame->GetActiveTool();
  }

  void Set( const BoolSetting& s, BoolSetting::ValueType v ){
    m_frame->Set( s, v );
  }

  void Set( const StrSetting& s, StrSetting::ValueType v){
    m_frame->Set( s, v );
  }

  void Set( const IntSetting& s, IntSetting::ValueType v ){
    m_frame->Set( s, v );
  }

  void Set( const ColorSetting& s, ColorSetting::ValueType v ){
    m_frame->Set( s, v );
  }

  void Set( const FloatSetting& s, FloatSetting::ValueType v ){
    m_frame->Set( s, v );
  }

  BoolSetting::ValueType Get( const BoolSetting& s ){
    return m_frame->GetSetting( s );
  }
  StrSetting::ValueType Get( const StrSetting& s){
    return m_frame->GetSetting( s );
  }
  IntSetting::ValueType Get( const IntSetting& s ){
    return m_frame->GetSetting( s );
  }
  ColorSetting::ValueType Get( const ColorSetting& s ){
    return m_frame->GetSetting( s );
  }
  FloatSetting::ValueType Get( const FloatSetting& s ){
    return m_frame->GetSetting( s );
  }
  void SetLayer( Layer layer ){
    m_frame->SelectLayer( layer );
  }
  Layer GetLayerType() const{
    return m_frame->GetLayerType();
  }

  const FaintSettings& GetToolSettings() const {
    return m_frame->GetToolSettings();
  }

  virtual size_t GetCanvasCount() const {
    return m_frame->GetCanvasCount();
  }

  virtual CanvasInterface& GetCanvas( size_t i ){
    return m_frame->GetCanvas( i );
  };

  virtual bool Exists( const CanvasId& id ){
    return m_frame->Exists( id );
  }

  virtual Point GetMousePos(){
    wxPoint mousePos = wxGetMousePosition();
    return Point( mousePos.x, mousePos.y );
  }

  StatusInterface& GetStatusInfo(){
    return m_statusbar;
  }

  void BeginTextEntry(){
    m_frame->BeginTextEntry();
  }

  void EndTextEntry(){
    m_frame->EndTextEntry();
  }

  virtual void SetInterpreterBackground( const faint::Color& c ){
    m_interpreterFrame->SetBackgroundColour(c);
  }

  virtual void SetInterpreterTextColor( const faint::Color& c ){
    m_interpreterFrame->SetTextColor(c);
  }

  virtual void Quit(){
    m_frame->Close(true);
  }

  virtual void OnDocumentChange(){
    m_frame->OnDocumentChange();
  }

  virtual void OnDocumentStateChange(){
    m_frame->OnStateChange();
  }

  virtual void OnZoomChange(){
    m_frame->OnZoomChange();
  }

  virtual void AddFormat( Format* fileFormat ){
    m_frame->AddFormat( fileFormat );
  }

  virtual std::vector<Format*> GetFormats(){
    return m_frame->GetFileFormats();
  }

  virtual void Maximize(){
    m_frame->Maximize( ! m_frame->IsMaximized() );
  }

  virtual void MaximizeInterpreter(){
    m_interpreterFrame->Maximize( !m_interpreterFrame->IsMaximized() );
  }

  virtual void AddPaletteColor( const faint::Color& c ){
    m_frame->AddColor( c );
  }

  virtual void PythonRunCommand( CanvasInterface* canvas, Command* command ){
    m_unrefreshed.insert( canvas );
    m_canvasId[ canvas ] = canvas->GetId();
    canvas->RunCommand( command );
  }

  virtual void PythonDone(){
    for ( std::set<CanvasInterface*>::iterator it = m_unrefreshed.begin(); it != m_unrefreshed.end(); ++it ){
      CanvasInterface* canvas = *it;
      if ( Exists( m_canvasId[canvas] ) ){
        canvas->Refresh();
      }
    }
    m_unrefreshed.clear();
  }

  void PythonNewPrompt(){
    m_interpreterFrame->NewPrompt();
  }

  void PythonContinuation(){
    m_interpreterFrame->NewContinuation();
  }

  void PythonGetKey(){
    m_interpreterFrame->GetKey();
  }

  void PythonPrint( const std::string& s ){
    m_interpreterFrame->Print(s);
  }

  void PythonIntFaintPrint( const std::string& s ){
    m_interpreterFrame->IntFaintPrint(s);
  }

  bool Bound( int key, int modifiers ) const{
    return m_binds.find( std::make_pair(key, modifiers) ) != m_binds.end();
  }
  void Bind( int key, int modifiers ){
    m_binds.insert( std::make_pair(key, modifiers) );
  }

  void Unbind( int key, int modifiers ){
    m_binds.erase( std::make_pair(key, modifiers) );
  }

  void DeleteOnClose( ToolBehavior* tool ){
    m_deleteOnClose = tool;
  }

private:
  MainFrame* m_frame;
  InterpreterFrame* m_interpreterFrame;
  SBInterface m_statusbar;
  std::set<CanvasInterface*> m_unrefreshed;
  std::map<CanvasInterface*, CanvasId> m_canvasId;
  std::set<std::pair<int,int> > m_binds;
  ToolBehavior* m_deleteOnClose;
};

FaintSettings GetDefaultToolSettings(){
  FaintSettings s;
  s.Set( ts_LineWidth, LITCRD(1.0) );
  s.Set( ts_LineArrowHead, 0 );
  s.Set( ts_LineStyle, faint::SOLID );
  s.Set( ts_LineCap, faint::CAP_BUTT );
  s.Set( ts_FillStyle, BORDER );
  s.Set( ts_Transparency, TRANSPARENT_BG );
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color(255,255,255) );
  wxFont f( 12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false );
  s.Set( ts_FontSize, f.GetPointSize() );
  s.Set( ts_FontFace, std::string(f.GetFaceName()) );
  s.Set( ts_FontBold, false );
  s.Set( ts_FontItalic, false );
  s.Set( ts_BrushSize, 5 );
  s.Set( ts_SwapColors, false );
  s.Set( ts_BrushShape, BRUSH_SQUARE );
  return s;
}

void InitFormats( std::vector<Format*>& formats ){
  formats.push_back( new FormatWX("png", "Portable Network Graphics", wxBITMAP_TYPE_PNG ) );
  formats.push_back( new FormatWX("bmp", "Windows Bitmap", wxBITMAP_TYPE_BMP ) );
  formats.push_back ( new FormatWX("jpg", "JPEG", wxBITMAP_TYPE_JPEG  ) );
  formats.push_back ( new FormatWX("ico", "Icon", wxBITMAP_TYPE_ICO  ) );
}

void InitFileHistory( Menubar* menubar, wxConfig* config, wxFileHistory* fileHistory ){
  fileHistory->UseMenu( menubar->GetRecentFilesMenu() );
  fileHistory->Load( *config );
  for ( int i = fileHistory->GetCount(); i != 0; i-- ){
    wxString str = fileHistory->GetHistoryFile( i - 1 );
    if ( !wxFileName( str ).FileExists() ){
      fileHistory->RemoveFileFromHistory( i - 1 );
    }
  }
}

MainFrame::MainFrame( const wxPoint& pos, const wxSize& size, ArtContainer& art, PaletteContainer& palettes, InterpreterFrame* pythonFrame )
  : wxFrame((wxFrame*)NULL, wxID_ANY, "Faint", pos, size ),
    m_artContainer( art ),
    m_appContext(0),
    m_notifier(0),
    m_menubar(0),
    m_pythonFrame(pythonFrame),
    m_tabControl(0),
    m_toolPanel(0),
    m_colorPanel(0),
    m_helpWindow( GetDataDir() + "/help/header.hhp" ),
    m_layerType( LAYER_RASTER ),
    m_activeTool(0),
    m_toolSettings(GetDefaultToolSettings()),
    m_palettes( palettes ),
    m_fileHistory(0),
    m_config(0),
    m_dialogShown(false),
    m_textEntryCount(0)
{
  m_appContext = new FrameContext( this, CreateFaintStatusBar(), m_pythonFrame );
  m_notifier = new FrameNotifier( this );
  m_menubar = CreateFaintMenubar();
  m_config = new wxConfig( "Faint" );
  m_fileHistory = new wxFileHistory();
  InitFileHistory( m_menubar, m_config, m_fileHistory );
  CreatePanels();

  // Fixme: Wrong place for such stuff, has nothing todo with main
  // frame. Consider App.
  InitFormats( m_formats );

  InitIcons();
  SetMinSize(wxSize(640, 480));
  m_toolPanel->SelectTool(T_LINE);
}

MainFrame::~MainFrame(){
  // Note: Deletion of the AppContext is handled outside MainFrame.
  // Fixme: Creation should be moved outside as well.
  m_appContext = 0;

  delete m_notifier;

  // These do not inherit from wxWindow and need to be deleted
  // manually.
  delete m_menubar;
  delete m_toolPanel;
  delete m_colorPanel;

  for ( size_t i = 0; i!= m_formats.size(); i++ ){
    delete m_formats[i];
  }

  delete m_fileHistory;
  delete m_config;
}

void MainFrame::AddColor( const faint::Color& c ){
  m_colorPanel->AddToPalette(c);
}

CanvasScroller* MainFrame::NewDocument( const CanvasInfo& info ){
  CanvasScroller* canvas = m_tabControl->NewDocument( info, true );
  return canvas;
}

#ifndef FAINT_VERSION
#define FAINT_VERSION "(unknown version)"
#endif

void MainFrame::ShowAboutDialog(){
  wxAboutDialogInfo info;
  info.SetName("Faint");
  info.SetWebSite("http://code.google.com/p/faint-graphics-editor/");
  info.AddDeveloper("Lukas Kemmer");
  info.SetLicense("Copyright 2009 Lukas Kemmer\nLicensed under the Apache License, Version 2.0");
  info.SetVersion(FAINT_VERSION);
  
  std::stringstream ss;
  ss << "wxWidgets version: " << wxMAJOR_VERSION << "."
     << wxMINOR_VERSION << "."
     << wxRELEASE_NUMBER << "."
     << wxSUBRELEASE_NUMBER << std::endl;

  ss << "Python version: " << m_pythonFrame->GetPythonVersion() << std::endl;
  ss << "Cairo version: " << faint::GetCairoVersion();
  info.SetDescription(ss.str());
  wxAboutBox(info);
}

void MainFrame::OpenRecent( int index ){
  wxString file = m_fileHistory->GetHistoryFile( index );
  Open( file, true );
}

void MainFrame::OpenAllRecent(){
  wxArrayString filenames;
  for ( size_t i = 0; i != m_fileHistory->GetCount(); i++ ){
    filenames.Add( m_fileHistory->GetHistoryFile( i ) );
  }
  Open(filenames);
}

void MainFrame::ClearRecentFiles(){
  for ( size_t i = m_fileHistory->GetCount(); i != 0; i-- ){
    m_fileHistory->RemoveFileFromHistory( i - 1 );
  }
  m_fileHistory->Save( *m_config );
}

void MainFrame::OnClose( wxCloseEvent& event ){
  if ( event.CanVeto() && m_tabControl->UnsavedDocuments() ){
    wxMessageDialog msgDialog( this, "One or more files have unsaved changes.\nExit anyway?",
      "Unsaved Changes", wxYES_NO | wxNO_DEFAULT );
    int choice = FaintShowModal( msgDialog );
    if ( choice == wxID_NO ){
      event.Veto();
      return;
    }
  }

  m_fileHistory->Save( *m_config );
  m_pythonFrame->Close( true );
  PopEventHandler();

  if ( wxTheClipboard->Open() ){
    wxTheClipboard->Flush();
    wxTheClipboard->Close();
  }
  event.Skip();
}

void MainFrame::Quit(){
  Close(false);
}

void MainFrame::BeginTextEntry(){
  m_textEntryCount++;
  m_menubar->BeginTextEntry();
}

void MainFrame::SelectLayer( Layer layer ){
  m_toolPanel->SelectLayer( layer );
}

void MainFrame::RefreshActiveCanvas(){
  CanvasScroller* canvas = GetActiveCanvas();
  if ( canvas != 0 ){
    canvas->ExternalRefresh();
  }
}

bool MainFrame::MainFrameFocused() const{
  return !( m_dialogShown ||
    m_helpWindow.HasFocus() ||
    m_pythonFrame->FaintHasFocus() );
}

bool MainFrame::EntryControlFocused() const{
  wxWindow* focused = wxWindow::FindFocus();
  if ( focused == 0 ){
    return false;
  }
  if ( dynamic_cast<wxSpinCtrl*>(focused) != 0 ){
    return true;
  }
  return false;
}

void MainFrame::EndTextEntry(){
  assert( m_textEntryCount > 0 );
  m_textEntryCount--;
  if ( m_textEntryCount == 0 ){
    m_menubar->EndTextEntry();
  }
}

int MainFrame::FaintShowModal( wxDialog& dialog ){
  // wxApp::FilterEvent catches keypresses even when
  // dialogs are shown. Using this function for all dialogs
  // shown by the frame allows handling it.
  m_dialogShown = true;
  int result = dialog.ShowModal();
  m_dialogShown = false;
  return result;
}

bool MainFrame::TextEntryActive() const{
  return m_textEntryCount > 0;
}

void MainFrame::ShowOpenFileDialog(){
  // Use the path from the active canvas, if available,
  // default path otherwise
  wxFileName oldFileName( GetActiveCanvas()->GetFilename() );
  wxFileDialog fd( this, "Open Image(s)", // Parent and caption
    oldFileName.GetPath(), "", // Path and filename
    "*.*", // Filter(s)
    wxFD_OPEN | wxFD_MULTIPLE | wxFD_CHANGE_DIR | wxFD_FILE_MUST_EXIST );

  if ( FaintShowModal(fd) == wxID_OK ){
    wxArrayString paths;
    fd.GetPaths( paths );
    Open( paths );
  }
}

void MainFrame::Open( wxArrayString& filePaths ){
  // Refresh the entire frame to erase any dialog or menu droppings before
  // starting potentially long-running file loading.
  Refresh();
  Update();

  // Freeze the panel to remove some refresh glitches in the
  // tool-settings on MSW during loading.
  m_toolPanel->AsWindow()->Freeze();
  for ( unsigned int i = 0; i!= filePaths.GetCount(); i++ ){
    // Only change tab for the first in a series of opened images.
    bool changePage = ( i == 0 );
    wxString& filePath = filePaths[i];
    Open( filePath, changePage );
  }
  m_toolPanel->AsWindow()->Thaw();
}

AppContext* MainFrame::GetContext(){
  return m_appContext;
}

ToolBehavior* MainFrame::GetActiveTool(){
  return m_activeTool;
}

void MainFrame::SetActiveTool( ToolBehavior* tool ){
  m_activeTool = tool;
}

CanvasInterface* MainFrame::Open(const wxString& filePath, bool changePage=false ){
  // Fixme: Use wxFilename etc.
  // Fixme: Add error check, for example non-existant .png on command line causes exception
  wxString extension = filePath.substr( filePath.size() - 3, 3 );
  for ( unsigned int i = 0; i!= m_formats.size(); i++ ){
    Format* format = m_formats[i];
    if ( format->GetFormat() == extension.Lower() && format->CanLoad() ){
      ImageProps props;
      format->Load( std::string(filePath.c_str()), props );
      if ( !props.IsOk() ){
	std::stringstream ss;
	ss << "Failed loading: " << filePath << std::endl << "Error: " << props.GetError();
	wxMessageDialog(this, ss.str(), "Failed loading", wxOK|wxCENTRE|wxICON_ERROR).ShowModal();
	return 0;
      }
      CanvasScroller* newCanvas = m_tabControl->NewDocument( props, changePage);
      newCanvas->SetFilename( std::string(filePath) );
      m_tabControl->RefreshTabName( newCanvas );
      wxFileName fn( filePath );
      fn.MakeAbsolute();
      m_fileHistory->AddFileToHistory( fn.GetLongPath() );
      return &(newCanvas->GetInterface());
    }
  }

  // Fixme: Report error
  return 0;
}

  void MainFrame::NextTab() {
  m_tabControl->SelectNext();
}

void MainFrame::PreviousTab(){
  m_tabControl->SelectPrevious();
}

void MainFrame::CloseActiveTab(){
  m_tabControl->CloseActive();
}

void MainFrame::PasteNew(){
  if ( !wxTheClipboard->Open() ){
    return;
  }

  faint::Bitmap bmp;
  if ( faint::GetBitmap( wxTheClipboard, bmp ) ){
    CanvasScroller* c = m_tabControl->NewDocument("", bmp, true);
    c->SetDirty();
    return;
  }

  std::vector<Object*> objects;
  if ( faint::GetObjects( wxTheClipboard, objects ) ){
    Rect objRect = BoundingRect( objects );
    Point p0( objRect.x, objRect.y );
    Size sz( objRect.x + objRect.w, objRect.y + objRect.h );

    Offset( objects, -p0 );
    m_tabControl->NewDocument(CanvasInfo( objRect.w, objRect.h, faint::Color(255,255,255)), objects );
    return;
  }

  // No object could be retrieved, the clipboard
  // must be closed manually
  wxTheClipboard->Close();
}

void MainFrame::ToggleMaximize(){
  if ( !IsFullScreen() ){
    Maximize( !IsMaximized() );
  }
  else {
    FaintFullScreen( false );
  }
}

bool MainFrame::Save( CanvasInterface& canvas ){
  std::string filename( canvas.GetFilename() );
  if ( filename.empty() ){
    return ShowSaveAsDialog(canvas);
  }
  else {
    return Save(canvas, filename);
  }
}

std::string FormatStringsSave( const std::vector<Format*>& m_formats ){
  std::string str = "";
  for ( size_t i = 0; i != m_formats.size(); i++ ){
    if ( i != 0 ){
      str += "|";
    }

    Format* f = m_formats[i];
    str += f->GetLabel();
    str += "|";
    str = str + "*." + f->GetFormat();
  }
  return str;
}

int FormatIndex( const std::vector<Format*>& m_formats, const wxString& extension ){
  for ( size_t i = 0; i != m_formats.size(); i++ ){
    if ( m_formats[i]->GetFormat() == extension ){
      return i;
    }
  }
  return 0;
}

bool MainFrame::Save(CanvasInterface& canvas, const wxString& filename){
  // Fixme: Error handling
  wxString extension = filename.substr( filename.size() - 3, 3 );
  bool saved = false;
  bool canLoad = false;
  for ( unsigned int i = 0; i!= m_formats.size(); i++ ){
    // Fixme: Pass the extension instead to allow better matching (e.g. .jpeg, .jpg, or for multi-formats)
    if ( m_formats[i]->GetFormat() == extension.Lower() && m_formats[i]->CanSave() ){
      Format* format = m_formats[i];
      saved = format->Save( std::string(filename), canvas );
      canLoad = format->CanLoad();
      break;
    }
  }
  if ( saved ) {
    canvas.SetFilename( std::string( filename.c_str() ) );
    canvas.ClearDirty();

    // Add to file history only if the format can be loaded
    if ( canLoad ){
      // Fixme: This won't support loadable formats not handled by Format.cpp
      wxFileName fn( filename );
      fn.MakeAbsolute();
      m_fileHistory->AddFileToHistory( fn.GetLongPath() );
    }
  }
  else {
    wxMessageDialog( this, "Failed saving", "Failed saving" ).ShowModal();
  }
  return saved;
}

void MainFrame::SelectAll(){
  CanvasScroller* canvas = m_tabControl->GetActiveCanvas();
  if ( canvas->ShouldDrawRaster() ){
    m_toolPanel->SelectTool( T_RECT_SEL );
  }
  else {
    m_toolPanel->SelectTool( T_OBJ_SEL );
  }
  canvas->SelectAll();
  m_toolPanel->ShowSettings( m_activeTool->GetSettings() );
}

void MainFrame::SelectNone(){
  CanvasInterface& canvas = m_appContext->GetActiveCanvas();
  canvas.ContextDeselect();
}

void MainFrame::FaintFullScreen( bool enable ){
  if ( enable ){
    Freeze();
    m_toolPanel->Hide();
    m_colorPanel->Hide();
    Thaw();
    ShowFullScreen( true );
  }
  else {
    Freeze();
    m_colorPanel->Show( m_frameSettings.palette_visible );
    m_toolPanel->Show( m_frameSettings.toolbar_visible );
    Thaw();
    ShowFullScreen( false );
  }
}

void MainFrame::OnToolChange( wxCommandEvent& event ){
  int id = event.GetId();
  DoSelectTool( static_cast<ToolId>( id ) );
}

void MainFrame::SelectTool( ToolId id ){
  m_toolPanel->SelectTool(id);
}

ToolId MainFrame::GetToolId() const{
  if ( m_activeTool == 0 ){
    return T_OTHER;
  }
  return m_activeTool->GetId();
}

const FaintSettings& MainFrame::GetToolSettings() const{
  return m_toolSettings;
}

void MainFrame::DoSelectTool( ToolId id ){
  if ( m_activeTool != 0 ){
    CanvasScroller* canvas = m_tabControl->GetActiveCanvas();
    canvas->Preempt();
    delete m_activeTool;
    m_activeTool = 0;
  }

  switch (id) {
  case T_RECT_SEL:
    m_activeTool = new RectangleSelectBehavior();
    break;
  case T_OBJ_SEL:
    m_activeTool = new ObjSelectBehavior( *m_appContext );
    break;
  case T_PEN:
    m_activeTool = new PenBehavior();
    break;
  case T_BRUSH:
    m_activeTool = new BrushBehavior();
    break;
  case T_PICKER:
    m_activeTool = new PickerBehavior();
    break;
  case T_LINE:
    m_activeTool = new LineBehavior();
    break;
  case T_SPLINE:
    m_activeTool = new SplineBehavior();
    break;
  case T_RECTANGLE:
    m_activeTool = new RectangleBehavior();
    break;
  case T_ELLIPSE:
    m_activeTool = new EllipseBehavior();
    break;
  case T_POLYGON:
    m_activeTool = new PolygonBehavior();
    break;
  case T_TEXT:
    m_activeTool = new TextBehavior();
    break;
  case T_FLOODFILL:
    m_activeTool = new FillBehavior();
    break;
  default:
    assert( false );
  };

  if ( id != T_OBJ_SEL ){
    // The object selection tool shows the settings from objects.
    // All other tools should be initialized with the current settings.
    m_activeTool->ToolSettingUpdate( m_toolSettings );
  }
  UpdateShownSettings();

  wxStatusBar* statusBar = GetStatusBar();
  statusBar->SetFieldsCount( m_activeTool->GetStatusFieldCount() + 1 );
}

void MainFrame::ShowRotateDialog(){
  CanvasScroller* canvas = m_tabControl->GetActiveCanvas();
  // Fixme: Setting the title here and then calling rather opaque
  // canvas functions is sort-of duplication.
  wxString title = "";
  ApplyTarget target = canvas->GetApplyTarget();
  if ( target == APPLY_OBJECT_SELECTION ){
    title = "Flip/Rotate Object";
  }
  else if ( target == APPLY_RASTER_SELECTION || target == APPLY_ACTIVE_TOOL ){
    title = "Flip/Rotate Selection";
  }
  else {
    assert( target == APPLY_IMAGE );
    title = "Flip/Rotate Image";
  }
  RotateDialog rotateDialog( this, title );
  if ( FaintShowModal(rotateDialog) != wxID_OK ){
    return;
  }

  RotateDialog::operation op = rotateDialog.GetOperation();
  CanvasInterface& canvasInterface = canvas->GetInterface();
  if ( op.choice == RotateDialog::FLIP_HORIZONTAL ){
    canvasInterface.ContextFlipHorizontal();
  }
  else if ( op.choice == RotateDialog::FLIP_VERTICAL ){
    canvasInterface.ContextFlipVertical();
  }
  else if ( op.choice == RotateDialog::ROTATE ){
    canvasInterface.ContextRotate90CW();
  }
}

void MainFrame::ShowResizeDialog(){
  CanvasScroller* activeCanvas = m_tabControl->GetActiveCanvas();
  // Fixme: Some of this should be updated to use IntSize etc. as early (code-wise) as possible
  // Note that only objects should support floating scaling
  bool scaleOnly = false;
  wxString title = "Resize Image";
  ApplyTarget target = activeCanvas->GetApplyTarget();
  if ( target == APPLY_RASTER_SELECTION || target == APPLY_ACTIVE_TOOL ){
    scaleOnly = true;
    title = "Resize Selection";
  }
  else if ( target == APPLY_OBJECT_SELECTION ){
    std::vector<Object*>& selectedObjects = activeCanvas->GetSelectedObjects();
    scaleOnly = true;
    title = selectedObjects.size() == 1 ? "Resize Object" : "Resize Objects";
  }
  else {
    assert( target == APPLY_IMAGE );
  }

  Size originalSize = activeCanvas->GetApplyTargetSize();
  // Fixme: Floating point size needed for objects
  ResizeDialog resizeDialog( this,
    title,
    ResizeDialogSettings( originalSize.w, originalSize.h, scaleOnly,
      m_frameSettings.default_resize_settings ),
    m_artContainer );

  if ( FaintShowModal( resizeDialog ) != wxID_OK ){
    return;
  }

  ResizeDialogSettings s = resizeDialog.GetSelection();
  if ( s.w <= 0 || s.h <= 0 ){
    // Invalid size specified, do nothing.
    return;
  }

  m_frameSettings.default_resize_settings = s;

  Size newSize( s.w, s.h );
  if ( target == APPLY_RASTER_SELECTION ){
    activeCanvas->ScaleRasterSelection( newSize );
    return;
  }
  else if ( target == APPLY_OBJECT_SELECTION ){
    activeCanvas->ScaleSelectedObject( newSize );
    return;
  }
  else if ( target == APPLY_ACTIVE_TOOL ){
    activeCanvas->ScaleToolBitmap( truncated(newSize) );
    return;
  }
  else {
    // Scale or resize the entire image
    if ( s.defaultButton == ResizeDialogSettings::RESIZE_TOP_LEFT ){
      // Fixme: Should be already
      IntSize iSz( newSize.w, newSize.h );
      activeCanvas->Resize( iSz );
    }
    else if ( s.defaultButton == ResizeDialogSettings::RESIZE_CENTER ){
      Size oldSize = floated(activeCanvas->GetBitmapSize()); // Fixme: Use the IntSize
      IntRect r(
        IntPoint( 0 - ( newSize.w - oldSize.w ) / 2,
          0 - ( newSize.h - oldSize.h ) / 2 ),
        IntSize( newSize.w, newSize.h ) );
      activeCanvas->Resize( r );
    }
    else if ( s.defaultButton == ResizeDialogSettings::RESCALE ){
      // Fixme: Should already be int
      activeCanvas->Rescale( IntSize( newSize.w, newSize.h ) );
    }
    activeCanvas->ExternalRefresh();
  }
}

bool MainFrame::ShowSaveAsDialog(CanvasInterface& canvas){
  wxFileName oldFileName( canvas.GetFilename() );
  int defaultFormatIndex = FormatIndex( m_formats, oldFileName.GetExt() );
  wxFileDialog dlg( this, "Save as", oldFileName.GetPath(), oldFileName.GetName(), FormatStringsSave( m_formats ).c_str(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
  dlg.SetFilterIndex( defaultFormatIndex );
  int result = FaintShowModal(dlg);
  if ( result != wxID_OK ){
    return false;
  }
  return Save(canvas, dlg.GetPath());
}

CanvasScroller* MainFrame::GetActiveCanvas(){
  return m_tabControl->GetActiveCanvas();
}

ColorChoice GetColorChoice( const FaintSettings& preferred, const FaintSettings& fallback ){
  ColorChoice c = {
    preferred.Has( ts_FgCol ) ? preferred.Get( ts_FgCol ) : fallback.Get( ts_FgCol ),
    preferred.Has( ts_BgCol ) ? preferred.Get( ts_BgCol ) : fallback.Get( ts_BgCol ) };
  return c;
}

void MainFrame::UpdateShownSettings(){
  const FaintSettings& activeSettings( m_activeTool->GetSettings() );
  m_toolPanel->ShowSettings( activeSettings );
  m_colorPanel->UpdateSelectedColors( GetColorChoice( activeSettings, m_toolSettings ) );
}

void MainFrame::UpdateToolSettings( const FaintSettings& s ){
  m_toolSettings.Update( s );
  m_activeTool->ToolSettingUpdate( m_toolSettings );
  UpdateShownSettings();
}

void MainFrame::OnStateChange(){
  CanvasScroller* canvas = GetActiveCanvas();
  if ( canvas == 0 ){
    return;
  }

  MenuSelection( canvas->ShouldDrawRaster() && ( canvas->HasRasterSelection() || canvas->HasToolSelection() ),
    canvas->ShouldDrawVector() && canvas->HasObjectSelection(),
    !canvas->GetObjects().empty() );
  m_toolPanel->ShowSettings( m_activeTool->GetSettings() );
  MenuDirty( canvas->IsDirty() );
  MenuUndoRedo( canvas->CanUndo(), canvas->CanRedo() );
  m_tabControl->RefreshActiveTabName();
}

void MainFrame::OnLayerTypeChange( wxCommandEvent& event ){
  m_layerType = ToLayer(event.GetInt());
  RefreshActiveCanvas();
  OnDocumentChange();
}

void MainFrame::OnDocumentChange(){
  OnZoomChange();
  OnStateChange();
}

void MainFrame::OnZoomChange(){
  ZoomLevel zoom(GetActiveCanvas()->GetZoomLevel());
  m_colorPanel->UpdateZoom(zoom);
  m_menubar->UpdateZoom(zoom);
}

void MainFrame::ShowHelp(){
  m_helpWindow.Show();
}

Layer MainFrame::GetLayerType() const{
  return m_layerType;
}

std::vector<Format*>& MainFrame::GetFileFormats(){
  return m_formats;
}

void MainFrame::AddFormat( Format* f ){
  m_formats.push_back(f);
}

void MainFrame::MenuSelection( bool rasterSelection, bool objectSelection, bool hasObjects ){
  m_menubar->Update( rasterSelection, objectSelection, hasObjects );
}

void MainFrame::MenuDirty( bool dirty ){
  wxMenuBar* menubar = GetMenuBar();
  menubar->Enable( wxID_SAVE, dirty );
}

void MainFrame::MenuUndoRedo( bool canUndo, bool canRedo ){
  wxMenuBar* menubar = GetMenuBar();
  menubar->Enable( wxID_UNDO, canUndo );
  menubar->Enable( wxID_REDO, canRedo );
}

void MainFrame::ToggleToolPanel( bool show ){
  // Update the setting state and and tool panel visibility
  //
  // Freezing the MainFrame removes a refresh-artifact when re-showing
  // the tool-panel on MSW (The wxStaticLine:s from the child panels
  // appeared in the canvas).
  Freeze();
  m_frameSettings.toolbar_visible = show;
  m_toolPanel->Show( show );
  Thaw();

  // Update the sizers to the new state of the tool panel
  //
  // Freezing the color panel removes flicker in the palette.
  // Layout(). This depends on the tool panel stretching across the
  // frame.
  m_colorPanel->Freeze();
  Layout();
  m_colorPanel->Thaw();
}

void MainFrame::ToggleColorbar( bool show ){
  m_frameSettings.palette_visible = show;
  m_colorPanel->Show( show );
  Layout();
}

void MainFrame::ToggleStatusbar( bool show ){
  m_frameSettings.statusbar_visible = show;
  GetStatusBar()->Show( show );
  Layout();
}

void MainFrame::CreatePanels(){
  // Top half, the tool panel and the drawing areas.
  wxBoxSizer* row1 = new wxBoxSizer( wxHORIZONTAL );
  m_toolPanel = new ToolPanel( this, *m_notifier, m_artContainer );
  row1->Add( m_toolPanel->AsWindow(), 0, wxEXPAND );
  m_tabControl = new TabControl( this );
  row1->Add( m_tabControl->Window(), 1, wxEXPAND );

  // Bottom half, the selected color, palette and zoom controls.
  m_colorPanel = new ColorPanel( this, m_toolSettings, *m_notifier, m_palettes );

  wxBoxSizer* rows = new wxBoxSizer( wxVERTICAL );
  rows->Add( row1, 1, wxEXPAND );
  rows->Add( m_colorPanel->AsWindow(), 0, wxEXPAND );
  SetSizer( rows );
  Layout();
}

wxStatusBar* MainFrame::CreateFaintStatusBar(){
  wxStatusBar* statusbar = new wxStatusBar( this, wxID_ANY );
  SetStatusBar( statusbar );
  return statusbar;
}

Menubar* MainFrame::CreateFaintMenubar(){
  Menubar* menubar = new Menubar();
  SetMenuBar( menubar->GetRawMenubar() );
  PushEventHandler(menubar->GetEventHandler());
  return menubar;
}

namespace faint{
  wxIcon GetIcon( ArtContainer& artContainer, const wxString& label ){
    wxIcon icon;
    icon.CopyFromBitmap( *(artContainer.Get( label ) ) );
    return icon;
  }
}

void MainFrame::InitIcons(){
  // Main frame icons
  wxIconBundle iconBundle;
  iconBundle.AddIcon( faint::GetIcon( m_artContainer, "faint_icon16" ) );
  iconBundle.AddIcon( faint::GetIcon( m_artContainer, "faint_icon32" ) );
  SetIcons( iconBundle );

  // Python interpreter icons
  wxIconBundle iconBundlePython;
  iconBundlePython.AddIcon( faint::GetIcon( m_artContainer, "faint_python_icon32" ) );
  iconBundlePython.AddIcon( faint::GetIcon( m_artContainer, "faint_python_icon16" ) );
  m_pythonFrame->SetIcons( iconBundlePython );
}

void MainFrame::OnAddColor( wxCommandEvent& event ){
  AddColor( event.GetInt() == ADD_FG ?
            m_toolSettings.Get( ts_FgCol ) :
            m_toolSettings.Get( ts_BgCol ) );
}

void MainFrame::OnCopyColorHex( wxCommandEvent& event ){
  int whichColor = event.GetInt();
  assert( whichColor == ADD_FG || whichColor == ADD_BG );
  wxColour color = to_wx( whichColor == ADD_FG ?
    m_toolSettings.Get( ts_FgCol ) :
    m_toolSettings.Get( ts_BgCol ) );

  if ( !wxTheClipboard->Open() ){
    // Fixme: Error message, at least discrete, e.g. log
    return;
  }
  faint::SetText( wxTheClipboard, std::string(color.GetAsString( wxC2S_HTML_SYNTAX )) );
}

void MainFrame::OnCopyColorRGB( wxCommandEvent& event ){
  int whichColor = event.GetInt();
  assert( whichColor == ADD_FG || whichColor == ADD_BG );
  faint::Color color =  whichColor == ADD_FG ?
    m_toolSettings.Get( ts_FgCol ) :
    m_toolSettings.Get( ts_BgCol );

  wxString text;
  text.Printf( "%d, %d, %d", color.r, color.g, color.b );

  if ( !wxTheClipboard->Open() ){
    // Fixme: Error message (log)
    return;
  }

  faint::SetText( wxTheClipboard, std::string( text ) );
}

void MainFrame::OnSwapColors( wxCommandEvent& ){
  faint::Color color = m_toolSettings.Get( ts_FgCol );
  Set( ts_FgCol, m_toolSettings.Get( ts_BgCol ) );
  Set( ts_BgCol, color );
}

void MainFrame::CloseDocument( CanvasInterface& canvas ){
  for ( size_t i = 0; i != m_tabControl->GetCanvasCount(); i++ ){
    CanvasInterface& other = m_tabControl->GetCanvas(i)->GetInterface();
    if ( &other == &canvas ){
      m_tabControl->Close( i );
      return;
    }
  }
}

size_t MainFrame::GetCanvasCount() const{
  return m_tabControl->GetCanvasCount();
}

CanvasInterface& MainFrame::GetCanvas( size_t i ){
  return m_tabControl->GetCanvas( i )->GetInterface();
}

bool MainFrame::Exists( const CanvasId& id  ) const {
  const size_t num = m_tabControl->GetCanvasCount();
  for ( size_t i = 0; i != num; i++ ){
    const CanvasId compare = m_tabControl->GetCanvas( i )->GetCanvasId();
    if ( compare == id ){
      return true;
    }
  }
  return false;
}

CanvasInfo MainFrame::GetDefaultCanvasInfo() const{
  return CanvasInfo( 640, 480, faint::Color( 255, 255, 255 ) );
}

void MainFrame::OnSetFocusEntry(wxCommandEvent&){
  if ( m_textEntryCount == 0 ){
    // Entry controls are numeric, not all shortcuts
    // need to be disabled
    const bool numeric = true;
    m_menubar->BeginTextEntry(numeric);
  }
  m_textEntryCount++;
}

void MainFrame::OnKillFocusEntry(wxCommandEvent&){
  assert( m_textEntryCount > 0 );
  m_textEntryCount -= 1;
  if ( m_textEntryCount == 0 ){
    m_menubar->EndTextEntry();
  }
}

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_COMMAND( -1, EVT_TOOL_CHANGE, MainFrame::OnToolChange )
EVT_COMMAND( -1, EVT_LAYER_TYPE_CHANGE, MainFrame::OnLayerTypeChange )
EVT_COMMAND( -1, EVT_ADD_COLOR, MainFrame::OnAddColor )
EVT_COMMAND( -1, EVT_COPY_RGB, MainFrame::OnCopyColorRGB )
EVT_COMMAND( -1, EVT_COPY_HEX, MainFrame::OnCopyColorHex )
EVT_COMMAND( -1, EVT_SWAP_COLORS, MainFrame::OnSwapColors )
EVT_COMMAND( -1, EVT_SET_FOCUS_ENTRY_CONTROL, MainFrame::OnSetFocusEntry )
EVT_COMMAND( -1, EVT_KILL_FOCUS_ENTRY_CONTROL, MainFrame::OnKillFocusEntry )
EVT_CLOSE( MainFrame::OnClose )
END_EVENT_TABLE()

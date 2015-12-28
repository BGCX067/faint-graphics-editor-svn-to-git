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

#include <string>
#include <sstream>
#include "wx/wx.h"
#include "wx/aboutdlg.h"
#include "wx/filename.h"
#include "wx/spinctrl.h" // Fixme: Nasty, for cast
#include "mainframe.hh"
#include "app/appcontext.hh"
#include "formats/format_gif.hh"
#include "formats/format_ico.hh"
#include "formats/format_wx.hh"
#include "gui/canvasscroller.hh"
#include "gui/colordialog.hh"
#include "gui/colorpanel.hh"
#include "gui/events.hh"
#include "gui/interpreterframe.hh"
#include "gui/resizedialogsettings.hh"
#include "gui/selectedcolorctrl.hh" // For color event identifiers
#include "gui/tabcontrol.hh"
#include "gui/toolbar.hh"
#include "gui/toolpanel.hh"
#include "gui/toolsettingpanel.hh"
#include "rendering/cairocontext.hh" // For get_cairo_version
#include "util/artcontainer.hh"
#include "util/clipboard.hh"
#include "util/convertwx.hh"
#include "util/formatting.hh"
#include "util/guiutil.hh"
#include "util/image.hh"
#include "util/objutil.hh"
#include "util/pathutil.hh"
#include "util/settingutil.hh"
#include "util/toolutil.hh"
#include "util/util.hh"

class SBInterface : public StatusInterface {
public:
  SBInterface( wxStatusBar* statusbar ) :
    m_statusbar( statusbar ) {}
  void SetMainText( const std::string& text ){
    m_statusbar->SetStatusText( wxString( text.c_str() ), 0 );
  }
  void SetText( const std::string& text, int field=0 ){
    assert(field < m_statusbar->GetFieldsCount());
    m_statusbar->SetStatusText( wxString( text.c_str() ), field + 1 );
  }
  void Clear(){
    for ( int i = 0; i != m_statusbar->GetFieldsCount(); i++ ){
      m_statusbar->SetStatusText("", i);
    }
  }
private:
  wxStatusBar* m_statusbar;
};

typedef LessUnique<bool, 0> from_control;

template<typename T>
void change_setting( MainFrame& frame, const T& setting, typename T::ValueType value, const from_control& fromControl ){
  Tool* tool = frame.GetActiveTool();
  if ( tool->GetSettings().Has( setting ) ){
    bool toolModified = tool->Set( setting, value );
    if ( toolModified ) {
      frame.GetActiveCanvas()->ExternalRefresh();
    }
    if ( tool->EatsSettings() ){
      if ( !fromControl.Get() ){
        frame.UpdateShownSettings();
        return;
      }
    }
  }
  frame.GetToolSettings().Set( setting, value );
  if ( !fromControl.Get() ){
    frame.UpdateShownSettings();
  }
}

class FrameContext : public AppContext {
public:
  FrameContext( MainFrame* frame, wxStatusBar* statusbar, HelpFrame* helpFrame, InterpreterFrame* interpreterFrame )
    : m_frame( frame ),
      m_helpFrame( helpFrame ),
      m_interpreterFrame( interpreterFrame ),
      m_statusbar( statusbar ),
      m_deleteOnClose(0),
      m_modalDialog(false)
  {}

  ~FrameContext(){
    delete m_deleteOnClose;
  }

  void AddFormat( Format* fileFormat ){
    m_frame->AddFormat( fileFormat );
  }

  void AddPaletteColor( const faint::Color& c ){
    m_frame->AddColor( c );
  }

  void BeginModalDialog(){
    m_modalDialog = true;
  }

  void BeginTextEntry(){
    m_frame->BeginTextEntry(); // Fixme: Consider moving the handling here.
  }

  void Bind( int key, int modifiers, bool global ){
    modkey_t modkey( std::make_pair(key, modifiers) );
    if ( global ){
      m_globalBinds.insert( modkey );
      m_binds.erase(modkey);
    }
    else{
      m_binds.insert(modkey);
      m_globalBinds.erase(modkey);
    }
  }

  bool Bound( int key, int modifiers ) const{
    return m_binds.find( std::make_pair(key, modifiers) ) != m_binds.end();
  }

  bool BoundGlobal( int key, int modifiers ) const{
    return m_globalBinds.find( std::make_pair(key, modifiers) ) != m_globalBinds.end();
  }

  void Close( CanvasInterface& canvas ){
    m_frame->CloseDocument( canvas );
  }

  void QueueLoad( const std::vector<std::string>& filenames ){
    m_frame->GetEventHandler()->QueueEvent( new OpenFilesEvent(filenames) );
  }

  void DeleteOnClose( Tool* tool ){
    m_deleteOnClose = tool;
  }

  void DialogOpenFile(){
    m_frame->ShowOpenFileDialog();
  }

  void EndModalDialog(){
    m_modalDialog = false;
  }

  void EndTextEntry(){
    m_frame->EndTextEntry();
  }

  bool Exists( const CanvasId& id ){
    return m_frame->Exists( id );
  }

  BoolSetting::ValueType Get( const BoolSetting& s ){
    return m_frame->GetShownSettings().Get(s);
  }

  StrSetting::ValueType Get( const StrSetting& s){
    return m_frame->GetShownSettings().Get(s);
  }

  IntSetting::ValueType Get( const IntSetting& s ){
    return m_frame->GetShownSettings().Get(s);
  }

  ColorSetting::ValueType Get( const ColorSetting& s ){
    return m_frame->GetShownSettings().Get(s);
  }

  FloatSetting::ValueType Get( const FloatSetting& s ){
    return m_frame->GetShownSettings().Get(s);
  }

  CanvasInterface& GetActiveCanvas(){
    return m_frame->GetActiveCanvas()->GetInterface();
  }

  Tool* GetActiveTool(){
    return m_frame->GetActiveTool();
  }

  CanvasInterface& GetCanvas( size_t i ){
    return m_frame->GetCanvas(i);
  };

  size_t GetCanvasCount() const {
    return m_frame->GetCanvasCount();
  }

  Grid GetDefaultGrid() const{
    return m_defaultGrid;
  }

  ImageInfo GetDefaultImageInfo(){
    return ImageInfo(IntSize(640,480), faint::Color(255,255,255));
  }

  std::vector<Format*> GetFormats(){
    return m_frame->GetFileFormats();
  }

  ResizeDialogSettings GetDefaultResizeDialogSettings() const{
    return m_defaultResizeSettings;
  }

  Layer::type GetLayerType() const{
    return m_frame->GetLayerType();
  }

  Point GetMousePos(){
    wxPoint mousePos = wxGetMousePosition();
    return Point( mousePos.x, mousePos.y );
  }

  StatusInterface& GetStatusInfo(){
    return m_statusbar;
  }

  ToolId GetToolId(){
    return m_frame->GetToolId();
  }

  Settings GetToolSettings() const {
    return m_frame->GetShownSettings();
  }

  CanvasInterface* Load( const std::string& path, const change_tab& changeTab ){
    return m_frame->Open( wxString( path ), changeTab  );
  }

  void Load( const std::vector<std::string>& filenames ){
    return m_frame->Open( filenames );
  }

  CanvasInterface* LoadAsFrames( const std::vector<std::string>& paths, const change_tab& changeTab ){
    std::vector<ImageProps> props;
    std::vector<Format*>& formats = m_frame->GetFileFormats();
    for ( size_t pathNum = 0; pathNum != paths.size(); pathNum++ ){
      wxFileName filePath(paths[pathNum]);
      extension_t extension(std::string(filePath.GetExt()));
      bool loaded = false;
      for ( size_t i = 0; i != formats.size(); i++ ){
        Format* format = formats[i];
        if ( format->Match(extension) && format->CanLoad() ){
          props.push_back(ImageProps());
          format->Load( std::string(filePath.GetFullPath()), props.back() );
          if ( !props.back().IsOk() ){
            // show_load_failed_error(m_frame, filePath, props.back().GetError() );
            return 0;
          }
          loaded = true;
        }
      }
      if ( !loaded ){
        // show_load_failed_error(m_frame, filePath, "One path could not be loaded.");
        return 0;
      }
    }
    CanvasScroller* canvas = m_frame->GetTabControl()->NewDocument(props, changeTab);
    CanvasInterface* canvasInterface = &(canvas->GetInterface());
    return canvasInterface;
  }

  void Maximize(){
    m_frame->Maximize( !m_frame->IsMaximized() );
  }

  void MaximizeInterpreter(){
    m_interpreterFrame->Maximize( !m_interpreterFrame->IsMaximized() );
  }

  bool ModalDialogShown() const{
    return m_modalDialog;
  }

  CanvasInterface& NewDocument( const ImageInfo& info ){
    CanvasScroller* canvas = m_frame->NewDocument( info );
    return canvas->GetInterface();
  }

  void OnActiveCanvasChanged(){
    m_frame->OnActiveCanvasChanged();
  }

  void OnDocumentStateChange(const CanvasId& id){
    m_frame->OnDocumentStateChange(id);
  }

  void OnZoomChange(){
    m_frame->OnZoomChange();
  }

  void PythonContinuation(){
    m_interpreterFrame->NewContinuation();
  }

  void PythonDone(){
    for ( std::set<CanvasInterface*>::iterator it = m_unrefreshed.begin(); it != m_unrefreshed.end(); ++it ){
      CanvasInterface* canvas = *it;
      if ( Exists( m_canvasId[canvas] ) ){
        canvas->Refresh();
      }
    }
    m_unrefreshed.clear();

    for ( std::map<CanvasInterface*, int>::iterator it = m_commandBundles.begin(); it != m_commandBundles.end(); ++it ){
      const int numCommands = it->second;
      if ( numCommands > 1 ){
        std::stringstream ss;
        ss << "Python Commands (" << numCommands << ")";
        CanvasInterface* canvas = it->first;
        canvas->BundleUndo(numCommands, ss.str());
      }
    }
    m_commandBundles.clear();
  }

  void PythonGetKey(){
    m_interpreterFrame->GetKey();
  }

  void PythonIntFaintPrint( const std::string& s ){
    m_interpreterFrame->IntFaintPrint(s);
  }

  void PythonNewPrompt(){
    m_interpreterFrame->NewPrompt();
  }

  void PythonPrint( const std::string& s ){
    m_interpreterFrame->Print(s);
  }

  void PythonQueueRefresh( CanvasInterface* canvas ){
    QueueRefresh(canvas);
  }

  void PythonRunCommand( CanvasInterface* canvas, Command* command ){
    QueueRefresh(canvas);
    canvas->RunCommand( command );
    if ( m_commandBundles.find(canvas) == m_commandBundles.end() ){
      m_commandBundles.insert(std::make_pair(canvas, 0));
    }
    m_commandBundles[canvas]++;
  }

  void Quit(){
    m_frame->Close(false); // False means don't force
  }

  void RaiseFrame(){
    m_frame->Raise();
  }

  void SelectTool( ToolId id ){
    m_frame->SelectTool( id );
  }

  void Set( const BoolSetting& s, BoolSetting::ValueType v ){
    change_setting( *m_frame, s, v, from_control(false) );
  }

  void Set( const StrSetting& s, StrSetting::ValueType v){
    change_setting( *m_frame, s, v, from_control(false) );
  }

  void Set( const IntSetting& s, IntSetting::ValueType v ){
    change_setting( *m_frame, s, v, from_control(false) );
  }

  void Set( const ColorSetting& s, ColorSetting::ValueType v ){
    change_setting( *m_frame, s, v, from_control(false) );
  }

  void Set( const FloatSetting& s, FloatSetting::ValueType v ){
    change_setting( *m_frame, s, v, from_control(false) );
  }

  void SetActiveCanvas( const CanvasId& id ){
    return m_frame->SetActiveCanvas(id);
  }

  void SetDefaultGrid( const Grid& grid ){
    m_defaultGrid = grid;
  }

  void SetDefaultResizeDialogSettings( const ResizeDialogSettings& settings ) {
    m_defaultResizeSettings = settings;
  }

  void SetInterpreterBackground( const faint::Color& c ){
    m_interpreterFrame->FaintSetBackgroundColour(c);
  }

  void SetInterpreterTextColor( const faint::Color& c ){
    m_interpreterFrame->SetTextColor(c);
  }

  void SetPalette( const std::vector<faint::Color>& colors ){
    m_frame->SetPalette(colors);
  }

  void SetLayer( Layer::type layer ){
    m_frame->SelectLayer( layer );
  }

  void ShowHelpFrame(){
    if ( m_helpFrame->IsHidden() ){
      m_helpFrame->Show();
      m_helpFrame->Raise();
    }
    else if ( m_helpFrame->IsIconized() ){
      m_helpFrame->Restore();
      m_helpFrame->Raise();
    }
    else {
      m_helpFrame->Hide();
    }
  }

  void ShowPythonConsole(){
    if ( m_interpreterFrame->IsHidden() ){
      m_interpreterFrame->Show();
      m_interpreterFrame->Raise();
    }
    else if ( m_interpreterFrame->IsIconized() ){
      m_interpreterFrame->Restore();
      m_interpreterFrame->Raise();
    }
    else {
      m_interpreterFrame->Hide();
    }
  }

  void Unbind( int key, int modifiers ){
    m_binds.erase( std::make_pair(key, modifiers) );
  }

  void UpdateShownSettings(){
    m_frame->UpdateShownSettings();
  }

  void UpdateToolSettings( const Settings& s ){
    m_frame->UpdateToolSettings( s );
  }

  void CloseFloatingWindows(){ // Non-virtual
    m_helpFrame->Close();
    m_interpreterFrame->Close(true);
  }

  std::string GetPythonVersion(){ // Non-virtual
    return m_interpreterFrame->GetPythonVersion();
  }

  bool FloatingWindowFocused() const{ // Non-virtual
    return m_helpFrame->FaintHasFocus() || m_interpreterFrame->FaintHasFocus();
  }

private:
  void QueueRefresh( CanvasInterface* canvas ){
    m_unrefreshed.insert( canvas );
    m_canvasId[ canvas ] = canvas->GetId();
  }
  MainFrame* m_frame;
  HelpFrame* m_helpFrame;
  InterpreterFrame* m_interpreterFrame;
  SBInterface m_statusbar;
  std::set<CanvasInterface*> m_unrefreshed;
  std::map<CanvasInterface*, CanvasId> m_canvasId;
  std::map<CanvasInterface*, int> m_commandBundles;
  typedef std::pair<int,int> modkey_t; // Fixme: Create a key class
  std::set<modkey_t> m_binds;
  std::set<modkey_t> m_globalBinds;
  Tool* m_deleteOnClose;
  bool m_modalDialog;
  Grid m_defaultGrid;
  ResizeDialogSettings m_defaultResizeSettings;
};

class FrameNotifier : public SettingNotifier {
public:
  FrameNotifier( MainFrame& frame, const from_control& fromCtrl )
    : m_frame(frame),
      m_fromCtrl(fromCtrl)
  {}
  void Notify( const BoolSetting& s, bool value ){
    change_setting(m_frame, s, value, m_fromCtrl);
  }
  void Notify( const IntSetting& s, int value ){
    change_setting(m_frame, s, value, m_fromCtrl);
  }
  void Notify( const StrSetting& s, const std::string& value ){
    change_setting(m_frame, s, value, m_fromCtrl);
  }
  void Notify( const ColorSetting& s, const faint::Color& value ){
    change_setting(m_frame, s, value, m_fromCtrl);
  }
  void Notify( const FloatSetting& s, FloatSetting::ValueType value ){
    change_setting(m_frame, s, value, m_fromCtrl);
  }
private:
  MainFrame& m_frame;
  from_control m_fromCtrl;
};

void init_formats( std::vector<Format*>& formats ){
  formats.push_back( new FormatWX(extension_t("png"),
      label_t("Portable Network Graphics"), wxBITMAP_TYPE_PNG ) );
  formats.push_back( new FormatGIF());
  formats.push_back( new FormatICO());
  formats.push_back( new FormatWX(extension_t("bmp"),
      label_t("Windows Bitmap"), wxBITMAP_TYPE_BMP ) );
  formats.push_back( new FormatWX(vector_of(extension_t("jpg"), extension_t("jpeg")),
      label_t("JPEG"), wxBITMAP_TYPE_JPEG ) );
  formats.push_back (new FormatWX(extension_t("ico"),
      label_t("Icon"), wxBITMAP_TYPE_ICO));
}

wxStatusBar* create_faint_statusbar(wxFrame* frame){
  wxStatusBar* statusbar = new wxStatusBar( frame, wxID_ANY );
  frame->SetStatusBar( statusbar );
  return statusbar;
}

std::string format_strings_save( const std::vector<Format*>& m_formats ){
  std::string str = "";
  for ( size_t i = 0; i != m_formats.size(); i++ ){
    if ( i != 0 ){
      str += "|";
    }

    Format* f = m_formats[i];
    str += f->GetLabel();
    str += "|";
    str = str + "*." + f->GetDefaultExtension();
  }
  return str;
}

int get_format_index( const std::vector<Format*>& formats, const extension_t& extension ){
  for ( size_t i = 0; i != formats.size(); i++ ){
    if ( formats[i]->Match(extension) ){
      return i;
    }
  }
  assert( false );
  return 0;
}

ColorChoice get_color_choice( const Settings& preferred, const Settings& fallback ){
  ColorChoice c = {
    preferred.Has( ts_FgCol ) ? preferred.Get( ts_FgCol ) : fallback.Get( ts_FgCol ),
    preferred.Has( ts_BgCol ) ? preferred.Get( ts_BgCol ) : fallback.Get( ts_BgCol ) };
  return c;
}

MainFrame::MainFrame( ArtContainer& art, PaletteContainer& palettes, HelpFrame* helpFrame, InterpreterFrame* interpreterFrame )
  : wxFrame((wxFrame*)NULL, wxID_ANY, "Faint", wxPoint(50,50), wxSize(800,700) ),
    m_artContainer( art ),
    m_appContext(0),
    m_notifier(0),
    m_menubar(0),
    m_tabControl(0),
    m_toolPanel(0),
    m_colorPanel(0),
    m_layerType( Layer::RASTER ),
    m_activeTool(0),
    m_toolSettings(default_tool_settings()),
    m_palettes( palettes ),
    m_textEntryCount(0)
{
  Bind(FAINT_CHANGE_TOOL, ToolChangeEventHandler(MainFrame::OnToolChange), this);
  Bind(FAINT_CHANGE_LAYER, LayerChangeEvtHandler(MainFrame::OnLayerTypeChange), this);
  Bind(FAINT_ADD_COLOR, ColorEventHandler(MainFrame::OnAddColor), this);
  Bind(FAINT_COPY_COLOR_HEX, ColorEventHandler(MainFrame::OnCopyColorHex), this);
  Bind(FAINT_COPY_COLOR_RGB, ColorEventHandler(MainFrame::OnCopyColorRGB), this);
  Bind(FAINT_OPEN_FILES, OpenFilesEventHandler(MainFrame::OnOpenFiles), this);

  m_appContext = new FrameContext( this, create_faint_statusbar(this), helpFrame, interpreterFrame );
  m_notifier = new FrameNotifier(*this, from_control(true));
  m_updatingNotifier = new FrameNotifier(*this, from_control(false));
  m_menubar = new Menubar(*this);
  CreatePanels();

  // Fixme: Wrong place for such stuff, has nothing todo with main
  // frame. Consider App.
  init_formats( m_formats );
  SetMinSize(wxSize(640, 480));
  m_toolPanel->SelectTool(T_LINE);
}

MainFrame::~MainFrame(){
  // Note: Deletion of the AppContext is handled outside MainFrame.
  // Fixme: Creation should be moved outside as well.
  m_appContext = 0;

  delete m_notifier;
  delete m_updatingNotifier;
  // These do not inherit from wxWindow and need to be deleted
  // manually.
  delete m_menubar;
  delete m_toolPanel;
  delete m_colorPanel;
  delete m_tabControl;
  for ( size_t i = 0; i!= m_formats.size(); i++ ){
    delete m_formats[i];
  }
}

void MainFrame::AddColor( const faint::Color& c ){
  m_colorPanel->AddToPalette(c);
}

void MainFrame::AddFormat( Format* f ){
  m_formats.push_back(f);
}

void MainFrame::BeginTextEntry(){
  m_textEntryCount++;
  m_menubar->BeginTextEntry();
  OnDocumentStateChange(GetActiveCanvas()->GetCanvasId()); // Fixme: To enable e.g. deselect-all for the EditText task, bit of a hack.
}

void MainFrame::CloseActiveTab(){
  m_tabControl->CloseActive();
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

void MainFrame::CreatePanels(){
  // Top half, the tool panel and the drawing areas.
  wxBoxSizer* row1 = new wxBoxSizer( wxHORIZONTAL );
  m_toolPanel = new ToolPanel( this, *m_notifier, m_appContext->GetStatusInfo(), m_artContainer );
  row1->Add( m_toolPanel->AsWindow(), 0, wxEXPAND );
  m_tabControl = new TabControl( this );
  row1->Add( m_tabControl->AsWindow(), 1, wxEXPAND );

  // Bottom half, the selected color, palette and zoom controls.
  m_colorPanel = new ColorPanel( this,
    m_toolSettings,
    *m_updatingNotifier,
    m_palettes,
    m_appContext->GetStatusInfo(),
    m_artContainer );

  wxBoxSizer* rows = new wxBoxSizer( wxVERTICAL );
  rows->Add( row1, 1, wxEXPAND );
  rows->Add( m_colorPanel->AsWindow(), 0, wxEXPAND );
  SetSizer( rows );
  Layout();
}

void MainFrame::DoSelectTool( ToolId id ){
  if ( m_activeTool != 0 ){
    CanvasScroller* canvas = m_tabControl->GetActiveCanvas();
    canvas->Preempt(PreemptOption::ALLOW_COMMAND);
    delete m_activeTool;
    m_activeTool = 0;
  }
  m_activeTool = new_tool(id);

  if ( id != T_OBJ_SEL && id != T_RECT_SEL ){ // Fixme: Genericize
    // The object selection tool shows the settings from objects.
    // All other tools should be initialized with the current settings.
    // Fixme: RasterSelection should behave like the object selection!
    m_activeTool->ToolSettingUpdate( m_toolSettings );
  }
  UpdateShownSettings();

  wxStatusBar* statusBar = GetStatusBar();
  statusBar->SetFieldsCount( m_activeTool->GetStatusFieldCount() + 1 );
}

void MainFrame::EndTextEntry(){
  assert( m_textEntryCount > 0 );
  m_textEntryCount--;
  if ( m_textEntryCount == 0 ){
    m_menubar->EndTextEntry();
  }
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

bool MainFrame::Exists( const CanvasId& id  ) const {
  return m_tabControl->Has(id);
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

CanvasScroller* MainFrame::GetActiveCanvas(){
  CanvasScroller* active = m_tabControl->GetActiveCanvas();
  assert(active != 0);
  return active;
}

Tool* MainFrame::GetActiveTool(){
  return m_activeTool;
}

CanvasInterface& MainFrame::GetCanvas( size_t i ){
  return m_tabControl->GetCanvas( i )->GetInterface();
}

size_t MainFrame::GetCanvasCount() const{
  return m_tabControl->GetCanvasCount();
}

AppContext* MainFrame::GetContext(){
  return m_appContext;
}

std::vector<Format*>& MainFrame::GetFileFormats(){
  return m_formats;
}

Layer::type MainFrame::GetLayerType() const{
  return m_layerType;
}

Settings MainFrame::GetShownSettings() const{
  Settings settings(m_toolSettings);
  settings.Update( m_activeTool->GetSettings() );
  return settings;
}

EntryMode::Type MainFrame::GetTextEntryMode() const{
  return m_menubar->GetTextEntryMode();
}

ToolId MainFrame::GetToolId() const{
  if ( m_activeTool == 0 ){
    return T_OTHER;
  }
  return m_activeTool->GetId();
}

Settings& MainFrame::GetToolSettings(){
  return m_toolSettings;
}

bool MainFrame::MainFrameFocused() const{
  return !( m_appContext->ModalDialogShown() ||
    m_appContext->FloatingWindowFocused());
}

CanvasScroller* MainFrame::NewDocument( const ImageInfo& info ){
  ImageProps props(info);
  CanvasScroller* canvas = m_tabControl->NewDocument( props, change_tab(true) );
  OnDocumentStateChange(canvas->GetCanvasId());
  return canvas;
}

void MainFrame::NextTab() {
  m_tabControl->SelectNext();
}

void MainFrame::OnActiveCanvasChanged(){
  OnZoomChange();
  OnDocumentStateChange(GetActiveCanvas()->GetCanvasId());
  m_activeTool->SelectionChange();
  UpdateShownSettings();
}

void MainFrame::OnAddColor( ColorEvent& event ){
  AddColor( event.GetColor() );
}

void MainFrame::OnClose( wxCloseEvent& event ){
  if ( m_tabControl->UnsavedDocuments() && event.CanVeto() ){
    bool quit = ask_exit_unsaved_changes(this);
    if ( !quit ){
      event.Veto();
      return;
    }
  }

  m_menubar->GetRecentFiles().Save();
  faint::Clipboard::Flush();
  m_appContext->CloseFloatingWindows();
  event.Skip();
}

void MainFrame::OnCopyColorHex( ColorEvent& event ){
  faint::Clipboard clipboard;
  if ( !clipboard.Good() ){
    show_copy_color_error(this);
    return;
  }
  clipboard.SetText(faint::utf8_string(str_hex(event.GetColor())));
}

void MainFrame::OnCopyColorRGB( ColorEvent& event ){
  faint::Clipboard clipboard;
  if ( !clipboard.Good() ){
    show_copy_color_error(this);
    return;
  }
  clipboard.SetText(faint::utf8_string(str_smart_rgba(event.GetColor())));
}

void MainFrame::OnKillFocusEntry(wxCommandEvent&){
  assert( m_textEntryCount > 0 );
  m_textEntryCount -= 1;
  if ( m_textEntryCount == 0 ){
    m_menubar->EndTextEntry();
  }
}

void MainFrame::OnLayerTypeChange( LayerChangeEvent& event ){
  m_layerType = event.GetLayer();
  GetActiveCanvas()->ExternalRefresh();
  OnDocumentStateChange(GetActiveCanvas()->GetCanvasId());
}

void MainFrame::OnOpenFiles( OpenFilesEvent& event ){
  Open(event.GetFileNames());
}

void MainFrame::OnSetFocusEntry(wxCommandEvent&){
  if ( m_textEntryCount == 0 ){
    // Entry controls are numeric, not all shortcuts need to be
    // disabled
    const bool numeric = true;
    m_menubar->BeginTextEntry(numeric);
  }
  m_textEntryCount++;
}

void MainFrame::OnSwapColors( wxCommandEvent& ){
  faint::Color fg = m_toolSettings.Get( ts_FgCol );
  faint::Color bg = m_toolSettings.Get( ts_BgCol );
  m_toolSettings.Set( ts_FgCol, bg );
  m_toolSettings.Set( ts_BgCol, fg );
  UpdateShownSettings();
}

void MainFrame::OnDocumentStateChange(const CanvasId& id){
  CanvasScroller* activeCanvas = GetActiveCanvas();
  assert(activeCanvas != 0);
  if ( activeCanvas->GetCanvasId() == id ){
    m_menubar->Update(activeCanvas->GetMenuFlags());
  }
  m_toolPanel->ShowSettings( m_activeTool->GetSettings() );
  m_tabControl->RefreshTabName(id);
  m_colorPanel->UpdateGrid();
  m_colorPanel->UpdateFrames(  activeCanvas->GetInterface().GetNumFrames() );
}

void MainFrame::OnToolChange( ToolChangeEvent& event ){
  DoSelectTool(event.GetTool());
}

void MainFrame::OnZoomChange(){
  ZoomLevel zoom(GetActiveCanvas()->GetZoomLevel());
  m_colorPanel->UpdateZoom(zoom);
  m_menubar->UpdateZoom(zoom);
}

void MainFrame::Open( const std::vector<std::string>& filePaths ){
  if ( m_tabControl->GetCanvasCount() != 0 ){
    // Refresh the entire frame to erase any dialog or menu droppings before
    // starting potentially long-running file loading.
    Refresh();
    Update();
  }

  std::vector<wxFileName> notFound; // Fixme: Ugly fix for issue 114 (though bundling file names is nicer than individual error messages)
  // Freeze the panel to remove some refresh glitches in the
  // tool-settings on MSW during loading.
  m_toolPanel->AsWindow()->Freeze();
  try {
    bool first = true;
    for ( unsigned int i = 0; i!= filePaths.size(); i++ ){
      wxString filePath(filePaths[i]);
      if ( wxFileName(filePath).FileExists(filePath) ){
        Open( filePath, change_tab(first) );
        first = false;
      }
      else {
        notFound.push_back(wxFileName(filePath));
      }
    }
  }
  catch ( const std::bad_alloc& ){
    show_error(this, Title("Insufficient memory to load all images."), "Out of memory");
  }
  m_toolPanel->AsWindow()->Thaw();
  if ( GetCanvasCount() == 0 ){
    NewDocument( m_appContext->GetDefaultImageInfo() );
  }

  if ( notFound.size() == 1 ){
    show_file_not_found_error(this, notFound.back());
  }
  else if ( notFound.size() > 1 ){
    std::string error = "Files not found: \n";
    for ( size_t i = 0; i != notFound.size(); i++ ){
      error += (std::string(notFound[i].GetFullPath()) + "\n");
    }
    show_error(this, Title("Files not found"), error);
  }
}

CanvasInterface* MainFrame::Open(const wxFileName& filePath, const change_tab& changeTab ){
  extension_t extension(std::string(filePath.GetExt()));
  for ( unsigned int i = 0; i!= m_formats.size(); i++ ){
    Format* format = m_formats[i];
    if ( format->Match(extension) && format->CanLoad() ){
      ImageProps props;
      format->Load( std::string(filePath.GetFullPath()), props );
      if ( !props.IsOk() ){
        show_load_failed_error( this, filePath, props.GetError() );
        return 0;
      }
      CanvasScroller* newCanvas = m_tabControl->NewDocument( props, changeTab );
      newCanvas->SetFilename( std::string(filePath.GetFullPath()) );
      m_tabControl->RefreshTabName( newCanvas->GetCanvasId() );
      wxFileName fn( filePath );
      fn.MakeAbsolute();
      m_menubar->GetRecentFiles().Add((std::string)fn.GetLongPath());
      if ( changeTab.Get() ){
        OnDocumentStateChange(newCanvas->GetCanvasId()); // Update menu
      }
      return &(newCanvas->GetInterface());
    }
  }
  show_file_not_supported_error( this, wxFileName(filePath) );
  return 0;
}

void MainFrame::PasteNew(){
  faint::Clipboard clipboard;
  if ( !clipboard.Good() ){
    return;
  }

  faint::Bitmap bmp;
  if ( clipboard.GetBitmap(bmp) ){
    ImageProps props(bmp);
    CanvasScroller* c = m_tabControl->NewDocument(props, change_tab(true));
    c->SetDirty();
    return;
  }

  objects_t objects;
  if ( clipboard.GetObjects(objects) ){
    Rect objRect = bounding_rect( objects );
    Point p0( objRect.x, objRect.y );
    offset_by( objects, -p0 );
    ImageProps props(rounded(objRect.GetSize()), objects);
    CanvasScroller* c = m_tabControl->NewDocument(props, change_tab(true));
    c->SetDirty();
    return;
  }
}

void MainFrame::PreviousTab(){
  m_tabControl->SelectPrevious();
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

Format* get_matching_format( std::vector<Format*> formats, const extension_t& ext ){
  for ( size_t i = 0; i != formats.size(); i++ ){
    Format* format = formats[i];
    if ( format->Match(ext) && format->CanSave() ){
      return format;
    }
  }
  return 0;
}

bool MainFrame::Save(CanvasInterface& canvas, const wxString& filename){
  // Fixme: Error handling
  const extension_t extension(std::string(wxFileName(filename).GetExt()));

  Format* format = get_matching_format(m_formats, extension);
  if ( format == 0 ){
    show_save_extension_error(this, extension);
    return false;
  }

  SaveResult result = format->Save( std::string(filename), canvas );
  if ( result.Failed() ){
    show_error(this, Title("Failed Saving"), result.ErrorDescription());
    return false;
  }

  canvas.SetFilename( std::string(filename) );
  canvas.ClearDirty();

  // Add to file history only if the format can be loaded
  if ( format->CanLoad() ){
    wxFileName fn( filename );
    fn.MakeAbsolute();
    m_menubar->GetRecentFiles().Add(std::string(fn.GetLongPath()));
  }
  return true;
}

void MainFrame::SelectLayer( Layer::type layer ){
  m_toolPanel->SelectLayer( layer );
}

void MainFrame::SelectTool( ToolId id ){
  m_toolPanel->SelectTool(id);
}

void MainFrame::SetActiveCanvas( const CanvasId& id ){
  m_tabControl->Select(id);
}

void MainFrame::SetActiveTool( Tool* tool ){ // fixme: Fishy. Used?
  m_activeTool = tool;
}

void MainFrame::SetPalette( const std::vector<faint::Color>& colors ){
  m_colorPanel->SetPalette(colors);
}

#ifndef FAINT_VERSION
#define FAINT_VERSION "(unknown version)"
#endif
void MainFrame::ShowAboutDialog(){
  wxAboutDialogInfo info;
  info.SetName("Faint");
  info.SetWebSite("http://code.google.com/p/faint-graphics-editor/");
  info.AddDeveloper("Lukas Kemmer");
  info.SetLicense("Copyright 2012 Lukas Kemmer\nLicensed under the Apache License, Version 2.0");
  info.SetVersion(FAINT_VERSION);

  std::stringstream ss;
  ss << "wxWidgets version: " << wxMAJOR_VERSION << "."
     << wxMINOR_VERSION << "."
     << wxRELEASE_NUMBER << "."
     << wxSUBRELEASE_NUMBER << std::endl;

  ss << "Python version: " << m_appContext->GetPythonVersion() << std::endl;
  ss << "Cairo version: " << faint::get_cairo_version() << std::endl;
  ss << "Pango version: " << faint::get_pango_version();
  info.SetDescription(ss.str());
  wxAboutBox(info);
}

void MainFrame::ShowOpenFileDialog(){
  // Use the path from the active canvas, if available,
  // default path otherwise
  wxFileName oldFileName( GetActiveCanvas()->GetFilename() );
  wxFileDialog fd( this, "Open Image(s)", // Parent and caption
    oldFileName.GetPath(), "", // Path and filename
    "*.*", // Filter(s)
    wxFD_OPEN | wxFD_MULTIPLE | wxFD_CHANGE_DIR | wxFD_FILE_MUST_EXIST );

  if ( faint::show_modal(fd) == wxID_OK ){
    wxArrayString paths;
    fd.GetPaths( paths );
    Open( to_vector(paths) );
  }
}

bool MainFrame::ShowSaveAsDialog(CanvasInterface& canvas){
  std::string oldFileNameStr( canvas.GetFilename() );
  wxFileName oldFileName(oldFileNameStr);
  int defaultFormatIndex = oldFileNameStr.empty() ? 0 :
    get_format_index( m_formats, extension_t(std::string(wxFileName(oldFileName).GetExt())));
  wxFileDialog dlg( this, "Save as", oldFileName.GetPath(), oldFileName.GetName(), format_strings_save( m_formats ).c_str(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
  dlg.SetFilterIndex( defaultFormatIndex );
  int result = faint::show_modal(dlg);
  if ( result != wxID_OK ){
    return false;
  }
  return Save(canvas, dlg.GetPath());
}

void MainFrame::ToggleColorbar( bool show ){
  m_frameSettings.palette_visible = show;
  m_colorPanel->Show( show );
  Layout();
}

void MainFrame::ToggleMaximize(){
  if ( !IsFullScreen() ){
    Maximize( !IsMaximized() );
  }
  else {
    FaintFullScreen( false );
  }
}

void MainFrame::ToggleStatusbar( bool show ){
  m_frameSettings.statusbar_visible = show;
  GetStatusBar()->Show( show );
  Layout();
}

void MainFrame::ToggleToolPanel( bool show ){
  // Update the setting state and and tool panel visibility.
  // Freezing the MainFrame removes a refresh-artifact when re-showing
  // the tool-panel on MSW (The wxStaticLine:s from the child panels
  // appeared in the canvas).
  Freeze();
  m_frameSettings.toolbar_visible = show;
  m_toolPanel->Show( show );
  Thaw();

  // Update the sizers to the new state of the tool panel.
  // Freezing the color panel removes flicker in the palette.
  // This depends on the tool panel stretching across the
  // frame.
  m_colorPanel->Freeze();
  Layout();
  m_colorPanel->Thaw();
}

void MainFrame::UpdateShownSettings(){
  const Settings& activeSettings( m_activeTool->GetSettings() );
  m_toolPanel->ShowSettings( activeSettings );
  m_colorPanel->UpdateSelectedColors( get_color_choice( activeSettings, m_toolSettings ) );
}

void MainFrame::UpdateToolSettings( const Settings& s ){
  m_toolSettings.Update( s );
  m_activeTool->ToolSettingUpdate( m_toolSettings );
  UpdateShownSettings();
}

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_COMMAND( -1, EVT_SWAP_COLORS, MainFrame::OnSwapColors )
EVT_COMMAND( -1, EVT_SET_FOCUS_ENTRY_CONTROL, MainFrame::OnSetFocusEntry )
EVT_COMMAND( -1, EVT_KILL_FOCUS_ENTRY_CONTROL, MainFrame::OnKillFocusEntry )
EVT_CLOSE( MainFrame::OnClose )
END_EVENT_TABLE()

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
#include "mainframe.hh"
#include "app/appcontext.hh"
#include "formats/format_cur.hh"
#include "formats/format_gif.hh"
#include "formats/format_ico.hh"
#include "formats/format_wx.hh"
#include "gui/canvasscroller.hh"
#include "gui/colorpanel.hh"
#include "gui/events.hh"
#include "gui/interpreterframe.hh"
#include "gui/resize-dialog-settings.hh"
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
#include "util/imageprops.hh"
#include "util/objutil.hh"
#include "util/pathutil.hh"
#include "util/settingutil.hh"
#include "util/toolutil.hh"
#include "util/util.hh"

class SBInterface : public StatusInterface {
public:
  SBInterface( wxStatusBar* statusbar ) :
    m_statusbar( statusbar ) {}

  void SetMainText( const std::string& text ) override{
    m_statusbar->SetStatusText( wxString( text.c_str() ), 0 );
  }

  void SetText( const std::string& text, int field=0 ) override{
    assert(field < m_statusbar->GetFieldsCount());
    m_statusbar->SetStatusText( wxString( text.c_str() ), field + 1 );
  }

  void Clear() override{
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

class DialogFeedbackImpl : public DialogFeedback{
public:
  DialogFeedbackImpl( CanvasScroller* canvas )
    : m_bitmap(nullptr),
      m_canvas(canvas)
  {}
  ~DialogFeedbackImpl(){
    // Fixme: Terrifying
    m_canvas->SetBitmapChimera(nullptr);
    delete m_bitmap;
  }

  faint::Bitmap& GetBitmap() override{
    if ( m_bitmap == nullptr ){
      m_bitmap = new faint::Bitmap(m_canvas->GetImageList().Active().GetBitmap());
    }
    return *m_bitmap;
  }

  CanvasInterface& GetCanvas() override{ // Fixme: Remove
    return m_canvas->GetInterface();
  }

  void Update() override{
    m_canvas->SetBitmapChimera(m_bitmap);
  }

private:
  faint::Bitmap* m_bitmap;
  CanvasScroller* m_canvas;
};

class FrameContext : public AppContext {
public:
  FrameContext( MainFrame* frame, wxStatusBar* statusbar, HelpFrame* helpFrame, InterpreterFrame* interpreterFrame )
    : m_frame( frame ),
      m_helpFrame( helpFrame ),
      m_interpreterFrame( interpreterFrame ),
      m_statusbar( statusbar ),
      m_deleteOnClose(nullptr),
      m_modalDialog(false)
  {}

  ~FrameContext(){
    delete m_deleteOnClose;
  }

  void AddFormat( Format* fileFormat ) override{
    m_frame->AddFormat( fileFormat );
  }

  void AddPaletteColor( const faint::Color& c ) override{
    m_frame->AddToPalette( faint::DrawSource(c) );
  }

  void BeginModalDialog() override{
    m_modalDialog = true;
  }

  void BeginTextEntry() override{
    m_frame->BeginTextEntry(); // Fixme: Consider moving the handling here.
  }

  void Bind( int key, int modifiers, bool global ) override{
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

  bool Bound( int key, int modifiers ) const override{
    return m_binds.find( std::make_pair(key, modifiers) ) != m_binds.end();
  }

  bool BoundGlobal( int key, int modifiers ) const override{
    return m_globalBinds.find( std::make_pair(key, modifiers) ) != m_globalBinds.end();
  }

  void Close( CanvasInterface& canvas ) override{
    m_frame->CloseDocument( canvas );
  }

  void QueueLoad( const faint::FileList& filenames ) override{
    m_frame->GetEventHandler()->QueueEvent( new OpenFilesEvent(filenames) );
  }

  void DeleteOnClose( Tool* tool ) override{
    m_deleteOnClose = tool;
  }

  void DialogOpenFile() override{
    m_frame->ShowOpenFileDialog();
  }

  void EndModalDialog() override{
    m_modalDialog = false;
  }

  void EndTextEntry() override{
    m_frame->EndTextEntry();
  }

  bool Exists( const CanvasId& id ) override{
    return m_frame->Exists( id );
  }

  BoolSetting::ValueType Get( const BoolSetting& s ) override{
    return m_frame->GetShownSettings().Get(s);
  }

  StrSetting::ValueType Get( const StrSetting& s) override{
    return m_frame->GetShownSettings().Get(s);
  }

  IntSetting::ValueType Get( const IntSetting& s ) override{
    return m_frame->GetShownSettings().Get(s);
  }

  ColorSetting::ValueType Get( const ColorSetting& s ) override{
    return m_frame->GetShownSettings().Get(s);
  }

  FloatSetting::ValueType Get( const FloatSetting& s ) override{
    return m_frame->GetShownSettings().Get(s);
  }

  CanvasInterface& GetActiveCanvas() override{
    return m_frame->GetActiveCanvas()->GetInterface();
  }

  Tool* GetActiveTool() override{
    return m_frame->GetActiveTool();
  }

  CanvasInterface& GetCanvas( size_t i ) override{
    return m_frame->GetCanvas(i);
  };

  size_t GetCanvasCount() const override{
    return m_frame->GetCanvasCount();
  }

  Grid GetDefaultGrid() const override{
    return m_defaultGrid;
  }

  ImageInfo GetDefaultImageInfo() override{
    return ImageInfo(IntSize(640,480), faint::Color(255,255,255));
  }

  std::vector<Format*> GetFormats() override{
    return m_frame->GetFileFormats();
  }

  ResizeDialogSettings GetDefaultResizeDialogSettings() const override{
    return m_defaultResizeSettings;
  }

  Layer GetLayerType() const override{
    return m_frame->GetLayerType();
  }

  Point GetMousePos() override{
    wxPoint mousePos = wxGetMousePosition();
    return Point( mousePos.x, mousePos.y );
  }

  StatusInterface& GetStatusInfo() override{
    return m_statusbar;
  }

  ToolId GetToolId() override{
    return m_frame->GetToolId();
  }

  Settings GetToolSettings() const override{
    return m_frame->GetShownSettings();
  }

  const TransparencyStyle& GetTransparencyStyle() const override{
    return m_transparencyStyle;
  }

  CanvasInterface* Load( const faint::FilePath& filePath, const change_tab& changeTab ) override{
    return m_frame->Open( filePath, changeTab  );
  }

  void Load( const faint::FileList& filePaths ) override{
    return m_frame->Open( filePaths );
  }

  CanvasInterface* LoadAsFrames( const faint::FileList& paths, const change_tab& changeTab ) override{
    std::vector<ImageProps> props;
    std::vector<Format*>& formats = m_frame->GetFileFormats();
    for ( size_t pathNum = 0; pathNum != paths.size(); pathNum++ ){
      faint::FilePath filePath( paths[pathNum] );
      extension_t extension(std::string(filePath.GetExtension()));
      bool loaded = false;
      for ( Format* format : formats ){
        if ( format->Match(extension) && format->CanLoad() ){
          props.push_back(ImageProps());
          format->Load( filePath, props.back() );
          if ( !props.back().IsOk() ){
	    // Fixme: Commented for some reason
            // show_load_failed_error(m_frame, filePath, props.back().GetError() );
            return nullptr;
          }
          loaded = true;
          break;
        }
      }
      if ( !loaded ){
        // show_load_failed_error(m_frame, filePath, "One path could not be loaded.");
        return nullptr;
      }
    }
    CanvasScroller* canvas = m_frame->GetTabControl()->NewDocument(props, changeTab,
      initially_dirty(true));
    CanvasInterface* canvasInterface = &(canvas->GetInterface());
    return canvasInterface;
  }

  void Maximize() override{
    m_frame->Maximize( !m_frame->IsMaximized() );
  }

  void MaximizeInterpreter() override{
    m_interpreterFrame->Maximize( !m_interpreterFrame->IsMaximized() );
  }

  bool ModalDialogShown() const override{
    return m_modalDialog;
  }

  CanvasInterface& NewDocument( const ImageInfo& info ) override{
    CanvasScroller* canvas = m_frame->NewDocument( info );
    return canvas->GetInterface();
  }

  void OnActiveCanvasChanged() override{
    m_frame->OnActiveCanvasChanged();
  }

  void OnDocumentStateChange(const CanvasId& id) override{
    m_frame->OnDocumentStateChange(id);
  }

  void OnZoomChange() override{
    m_frame->OnZoomChange();
  }

  void PythonContinuation() override{
    m_interpreterFrame->NewContinuation();
  }

  void PythonDone() override{
    for ( CanvasInterface* canvas : m_unrefreshed ){
      if ( Exists( m_canvasIds[canvas] ) ){
        canvas->Refresh();
      }
    }
    m_unrefreshed.clear();

    for ( CanvasInterface* canvas : m_commandBundles ){
      canvas->CloseUndoBundle();
    }
    m_commandBundles.clear();
  }

  void PythonGetKey() override{
    m_interpreterFrame->GetKey();
  }

  void PythonIntFaintPrint( const std::string& s ) override{
    m_interpreterFrame->IntFaintPrint(s);
  }

  void PythonNewPrompt() override{
    m_interpreterFrame->NewPrompt();
  }

  void PythonPrint( const std::string& s ) override{
    m_interpreterFrame->Print(s);
  }

  void PythonQueueRefresh( CanvasInterface* canvas ) override{
    QueueRefresh(canvas);
  }

  void PythonRunCommand( CanvasInterface* canvas, Command* command ) override{
    QueueRefresh(canvas);
    if ( std::find(m_commandBundles.begin(), m_commandBundles.end(), canvas) == m_commandBundles.end() ){
      m_commandBundles.push_back(canvas);
      canvas->OpenUndoBundle();
    }
    canvas->RunCommand( command );
  }

  void PythonRunCommand( CanvasInterface* canvas, Command* command, const FrameId& frameId ) override{
    QueueRefresh(canvas);
    if ( std::find(m_commandBundles.begin(), m_commandBundles.end(), canvas) == m_commandBundles.end() ){
      m_commandBundles.push_back(canvas);
      canvas->OpenUndoBundle();
    }
    canvas->RunCommand( command, frameId );
  }

  void Quit() override{
    m_frame->Close(false); // False means don't force
  }

  void RaiseFrame() override{
    m_frame->Raise();
  }

  void SelectTool( ToolId id ) override{
    m_frame->SelectTool( id );
  }

  void Set( const BoolSetting& s, BoolSetting::ValueType v ) override{
    change_setting( *m_frame, s, v, from_control(false) );
  }

  void Set( const StrSetting& s, StrSetting::ValueType v) override{
    change_setting( *m_frame, s, v, from_control(false) );
  }

  void Set( const IntSetting& s, IntSetting::ValueType v ) override{
    change_setting( *m_frame, s, v, from_control(false) );
  }

  void Set( const ColorSetting& s, ColorSetting::ValueType v ) override{
    change_setting( *m_frame, s, v, from_control(false) );
  }

  void Set( const FloatSetting& s, FloatSetting::ValueType v ) override{
    change_setting( *m_frame, s, v, from_control(false) );
  }

  void SetActiveCanvas( const CanvasId& id ) override{
    return m_frame->SetActiveCanvas(id);
  }

  void SetDefaultGrid( const Grid& grid ) override{
    m_defaultGrid = grid;
  }

  void SetDefaultResizeDialogSettings( const ResizeDialogSettings& settings ) override{
    m_defaultResizeSettings = settings;
  }

  void SetInterpreterBackground( const faint::Color& c ) override{
    m_interpreterFrame->FaintSetBackgroundColour(c);
  }

  void SetInterpreterTextColor( const faint::Color& c ) override{
    m_interpreterFrame->SetTextColor(c);
  }

  void SetPalette( const faint::DrawSourceMap& drawSourceMap ) override{
    m_frame->SetPalette(drawSourceMap);
  }

  void SetTransparencyStyle(const TransparencyStyle& style) override{
    m_transparencyStyle = style;
    // Fixme: Trigger redraw on visible canvases
  }

  void SetLayer( Layer layer ) override{
    m_frame->SelectLayer( layer );
  }

  void Show( CommandDialog& dialog ) override{
    DialogFeedbackImpl feedback(m_frame->GetActiveCanvas());
    if ( faint::show_modal(dialog, m_frame, feedback ) ){
      Command* cmd = dialog.GetCommand();
      if ( cmd != nullptr ){
        CanvasInterface& canvas( GetActiveCanvas() );
        canvas.RunCommand(cmd);
        canvas.Refresh();
      }
    }
  }

  void ShowHelpFrame() override{
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

  void ShowPythonConsole() override{
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

  void Unbind( int key, int modifiers ) override{
    m_binds.erase( std::make_pair(key, modifiers) );
  }

  void UpdateShownSettings() override{
    m_frame->UpdateShownSettings();
  }

  void UpdateToolSettings( const Settings& s ) override{
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
    m_canvasIds[ canvas ] = canvas->GetId();
  }
  MainFrame* m_frame;
  HelpFrame* m_helpFrame;
  InterpreterFrame* m_interpreterFrame;
  SBInterface m_statusbar;
  std::set<CanvasInterface*> m_unrefreshed;
  std::map<CanvasInterface*, CanvasId> m_canvasIds;
  std::vector<CanvasInterface*> m_commandBundles;
  typedef std::pair<int,int> modkey_t; // Fixme: Create a key class
  std::set<modkey_t> m_binds;
  std::set<modkey_t> m_globalBinds;
  Tool* m_deleteOnClose;
  bool m_modalDialog;
  Grid m_defaultGrid;
  ResizeDialogSettings m_defaultResizeSettings;
  TransparencyStyle m_transparencyStyle;
};

class FrameNotifier : public SettingNotifier {
public:
  FrameNotifier( MainFrame& frame, const from_control& fromCtrl )
    : m_frame(frame),
      m_fromCtrl(fromCtrl)
  {}
  void Notify( const BoolSetting& s, bool value ) override{
    change_setting(m_frame, s, value, m_fromCtrl);
  }
  void Notify( const IntSetting& s, int value ) override{
    change_setting(m_frame, s, value, m_fromCtrl);
  }
  void Notify( const StrSetting& s, const std::string& value ) override{
    change_setting(m_frame, s, value, m_fromCtrl);
  }
  void Notify( const ColorSetting& s, const faint::DrawSource& value ) override{
    change_setting(m_frame, s, value, m_fromCtrl);
  }
  void Notify( const FloatSetting& s, FloatSetting::ValueType value ) override{
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
  formats.push_back( new FormatCUR());
  formats.push_back( new FormatWX(extension_t("bmp"),
      label_t("Windows Bitmap"), wxBITMAP_TYPE_BMP ) );
  formats.push_back( new FormatWX(vector_of(extension_t("jpg"), extension_t("jpeg")),
      label_t("JPEG"), wxBITMAP_TYPE_JPEG ) );
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
  return -1;
}

ColorChoice get_color_choice( const Settings& preferred, const Settings& fallback ){
  ColorChoice c = {
    preferred.Has( ts_FgCol ) ? preferred.Get( ts_FgCol ) : fallback.Get( ts_FgCol ),
    preferred.Has( ts_BgCol ) ? preferred.Get( ts_BgCol ) : fallback.Get( ts_BgCol ) };
  return c;
}

MainFrame::MainFrame( ArtContainer& art, PaletteContainer& palettes, HelpFrame* helpFrame, InterpreterFrame* interpreterFrame )
  : wxFrame(nullptr, wxID_ANY, "Faint", wxPoint(50,50), wxSize(800,700) ),
    m_artContainer( art ),
    m_appContext(nullptr),
    m_notifier(nullptr),
    m_menubar(nullptr),
    m_tabControl(nullptr),
    m_toolPanel(nullptr),
    m_colorPanel(nullptr),
    m_layerType( Layer::RASTER ),
    m_activeTool(nullptr),
    m_toolSettings(default_tool_settings()),
    m_palettes( palettes ),
    m_textEntryCount(0)
{
  Bind(FAINT_CHANGE_TOOL, ToolChangeEventHandler(MainFrame::OnToolChange), this);
  Bind(FAINT_CHANGE_LAYER, LayerChangeEvtHandler(MainFrame::OnLayerTypeChange), this);
  Bind(FAINT_ADD_TO_PALETTE, DrawSourceEventHandler(MainFrame::OnAddToPalette), this);
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
  m_toolPanel->SelectTool(ToolId::LINE);
}

MainFrame::~MainFrame(){
  // Note: Deletion of the AppContext is handled outside MainFrame.
  // Fixme: Creation should be moved outside as well.
  m_appContext = nullptr;

  delete m_notifier;
  delete m_updatingNotifier;
  // These do not inherit from wxWindow and need to be deleted
  // manually.
  delete m_menubar;
  delete m_toolPanel;
  delete m_colorPanel;
  delete m_tabControl;
  for ( Format* format : m_formats ){
    delete format;
  }
}

void MainFrame::AddToPalette( const faint::DrawSource& src ){
  m_colorPanel->AddToPalette(src);
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
  // Freeze the color panel to avoid the FrameControl refreshing
  // during page close on GTK. (See issue 122).
  m_colorPanel->Freeze();
  m_tabControl->CloseActive();
  m_colorPanel->Thaw();

}

void MainFrame::CloseDocument( CanvasInterface& canvas ){
  for ( size_t i = 0; i != m_tabControl->GetCanvasCount(); i++ ){
    CanvasInterface& other = m_tabControl->GetCanvas(i)->GetInterface();
    if ( &other == &canvas ){
      // Freeze the color panel to avoid the FrameControl refreshing
      // during page close on GTK. (See issue 122).
      m_colorPanel->Freeze();
      m_tabControl->Close( i );
      m_colorPanel->Thaw();
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
  if ( m_activeTool != nullptr ){
    CanvasScroller* canvas = m_tabControl->GetActiveCanvas();
    canvas->Preempt(PreemptOption::ALLOW_COMMAND);
    delete m_activeTool;
    m_activeTool = nullptr;
  }
  m_activeTool = new_tool(id);
  if ( doesnt_handle_selection(id) ){
    // The selection tool show the settings from the selection (either
    // objects, or a floating raster selection)
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

void MainFrame::FaintInitialize(){
  // Pretty much all of Faint expects there to be an active canvas, so
  // create one early. It would be nicer if no blank "untitled"-image
  // was created when starting with files from the command line, but
  // this caused crashes if the loading failed (even with attempt to open a blank document on failure)
  // ...Perhaps the loading should be detached from the MainFrame and
  // everything, but that's for some other time.
  NewDocument(m_appContext->GetDefaultImageInfo());
}

CanvasScroller* MainFrame::GetActiveCanvas(){
  CanvasScroller* active = m_tabControl->GetActiveCanvas();
  assert(active != nullptr);
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

Layer MainFrame::GetLayerType() const{
  return m_layerType;
}

Settings MainFrame::GetShownSettings() const{
  Settings settings(m_toolSettings);
  settings.Update( m_activeTool->GetSettings() );
  return settings;
}

EntryMode MainFrame::GetTextEntryMode() const{
  return m_menubar->GetTextEntryMode();
}

ToolId MainFrame::GetToolId() const{
  if ( m_activeTool == nullptr ){
    return ToolId::OTHER;
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
  CanvasScroller* canvas = m_tabControl->NewDocument( props, change_tab(true), initially_dirty(false) );
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

void MainFrame::OnAddToPalette( DrawSourceEvent& event ){
  AddToPalette( event.GetDrawSource() );
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
  faint::DrawSource fg(m_appContext->Get( ts_FgCol ));
  faint::DrawSource bg(m_appContext->Get( ts_BgCol ));
  m_appContext->Set( ts_FgCol, bg );
  m_appContext->Set( ts_BgCol, fg );
}

void MainFrame::OnDocumentStateChange(const CanvasId& id){
  CanvasScroller* activeCanvas = GetActiveCanvas();
  assert(activeCanvas != nullptr);
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

void MainFrame::Open( const faint::FileList& paths ){
  if ( m_tabControl->GetCanvasCount() != 0 ){
    // Refresh the entire frame to erase any dialog or menu droppings before
    // starting potentially long-running file loading.
    Refresh();
    Update();
  }

  faint::FileList notFound; // Fixme: Ugly fix for issue 114 (though bundling file names is nicer than individual error messages)
  // Freeze the panel to remove some refresh glitches in the
  // tool-settings on MSW during loading.
  m_toolPanel->AsWindow()->Freeze();
  try {
    bool first = true;
    for ( unsigned int i = 0; i!= paths.size(); i++ ){
      const faint::FilePath& filePath( paths[i] );
      if ( exists(filePath) ){
        Open( filePath, change_tab(first) );
        first = false;
      }
      else {
        notFound.push_back(filePath); // Fixme
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
    wxString error = "Files not found: \n";
    for ( size_t i = 0; i != notFound.size(); i++ ){
      error += (notFound[i].ToWx().GetLongPath() + "\n");
    }
    show_error(this, Title("Files not found"), error);
  }
}

CanvasInterface* MainFrame::Open(const faint::FilePath& filePath, const change_tab& changeTab ){
  extension_t extension(std::string(filePath.GetExtension())); // Fixme
  for ( unsigned int i = 0; i!= m_formats.size(); i++ ){
    Format* format = m_formats[i];
    if ( format->Match(extension) && format->CanLoad() ){
      ImageProps props;
      format->Load( filePath, props );
      if ( !props.IsOk() ){
        show_load_failed_error( this, filePath, props.GetError() );
        return nullptr;
      }
      CanvasScroller* newCanvas = m_tabControl->NewDocument( props, changeTab, initially_dirty(false) );
      newCanvas->NotifySaved( filePath );
      m_tabControl->RefreshTabName( newCanvas->GetCanvasId() ); // Fixme: prolly done automatically due to NotifySaved
      m_menubar->GetRecentFiles().Add(filePath);
      if ( changeTab.Get() ){
        OnDocumentStateChange(newCanvas->GetCanvasId()); // Update menu // Fixme: prolly done automatically
      }
      if ( props.GetNumWarnings() != 0 ){
        show_load_warnings( this, props );
      }
      return &(newCanvas->GetInterface());
    }
  }
  show_file_not_supported_error( this, filePath );
  return nullptr;
}

void MainFrame::PasteNew(){
  faint::Clipboard clipboard;
  if ( !clipboard.Good() ){
    return;
  }

  faint::Bitmap bmp;
  if ( clipboard.GetBitmap(bmp) ){
    ImageProps props(bmp);
    m_tabControl->NewDocument(props, change_tab(true), initially_dirty(true));
    return;
  }

  objects_t objects;
  if ( clipboard.GetObjects(objects) ){
    Rect objRect = bounding_rect( objects );
    Point p0( objRect.x, objRect.y );
    offset_by( objects, -p0 );
    ImageProps props(rounded(objRect.GetSize()), objects);
    m_tabControl->NewDocument(props, change_tab(true), initially_dirty(true));
    return;
  }
}

void MainFrame::PreviousTab(){
  m_tabControl->SelectPrevious();
}

bool MainFrame::Save( CanvasInterface& canvas ){
  Optional<faint::FilePath> filePath( canvas.GetFilePath() );
  if ( filePath.NotSet() ){
    return ShowSaveAsDialog(canvas);
  }
  else {
    return Save(canvas, filePath.Get());
  }
}

Format* get_matching_format( std::vector<Format*> formats, const extension_t& ext ){
  for ( size_t i = 0; i != formats.size(); i++ ){
    Format* format = formats[i];
    if ( format->Match(ext) && format->CanSave() ){
      return format;
    }
  }
  return nullptr;
}

bool MainFrame::Save(CanvasInterface& canvas, const faint::FilePath& filePath ){
  // Fixme: Error handling
  const extension_t extension(filePath.GetExtension());
  Format* format = get_matching_format(m_formats, extension);
  if ( format == nullptr ){
    show_save_extension_error(this, extension);
    return false;
  }

  SaveResult result = format->Save( filePath, canvas );
  if ( result.Failed() ){
    show_error(this, Title("Failed Saving"), result.ErrorDescription());
    return false;
  }

  canvas.NotifySaved( filePath );
  // Add to file history only if the format can be loaded
  if ( format->CanLoad() ){
    m_menubar->GetRecentFiles().Add(filePath);
  }
  return true;
}

void MainFrame::SelectLayer( Layer layer ){
  m_toolPanel->SelectLayer( layer );
}

void MainFrame::SelectTool( ToolId id ){
  m_toolPanel->SelectTool(id);
}

void MainFrame::SetActiveCanvas( const CanvasId& id ){
  m_tabControl->Select(id);
}

void MainFrame::SetPalette( const faint::DrawSourceMap& drawSourceMap ){
  m_colorPanel->SetPalette(drawSourceMap);
}

#ifndef FAINT_VERSION
#define FAINT_VERSION "(unknown version)"
#endif
void MainFrame::ShowAboutDialog(){
  wxAboutDialogInfo info;
  info.SetName("Faint");
  info.SetWebSite("http://code.google.com/p/faint-graphics-editor/");
  info.AddDeveloper("Lukas Kemmer");
  info.SetLicense("Copyright 2013 Lukas Kemmer\nLicensed under the Apache License, Version 2.0");
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
  // Use the path from the active canvas, if available, default path
  // otherwise
  Optional<faint::FilePath> oldFileName( GetActiveCanvas()->GetFilePath() );
  std::string initialPath( oldFileName.IsSet() ?
    oldFileName.Get().DirPath() :
    std::string("") );

  wxFileDialog fd( this, "Open Image(s)", // Parent and caption
    initialPath, "", // Path and filename
    "*.*", // Filter(s)
    wxFD_OPEN | wxFD_MULTIPLE | wxFD_CHANGE_DIR | wxFD_FILE_MUST_EXIST );

  if ( faint::show_modal(fd) == wxID_OK ){
    wxArrayString paths;
    fd.GetPaths( paths );
    Open( to_FileList(paths) );
  }
}

bool MainFrame::ShowSaveAsDialog(CanvasInterface& canvas){
  Optional<faint::FilePath> oldFilePath( canvas.GetFilePath() );
  wxFileName initialPath( oldFilePath.IsSet() ?
    oldFilePath.Get().ToWx() :
    wxFileName() );
  int defaultFormatIndex = oldFilePath.NotSet() ? 0 :
    get_format_index( m_formats, extension_t(std::string(initialPath.GetExt())));

  wxFileDialog dlg( this, "Save as",
    initialPath.GetPath(),
    initialPath.GetName(),
    format_strings_save( m_formats ).c_str(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

  if ( defaultFormatIndex != -1 ){
    dlg.SetFilterIndex( defaultFormatIndex );
  }

  int result = faint::show_modal(dlg);
  if ( result != wxID_OK ){
    return false;
  }
  return Save(canvas, faint::FilePath::FromAbsoluteWx(dlg.GetPath()));
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

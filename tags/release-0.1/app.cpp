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

#include <fstream>
#include "app.hh"
#include "appcontext.hh"
#include "cursors.hh"
#include "gui/interpreterframe.hh"
#include "paletteparser.hh"
#include "python/pyinterface.hh"
#include "util/bindkey.hh"
#include "util/pathutil.hh"
#include "wx/clipbrd.h"
#include "wx/cmdline.h"
#include "wx/dir.h"
#include "wx/filename.h"
#include "wx/snglinst.h"
#include "wx/persist.h"
#include "wx/persist/toplevel.h"
class Canvas;

const wxString faintPort("3793"); // Fixme: This is dumb.

static const wxCmdLineEntryDesc g_cmdLineDesc[] = {
  { wxCMD_LINE_SWITCH, "h", "help", "displays help on the command line parameters", wxCMD_LINE_VAL_NONE,
    wxCMD_LINE_OPTION_HELP },
  { wxCMD_LINE_SWITCH, "s", "silent", "disables the GUI", wxCMD_LINE_VAL_STRING,
    wxCMD_LINE_PARAM_OPTIONAL },
  { wxCMD_LINE_PARAM, "e", "whatev", "image files", wxCMD_LINE_VAL_STRING,
    wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE },
  { wxCMD_LINE_SWITCH, "i", "newinst", "force new instance", wxCMD_LINE_VAL_STRING,
    wxCMD_LINE_PARAM_OPTIONAL },
  { wxCMD_LINE_SWITCH, "ii", "noserver", "prevent this instance from becoming a main app", wxCMD_LINE_VAL_STRING,
    wxCMD_LINE_PARAM_OPTIONAL },
  { wxCMD_LINE_OPTION, "", "run", "Run a Python script file after loading images", wxCMD_LINE_VAL_STRING,
    wxCMD_LINE_PARAM_OPTIONAL},
  { wxCMD_LINE_NONE, "", "", "", wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL } // Sentinel
};

void Application::PushFile( const wxString& filename ){
  m_frame->Open( filename, false );
}

wxCursor LoadCursor( wxBitmap* bmp, unsigned int hotspot_x, unsigned int hotspot_y ){
  wxImage img = bmp->ConvertToImage();
  img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotspot_x );
  img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hotspot_y );
  img.SetMaskColour( 255, 0, 255 );
  return wxCursor(img);
}

bool Application::SetupSingleInst(){
  // Setup the single-instance checker to match Faint-instances for
  // this user
  const wxString name = wxString::Format("Faint-%s", wxGetUserId().c_str() );
  m_singleInstanceChk = new wxSingleInstanceChecker( name );

  if ( !m_singleInstanceChk->IsAnotherRunning() ){
    if ( m_cmd_preventServer ){
      delete m_singleInstanceChk;
      m_singleInstanceChk = 0;
    }
    else {
      // Set this up as the main-application
      m_faintServer = new FaintServer;
      if ( !m_faintServer->Create(faintPort) ){
        wxLogError("Failed to create an IPC-service.");
      }
    }

    return true;
  }

  else if ( m_cmd_forceNew || m_cmdLineFiles.size() == 0 ) {
    // Either: No files passed to Faint, or a new instance was explicitly requested
    // (Todo: Consider raising current application if m_cmdLineFiles.size() == 0
    // as I always end up starting several Faint anyway by mistake)
    return true;
  }

  // Faint is running since previously and should be passed the
  // filenames from this instance which should then shut down
  FaintClient* client = new FaintClient();
  wxString hostname = "localhost";
  wxConnectionBase* connection = 0;

  int attempt = 0;
  while ( attempt < 3 && connection == 0 ){
    connection = client->MakeConnection( hostname, faintPort, "faint");
    attempt++;
    if ( connection == 0 ){
      wxThread::Sleep( 500 );
    }
  }

  if ( connection == 0 ){
    // Connecting failed to the old instance failed. Start this
    // instance normally.
    delete client;
    return true;
  }

  // Pass all the files to the old instance
  for ( size_t i = 0; i!= m_cmdLineFiles.size(); i++ ){
    connection->Execute( m_cmdLineFiles[i] );
  }

  connection->Disconnect();
  delete client;
  delete connection;

  // Prevent a new instance
  return false;
}

Application::Application()
  : m_frame(0),
    m_interpreterFrame(0),
    m_cmd_silentMode(false),
    m_cmd_script(false),
    m_cmd_preventServer(false),
    m_cmd_forceNew(false),
    m_singleInstanceChk(0),
    m_faintServer(0),
    m_appContext(0)
{}

bool Application::OnInit(){
  // Perform default init for command-line etc.
  if ( !wxApp::OnInit() ){
    return false;
  }

  if ( !SetupSingleInst() ){
    return false;
  }

  wxInitAllImageHandlers();
  LoadGraphics();
  InitCursors();
  InitPalettes();

  m_interpreterFrame = new InterpreterFrame( 0 );
  m_interpreterFrame->SetName("InterpreterFrame");
  wxPersistenceManager::Get().RegisterAndRestore(m_interpreterFrame);
  m_frame = new MainFrame( wxPoint(50, 50), wxSize(800, 700), m_artContainer, m_palettes, m_interpreterFrame );
  m_frame->SetName("MainFrame");
  wxPersistenceManager::Get().RegisterAndRestore(m_frame);
  m_appContext = m_frame->GetContext();
  initPython();
  if ( ! m_cmd_silentMode ){
    m_frame->Show(true);
  }

  if ( m_cmdLineFiles.GetCount() > 0 ) {
    m_frame->Open( m_cmdLineFiles );
  }
  else {
    m_frame->NewDocument( m_frame->GetDefaultCanvasInfo() );
  }
  runUserPythonIni();

  SetTopWindow( m_frame );
  return true;
}

int Application::OnRun(){
  if ( m_scriptName != "" ){
    const wxString cmd = "execfile('" + m_scriptName + "')";
    runCmd( std::string(cmd) );
  }

  if ( m_cmd_silentMode ){
    return 0;
  }
  return wxApp::OnRun();
}

void Application::OnInitCmdLine(wxCmdLineParser& parser){
  parser.SetDesc (g_cmdLineDesc);
}

bool Application::OnCmdLineParsed(wxCmdLineParser& parser){
  m_cmd_silentMode = parser.Found("s");
  m_cmd_forceNew = parser.Found("i");
  m_cmd_preventServer = parser.Found("ii");
  parser.Found("run", &m_scriptName);
  for ( size_t i = 0; i!= parser.GetParamCount(); i++ ){
    wxFileName file( parser.GetParam(i) );
    file.MakeAbsolute();
    m_cmdLineFiles.Add( file.GetLongPath() );
  }

  return true;
}

MainFrame* Application::GetFrame(){
  return m_frame;
}

InterpreterFrame* Application::GetInterpreterFrame(){
  return m_interpreterFrame;
}

int Application::OnExit(){
  delete m_singleInstanceChk;
  delete m_faintServer;
  delete m_appContext;
  return wxApp::OnExit();
}

std::vector<Format*>& Application::GetFileFormats(){
  return m_frame->GetFileFormats();
}

void Application::InitCursors(){
  m_cursors[CURSOR_ARROW] = wxCursor(wxCURSOR_ARROW);
  m_cursors[CURSOR_PEN] = LoadCursor( m_artContainer.Get("cur_pen") , 0, 11);
  m_cursors[CURSOR_CROSSHAIR] = LoadCursor( m_artContainer.Get("cur_crosshair"), 8, 8);
  m_cursors[CURSOR_TEXT_CROSSHAIR] = LoadCursor( m_artContainer.Get("cur_text_cross"), 8, 8);
  m_cursors[CURSOR_PICKER] = LoadCursor(m_artContainer.Get("cur_picker"), 0, 11);
  m_cursors[CURSOR_FILL] = LoadCursor(m_artContainer.Get("cur_bucket"), 5, 14);
  m_cursors[CURSOR_BRUSH] = LoadCursor(m_artContainer.Get("cur_brush"), 8, 8);
  m_cursors[CURSOR_CARET] = wxCursor(wxCURSOR_IBEAM);
  m_cursors[CURSOR_MOVE] = LoadCursor(m_artContainer.Get("cur_move"), 8, 8);
  m_cursors[CURSOR_CLONE] = LoadCursor(m_artContainer.Get("cur_clone"), 8, 10);
  m_cursors[CURSOR_SQUARE_CROSS] = LoadCursor( m_artContainer.Get("cur_square_cross"), 8,8);
  m_cursors[CURSOR_ROTATE_RIGHT] = LoadCursor( m_artContainer.Get("cur_rotate_right"),6,7);
  m_cursors[CURSOR_MOVE_POINT] = LoadCursor( m_artContainer.Get("cur_move_point"),8,8);
  m_cursors[CURSOR_RESIZE_NW] = wxCursor(wxCURSOR_SIZENESW);
  m_cursors[CURSOR_RESIZE_NE] = wxCursor(wxCURSOR_SIZENWSE);
  m_cursors[CURSOR_RESIZE_WE] = wxCursor(wxCURSOR_SIZEWE);
  m_cursors[CURSOR_RESIZE_NS] = wxCursor(wxCURSOR_SIZENS);

  #ifdef __WXMSW__
  // wxCURSOR_BLANK Requires including wx.rc for MSW.
  // Since I'm lazy, create a blank cursor instead.
  wxImage image(1,1);
  image.SetMask(true);
  image.SetMaskColour(0, 0, 0);
  m_cursors[CURSOR_BLANK] = wxCursor( image );
  #else
  m_cursors[CURSOR_BLANK] = wxCursor(wxCURSOR_BLANK);
  #endif
}

wxCursor& Application::GetCursor( unsigned int id ){
  assert( id <= m_cursors.size() );
  return m_cursors[ id ];
}

wxString GetCursorFile( const wxString& cursorLabel ){
  return wxString("graphics/cursor_") + cursorLabel + wxString(".png");
}

void Application::LoadGraphics(){
  m_artContainer.SetRoot( GetDataDir() );
  struct FileAndLabel{
    wxString fn;
    wxString label;
  };
  FileAndLabel files[] = {
    // Tool icons
    { "graphics/toolicon_brush.png", "toolicon_brush"},
    { "graphics/toolicon_ellipse.png", "toolicon_ellipse" },
    { "graphics/toolicon_floodfill.png", "toolicon_floodfill" },
    { "graphics/toolicon_line.png", "toolicon_line" },
    { "graphics/toolicon_pen.png", "toolicon_pen" },
    { "graphics/toolicon_picker.png", "toolicon_picker" },
    { "graphics/toolicon_rectangle.png", "toolicon_rectangle" },
    { "graphics/toolicon_rectsel.png", "toolicon_rectsel" },
    { "graphics/toolicon_selobject.png", "toolicon_selobject" },
    { "graphics/toolicon_spline.png", "toolicon_spline" },
    { "graphics/toolicon_text.png", "toolicon_text" },
    { "graphics/toolicon_polygon.png", "toolicon_polygon" },
    { "graphics/toolicon_shape.png", "toolicon_shape" },

    // Tool settings
    {"graphics/choice_opaque.png", "sett_opaque_bg"},
    {"graphics/choice_transparent.png", "sett_transparent_bg"},

    {"graphics/fillstyle_border.png", "sett_fill_border"},
    {"graphics/fillstyle_border_and_fill.png", "sett_fill_border_fill"},
    {"graphics/fillstyle_fill.png", "sett_fill_fill"},

    {"graphics/icon_faint16.png", "faint_icon16"},
    {"graphics/icon_faint32.png", "faint_icon32"},
    {"graphics/icon_faint_python32.png", "faint_python_icon32"},
    {"graphics/icon_faint_python16.png", "faint_python_icon16"},
    {"graphics/icon_palette.png", "palette_icon"},

    {"graphics/layer_raster.png", "layer_raster"},
    {"graphics/layer_vector.png", "layer_object"},

    {"graphics/linestyle_long_dash.png", "sett_linestyle_long_dash"},
    {"graphics/linestyle_solid.png", "sett_linestyle_solid"},

    {"graphics/textbox_border.png", "sett_txt_border"},
    {"graphics/textbox_border_fill.png", "sett_txt_border_fill"},
    {"graphics/textbox_no_box.png", "sett_txt_no_box"},
    {"graphics/textbox_no_box_opaque_bg.png", "sett_txt_no_box_opaque"},
    {"graphics/brush_circle.png", "sett_brush_circle"},
    {"graphics/brush_rect.png", "sett_brush_rectangle"},
    {"graphics/line_arrow_front.png", "sett_line_arrow_front"},
    {"graphics/line_no_arrow.png", "sett_line_no_arrow"},
    {"graphics/resizedlg_center.png", "resizedlg_center"},
    {"graphics/resizedlg_topleft.png", "resizedlg_topleft"},
    {"graphics/resizedlg_scale.png", "resizedlg_scale"},

    // Cursors
    { GetCursorFile("brush"), "cur_brush"},
    { GetCursorFile("bucket"), "cur_bucket"},
    { GetCursorFile("crosshair"), "cur_crosshair"},
    { GetCursorFile("move"), "cur_move"},
    { GetCursorFile("clone"), "cur_clone"},
    { GetCursorFile("pen"), "cur_pen"},
    { GetCursorFile("picker"), "cur_picker"},
    { GetCursorFile("text_cross"), "cur_text_cross"},
    { GetCursorFile("square_cross"), "cur_square_cross"},
    { GetCursorFile("rotate_right"), "cur_rotate_right"},
    { GetCursorFile("move_point"), "cur_move_point"}
  };

  unsigned int numFiles = sizeof( files ) / sizeof( FileAndLabel );
  for ( unsigned int i = 0; i!= numFiles; i++ ){
    FileAndLabel& currFile = files[i];
    m_artContainer.Load( currFile.fn, currFile.label );
  }
}

void Application::InitPalettes(){
  wxDir dir(GetDataDir() + "/palettes");
  assert (dir.IsOpened());
  std::string std_dirname( dir.GetName().char_str() );

  wxString filename;
  bool morefiles = dir.GetFirst(&filename);
  wxString suffix = ".txt";
  wxString prefix;
  while ( morefiles ){
    if ( filename.EndsWith(suffix, &prefix ) ){
      std::string str_filename(filename.char_str());
      std::ifstream file((std_dirname + "/" + str_filename).c_str());

      if ( file ){
        m_palettes[ prefix ] = parse( file );
      }
    }
    morefiles = dir.GetNext(&filename);
  }
}

AppContext& Application::GetAppContext(){
  return *m_appContext;
}

int Application::FilterEvent( wxEvent& untypedEvent ){
  // Filter for key events so that bound keys are relayed to Python
  // regardless of what control has focus.  This prevents controls
  // like tool buttons, the aui-toolbar etc from swallow keypresses
  // when they have focus.
  if ( untypedEvent.GetEventType() == wxEVT_KEY_DOWN ){
    if ( !m_frame->MainFrameFocused() ){
      // Ignore Python-binds if a window or dialog is shown above
      // the mainframe
      return Event_Skip;
    }
    if ( m_frame->TextEntryActive() ){
      // Python binds would interfere with the text tool
      return Event_Skip;
    }
    if ( m_frame->EntryControlFocused() ){
      // Especially numeric binds would cause interfere with text edit
      // controls
      return Event_Skip;
    }

    // Note: Afaik, filterEvent does not happen while a menu is open, so no check
    // to avoid this is performed

    wxKeyEvent& event = (wxKeyEvent&)untypedEvent;
    int keyCode = event.GetKeyCode();
    if ( keyCode == WXK_CONTROL || keyCode == WXK_SHIFT || keyCode == WXK_ALT ){
      return Event_Skip;
    }
    
    int modifiers = python_bind_modifiers(event);
    if ( !m_appContext->Bound( keyCode, modifiers ) ){
      // Allow normal key handling for unbound keys
      return Event_Skip;
    }
    wxString commandString;
    commandString.Printf("keypress(%d, %d)", keyCode, modifiers );
    runCmd( std::string(commandString) );
    return Event_Processed;
  }
  return Event_Skip;
}

IMPLEMENT_APP( Application )

AppContext& GetAppContext(){
  return wxGetApp().GetAppContext();
}


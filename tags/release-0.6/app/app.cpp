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
#include <sstream>
#include "app/app.hh"
#include "app/appcontext.hh"
#include "gui/interpreterframe.hh"
#include "gui/helpframe.hh"
#include "gui/mainframe.hh"
#include "util/image.hh"
#include "util/paletteparser.hh"
#include "python/pyinterface.hh"
#include "util/bindkey.hh"
#include "util/convertwx.hh"
#include "util/guiutil.hh"
#include "util/pathutil.hh"
#include "wx/clipbrd.h"
#include "wx/cmdline.h"
#include "wx/dir.h"
#include "wx/filename.h"
class Canvas;

static const wxCmdLineEntryDesc g_cmdLineDesc[] = {
  { wxCMD_LINE_SWITCH, "h", "help", "Displays help on the command line parameters", wxCMD_LINE_VAL_NONE,
    wxCMD_LINE_OPTION_HELP },
  { wxCMD_LINE_SWITCH, "s", "silent", "Disables the GUI. Requires specifying a script with with --run.", wxCMD_LINE_VAL_STRING,
    wxCMD_LINE_PARAM_OPTIONAL },
  { wxCMD_LINE_PARAM, "e", "whatev", "Image files", wxCMD_LINE_VAL_STRING,
    wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE },
  { wxCMD_LINE_SWITCH, "i", "newinst", "Force new instance", wxCMD_LINE_VAL_STRING,
    wxCMD_LINE_PARAM_OPTIONAL },
  { wxCMD_LINE_SWITCH, "ii", "noserver", "Prevent this instance from becoming a main app", wxCMD_LINE_VAL_STRING,
    wxCMD_LINE_PARAM_OPTIONAL },
  { wxCMD_LINE_OPTION, "", "port", "Specify port used for IPC.", wxCMD_LINE_VAL_STRING,
    wxCMD_LINE_PARAM_OPTIONAL},
  { wxCMD_LINE_OPTION, "", "run", "Run a Python script file after loading images", wxCMD_LINE_VAL_STRING,
    wxCMD_LINE_PARAM_OPTIONAL},
  { wxCMD_LINE_NONE, "", "", "", wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL } // Sentinel
};

Application::Application()
  : m_appContext(0),
    m_faintInstance(0),
    m_frame(0),
    m_helpFrame(0),
    m_interpreterFrame(0)
{}

int Application::FilterEvent( wxEvent& untypedEvent ){
  // Filter for key events so that bound keys are relayed to Python
  // regardless of what control has focus.  This prevents controls
  // like tool buttons, the aui-toolbar etc from swallowing keypresses
  // when they have focus.
  if ( untypedEvent.GetEventType() == wxEVT_KEY_DOWN ){
    if ( !m_frame->MainFrameFocused() ){
      // Ignore Python-binds if a window or dialog is shown above the
      // mainframe
      return Event_Skip;
    }
    if ( m_frame->TextEntryActive() ){
      // Python binds would interfere with text entry.
      return Event_Skip;
    }
    if ( m_frame->EntryControlFocused() ){
      // Especially numeric binds would cause interfere with text edit
      // controls
      return Event_Skip;
    }

    // Note: Afaik, FilterEvent does not happen while a menu is open, so no check
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
    faint::run_python_str( std::string(commandString) );
    return Event_Processed;
  }
  return Event_Skip;
}

AppContext& Application::GetAppContext(){
  return *m_appContext;
}

const ArtContainer& Application::GetArtContainer() const{
  return m_art;
}

wxCursor& Application::GetCursor( Cursor::type id ){
  std::map<Cursor::type, wxCursor>::iterator it = m_cursors.find(id);
  assert( it != m_cursors.end() );
  return it->second;
}

std::vector<Format*>& Application::GetFileFormats(){
  return m_frame->GetFileFormats();
}

MainFrame* Application::GetFrame(){
  return m_frame;
}

HelpFrame* Application::GetHelpFrame(){
  return m_helpFrame;
}

InterpreterFrame* Application::GetInterpreterFrame(){
  return m_interpreterFrame;
}

void Application::InitCursors(){
  using faint::cur_from_bmp;
  m_cursors[Cursor::ARROW] = wxCursor(wxCURSOR_ARROW);
  m_cursors[Cursor::PEN] = cur_from_bmp( m_art.Get("cur_pen") , 0, 11);
  m_cursors[Cursor::CROSSHAIR] = cur_from_bmp( m_art.Get("cur_crosshair"), 8, 8);
  m_cursors[Cursor::TEXT_CROSSHAIR] = cur_from_bmp( m_art.Get("cur_text_cross"), 8, 8);
  m_cursors[Cursor::PICKER] = cur_from_bmp(m_art.Get("cur_picker"), 0, 11);
  m_cursors[Cursor::FILL] = cur_from_bmp(m_art.Get("cur_bucket"), 5, 14);
  m_cursors[Cursor::BRUSH] = cur_from_bmp(m_art.Get("cur_brush"), 8, 8);
  m_cursors[Cursor::CARET] = wxCursor(wxCURSOR_IBEAM);
  m_cursors[Cursor::MOVE] = cur_from_bmp(m_art.Get("cur_move"), 8, 8);
  m_cursors[Cursor::MOVE_SELECTION] = cur_from_bmp(m_art.Get("cur_move_sel"), 8, 8);
  m_cursors[Cursor::CLONE] = cur_from_bmp(m_art.Get("cur_clone"), 8, 10);
  m_cursors[Cursor::SQUARE_CROSS] = cur_from_bmp( m_art.Get("cur_square_cross"), 8,8);
  m_cursors[Cursor::ROTATE_RIGHT] = cur_from_bmp( m_art.Get("cur_rotate_right"),6,7);
  m_cursors[Cursor::MOVE_POINT] = cur_from_bmp( m_art.Get("cur_move_point"),8,8);
  m_cursors[Cursor::RESIZE_NW] = wxCursor(wxCURSOR_SIZENESW);
  m_cursors[Cursor::RESIZE_NE] = wxCursor(wxCURSOR_SIZENWSE);
  m_cursors[Cursor::RESIZE_WE] = wxCursor(wxCURSOR_SIZEWE);
  m_cursors[Cursor::RESIZE_NS] = wxCursor(wxCURSOR_SIZENS);
  m_cursors[Cursor::ADD_POINT] = cur_from_bmp( m_art.Get("cur_add_point"), 8,10);
  m_cursors[Cursor::DRAG_FRAME] = cur_from_bmp( m_art.Get("cur_drag_frame") , 0, 0);
  m_cursors[Cursor::DRAG_COPY_FRAME] = cur_from_bmp( m_art.Get("cur_drag_copy_frame") , 0, 0);
  #ifdef __WXMSW__
  // wxCURSOR_BLANK Requires including wx.rc for MSW.
  // Since I'm lazy, create a blank cursor instead.
  wxImage image(1,1);
  image.SetMask(true);
  image.SetMaskColour(0, 0, 0);
  m_cursors[Cursor::BLANK] = wxCursor( image );
  #else
  m_cursors[Cursor::BLANK] = wxCursor(wxCURSOR_BLANK);
  #endif
}

void Application::InitPalettes(){
  wxDir dir(get_palette_dir());
  assert (dir.IsOpened());
  std::string std_dirname( dir.GetName().c_str() );

  wxString filename;
  bool morefiles = dir.GetFirst(&filename);
  wxString suffix = ".txt";
  wxString prefix;
  while ( morefiles ){
    if ( filename.EndsWith(suffix, &prefix ) ){
      std::string str_filename(filename.c_str());
      std::ifstream file((std_dirname + "/" + str_filename).c_str());

      if ( file ){
        m_palettes[std::string(prefix)] = parse( file );
      }
    }
    morefiles = dir.GetNext(&filename);
  }
}

std::string get_cursor_file( const std::string& cursorLabel ){
  return std::string("graphics/cursor_") + cursorLabel + std::string(".png");
}

void Application::LoadGraphics(){
  m_art.SetRoot( get_data_dir() );
  struct FileAndLabel{
    std::string fn;
    std::string label;
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
    {"graphics/alpha_blending.png", "sett_alpha_blend"},
    {"graphics/line_polyline.png", "sett_line_polyline"},
    {"graphics/edit_points.png", "sett_edit_points"},

    {"graphics/fillstyle_border.png", "sett_fill_border"},
    {"graphics/fillstyle_border_and_fill.png", "sett_fill_border_fill"},
    {"graphics/fillstyle_fill.png", "sett_fill_fill"},

    {"graphics/icon_faint16.png", "faint_icon16"},
    {"graphics/icon_faint32.png", "faint_icon32"},
    {"graphics/icon_faint_python32.png", "faint_python_icon32"},
    {"graphics/icon_faint_python16.png", "faint_python_icon16"},
    {"graphics/icon_help16.png", "help_icon16"},
    {"graphics/icon_help32.png", "help_icon32"},
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
    {"graphics/resizedlg_scale.png", "resizedlg_scale"},
    {"graphics/resizedlg_placement.png", "resizedlg_placement"},
    {"graphics/grid_on.png", "grid_on"},
    {"graphics/grid_off.png", "grid_off"},
    {"graphics/add-frame.png", "add_frame"},
    
    // Cursors
    { get_cursor_file("brush"), "cur_brush"},
    { get_cursor_file("bucket"), "cur_bucket"},
    { get_cursor_file("crosshair"), "cur_crosshair"},
    { get_cursor_file("move"), "cur_move"},
    { get_cursor_file("move_selection"), "cur_move_sel"},
    { get_cursor_file("clone"), "cur_clone"},
    { get_cursor_file("pen"), "cur_pen"},
    { get_cursor_file("picker"), "cur_picker"},
    { get_cursor_file("text_cross"), "cur_text_cross"},
    { get_cursor_file("square_cross"), "cur_square_cross"},
    { get_cursor_file("rotate_right"), "cur_rotate_right"},
    { get_cursor_file("move_point"), "cur_move_point"},
    { get_cursor_file("add_point"), "cur_add_point"},
    { get_cursor_file("drag_frame"), "cur_drag_frame"},
    { get_cursor_file("drag_copy_frame"), "cur_drag_copy_frame"}
  };

  unsigned int numFiles = sizeof( files ) / sizeof( FileAndLabel );
  for ( unsigned int i = 0; i!= numFiles; i++ ){
    FileAndLabel& currFile = files[i];
    m_art.Load( currFile.fn, currFile.label );
  }
}

std::string get_string(const wxCmdLineParser& parser, const std::string& name, const std::string& defaultStr=""){
  wxString str;
  parser.Found(name, &str);
  if ( str.size() == 0 ){
    return defaultStr;
  }
  return std::string(str);
}

bool valid_port( const std::string& str ){
  for ( size_t i = 0; i != str.size(); i++ ){
    if ( !isdigit(str[i]) ){
      return false;
    }
  }
  std::stringstream ss(str);
  int i = 0;
  ss >> i;
  return 0 <= i && i <= 65535;
}

bool Application::OnCmdLineParsed(wxCmdLineParser& parser){
  m_cmd.silentMode = parser.Found("s");
  m_cmd.forceNew = parser.Found("i");
  m_cmd.preventServer = parser.Found("ii");
  m_cmd.port = get_string(parser, "port", g_defaultFaintPort);
  m_cmd.scriptName = get_string(parser, "run");

  if ( !valid_port(m_cmd.port) ){
    wxMessageOutput* msgOut = wxMessageOutput::Get();
    assert(msgOut != 0);
    msgOut->Printf( "Error: Invalid port specified." );
    return false;
  }
  if ( m_cmd.silentMode && m_cmd.scriptName.empty() ){
    wxMessageOutput* msgOut = wxMessageOutput::Get();
    assert(msgOut != 0);
    msgOut->Printf( "Error: --silent requires a script specified with --run <scriptname>" ); // Fixme: wxMessageOutput seems to truncate the next prompt in msw
    return false;
  }
  for ( size_t i = 0; i!= parser.GetParamCount(); i++ ){
    wxFileName file( parser.GetParam(i) );
    file.MakeAbsolute();
    m_cmd.files.push_back( std::string(file.GetLongPath()) );
  }

  return true;
}

int Application::OnExit(){
  delete m_appContext;
  delete m_helpFrame;
  delete m_faintInstance;
  return wxApp::OnExit();
}

bool Application::OnInit(){
  // Perform default init for command-line etc.
  if ( !wxApp::OnInit() ){
    return false;
  }

  m_faintInstance = create_faint_instance( m_cmd.files,
    allow_server(!m_cmd.preventServer), force_start(m_cmd.forceNew), m_cmd.port );

  if ( !m_faintInstance->AllowStart() ){
    return false;
  }

  wxInitAllImageHandlers();
  LoadGraphics();
  InitCursors();
  InitPalettes();

  // Create frames and restore their states from the last run
  m_interpreterFrame = new InterpreterFrame();
  faint::restore_persisted_state(m_interpreterFrame, storage_name("InterpreterFrame"));
  m_interpreterFrame->FaintSetIcons(faint::get_icon(m_art, "faint_python_icon16"),
    faint::get_icon(m_art, "faint_python_icon32"));

  m_helpFrame = new HelpFrame(get_help_dir());
  faint::restore_persisted_state(m_helpFrame->GetRawFrame(), storage_name("HelpFrame"));
  m_helpFrame->SetIcons(faint::get_icon(m_art, "help_icon16"),
    faint::get_icon(m_art, "help_icon32"));

  m_frame = new MainFrame(m_art, m_palettes, m_helpFrame, m_interpreterFrame );
  m_frame->SetIcons(faint::bundle_icons(faint::get_icon(m_art, "faint_icon16"), faint::get_icon(m_art, "faint_icon32") ) );
  faint::restore_persisted_state(m_frame, storage_name("MainFrame"));
  m_appContext = m_frame->GetContext();

  faint::init_python();
  if ( !m_cmd.silentMode ){
    m_frame->Show(true);
  }

  if ( !m_cmd.files.empty() ) {
    m_frame->Open( m_cmd.files );
    if ( m_appContext->GetCanvasCount() == 0 ){
      m_frame->NewDocument(m_appContext->GetDefaultImageInfo() );
    }
  }
  else {
    m_frame->NewDocument( m_appContext->GetDefaultImageInfo() );
  }

  faint::run_python_user_ini();

  SetTopWindow( m_frame );
  return true;
}

void Application::OnInitCmdLine(wxCmdLineParser& parser){
  parser.SetDesc (g_cmdLineDesc);
}

int Application::OnRun(){
  if ( m_cmd.scriptName != "" ){
    faint::run_python_file(m_cmd.scriptName);
  }

  if ( m_cmd.silentMode ){
    return 0;
  }
  return wxApp::OnRun();
}

IMPLEMENT_APP( Application )

AppContext& GetAppContext(){
  return wxGetApp().GetAppContext();
}

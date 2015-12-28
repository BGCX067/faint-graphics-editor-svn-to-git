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

#include "wx/clipbrd.h"
#include "wx/cmdline.h"
#include "wx/dir.h"
#include "wx/filename.h"
#include <fstream>
#include <sstream>

#include "app/app.hh"
#include "app/appcontext.hh"
#include "gui/interpreterframe.hh"
#include "gui/helpframe.hh"
#include "gui/mainframe.hh"
#include "python/pyinterface.hh"
#include "util/formatting.hh"
#include "util/image.hh"
#include "util/paletteparser.hh"
#include "util/bindkey.hh"
#include "util/convertwx.hh"
#include "util/guiutil.hh"
#include "util/keycode.hh"
#include "util/pathutil.hh"
#include "generated/load-resources.hh"
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
  : m_appContext(nullptr),
    m_faintInstance(nullptr),
    m_frame(nullptr),
    m_helpFrame(nullptr),
    m_interpreterFrame(nullptr)
{}

int Application::FilterEvent( wxEvent& untypedEvent ){
  // Filter for key events so that bound keys are relayed to Python
  // regardless of what control has focus.  This prevents controls
  // like tool buttons, the aui-toolbar etc from swallowing keypresses
  // when they have focus.
  if ( untypedEvent.GetEventType() != wxEVT_KEY_DOWN ){
    return Event_Skip;
  }
  wxKeyEvent& event = (wxKeyEvent&)untypedEvent;
  int keyCode = event.GetKeyCode();
  int modifiers = python_bind_modifiers(event);

  if ( key::modifier( keyCode ) ){
    // Modifier keys (like Shift) can not be bound separately.
    return Event_Skip;
  }

  const bool boundGlobal = m_appContext->BoundGlobal( keyCode, modifiers );
  const bool bound = m_appContext->Bound( keyCode, modifiers );
  if ( !bound && !boundGlobal ){
    // Allow normal key handling for unbound keys
    return Event_Skip;
  }

  if ( !m_frame->MainFrameFocused() && !boundGlobal){
    // Ignore non-global Python-binds if a window or dialog is shown above the
    // mainframe
    return Event_Skip;
  }

  // Note: It appears that FilterEvent does not happen while a menu is open, so no check
  // to avoid this is performed.

  // Non-global binds must check if text-entry is active
  if ( !boundGlobal ){
    EntryMode entryMode = m_frame->GetTextEntryMode();
    if ( entryMode == EntryMode::ALPHA_NUMERIC && key::affects_alphanumeric_entry(keyCode, modifiers) ){
      return Event_Skip;
    }
    else if ( entryMode == EntryMode::NUMERIC && key::affects_numeric_entry(keyCode) ){
      return Event_Skip;
    }
  }

  // Run the key bind, and swallow the key-press.
  faint::python_keypress(keyCode, modifiers);
  return Event_Processed;
}

AppContext& Application::GetAppContext(){
  return *m_appContext;
}

const ArtContainer& Application::GetArtContainer() const{
  return m_art;
}

const wxCursor& Application::GetCursor( Cursor id ){
  return m_art.Get(id);
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

static void show_palette_load_error( const wxFileName& filename, const faint::PaletteFileError& error ){
  std::stringstream ss;
  ss << "Error in palette file!" << "\n\n" <<
    "File: " << filename.GetFullPath() << "\n" << "Line: " << error.GetLineNum();
  wxMessageDialog dlg(nullptr, ss.str(), "Palette File Error", wxOK|wxICON_ERROR);
  dlg.ShowModal();
}

void Application::InitPalettes(){
  m_palettes["default"] = faint::DrawSourceMap();

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
        try{
          m_palettes[std::string(prefix)] = faint::parse_palette_stream( file );
        }
        catch ( const faint::PaletteFileError& error ){
          show_palette_load_error(wxFileName(std_dirname, filename), error);
        }
      }
    }
    morefiles = dir.GetNext(&filename);
  }
}

static std::string get_string(const wxCmdLineParser& parser, const std::string& name, const std::string& defaultStr=""){
  wxString str;
  parser.Found(name, &str);
  if ( str.empty() ){
    return defaultStr;
  }
  return std::string(str);
}

static bool valid_port( const std::string& str ){
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
    faint::console_message("Error: Invalid port specified " +
      faint::bracketed(m_cmd.port));
    return false;
  }
  if ( m_cmd.silentMode && m_cmd.scriptName.empty() ){
    faint::console_message("Error: --silent requires a script specified with --run <scriptname>");
    return false;
  }

  for ( size_t i = 0; i!= parser.GetParamCount(); i++ ){
    const wxString param(parser.GetParam(i));
    wxFileName absPath( absoluted(wxFileName(param)) );
    if ( absPath.IsDir() ){
      faint::console_message(wxString("Error: Folder path specified on command line - image path expected. (") + param + ")");
      return false;
    }
    m_cmd.files.push_back( faint::FilePath::FromAbsoluteWx(absPath) );
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
  load_faint_resources(m_art);
  InitPalettes();

  // Create frames and restore their states from the last run
  m_interpreterFrame = new InterpreterFrame();
  faint::restore_persisted_state(m_interpreterFrame, storage_name("InterpreterFrame"));
  m_interpreterFrame->FaintSetIcons(faint::get_icon(m_art, Icon::FAINT_PYTHON16),
    faint::get_icon(m_art, Icon::FAINT_PYTHON32));

  m_helpFrame = new HelpFrame(get_help_dir(), m_art);
  faint::restore_persisted_state(m_helpFrame->GetRawFrame(), storage_name("HelpFrame"));
  m_helpFrame->SetIcons(faint::get_icon(m_art, Icon::HELP16),
    faint::get_icon(m_art, Icon::HELP32));

  m_frame = new MainFrame(m_art, m_palettes, m_helpFrame, m_interpreterFrame );
  m_frame->SetIcons(faint::bundle_icons(faint::get_icon(m_art, Icon::FAINT16), faint::get_icon(m_art, Icon::FAINT32) ) );
  faint::restore_persisted_state(m_frame, storage_name("MainFrame"));
  m_appContext = m_frame->GetContext();

  faint::init_python();
  if ( !m_cmd.silentMode ){
    m_frame->Show(true);
  }
  m_frame->FaintInitialize();
  faint::run_python_user_ini();
  m_interpreterFrame->AddNames(faint::list_ifaint_names());
  if ( !m_cmd.files.empty() ){
    m_frame->Open(m_cmd.files);
  }
  SetTopWindow( m_frame );
  return true;
}

void Application::OnInitCmdLine(wxCmdLineParser& parser){
  parser.SetDesc (g_cmdLineDesc);
}

int Application::OnRun(){
  if ( m_cmd.scriptName != "" ){
    if ( !file_exists(m_cmd.scriptName) ){
      if ( m_cmd.silentMode ){
        faint::console_message(space_sep( "Python file specified with --run not found:",
          m_cmd.scriptName));
        return 0; // Exit
      }
      else {
        show_error(null_parent(), Title("Script not found"), endline_sep( "Python file specified with --run not found:",
          m_cmd.scriptName));
      }
    }
    else {
      faint::run_python_file(m_cmd.scriptName);
    }
  }

  if ( m_cmd.silentMode ){
    return 0; // Exit
  }

  return wxApp::OnRun();
}

IMPLEMENT_APP( Application )

AppContext& GetAppContext(){
  return wxGetApp().GetAppContext();
}

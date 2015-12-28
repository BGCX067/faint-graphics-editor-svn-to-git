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

#include <fstream> // Fixme: Remove
#include <sstream>
#include "wx/app.h"
#include "wx/cmdline.h"
#include "wx/dir.h"
#include "wx/filename.h"
#include "wx/msgdlg.h"
#include "app/app.hh"
#include "app/one-instance.hh"
#include "generated/load-resources.hh"
#include "gui/faint-window.hh"
#include "gui/help-frame.hh"
#include "gui/interpreter-frame.hh"
#include "python/py-interface.hh"
#include "python/py-keypress.hh"
#include "text/formatting.hh"
#include "util/convert-wx.hh"
#include "util/file-path-util.hh"
#include "util/keycode.hh"
#include "util/optional.hh"
#include "util/palette-container.hh"
#include "util/parse-palette.hh"
#include "util/settings.hh"

namespace faint{
static utf8_string get_default_faint_port(){
  return utf8_string("3793");
}

struct CommandLine{
  CommandLine() :
    forceNew(false),
    preventServer(false),
    silentMode(false),
    script(false),
    port(get_default_faint_port())
  {}
  bool forceNew; // single instance
  bool preventServer; // single instance
  bool silentMode;
  bool script;
  Optional<FilePath> scriptPath;
  utf8_string port;
  FileList files;
};


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

static void show_palette_load_error( const wxFileName& filename, const PaletteFileError& error ){
  std::stringstream ss;
  ss << "Error in palette file!" << "\n\n" <<
    "File: " << filename.GetFullPath() << "\n" << "Line: " << error.GetLineNum();
  wxMessageDialog dlg(nullptr, ss.str(), "Palette File Error", wxOK|wxICON_ERROR);
  dlg.ShowModal();
}

static utf8_string get_string(const wxCmdLineParser& parser, const std::string& name, const utf8_string& defaultStr=utf8_string("")){
  wxString str;
  parser.Found(name, &str);
  if ( str.empty() ){
    return defaultStr;
  }
  return to_faint(str);
}

static bool valid_port( const std::string& str ){
  for ( char c : str ){
    if ( !isdigit(c) ){
      return false;
    }
  }
  std::stringstream ss(str);
  int i = 0;
  ss >> i;
  return 0 <= i && i <= 65535;
}

class Application : public wxApp{
public:
  Application()
    : m_appContext(nullptr),
      m_faintInstance(nullptr),
      m_frame(nullptr),
      m_helpFrame(nullptr),
      m_interpreterFrame(nullptr)
  {}

  int FilterEvent( wxEvent& untypedEvent ) override{
    // Filter for key events so that bound keys are relayed to Python
    // regardless of what control has focus. This prevents controls like
    // tool buttons, the aui-toolbar etc. from swallowing keypresses
    // when they have focus.

    wxEventType eventType = untypedEvent.GetEventType();
    if ( eventType != wxEVT_KEY_DOWN && eventType != wxEVT_KEY_UP ){
      return Event_Skip;
    }
    const wxKeyEvent& event = (wxKeyEvent&)untypedEvent;
    const int keyCode = event.GetKeyCode();

    if ( key::modifier( keyCode ) ){
      m_frame->ModifierKeyChange();

      // Modifier keys (like Ctrl, Shift) can not be bound separately -
      // stop special handling here.
      return Event_Skip;
    }
    if ( eventType == wxEVT_KEY_UP ){
      // Key up is only relevant for modifier refresh.
      return Event_Skip;
    }

    KeyPress key(key_modifiers(event), Key(keyCode));
    const bool boundGlobal = m_appContext->BoundGlobal(key);
    const bool bound = m_appContext->Bound(key);
    if ( !bound && !boundGlobal ){
      // Allow normal key handling for unbound keys
      return Event_Skip;
    }

    if ( !m_frame->MainFrameFocused() && !boundGlobal){
      // Ignore non-global Python-binds if a window or dialog is shown
      // above the mainframe
      return Event_Skip;
    }

    // Note: It appears that FilterEvent does not happen while a menu is
    // open, so no check to avoid this is performed.

    // Non-global binds must check if text-entry is active
    if ( !boundGlobal ){
      EntryMode entryMode = m_frame->GetTextEntryMode();
      if ( entryMode == EntryMode::ALPHA_NUMERIC && key::affects_alphanumeric_entry(key) ){
        return Event_Skip;
      }
      else if ( entryMode == EntryMode::NUMERIC && key::affects_numeric_entry(key) ){
        return Event_Skip;
      }
    }

    // Run the key bind, and swallow the key-press.
    python_keypress(key);
    return Event_Processed;
  }

  AppContext& GetAppContext() const{
    return *m_appContext;
  }

  const ArtContainer& GetArtContainer() const{
    return m_art;
  }

  bool OnCmdLineParsed(wxCmdLineParser& parser) override{
    m_cmd.silentMode = parser.Found("s");
    m_cmd.forceNew = parser.Found("i");
    m_cmd.preventServer = parser.Found("ii");
    m_cmd.port = get_string(parser, "port", get_default_faint_port());
    utf8_string scriptPath = get_string(parser, "run");
    m_cmd.scriptPath = make_absolute_file_path(scriptPath);

    if ( !valid_port(m_cmd.port.str()) ){
      console_message("Error: Invalid port specified " +
        bracketed(m_cmd.port.str())); // Fixme: reconsider str.
      return false;
    }

    if ( m_cmd.silentMode && m_cmd.scriptPath.NotSet() ){
      console_message("Error: --silent requires a script specified with --run <scriptname>");
      return false;
    }

    for ( size_t i = 0; i!= parser.GetParamCount(); i++ ){
      const wxString param(parser.GetParam(i));
      wxFileName absPath( absoluted(wxFileName(param)) );
      if ( absPath.IsDir() ){
        console_message(wxString("Error: Folder path specified on command line - image path expected. (") + param + ")");
        return false;
      }
      m_cmd.files.push_back( FilePath::FromAbsoluteWx(absPath) );
    }
    return true;
  }

  int OnExit() override{
    delete m_appContext;
    delete m_helpFrame;
    delete m_faintInstance;
    return wxApp::OnExit();
  }

  bool OnInit() override{
    // Perform default init for command-line etc.
    if ( !wxApp::OnInit() ){
      return false;
    }

    m_faintInstance = create_faint_instance( m_cmd.files,
      allow_server(!m_cmd.preventServer), force_start(m_cmd.forceNew), m_cmd.port.str() ); // Fixme: get rid of .str(), should use an integer for port

    if ( !m_faintInstance->AllowStart() ){
      return false;
    }

    wxInitAllImageHandlers();
    load_faint_resources(m_art);
    InitPalettes();

    // Create frames and restore their states from the last run
    m_interpreterFrame = new InterpreterFrame();
    restore_persisted_state(m_interpreterFrame, storage_name("InterpreterFrame"));
    m_interpreterFrame->FaintSetIcons(get_icon(m_art, Icon::FAINT_PYTHON16),
      get_icon(m_art, Icon::FAINT_PYTHON32));

    m_helpFrame = new HelpFrame(get_help_dir(), m_art);
    restore_persisted_state(m_helpFrame->GetRawFrame(), storage_name("HelpFrame"));
    m_helpFrame->SetIcons(get_icon(m_art, Icon::HELP16),
      get_icon(m_art, Icon::HELP32));

    m_frame = new FaintWindow(m_art, m_palettes, m_helpFrame, m_interpreterFrame, m_cmd.silentMode );
    m_frame->SetIcons(bundle_icons(get_icon(m_art, Icon::FAINT16), get_icon(m_art, Icon::FAINT32) ) );
    restore_persisted_state(m_frame, storage_name("MainFrame"));
    m_appContext = &(m_frame->GetContext());

    bool ok = init_python();
    if ( !ok ){
      show_error(null_parent(), Title("Faint Internal Error"), "Faint crashed!\n\n...while running envsetup.py");
      delete m_frame;
      delete m_appContext;
      delete m_faintInstance;
      delete m_helpFrame;
      return false;
    }
    if ( !m_cmd.silentMode ){
      m_frame->Show(true);
    }
    m_frame->FaintInitialize();
    bool configOk = run_python_user_config(*m_appContext);
    if ( !configOk ){
      // Show the console where some error info should have been printed.
      m_appContext->ShowPythonConsole();
      if ( m_cmd.scriptPath.IsSet() ){
        m_appContext->PythonIntFaintPrint(space_sep("Script", quoted(m_cmd.scriptPath.Get().Str()), "ignored due to configuration file error.\n"));
        m_cmd.scriptPath.Clear();
      }
    }
    m_interpreterFrame->AddNames(list_ifaint_names());
    if ( !m_cmd.files.empty() ){
      m_frame->Open(m_cmd.files);
    }
    SetTopWindow( m_frame );
    return true;
  }

  void OnInitCmdLine(wxCmdLineParser& parser) override{
    parser.SetDesc(g_cmdLineDesc);
  }

  int OnRun() override{
    if ( m_cmd.scriptPath.IsSet() ){
      FilePath scriptPath(m_cmd.scriptPath.Get());
      if ( !exists(scriptPath) ){
        if ( m_cmd.silentMode ){
          console_message(to_wx(space_sep(utf8_string(
            "Python file specified with --run not found:"),
            scriptPath.Str())));
          return 0; // Exit
        }
        else {
          show_error(null_parent(), Title("Script not found"),
            to_wx(endline_sep( utf8_string("Python file specified with --run not found:"),
            scriptPath.Str())));
        }
      }
      else {
        // Fixme: Should return the error from run_python_file, since it should be
        // printed to the command line if --silent
        bool scriptOk = run_python_file(scriptPath, *m_appContext);
        if ( !scriptOk ){
          m_appContext->ShowPythonConsole();
        }
      }
    }

    if ( m_cmd.silentMode ){
      return 0; // Exit
    }

    m_appContext->PythonNewPrompt();
    return wxApp::OnRun();
  }

private:
  void InitPalettes(){
    m_palettes["default"] = PaintMap();

    wxDir dir(to_wx(get_palette_dir().Str()));
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
            m_palettes[std::string(prefix)] = parse_palette_stream( file );
          }
          catch ( const PaletteFileError& error ){
            show_palette_load_error(wxFileName(std_dirname, filename), error);
          }
        }
      }
      morefiles = dir.GetNext(&filename);
    }
  }

  AppContext* m_appContext;
  ArtContainer m_art;
  CommandLine m_cmd;
  FaintInstance* m_faintInstance;
  FaintWindow* m_frame;
  HelpFrame* m_helpFrame;
  InterpreterFrame* m_interpreterFrame;
  PaletteContainer m_palettes;
};

}

IMPLEMENT_APP(faint::Application)

namespace faint{

AppContext& get_app_context(){
  return wxGetApp().GetAppContext();
}

const ArtContainer& get_art_container(){
  return wxGetApp().GetArtContainer();
}

}

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

#ifndef FAINT_APP_HH
#define FAINT_APP_HH
#include "wx/wx.h"
#include <map>
#include "app/appcontext.hh"
#include "app/oneinstance.hh"
#include "gui/cursors.hh"
#include "util/palettecontainer.hh"
#include "util/artcontainer.hh"
#include "util/settings.hh"

class InterpreterFrame;
class HelpFrame;
class MainFrame;

namespace faint{
  struct CommandLine{
    CommandLine() :
      forceNew(false),
      preventServer(false),
      silentMode(false),
      script(false),
      port(g_defaultFaintPort)
    {}
    bool forceNew; // single instance
    bool preventServer; // single instance
    bool silentMode;
    bool script;
    std::string scriptName;
    std::string port;
    std::vector<std::string> files;
  };
}

class Application : public wxApp{
public:
  Application();
  virtual int FilterEvent(wxEvent&);
  AppContext& GetAppContext();
  const ArtContainer& GetArtContainer() const;
  wxCursor& GetCursor( Cursor::type );
  std::vector<Format*>& GetFileFormats();
  MainFrame* GetFrame();
  HelpFrame* GetHelpFrame();
  InterpreterFrame* GetInterpreterFrame();
  virtual bool OnCmdLineParsed(wxCmdLineParser&);
  virtual int OnExit();
  virtual bool OnInit();
  virtual void OnInitCmdLine(wxCmdLineParser&);
  virtual int OnRun();
private:
  void InitCursors();
  void InitPalettes();
  void LoadGraphics();
  bool SetupSingleInst();
  AppContext* m_appContext;
  ArtContainer m_art;
  faint::CommandLine m_cmd;
  std::map<Cursor::type, wxCursor> m_cursors;
  FaintInstance* m_faintInstance;
  MainFrame* m_frame;
  HelpFrame* m_helpFrame;
  InterpreterFrame* m_interpreterFrame;
  PaletteContainer m_palettes;

};

DECLARE_APP(Application)
#endif

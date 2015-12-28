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
#include "artcontainer.hh"
#include "mainframe.hh"
#include "oneinstance.hh"
#include "palettecontainer.hh"
#include "settings.hh"
class wxSingleInstanceChecker;

class InterpreterFrame;

class Application : public wxApp{
public:
  Application();

  virtual bool OnInit();
  virtual int OnRun();
  virtual int OnExit();

  void OnInitCmdLine(wxCmdLineParser& parser);
  bool OnCmdLineParsed(wxCmdLineParser& parser);
  wxCursor& GetCursor( unsigned int id );

  MainFrame* GetFrame();
  InterpreterFrame* GetInterpreterFrame();

  AppContext& GetAppContext();

  void PushFile( const wxString& filename );

  std::vector<Format*>& GetFileFormats();
  virtual int FilterEvent(wxEvent&);

private:
  bool SetupSingleInst();
  void LoadGraphics();
  void InitCursors();
  void InitPalettes();

  MainFrame* m_frame;
  InterpreterFrame* m_interpreterFrame;
  ArtContainer m_artContainer;
  PaletteContainer m_palettes;
  std::map<unsigned int, wxCursor> m_cursors;
  bool m_cmd_silentMode;
  wxArrayString m_cmdLineFiles;
  bool m_cmd_script;
  wxString m_scriptName;

  // Single instance
  bool m_cmd_preventServer;
  bool m_cmd_forceNew;
  wxSingleInstanceChecker* m_singleInstanceChk;
  FaintServer* m_faintServer;

  AppContext* m_appContext;
};

DECLARE_APP(Application)

#endif

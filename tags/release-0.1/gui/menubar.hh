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

#ifndef FAINT_MENUBAR
#define FAINT_MENUBAR
#include "menupredicate.hh"
#include <vector>

class ZoomLevel;

// The menubar at the top of the main frame.
// Fixme: Add info about what magic I handle
class Menubar{
public:
  Menubar();
  ~Menubar();
  wxMenuBar* GetRawMenubar();
  wxMenu* GetRecentFilesMenu();
  wxEvtHandler* GetEventHandler();

  // Enable/disable menu items depending on parameters
  void Update( bool rasterSelection, bool objectSelection, bool hasObjects );

  // Enable/disable zoom menu items depending on zoom state
  void UpdateZoom( const ZoomLevel& );

  // Disable shortcuts that might interfere with text entry
  void BeginTextEntry( bool numericOnly=false );

  // Re-enable shortcuts after text entry
  void EndTextEntry();
private:
  enum ToggleShortcut{
    TOGGLE_NEVER, // Never disable shortcut
    TOGGLE_ALPHA, // Disable shortcut during alphabetic entry, but keep it for numeric entry
    TOGGLE_ALL // Disable shortcut for any entry field
  };

  class Label{
  public:
    Label( const char*, ToggleShortcut=TOGGLE_NEVER );
    Label( const char*, const char*, ToggleShortcut=TOGGLE_NEVER );
    std::string GetLabelText() const;
    std::string GetHelpText() const;
    ToggleShortcut GetToggle() const;
  private:
    std::string m_label;
    std::string m_help;
    ToggleShortcut m_toggle;
  };

  class ToggleLabel{
  public:
    ToggleLabel( int id, const std::string&, bool=false );
    void EnableShortcut( wxMenuBar* ) const;
    void DisableShortcut( wxMenuBar*, bool numeric ) const;
  private:
    int m_id;
    std::string m_label;
    std::string m_shortcut;
    bool m_disableNumeric;
  };

  void Initialize();
  void Add( wxMenu*, int );
  void Add( wxMenu*, int,  const Label& );
  void Add( wxMenu*, int, const Label&, const faint::MenuPred& );
  void Add( wxMenu*, int, const faint::MenuPred& );
  void AddShortcutDisable( int, const Label& );
  void AddCheck( wxMenu*, int, const Label& );
  wxMenuBar* m_wxMenubar;
  wxMenu* m_recentFilesMenu;
  wxEvtHandler* m_eventHandler;
  std::vector<ToggleLabel> m_notWhileTexting;
  std::vector<faint::BoundMenuPred> m_menuPredicates;
};

#endif

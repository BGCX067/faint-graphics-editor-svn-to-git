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
#include <vector>
#include "gui/menupredicate.hh"
#include "util/path.hh"
class ZoomLevel;
class MainFrame;

class RecentFiles {
public:
  virtual ~RecentFiles();
  virtual void Add(const faint::FilePath&) = 0;
  virtual void Clear() = 0;
  virtual faint::FilePath Get(size_t) const = 0;
  virtual void Save() = 0;
  virtual size_t Size() const = 0;
  virtual void UndoClear() = 0;
};

namespace faint {
  class MenuRefWX;
}

enum class EntryMode{NONE, ALPHA_NUMERIC, NUMERIC};

// The menubar at the top of the main frame. Handles creation,
// enabling/disabling and so on.
// Also creates an event handler which must be pushed as a window
// event handler to receive menu events.
class Menubar{
public:
  Menubar(MainFrame&);
  ~Menubar();

  // Disable shortcuts that might interfere with text entry
  void BeginTextEntry( bool numericOnly=false );

  // Re-enable shortcuts after text entry
  void EndTextEntry();
  RecentFiles& GetRecentFiles();
  EntryMode GetTextEntryMode() const;

  // Enable/disable menu items depending on parameters
  void Update( const faint::MenuFlags& );

  // Enable/disable zoom menu items depending on zoom state
  void UpdateZoom( const ZoomLevel& );

private:
  enum ToggleShortcut{
    TOGGLE_NEVER, // Never disable this shortcut
    TOGGLE_ALPHABETIC, // Disable this shortcut during alphabetic entry, but keep it for numeric entry
    TOGGLE_ALL // Disable this shortcut when any entry field has focus
  };

  class Label{
  public:
    Label( const std::string&, ToggleShortcut=TOGGLE_NEVER );
    Label( const std::string&, const std::string&, ToggleShortcut=TOGGLE_NEVER );
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

  void Initialize(MainFrame&);
  void Add( wxMenu*, int,  const Label&, const wxObjectEventFunction&);
  void Add( wxMenu*, int, const Label&, const faint::MenuPred&, const wxObjectEventFunction& );
  void Add( wxMenu*, int, const faint::MenuPred&, const wxObjectEventFunction& );
  void AddShortcutDisable(int,const Label&);
  void AddCheck( wxMenu*, int, const Label&, const wxObjectEventFunction&, bool checked );
  int NewId();
  int m_maxId;
  faint::MenuRefWX* m_menuRef;
  wxMenu* m_recentFilesMenu;
  wxEvtHandler* m_eventHandler;
  std::vector<ToggleLabel> m_notWhileTexting;
  std::vector<faint::BoundMenuPred> m_menuPredicates;
  RecentFiles* m_recentFiles;
  EntryMode m_entryMode;
};

#endif

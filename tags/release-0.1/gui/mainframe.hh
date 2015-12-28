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

#ifndef FAINT_MAINFRAME_HH
#define FAINT_MAINFRAME_HH
#include <vector>
#include <map>
#include "artcontainer.hh"
#include "palettecontainer.hh"
#include "toolbar.hh"
#include "tools/toolbehavior.hh"
#include "settings.hh"
#include "gui/canvasscroller.hh" // Fixme: Necessary due to template impl:s in MainFrame
#include "gui/resizedlg/resizedialogsettings.hh"
#include "toolsettingpanel.hh"
#include "canvasinterface.hh"
#include "gui/helpwindow.hh"
#include "menubar.hh"

class wxFileHistory;
class wxConfig;
class TabControl;
class Format;
class InterpreterFrame;
class FrameNotifier;
class AppContext;
class ToolPanel;
class ColorPanel;
class CanvasScroller;

struct FrameSettings {
  FrameSettings(){
    toolbar_visible = palette_visible = statusbar_visible = true;
  }
  bool toolbar_visible;
  bool palette_visible;
  bool statusbar_visible;
  ResizeDialogSettings default_resize_settings;
};

class MainFrame : public wxFrame {
public:
  MainFrame( const wxPoint&, const wxSize&,
      ArtContainer&, PaletteContainer&, InterpreterFrame* );
  ~MainFrame();

  bool MainFrameFocused() const;
  bool EntryControlFocused() const;
  // Tab navigation
  void NextTab();
  void PreviousTab();
  void CloseActiveTab();
  
  // Windows
  void ShowHelp();
  void PasteNew();
  void ToggleMaximize();
  void ShowAboutDialog();
  void ShowOpenFileDialog();
  void ShowResizeDialog();
  void ShowRotateDialog();
  bool ShowSaveAsDialog(CanvasInterface&);
  void ClearRecentFiles();
  void OpenAllRecent();
  void OpenRecent( int index ); // Fixme: Move out of mainframe

  // Document handling
  CanvasScroller* NewDocument( const CanvasInfo& );
  bool Save( CanvasInterface& );
  bool Save( CanvasInterface&, const wxString& filename);

  void SelectTool( ToolId );
  ToolId GetToolId() const;
  void Open( wxArrayString& filePaths );
  CanvasInterface* Open( const wxString& filePath, bool changeTab );

  void OnDocumentChange();
  void OnZoomChange();
  void OnAddColor( wxCommandEvent& );
  void OnClose( wxCloseEvent& );
  void OnCopyColorHex( wxCommandEvent& );
  void OnCopyColorRGB( wxCommandEvent& );
  void OnLayerTypeChange( wxCommandEvent& );
  void OnSwapColors( wxCommandEvent& );
  void OnStateChange();
  void OnToolChange( wxCommandEvent& );

  void Quit();
  void SelectAll();
  void SelectNone();
  void BeginTextEntry();
  void EndTextEntry();
  bool TextEntryActive() const;
  ToolBehavior* GetActiveTool();
  void SetActiveTool(ToolBehavior*);
  Layer GetLayerType() const;
  void AddFormat( Format* );
  std::vector<Format*>& GetFileFormats();

  template<typename T >
  const typename T::ValueType& GetSetting( const T& setting ){
    return m_toolSettings.Get( setting );
  }

  CanvasScroller* GetActiveCanvas();
  const FaintSettings& GetToolSettings() const;
  void AddColor( const faint::Color& );
  void ToggleToolPanel( bool );
  void ToggleColorbar( bool );
  void ToggleStatusbar( bool );
  void UpdateShownSettings();
  void UpdateToolSettings( const FaintSettings& );
  template <typename T>
  void Set( const T& setting, typename T::ValueType value ){
    if ( m_activeTool->GetSettings().Has( setting ) ){
      ToolRefresh toolResult = m_activeTool->Set( setting, value );
      if ( toolResult == TOOL_COMMIT ){
        GetActiveCanvas()->CommitTool( m_activeTool );
      }
      else if ( toolResult == TOOL_OVERLAY ){
        GetActiveCanvas()->ExternalRefresh();
      }
      if ( m_activeTool->EatsSettings() ){
        UpdateShownSettings();
        return;
      }
    }
    m_toolSettings.Set( setting, value );
    UpdateShownSettings();
  }
  AppContext* GetContext();
  void CloseDocument( CanvasInterface& );
  CanvasInfo GetDefaultCanvasInfo() const;
  size_t GetCanvasCount() const;
  CanvasInterface& GetCanvas( size_t );
  void SelectLayer( Layer );
  bool Exists( const CanvasId& ) const;
  void FaintFullScreen( bool enable );
private:
  int FaintShowModal( wxDialog& );
  void DoSelectTool( ToolId );
  void CreatePanels();
  wxStatusBar* CreateFaintStatusBar();
  Menubar* CreateFaintMenubar();
  void InitIcons();
  void MenuDirty( bool dirty );
  void MenuSelection( bool rasterSelection, bool objectSelection, bool hasObjects );
  void MenuUndoRedo( bool undo, bool redo );
  void RefreshActiveCanvas();
  void OnSetFocusEntry(wxCommandEvent&);
  void OnKillFocusEntry(wxCommandEvent&);

  // Resources
  ArtContainer& m_artContainer;

  // Contexts/message relaying
  AppContext* m_appContext;
  FrameNotifier* m_notifier;

  // Windows/Panels
  Menubar* m_menubar;
  InterpreterFrame* m_pythonFrame;
  TabControl* m_tabControl;
  ToolPanel* m_toolPanel;
  ColorPanel* m_colorPanel;
  HelpWindow m_helpWindow;

  // Settings
  Layer m_layerType;
  ToolBehavior* m_activeTool;
  FaintSettings m_toolSettings;
  FrameSettings m_frameSettings;
  PaletteContainer& m_palettes;

  // File formats/Recent files
  std::vector<Format*> m_formats;
  wxFileHistory* m_fileHistory;
  wxConfig* m_config;

  // Silly book-keeping
  bool m_dialogShown;
  int m_textEntryCount;
  DECLARE_EVENT_TABLE()
};

#endif

// Copyright 2012 Lukas Kemmer
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
#include "app/appcontext.hh" // For change_tab
#include "gui/helpframe.hh"
#include "gui/toolbar.hh"
#include "util/palettecontainer.hh"
#include "util/settings.hh"
#include "util/canvasinterface.hh"
#include "gui/menubar.hh"

class ArtContainer;
class wxFileName;
class wxConfig;
class TabControl;
class Format;
class InterpreterFrame;
class FrameNotifier;
class ToolPanel;
class ColorPanel;
class CanvasScroller;
class ImageInfo;
class OpenFilesEvent;
class ColorEvent;
class ToolChangeEvent;
class LayerChangeEvent;
class FrameContext;
struct FrameSettings {
  FrameSettings(){
    toolbar_visible = palette_visible = statusbar_visible = true;
  }
  bool toolbar_visible;
  bool palette_visible;
  bool statusbar_visible;
};

DECLARE_EVENT_TYPE(EVT_EXIT_FAINT, -1)

class MainFrame : public wxFrame {
public:
  MainFrame( ArtContainer&, PaletteContainer&, HelpFrame*, InterpreterFrame* );
  ~MainFrame();
  void AddColor( const faint::Color& );
  void AddFormat( Format* );
  void BeginTextEntry();
  void CloseActiveTab();
  void CloseDocument( CanvasInterface& );
  void EndTextEntry();
  bool EntryControlFocused() const;
  bool Exists( const CanvasId& ) const;
  void FaintFullScreen( bool enable );
  CanvasScroller* GetActiveCanvas();
  Tool* GetActiveTool();
  CanvasInterface& GetCanvas( size_t );
  size_t GetCanvasCount() const;
  AppContext* GetContext();
  std::vector<Format*>& GetFileFormats();
  Layer::type GetLayerType() const;

  // Returns the currently shown settings, including overrides
  // by tools (especially the ObjSelectTool).
  Settings GetShownSettings() const;
  TabControl* GetTabControl(){
    return m_tabControl;
  }

  EntryMode::Type GetTextEntryMode() const;
  ToolId GetToolId() const;

  // Returns the applications configured settings, ignoring overrides by tools
  Settings& GetToolSettings();
  bool MainFrameFocused() const;
  CanvasScroller* NewDocument( const ImageInfo& );
  void NextTab();
  void OnActiveCanvasChanged();
  void OnDocumentStateChange(const CanvasId&);
  void OnZoomChange();
  void Open( const std::vector<std::string>& );
  CanvasInterface* Open( const wxFileName&, const change_tab& );
  void PasteNew();
  void PreviousTab();
  bool Save( CanvasInterface& );
  bool Save( CanvasInterface&, const wxString& filename);
  void SelectLayer( Layer::type );
  void SelectTool( ToolId );
  void SetActiveCanvas( const CanvasId& );
  void SetActiveTool(Tool*);
  void SetPalette( const std::vector<faint::Color>& );
  void ShowAboutDialog();
  void ShowOpenFileDialog();
  bool ShowSaveAsDialog(CanvasInterface&);
  void ToggleColorbar( bool );
  void ToggleMaximize();
  void ToggleStatusbar( bool );
  void ToggleToolPanel( bool );
  void UpdateShownSettings();
  void UpdateToolSettings( const Settings& );
private:
  void CreatePanels();
  void DoSelectTool( ToolId );
  void OnAddColor(ColorEvent&);
  void OnOpenFiles(OpenFilesEvent&);
  void OnClose( wxCloseEvent& );
  void OnCopyColorHex( ColorEvent& );
  void OnCopyColorRGB( ColorEvent& );
  void OnKillFocusEntry(wxCommandEvent&);
  void OnLayerTypeChange( LayerChangeEvent& );
  void OnSetFocusEntry(wxCommandEvent&);
  void OnSwapColors( wxCommandEvent& );
  void OnToolChange( ToolChangeEvent& );

  // Resources
  ArtContainer& m_artContainer;

  // Contexts/message relaying
  FrameContext* m_appContext;
  FrameNotifier* m_notifier;
  FrameNotifier* m_updatingNotifier;

  // Windows/Panels
  Menubar* m_menubar;
  TabControl* m_tabControl;
  ToolPanel* m_toolPanel;
  ColorPanel* m_colorPanel;

  // Settings
  Layer::type m_layerType;
  Tool* m_activeTool;
  Settings m_toolSettings;
  FrameSettings m_frameSettings;
  PaletteContainer& m_palettes;
  std::vector<Format*> m_formats;

  // Silly book-keeping
  int m_textEntryCount;
  DECLARE_EVENT_TABLE()
};

#endif

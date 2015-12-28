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

#ifndef FAINT_WINDOW_HH
#define FAINT_WINDOW_HH
#include <map>
#include <memory>
#include <vector>
#include "wx/frame.h"
#include "app/app-context.hh" // For change_tab
#include "gui/color-panel.hh"
#include "gui/help-frame.hh"
#include "gui/menu-bar.hh"
#include "gui/tab-ctrl.hh"
#include "gui/tool-bar.hh"
#include "gui/tool-panel.hh"
#include "util/canvas.hh"
#include "util/cleaner.hh"
#include "util/common-fwd.hh"
#include "util/palette-container.hh"
#include "util/settings.hh"

namespace faint{
class ArtContainer;
class CanvasPanel;
class ColorPanel;
class FrameContext;
class InterpreterFrame;
class ToolPanel;

struct FrameSettings {
  FrameSettings(){
    toolbar_visible = palette_visible = statusbar_visible = true;
  }
  bool toolbar_visible;
  bool palette_visible;
  bool statusbar_visible;
};

class FaintWindow : public wxFrame {
  // The Faint application window.
public:
  FaintWindow(ArtContainer&, PaletteContainer&, HelpFrame*, InterpreterFrame*, bool silentMode);
  void AddToPalette(const Paint&);
  void AddFormat(Format*);
  void BeginTextEntry();
  void CloseActiveTab();
  void CloseDocument(Canvas&);
  void EndTextEntry();
  bool Exists(const CanvasId&) const;
  void FaintFullScreen(bool enable);
  void FaintInitialize();
  Canvas& GetActiveCanvas();
  Tool* GetActiveTool();
  Canvas& GetCanvas(int);
  int GetCanvasCount() const;
  AppContext& GetContext();
  std::vector<Format*>& GetFileFormats();
  Layer GetLayerType() const;

  // Returns the currently shown settings, including overrides by
  // tools (especially the ObjSelectTool).
  Settings GetShownSettings() const;
  TabCtrl& GetTabControl();
  EntryMode GetTextEntryMode() const;
  ToolId GetToolId() const;

  // Returns the applications configured settings, ignoring overrides
  // by tools
  Settings& GetToolSettings();

  bool MainFrameFocused() const;

  // Tells the FaintWindow that a modifier key was pressed or
  // released, for cursor refresh purposes.
  void ModifierKeyChange();

  Canvas& NewDocument(const ImageInfo&);
  void NextTab();
  void Open(const FileList&);
  Canvas* Open(const FilePath&, const change_tab&);
  void PasteNew();
  void PreviousTab();
  bool Save(Canvas&);
  bool Save(Canvas&,const FilePath&, int);
  void SelectLayer(Layer);
  void SelectTool(ToolId);
  void SetActiveCanvas(const CanvasId&);
  void SetPalette(const PaintMap&);
  bool ShowSaveAsDialog(Canvas&);
  void ToggleColorbar(bool);
  void ToggleMaximize();
  void ToggleStatusbar(bool);
  void ToggleToolPanel(bool);
  void UpdateShownSettings();
  void UpdateToolSettings(const Settings&);
#ifdef __WXMSW__
  WXLRESULT MSWWindowProc(WXUINT, WXWPARAM, WXLPARAM) override;
#endif
private:
  void CreatePanels();
  void DoSelectTool(ToolId);
  CanvasPanel* GetActiveCanvasPanel();
  void UpdateCanvasState(const CanvasId&);
  void UpdateMenu();
  void UpdateZoom();

  // Resources
  ArtContainer& m_artContainer;

  // The application context
  FrameContext* m_appContext = nullptr;

  // Windows/Panels
  std::unique_ptr<Menubar> m_menubar;
  std::unique_ptr<TabCtrl> m_tabControl;
  std::unique_ptr<ToolPanel> m_toolPanel;
  std::unique_ptr<ColorPanel> m_colorPanel;
  std::vector<std::unique_ptr<Cleaner>> m_cleanup;

  // Settings
  Layer m_layerType = Layer::RASTER;
  Tool* m_activeTool = nullptr;
  Settings m_toolSettings;
  FrameSettings m_frameSettings;
  PaletteContainer& m_palettes;
  std::vector<Format*> m_formats;

  // Silly book-keeping
  int m_textEntryCount = 0;
  bool m_silentMode = false;
};

} // namespace

#endif

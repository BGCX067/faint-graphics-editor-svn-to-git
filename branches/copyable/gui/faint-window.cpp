// -*- coding: us-ascii-unix -*-
// Copyright 2009 Lukas Kemmer
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

#include <algorithm>
#include <string>
#include <sstream>
#include "wx/filename.h"
#include "wx/filedlg.h"
#include "app/app-context.hh"
#include "gui/canvas-panel.hh"
#include "gui/color-panel.hh"
#include "gui/events.hh"
#include "gui/faint-window.hh"
#include "gui/freezer.hh"
#include "gui/interpreter-frame.hh"
#include "gui/resize-dialog-settings.hh"
#include "gui/tool-bar.hh"
#include "gui/tool-panel.hh"
#include "gui/tool-setting-panel.hh"
#include "gui/setting-events.hh"
#include "tablet/tablet-event.hh"
#include "text/formatting.hh"
#include "util/active-canvas.hh"
#include "util/art-container.hh"
#include "util/clipboard.hh"
#include "util/convenience.hh"
#include "util/convert-wx.hh"
#include "util/file-format-util.hh"
#include "util/file-path-util.hh"
#include "util/gui-util.hh"
#include "util/image-props.hh"
#include "util/image.hh"
#include "util/mouse.hh"
#include "util/save.hh"
#include "util/setting-util.hh"
#include "util/tool-util.hh"
#include "util/visit-selection.hh"
#ifdef __WXMSW__
#include "wx/msw/private.h"
#include "tablet/msw/tablet-error-message.hh"
#include "tablet/msw/tablet-init.hh"
#endif

namespace faint{

class SBInterface : public StatusInterface {
public:
  SBInterface(wxStatusBar& statusbar)
    : m_statusbar(statusbar)
  {}

  void SetMainText(const utf8_string& text) override{
    m_statusbar.SetStatusText(to_wx(text), 0);
  }

  void SetText(const utf8_string& text, int field=0) override{
    assert(field < m_statusbar.GetFieldsCount());
    m_statusbar.SetStatusText(to_wx(text), field + 1);
  }

  void Clear() override{
    for (int i = 0; i != m_statusbar.GetFieldsCount(); i++){
      m_statusbar.SetStatusText("", i);
    }
  }
private:
  wxStatusBar& m_statusbar;
};

typedef LessDistinct<bool, 0> from_control;

template<typename T>
void change_setting(FaintWindow& frame, const T& setting, typename T::ValueType value, const from_control& fromControl){
  Tool* tool = frame.GetActiveTool();
  if (tool->GetSettings().Has(setting)){
    bool toolModified = tool->Set({setting, value});
    if (toolModified) {
      frame.GetActiveCanvas().Refresh();
    }
    if (tool->EatsSettings()){
      if (!fromControl.Get()){
        frame.UpdateShownSettings();
      }
      return;
    }
  }
  frame.GetToolSettings().Set(setting, value);
  if (!fromControl.Get()){
    frame.UpdateShownSettings();
  }
}

static void change_settings(FaintWindow& frame, const Settings& newSettings){
  Tool* tool = frame.GetActiveTool();
  bool toolModified = tool->UpdateSettings(newSettings);
  if (toolModified){
    frame.GetActiveCanvas().Refresh();
    if (tool->EatsSettings()){
      frame.UpdateShownSettings();
      return;
    }
  }

  frame.GetToolSettings().Update(newSettings);
  frame.UpdateShownSettings();
}

class DialogFeedbackImpl : public DialogFeedback{
public:
  DialogFeedbackImpl(Canvas& canvas)
    : m_canvas(canvas)
  {}

  Bitmap& GetBitmap() override{
    if (m_bitmap == nullptr){
      Initialize();
    }
    return *m_bitmap;
  }

  bool HasSelection() const override {
    const Image& active = m_canvas.GetImage();
    const RasterSelection& selection = active.GetRasterSelection();
    return !selection.Empty();
  }

  void Update() override{
    if (m_rasterSelection != nullptr && m_rasterSelection->Floating()){
      m_rasterSelection->SetFloatingBitmap(*m_bitmap, m_rasterSelection->TopLeft());
      m_canvas.SetMirage(m_rasterSelection);
    }
    else{
      m_canvas.SetMirage(m_bitmap);
    }
  }

private:
  void Initialize(){
    const Image& active = m_canvas.GetImage();
    const RasterSelection& selection = active.GetRasterSelection();
    sel::visit(selection,
      [&](const sel::Empty&){
        // Fixme: Handle possible OOM ..or I dunno, use some fake bitmap
        active.GetBg().Visit(
          [&](const Bitmap& bmp){
            m_bitmap = std::make_shared<Copyable<Bitmap>>(bmp);
          },
          [&](const ColorSpan& span){
            m_bitmap = std::make_shared<Copyable<Bitmap>>(span.size,
              span.color);
          });
      },
      [&](const sel::Rectangle& r){
        // Create a floating selection chimera from the non-floating
        // selection
        active.GetBg().Visit(
          [&](const Bitmap& bmp){
            m_bitmap = std::make_shared<Copyable<Bitmap>>(make_copyable(subbitmap(bmp,
               r.Rect())));
            m_rasterSelection = std::make_shared<RasterSelection>();
            m_rasterSelection->SetState(SelectionState(*m_bitmap,
              selection.TopLeft()));
          },
          [&](const ColorSpan& span){
            m_bitmap = std::make_shared<Copyable<Bitmap>>(
              r.Rect().GetSize(), span.color);
            m_rasterSelection = std::make_shared<RasterSelection>();
            m_rasterSelection->SetState(SelectionState(*m_bitmap,
              r.TopLeft()));
          });
      },
      [&](const sel::Floating& f){
        m_rasterSelection = std::make_shared<RasterSelection>(selection);
        m_bitmap = std::make_shared<Copyable<Bitmap>>(f.GetBitmap());
      });
  }

  std::shared_ptr<Copyable<Bitmap>> m_bitmap;
  Canvas& m_canvas;
  std::shared_ptr<RasterSelection> m_rasterSelection;
};

static Optional<DirPath> get_canvas_dir(const Canvas& canvas){
  Optional<FilePath> oldFileName(canvas.GetFilePath());
  return oldFileName.IsSet() ?
    option(oldFileName.Get().StripFileName()) :
    no_option();
}

class FrameContext : public AppContext {
public:
  FrameContext(FaintWindow& frame, wxStatusBar& statusbar, HelpFrame& helpFrame, InterpreterFrame& interpreterFrame)
    : m_frame(frame),
      m_helpFrame(helpFrame),
      m_interpreterFrame(interpreterFrame),
      m_statusbar(statusbar),
      m_modalDialog(false),
      m_tabletCursor(TABLET_CURSOR_PUCK)
  {}

  void AddFormat(Format* fileFormat) override{
    m_frame.AddFormat(fileFormat);
  }

  void AddPaletteColor(const Color& c) override{
    m_frame.AddToPalette(Paint(c));
  }

  void BeginModalDialog() override{
    m_modalDialog = true;
  }

  void BeginTextEntry() override{
    m_frame.BeginTextEntry();
  }

  void Bind(const KeyPress& key, const bind_global& bindGlobal) override{
    if (bindGlobal.Get()){
      m_globalBinds.insert(key);
      m_binds.erase(key);
    }
    else{
      m_binds.insert(key);
      m_globalBinds.erase(key);
    }
  }

  bool Bound(const KeyPress& key) const override{
    return m_binds.find(key) != m_binds.end();
  }

  bool BoundGlobal(const KeyPress& key) const override{
    return m_globalBinds.find(key) != m_globalBinds.end();
  }

  void Close(Canvas& canvas) override{
    m_frame.CloseDocument(canvas);
  }

  void DialogOpenFile() override{
    FileList paths = show_open_file_dialog(m_frame,
      Title("Open Image(s)"),
      get_canvas_dir(GetActiveCanvas()),
      to_wx(combined_file_dialog_filter(utf8_string("Image files"),
        loading_file_formats(m_frame.GetFileFormats()))));
    if (!paths.empty()){
      Load(paths);
    }
  }

  void DialogSaveAs(Canvas& canvas) override{
    m_frame.ShowSaveAsDialog(canvas);
  }

  void EndModalDialog() override{
    m_modalDialog = false;
  }

  void EndTextEntry() override{
    m_frame.EndTextEntry();
  }

  bool Exists(const CanvasId& id) override{
    return m_frame.Exists(id);
  }

  BoolSetting::ValueType Get(const BoolSetting& s) override{
    return m_frame.GetShownSettings().Get(s);
  }

  StrSetting::ValueType Get(const StrSetting& s) override{
    return m_frame.GetShownSettings().Get(s);
  }

  IntSetting::ValueType Get(const IntSetting& s) override{
    return m_frame.GetShownSettings().Get(s);
  }

  ColorSetting::ValueType Get(const ColorSetting& s) override{
    return m_frame.GetShownSettings().Get(s);
  }

  FloatSetting::ValueType Get(const FloatSetting& s) override{
    return m_frame.GetShownSettings().Get(s);
  }

  Canvas& GetActiveCanvas() override{
    return m_frame.GetActiveCanvas();
  }

  Tool* GetActiveTool() override{
    return m_frame.GetActiveTool();
  }

  Canvas& GetCanvas(int i) override{
    return m_frame.GetCanvas(i);
  };

  int GetCanvasCount() const override{
    return m_frame.GetCanvasCount();
  }

  Grid GetDefaultGrid() const override{
    return m_defaultGrid;
  }

  ImageInfo GetDefaultImageInfo() override{
    return ImageInfo(IntSize(640,480), Color(255,255,255), create_bitmap(true));
  }

  std::vector<Format*> GetFormats() override{
    return m_frame.GetFileFormats();
  }

  ResizeDialogSettings GetDefaultResizeDialogSettings() const override{
    return m_defaultResizeSettings;
  }

  Layer GetLayerType() const override{
    return m_frame.GetLayerType();
  }

  IntPoint GetMousePos() override{
    return to_faint(wxGetMousePosition());
  }

  StatusInterface& GetStatusInfo(){ // Non virtual
    return m_statusbar;
  }

  ToolId GetToolId() override{
    return m_frame.GetToolId();
  }

  Settings GetToolSettings() const override{
    return m_frame.GetShownSettings();
  }

  const TransparencyStyle& GetTransparencyStyle() const override{
    return m_transparencyStyle;
  }

  bool IsFullScreen() const override{
    return m_frame.IsFullScreen();
  }

  Canvas* Load(const FilePath& filePath, const change_tab& changeTab) override{
    return m_frame.Open(filePath, changeTab );
  }

  void Load(const FileList& filePaths) override{
    return m_frame.Open(filePaths);
  }

  Canvas* LoadAsFrames(const FileList& paths, const change_tab& changeTab) override{
    std::vector<ImageProps> props;
    const std::vector<Format*> formats = loading_file_formats(m_frame.GetFileFormats());
    for (const FilePath& filePath : paths){
      FileExtension extension(filePath.Extension());
      bool loaded = false;
      for (Format* format : formats){
        if (format->Match(extension)){
          props.emplace_back();
          format->Load(filePath, props.back());
          if (!props.back().IsOk()){
            // Fixme: Commented for some reason
            // show_load_failed_error(m_frame, filePath, props.back().GetError());
            return nullptr;
          }
          loaded = true;
          break;
        }
      }
      if (!loaded){
        // show_load_failed_error(m_frame, filePath, "One path could not be loaded.");
        return nullptr;
      }
    }
    auto canvas = m_frame.GetTabControl().NewDocument(std::move(props),
      changeTab, initially_dirty(true));
    Canvas* canvasInterface = &(canvas->GetInterface());
    return canvasInterface;
  }

  void Maximize() override{
    m_frame.Maximize(!m_frame.IsMaximized());
  }

  void MaximizeInterpreter() override{
    m_interpreterFrame.Maximize(!m_interpreterFrame.IsMaximized());
  }

  bool ModalDialogShown() const override{
    return m_modalDialog;
  }

  Canvas& NewDocument(const ImageInfo& info) override{
    return m_frame.NewDocument(info);
  }

  Canvas& NewDocument(ImageProps&& props) override{
    TabCtrl& tabCtrl = m_frame.GetTabControl();
    CanvasPanel* canvas = tabCtrl.NewDocument(std::move(props),
      change_tab(true), initially_dirty(true));
    return canvas->GetInterface();
  }

  void NextTab() override{
    m_frame.NextTab();
  }

  void PreviousTab() override{
    m_frame.PreviousTab();
  }

  void PythonContinuation() override{
    m_interpreterFrame.NewContinuation();
  }

  void PythonDone() override{
    for (Canvas* canvas : m_unrefreshed){
      if (Exists(m_canvasIds[canvas])){
        canvas->Refresh();
      }
    }
    m_unrefreshed.clear();

    for (Canvas* canvas : m_commandBundles){
      canvas->CloseUndoBundle();
    }
    m_commandBundles.clear();
  }

  void PythonGetKey() override{
    m_interpreterFrame.GetKey();
  }

  void PythonIntFaintPrint(const utf8_string& s) override{
    m_interpreterFrame.IntFaintPrint(s);
  }

  void PythonNewPrompt() override{
    m_interpreterFrame.NewPrompt();
  }

  void PythonPrint(const utf8_string& s) override{
    m_interpreterFrame.Print(s);
  }

  void PythonQueueRefresh(Canvas* canvas) override{
    QueueRefresh(canvas);
  }

  void PythonRunCommand(Canvas* canvas, Command* command) override{
    QueueRefresh(canvas);
    if (std::find(begin(m_commandBundles), end(m_commandBundles), canvas) == m_commandBundles.end()){
      m_commandBundles.push_back(canvas);
      canvas->OpenUndoBundle();
    }
    canvas->RunCommand(command);
  }

  void PythonRunCommand(Canvas* canvas, Command* command, const FrameId& frameId) override{
    QueueRefresh(canvas);
    if (std::find(begin(m_commandBundles), end(m_commandBundles), canvas) == end(m_commandBundles)){
      m_commandBundles.push_back(canvas);
      canvas->OpenUndoBundle();
    }
    canvas->RunCommand(command, frameId);
  }

  void QueueLoad(const FileList& filenames) override{
    m_frame.GetEventHandler()->QueueEvent(new OpenFilesEvent(filenames));
  }

  void Quit() override{
    m_frame.Close(false); // False means don't force
  }

  void RaiseFrame() override{
    m_frame.Raise();
  }

  bool Save(Canvas& canvas) override{
    return m_frame.Save(canvas);
  }

  void SelectTool(ToolId id) override{
    m_frame.SelectTool(id);
  }

  void Set(const BoolSetting& s, BoolSetting::ValueType v) override{
    change_setting(m_frame, s, v, from_control(false));
  }

  void Set(const StrSetting& s, StrSetting::ValueType v) override{
    change_setting(m_frame, s, v, from_control(false));
  }

  void Set(const IntSetting& s, IntSetting::ValueType v) override{
    change_setting(m_frame, s, v, from_control(false));
  }

  void Set(const ColorSetting& s, ColorSetting::ValueType v) override{
    change_setting(m_frame, s, v, from_control(false));
  }

  void Set(const FloatSetting& s, FloatSetting::ValueType v) override{
    change_setting(m_frame, s, v, from_control(false));
  }

  void SetActiveCanvas(const CanvasId& id) override{
    return m_frame.SetActiveCanvas(id);
  }

  void SetDefaultGrid(const Grid& grid) override{
    m_defaultGrid = grid;
  }

  void SetDefaultResizeDialogSettings(const ResizeDialogSettings& settings) override{
    m_defaultResizeSettings = settings;
  }

  void SetInterpreterBackground(const ColRGB& c) override{
    m_interpreterFrame.FaintSetBackgroundColour(c);
  }

  void SetInterpreterTextColor(const ColRGB& c) override{
    m_interpreterFrame.SetTextColor(c);
  }

  void SetPalette(const PaintMap& paintMap) override{
    m_frame.SetPalette(paintMap);
  }

  void SetTransparencyStyle(const TransparencyStyle& style) override{
    m_transparencyStyle = style;
    m_frame.Refresh();
  }

  void SetLayer(Layer layer) override{
    m_frame.SelectLayer(layer);
  }

  void ModalFull(const dialog_func& show_dialog) override{
    Canvas& canvas = m_frame.GetActiveCanvas();
    DialogFeedbackImpl feedback(canvas);
    BeginModalDialog();
    Optional<Command*> maybeCmd = show_dialog(m_frame, feedback, canvas);
    EndModalDialog();
    if (maybeCmd.NotSet()){
      canvas.Refresh();
      return;
    }
    canvas.RunCommand(maybeCmd.Get());
    canvas.Refresh();
  }

  void Modal(const bmp_dialog_func& show_dialog) override{
    Canvas& canvas(GetActiveCanvas());
    DialogFeedbackImpl feedback(canvas);
    BeginModalDialog();
    Optional<BitmapCommand*> maybeCmd = show_dialog(m_frame, feedback);
    EndModalDialog();
    if (maybeCmd.NotSet()){
      canvas.Refresh();
      return;
    }

    BitmapCommand* cmd = maybeCmd.Get();
    const RasterSelection& selection = canvas.GetRasterSelection();
    if (selection.Empty()){
      canvas.RunCommand(target_full_image(cmd));
    }
    else if (selection.Floating()){
      canvas.RunCommand(target_floating_selection(cmd));
    }
    else{
      canvas.RunCommand(target_rectangle(cmd, selection.GetRect()));
    }
    canvas.Refresh();
  }


  void ToggleHelpFrame() override{
    toggle_top_level_window(m_helpFrame);
  }

  void TogglePythonConsole() override{
    toggle_top_level_window(m_interpreterFrame);
  }

  void ShowPythonConsole() override{
    m_interpreterFrame.Show();
    m_interpreterFrame.Raise();
  }

  int TabletGetCursor() override{
    return m_tabletCursor;
  }

  // Note: Not an override, used directly by FaintWindow
  void TabletSetCursor(int tabletCursor){
    m_tabletCursor = tabletCursor;
  }

  void ToggleColorbar(bool show) override{
    m_frame.ToggleColorbar(show);
  }

  void ToggleFullScreen(bool fullScreen) override{
    m_frame.FaintFullScreen(fullScreen);
  }

  void ToggleMaximize() override{
    m_frame.ToggleMaximize();
  }

  void ToggleToolPanel(bool show) override{
    m_frame.ToggleToolPanel(show);
  }

  void ToggleStatusbar(bool show) override{
    m_frame.ToggleStatusbar(show);
  }

  void Unbind(const KeyPress& key) override{
    m_binds.erase(key);
  }

  void UpdateShownSettings() override{
    m_frame.UpdateShownSettings();
  }

  void UpdateToolSettings(const Settings& s) override{
    m_frame.UpdateToolSettings(s);
  }

  void CloseFloatingWindows(){ // Non-virtual
    m_helpFrame.Close();
    m_interpreterFrame.Close(true);
  }

  bool FloatingWindowFocused() const{ // Non-virtual
    return m_helpFrame.HasFocus() || m_interpreterFrame.FaintHasFocus();
  }

private:
  void QueueRefresh(Canvas* canvas){
    m_unrefreshed.insert(canvas);
    m_canvasIds[ canvas ] = canvas->GetId();
  }
  FaintWindow& m_frame;
  HelpFrame& m_helpFrame;
  InterpreterFrame& m_interpreterFrame;
  SBInterface m_statusbar;
  std::set<Canvas*> m_unrefreshed;
  std::map<Canvas*, CanvasId> m_canvasIds;
  std::vector<Canvas*> m_commandBundles;
  std::set<KeyPress> m_binds;
  std::set<KeyPress> m_globalBinds;
  bool m_modalDialog;
  Grid m_defaultGrid;
  ResizeDialogSettings m_defaultResizeSettings;
  TransparencyStyle m_transparencyStyle;
  int m_tabletCursor;
};

static wxStatusBar& create_faint_statusbar(wxFrame* frame){
  // The status bar is cleaned up by wxWidgets at frame destruction.
  wxStatusBar* statusbar = new wxStatusBar(frame, wxID_ANY);
  frame->SetStatusBar(statusbar);
  return *statusbar;
}

FaintWindow::FaintWindow(ArtContainer& art, PaletteContainer& palettes, HelpFrame* helpFrame, InterpreterFrame* interpreterFrame, bool silentMode)
  : wxFrame(nullptr, wxID_ANY, "Faint", wxPoint(50,50), wxSize(800,700)),
    m_artContainer(art),
    m_toolSettings(default_tool_settings()),
    m_palettes(palettes),
    m_silentMode(silentMode)
{
  Bind(EVT_FAINT_ACTIVE_CANVAS_CHANGE,
    [&](CanvasChangeEvent& e){
      UpdateZoom();
      m_colorPanel->UpdateGrid();
      UpdateCanvasState(e.GetCanvasId());
      m_activeTool->SelectionChange();
      UpdateShownSettings();
    });

  Bind(EVT_FAINT_CANVAS_CHANGE,
    [&](CanvasChangeEvent& e){
      UpdateCanvasState(e.GetCanvasId());
    });

  Bind(EVT_FAINT_ZOOM_CHANGE,
    [&](CanvasChangeEvent& e){
      if (e.GetCanvasId() == GetActiveCanvas().GetId()){
        UpdateZoom();
      }
    });

  Bind(EVT_FAINT_GRID_CHANGE,
    [&](CanvasChangeEvent& e){
      if (e.GetCanvasId() == GetActiveCanvas().GetId()){
        m_colorPanel->UpdateGrid();
      }
    });

  Bind(EVT_FAINT_TOOL_CHANGE,
    [&](const ToolChangeEvent& event){
      DoSelectTool(event.GetTool());
    });

  Bind(EVT_FAINT_LAYER_CHANGE,
    [&](const LayerChangeEvent event){
      m_layerType = event.GetLayer();
      CanvasPanel* activeCanvas = GetActiveCanvasPanel();
      activeCanvas->Refresh();
      UpdateCanvasState(activeCanvas->GetCanvasId());
    });

  Bind(EVT_FAINT_ADD_TO_PALETTE,
    [&](const PaintEvent& event){
      AddToPalette(event.GetPaint());
    });

  Bind(EVT_FAINT_COPY_COLOR_HEX,
    [&](const ColorEvent& event){
      Clipboard clipboard;
      if (!clipboard.Good()){
        show_copy_color_error(this);
        return;
      }
      clipboard.SetText(utf8_string(str_hex(event.GetColor())));
    });

  Bind(EVT_FAINT_COPY_COLOR_RGB,
    [&](const ColorEvent& event){
      Clipboard clipboard;
      if (!clipboard.Good()){
        show_copy_color_error(this);
        return;
      }
      clipboard.SetText(utf8_string(str_smart_rgba(event.GetColor())));
    });

  Bind(EVT_FAINT_OPEN_FILES,
    [&](const OpenFilesEvent& event){
      Open(event.GetFileNames());
    });

  Bind(EVT_SWAP_COLORS,
    [&](const wxEvent&){
      Paint fg(m_appContext->Get(ts_Fg));
      Paint bg(m_appContext->Get(ts_Bg));
      m_appContext->Set(ts_Fg, bg);
      m_appContext->Set(ts_Bg, fg);
    });

  Bind(EVT_SET_FOCUS_ENTRY_CONTROL,
    [&](const wxEvent&){
      if (m_textEntryCount == 0){
        // Entry controls are numeric, not all shortcuts need to be
        // disabled
        const bool numeric = true;
        m_menubar->BeginTextEntry(numeric);
      }
      m_textEntryCount++;
    });

  Bind(EVT_KILL_FOCUS_ENTRY_CONTROL,
    [&](const wxEvent&){
      assert(m_textEntryCount > 0);
      m_textEntryCount -= 1;
      if (m_textEntryCount == 0){
        m_menubar->EndTextEntry();
      }
    });

  Bind(wxEVT_CLOSE_WINDOW,
    [&](wxCloseEvent& event){
      if (m_tabControl->UnsavedDocuments() && event.CanVeto()){
        bool quit = ask_exit_unsaved_changes(this);
        if (!quit){
          event.Veto();
          return;
        }
      }

      m_menubar->GetRecentFiles().Save();
      Clipboard::Flush();
      m_appContext->CloseFloatingWindows();
      event.Skip();
    });

  Bind(EVT_TABLET,
    [&](const TabletEvent& event){
      m_appContext->TabletSetCursor(event.GetCursor());
    });

  m_appContext = new FrameContext(*this, create_faint_statusbar(this),
     *helpFrame, *interpreterFrame);

  // Fixme: Use make_unique in c++14
  m_menubar.reset(new Menubar(*this, *m_appContext, m_artContainer));
  CreatePanels();

  m_toolPanel->AsWindow()->Bind(EVT_FAINT_INT_SETTING_CHANGE,
    [&](const SettingEvent<IntSetting>& e){
      change_setting(*this, e.GetSetting(), e.GetValue(), from_control(true));
    });

  m_toolPanel->AsWindow()->Bind(EVT_FAINT_FLOAT_SETTING_CHANGE,
    [&](const SettingEvent<FloatSetting>& e){
      change_setting(*this, e.GetSetting(), e.GetValue(), from_control(true));
    });

  m_toolPanel->AsWindow()->Bind(EVT_FAINT_BOOL_SETTING_CHANGE,
    [&](const SettingEvent<BoolSetting>& e){
      change_setting(*this, e.GetSetting(), e.GetValue(), from_control(true));
    });

  m_toolPanel->AsWindow()->Bind(EVT_FAINT_SETTINGS_CHANGE,
    [&](const SettingsEvent& e){
      // Note that m_fromCtrl is not used when changing multiple
      // settings. This is because font dialog can change the font
      // size, which is also shown in another control which must be
      // updated, so there's no point in trying to not refresh controls.
      change_settings(*this, e.GetSettings());
    });

  m_colorPanel->AsWindow()->Bind(EVT_FAINT_COLOR_SETTING_CHANGE,
    [&](const SettingEvent<ColorSetting>& e){
      change_setting(*this, e.GetSetting(), e.GetValue(), from_control(false));
    });

  // Fixme: Wrong place for such stuff, has nothing todo with main
  // frame. Consider App.
  m_formats = built_in_file_formats();
  SetMinSize(wxSize(640, 480));
  m_toolPanel->SelectTool(ToolId::LINE);
}


void FaintWindow::AddToPalette(const Paint& src){
  m_colorPanel->AddToPalette(src);
}

void FaintWindow::AddFormat(Format* f){
  m_formats.push_back(f);
}

void FaintWindow::BeginTextEntry(){
  m_textEntryCount++;
  m_menubar->BeginTextEntry();

  // To enable e.g. "Select none" for the EditText task, bit of a hack.
  UpdateMenu();
}

void FaintWindow::CloseActiveTab(){
  // Freeze the color panel to avoid the FrameControl refreshing
  // during page close on GTK. (See issue 122).
  auto freezer = freeze(m_colorPanel);
  m_tabControl->CloseActive();
}

void FaintWindow::CloseDocument(Canvas& canvas){
  for (int i = 0; i != m_tabControl->GetCanvasCount(); i++){
    Canvas& other = m_tabControl->GetCanvas(i)->GetInterface();
    if (&other == &canvas){
      // Freeze the color panel to avoid the FrameControl refreshing
      // during page close on GTK. (See issue 122).
      auto freezer = freeze(m_colorPanel);
      m_tabControl->Close(i);
      return;
    }
  }
}

void FaintWindow::CreatePanels(){
  // Top half, the tool panel and the drawing areas.
  wxBoxSizer* row1 = new wxBoxSizer(wxHORIZONTAL);
  m_toolPanel.reset(new ToolPanel(this, m_appContext->GetStatusInfo(), m_artContainer)); // Fixme: make_unique in c++14
  row1->Add(m_toolPanel->AsWindow(), 0, wxEXPAND);
  m_tabControl.reset(new TabCtrl(this, *m_appContext, m_appContext->GetStatusInfo()));
  row1->Add(m_tabControl->AsWindow(), 1, wxEXPAND);

  // Bottom half, the selected color, palette and zoom controls.
  m_colorPanel.reset(new ColorPanel(this,
    m_toolSettings,
    m_palettes,
    *m_appContext,
    m_appContext->GetStatusInfo(),
    m_artContainer));

  auto rows = new wxBoxSizer(wxVERTICAL);
  rows->Add(row1, 1, wxEXPAND);
  rows->Add(m_colorPanel->AsWindow(), 0, wxEXPAND);
  SetSizer(rows);
  Layout();
}

void FaintWindow::DoSelectTool(ToolId id){
  if (m_activeTool != nullptr){
    auto canvas = m_tabControl->GetActiveCanvas();
    canvas->Preempt(PreemptOption::ALLOW_COMMAND);
    delete m_activeTool;
    m_activeTool = nullptr;
  }
  m_activeTool = new_tool(id, m_toolSettings, ActiveCanvas(*m_appContext));
  UpdateShownSettings();

  wxStatusBar* statusBar = GetStatusBar();
  statusBar->SetFieldsCount(m_activeTool->GetStatusFieldCount() + 1);
}

void FaintWindow::EndTextEntry(){
  assert(m_textEntryCount > 0);
  m_textEntryCount--;
  if (m_textEntryCount == 0){
    m_menubar->EndTextEntry();
  }
}

bool FaintWindow::Exists(const CanvasId& id ) const {
  return m_tabControl->Has(id);
}

void FaintWindow::FaintFullScreen(bool enable){
  if (enable){
    auto freezer = freeze(this);
    m_toolPanel->Hide();
    m_colorPanel->Hide();
    m_tabControl->HideTabs();
  }
  else {
    auto freezer = freeze(this);
    m_colorPanel->Show(m_frameSettings.palette_visible);
    m_toolPanel->Show(m_frameSettings.toolbar_visible);
    m_tabControl->ShowTabs();
  }
  ShowFullScreen(enable);
}

void FaintWindow::FaintInitialize(){
  // Pretty much all of Faint expects there to be an active canvas, so
  // create one early. It would be nicer if no blank "untitled"-image
  // was created when starting with files from the command line, but
  // this caused crashes if the loading failed (even with attempt to
  // open a blank document on failure)
  // ...Perhaps the loading should be detached from the FaintWindow and
  // everything, but that's for some other time.
  NewDocument(m_appContext->GetDefaultImageInfo());

  #ifdef __WXMSW__
  tablet::InitResult tabletResult = tablet::initialize(wxGetInstance(),
    GetHWND());

  if (tabletResult == tablet::InitResult::OK){
    tablet::activate(true);
    m_cleanup.emplace_back(create_cleaner([](){tablet::uninitialize();}));
  }
  else{
    tablet::show_tablet_error_message(this, tabletResult);
  }
  #endif
}

Canvas& FaintWindow::GetActiveCanvas(){
  return GetActiveCanvasPanel()->GetInterface();
}

CanvasPanel* FaintWindow::GetActiveCanvasPanel(){
  auto active = m_tabControl->GetActiveCanvas();
  assert(active != nullptr);
  return active;
}

Tool* FaintWindow::GetActiveTool(){
  return m_activeTool;
}

Canvas& FaintWindow::GetCanvas(int i){
  return m_tabControl->GetCanvas(i)->GetInterface();
}

int FaintWindow::GetCanvasCount() const{
  return m_tabControl->GetCanvasCount();
}

AppContext& FaintWindow::GetContext(){
  return *m_appContext;
}

std::vector<Format*>& FaintWindow::GetFileFormats(){
  return m_formats;
}

Layer FaintWindow::GetLayerType() const{
  return m_layerType;
}

Settings FaintWindow::GetShownSettings() const{
  Settings settings(m_toolSettings);
  settings.Update(m_activeTool->GetSettings());
  return settings;
}

TabCtrl& FaintWindow::GetTabControl(){
  return *m_tabControl;
}

EntryMode FaintWindow::GetTextEntryMode() const{
  return m_menubar->GetTextEntryMode();
}

ToolId FaintWindow::GetToolId() const{
  if (m_activeTool == nullptr){
    return ToolId::OTHER;
  }
  return m_activeTool->GetId();
}

Settings& FaintWindow::GetToolSettings(){
  return m_toolSettings;
}

bool FaintWindow::MainFrameFocused() const{
  return !(m_appContext->ModalDialogShown() ||
    m_appContext->FloatingWindowFocused());
}

void FaintWindow::ModifierKeyChange(){
  if (!MainFrameFocused()){
    return;
  }

  auto active = GetActiveCanvasPanel();
  if (active->HasFocus()){
    // The key-handler of the CanvasPanel will get this
    // keypress.
    return;
  }

  IntPoint mousePos = mouse::screen_position();
  if (active->GetScreenRect().Contains(to_wx(mousePos))){
    active->MousePosRefresh();
  }
}

#ifdef __WXMSW__
WXLRESULT FaintWindow::MSWWindowProc(WXUINT msg, WXWPARAM wParam, WXLPARAM lParam){
  if (tablet::is_wt_packet(msg)){
    tablet::WTP packet = tablet::get_wt_packet(wParam, lParam);
    ScreenToClient(&packet.x, &packet.y);
    TabletEvent event(packet.x, packet.y, packet.pressure, packet.cursor);
    AddPendingEvent(event);
  }
  if (msg == WM_ACTIVATE){
    tablet::msg_activate(wParam, lParam);
  }
  return wxFrame::MSWWindowProc(msg, wParam, lParam);
}
#endif

Canvas& FaintWindow::NewDocument(const ImageInfo& info){
  CanvasPanel* canvas = m_tabControl->NewDocument(ImageProps(info),
    change_tab(true), initially_dirty(false));
  // Fixme: Check nullptr?
  return canvas->GetInterface();
}

void FaintWindow::NextTab() {
  m_tabControl->SelectNext();
}

void FaintWindow::UpdateCanvasState(const CanvasId& id){
  if (GetActiveCanvasPanel()->GetCanvasId() == id){
    UpdateMenu();
  }
  m_toolPanel->ShowSettings(m_activeTool->GetSettings());
  m_colorPanel->UpdateFrames();
}

void FaintWindow::UpdateMenu(){
  m_menubar->Update(GetActiveCanvasPanel()->GetMenuFlags());
}

void FaintWindow::UpdateZoom(){
  ZoomLevel zoom(GetActiveCanvasPanel()->GetZoomLevel());
  m_colorPanel->UpdateZoom(zoom);
  m_menubar->UpdateZoom(zoom);
}

void FaintWindow::Open(const FileList& paths){
  if (m_tabControl->GetCanvasCount() != 0){
    // Refresh the entire frame to erase any dialog or menu droppings before
    // starting potentially long-running file loading.
    Refresh();
    Update();
  }

  FileList notFound; // Fixme: Ugly fix for issue 114 (though bundling file names is nicer than individual error messages)
  try {
    // Freeze the panel to remove some refresh glitches in the
    // tool-settings on MSW during loading.
    auto freezer = freeze(m_toolPanel->AsWindow());
    bool first = true;
    for (const FilePath& filePath : paths){
      if (exists(filePath)){
        Open(filePath, change_tab(then_false(first)));
      }
      else {
        notFound.push_back(filePath); // Fixme
      }
    }
  }
  catch (const std::bad_alloc&){
    show_error(this, Title("Insufficient memory to load all images."), "Out of memory");
  }

  if (GetCanvasCount() == 0){
    NewDocument(m_appContext->GetDefaultImageInfo());
  }

  if (notFound.size() == 1){
    show_file_not_found_error(this, notFound.back());
  }
  else if (notFound.size() > 1){
    utf8_string error = "Files not found: \n";
    for (const FilePath& path : notFound){
      error += path.Str() + "\n";
    }
    show_error(this, Title("Files not found"), to_wx(error));
  }
}

Canvas* FaintWindow::Open(const FilePath& filePath, const change_tab& changeTab){
  FileExtension extension(filePath.Extension()); // Fixme

  for (Format* format : loading_file_formats(m_formats)){
    if (format->Match(extension)){
      ImageProps props;
      format->Load(filePath, props);
      if (!props.IsOk()){
        if (!m_silentMode){
          show_load_failed_error(this, filePath, props.GetError());
        }
        return nullptr;
      }
      CanvasPanel* newCanvas = m_tabControl->NewDocument(std::move(props),
        changeTab, initially_dirty(false));
      // Fixme: Either check for null or return reference
      newCanvas->NotifySaved(filePath);
      m_menubar->GetRecentFiles().Add(filePath);
      if (props.GetNumWarnings() != 0){
        if (!m_silentMode){
          show_load_warnings(this, props);
        }
      }
      return &(newCanvas->GetInterface());
    }
  }
  if (!m_silentMode){
    show_file_not_supported_error(this, filePath);
  }
  return nullptr;
}

void FaintWindow::PreviousTab(){
  m_tabControl->SelectPrevious();
}

bool FaintWindow::Save(Canvas& canvas){
  Optional<FilePath> filePath(canvas.GetFilePath());
  if (filePath.IsSet() && has_save_format(m_formats, filePath.Get().Extension())){
    return Save(canvas, filePath.Get(), -1);
  }
  else {
    return ShowSaveAsDialog(canvas);
  }
}

bool FaintWindow::Save(Canvas& canvas, const FilePath& filePath, int filterIndex){

  Format* format = get_save_format(m_formats, filePath.Extension(), filterIndex);
  if (format == nullptr){
    show_error(this, Title("Failed Saving"),
      to_wx(str_no_matching_format(filePath.Extension())));
    return false;
  }
  SaveResult result = format->Save(filePath, canvas);
  if (result.Failed()){
    show_error(this, Title("Failed Saving"), to_wx(result.ErrorDescription()));
    return false;
  }

  canvas.NotifySaved(filePath);
  if (format->CanLoad() || get_load_format(m_formats, filePath.Extension()) != nullptr){
    // Add to file history only if the format can be loaded
    m_menubar->GetRecentFiles().Add(filePath);
  }
  return true;
}

void FaintWindow::SelectLayer(Layer layer){
  m_toolPanel->SelectLayer(layer);
}

void FaintWindow::SelectTool(ToolId id){
  m_toolPanel->SelectTool(id);
}

void FaintWindow::SetActiveCanvas(const CanvasId& id){
  m_tabControl->Select(id);
}

void FaintWindow::SetPalette(const PaintMap& paintMap){
  m_colorPanel->SetPalette(paintMap);
}

bool FaintWindow::ShowSaveAsDialog(Canvas& canvas){
  Optional<FilePath> oldFilePath(canvas.GetFilePath());
  wxFileName initialPath(oldFilePath.IsSet() ?
    wxFileName(to_wx(oldFilePath.Get().Str())) :
    wxFileName());
  int defaultFormatIndex = oldFilePath.NotSet() ? 0 :
    get_file_format_index(m_formats, FileExtension(initialPath.GetExt()));

  wxFileDialog dlg(this, "Save as",
    initialPath.GetPath(),
    initialPath.GetName(),
    to_wx(file_dialog_filter(saving_file_formats(m_formats))), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (defaultFormatIndex != -1){
    dlg.SetFilterIndex(defaultFormatIndex);
  }

  int result = show_modal(dlg);
  if (result != wxID_OK){
    return false;
  }

  return Save(canvas, FilePath::FromAbsoluteWx(dlg.GetPath()), dlg.GetFilterIndex());
}

void FaintWindow::ToggleColorbar(bool show){
  m_frameSettings.palette_visible = show;
  m_colorPanel->Show(show);
  Layout();
}

void FaintWindow::ToggleMaximize(){
  if (!IsFullScreen()){
    Maximize(!IsMaximized());
  }
  else {
    FaintFullScreen(false);
  }
}

void FaintWindow::ToggleStatusbar(bool show){
  m_frameSettings.statusbar_visible = show;
  GetStatusBar()->Show(show);
  Layout();
}

void FaintWindow::ToggleToolPanel(bool show){
  // Update the setting state and and tool panel visibility. Freezing
  // the FaintWindow removes a refresh-artifact when re-showing the
  // tool-panel on MSW (The wxStaticLine:s from the child panels
  // appeared in the canvas).
  {
    auto freezer = freeze(this);
    m_frameSettings.toolbar_visible = show;
    m_toolPanel->Show(show);
  }

  // Update the sizers to the new state of the tool panel. Freezing
  // the color panel removes flicker in the palette. This depends on
  // the tool panel stretching across the frame.
  auto freezer = freeze(m_colorPanel);
  Layout();
}

void FaintWindow::UpdateShownSettings(){
  const Settings& activeSettings(m_activeTool->GetSettings());
  m_toolPanel->ShowSettings(activeSettings);
  m_colorPanel->UpdateSelectedColors(get_color_choice(activeSettings,
    m_toolSettings, m_activeTool->EatsSettings()));
}

void FaintWindow::UpdateToolSettings(const Settings& s){
  m_toolSettings.Update(s);
  m_activeTool->UpdateSettings(m_toolSettings);
  UpdateShownSettings();
}

} // namespace

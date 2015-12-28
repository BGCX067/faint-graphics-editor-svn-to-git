// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#ifndef FAINT_WINDOW_APP_CONTEXT_HH
#define FAINT_WINDOW_APP_CONTEXT_HH
#include "app/app-context.hh"
#include "app/context-commands.hh"
#include "bitmap/draw.hh"
#include "gui/interpreter-frame.hh"
#include "gui/resize-dialog-options.hh"
#include "gui/tab-ctrl.hh" // Fixme: Remove
#include "gui/transparency-style.hh"
#include "util/bound-setting.hh"
#include "util/image.hh"
#include "util/image-props.hh"
#include "util/pos-info-constants.hh"
#include "util/visit-selection.hh"
#include "util-wx/convert-wx.hh"
#include "util-wx/file-format-util.hh"
#include "util-wx/gui-util.hh"

namespace faint{

static Optional<DirPath> get_canvas_dir(const Canvas& canvas){
  Optional<FilePath> oldFileName(canvas.GetFilePath());
  return oldFileName.IsSet() ?
    option(oldFileName.Get().StripFileName()) :
    no_option();
}

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

using from_control = LessDistinct<bool, 0>;

template<typename T>
void change_setting(FaintWindow& window, const T& setting, typename T::ValueType value, const from_control& fromControl){
  Tool* tool = window.GetActiveTool();
  if (tool->GetSettings().Has(setting)){
    bool toolModified = tool->Set({setting, value});
    if (toolModified) {
      window.GetActiveCanvas().Refresh();
    }
    if (tool->EatsSettings()){
      if (!fromControl.Get()){
        window.UpdateShownSettings();
      }
      return;
    }
  }
  window.GetToolSettings().Set(setting, value);
  if (!fromControl.Get()){
    window.UpdateShownSettings();
  }
}

class DialogFeedbackImpl : public DialogFeedback{
public:
  DialogFeedbackImpl(Canvas& canvas)
    : m_canvas(canvas)
  {}

  const Bitmap& GetBitmap() override{
    if (m_bitmap == nullptr){
      Initialize();
    }
    return *m_bitmap;
  }

  void SetBitmap(const Bitmap& bmp) override{
    if (m_bitmap == nullptr){
      m_bitmap = std::make_shared<Bitmap>(bmp);
    }
    else{
      *m_bitmap = bmp;
    }
    Update();
  }

  void SetBitmap(Bitmap&& bmp) override{
    if (m_bitmap == nullptr){
      m_bitmap = std::make_shared<Bitmap>(std::move(bmp));
    }
    else{
      *m_bitmap = std::move(bmp);
    }
    Update();
  }

private:
  void Update(){
    if (m_rasterSelection != nullptr && m_rasterSelection->Floating()){
      m_rasterSelection->SetFloatingBitmap(*m_bitmap,
        m_rasterSelection->TopLeft());
      m_canvas.SetMirage(m_rasterSelection);
    }
    else{
      m_canvas.SetMirage(m_bitmap);
    }
  }

  void Initialize(){
    const Image& active = m_canvas.GetImage();
    const RasterSelection& selection = active.GetRasterSelection();
    sel::visit(selection,
      [&](const sel::Empty&){
        // Fixme: Handle possible OOM ..or I dunno, use some fake bitmap
        active.GetBackground().Visit(
          [&](const Bitmap& bmp){
            m_bitmap = std::make_shared<Bitmap>(bmp);
          },
          [&](const ColorSpan& span){
            m_bitmap = std::make_shared<Bitmap>(span.size,
              span.color);
          });
      },
      [&](const sel::Rectangle& r){
        // Create a floating selection chimera from the non-floating
        // selection
        active.GetBackground().Visit(
          [&](const Bitmap& bmp){
            m_bitmap = std::make_shared<Bitmap>(subbitmap(bmp,
              r.Rect()));
            m_rasterSelection = std::make_shared<RasterSelection>();
            m_rasterSelection->SetState(SelectionState(*m_bitmap,
              selection.TopLeft()));
          },
          [&](const ColorSpan& span){
            m_bitmap = std::make_shared<Bitmap>(
              r.Rect().GetSize(), span.color);
            m_rasterSelection = std::make_shared<RasterSelection>();
            m_rasterSelection->SetState(SelectionState(*m_bitmap,
              r.TopLeft()));
          });
      },
      [&](const sel::Floating& f){
        m_rasterSelection = std::make_shared<RasterSelection>(selection);
        m_bitmap = std::make_shared<Bitmap>(f.GetBitmap());
      });
  }

  std::shared_ptr<Bitmap> m_bitmap;
  Canvas& m_canvas;
  std::shared_ptr<RasterSelection> m_rasterSelection;
};

class FaintDialogContext : public DialogContext{
public:
  FaintDialogContext(AppContext& app)
    : m_app(app)
  {}
private:
  void BeginModalDialog() override{
    m_app.BeginModalDialog();
  }

  void EndModalDialog() override{
    m_app.EndModalDialog();
  }

private:
  AppContext& m_app;
};

class FaintWindowContext : public AppContext {
public:
  FaintWindowContext(FaintWindow& window, wxStatusBar& statusbar, HelpFrame& helpFrame, InterpreterFrame& interpreterFrame)
    : m_dialogContext(*this),
      m_faintWindow(window),
      m_helpFrame(helpFrame),
      m_interpreterFrame(interpreterFrame),
      m_modalDialog(false),
      m_statusbar(statusbar),
      m_tabletCursor(TABLET_CURSOR_PUCK)
  {}

  void AddFormat(Format* fileFormat) override{
    m_faintWindow.AddFormat(fileFormat);
  }

  void AddToPalette(const Paint& paint) override{
    m_faintWindow.AddToPalette(paint);
  }

  void BeginModalDialog() override{
    m_modalDialog = true;
  }

  void BeginTextEntry() override{
    m_faintWindow.BeginTextEntry();
  }

  void Close(Canvas& canvas) override{
    m_faintWindow.CloseDocument(canvas);
  }

  void DialogOpenFile() override{
    FileList paths = show_open_file_dialog(m_faintWindow.GetRawFrame(),
      Title("Open Image(s)"),
      get_canvas_dir(GetActiveCanvas()),
      to_wx(combined_file_dialog_filter(utf8_string("Image files"),
        loading_file_formats(m_faintWindow.GetFileFormats()))));
    if (!paths.empty()){
      Load(paths);
    }
  }

  void DialogSaveAs(Canvas& canvas, bool backup) override{
    m_faintWindow.ShowSaveAsDialog(canvas, backup);
  }

  void EndModalDialog() override{
    m_modalDialog = false;
  }

  void EndTextEntry() override{
    m_faintWindow.EndTextEntry();
  }

  bool Exists(const CanvasId& id) override{
    return m_faintWindow.Exists(id);
  }

  bool FaintWindowFocused() const override{
    return !(ModalDialogShown() || FloatingWindowFocused());
  }

  BoolSetting::ValueType Get(const BoolSetting& s) override{
    return m_faintWindow.GetShownSettings().Get(s);
  }

  StrSetting::ValueType Get(const StrSetting& s) override{
    return m_faintWindow.GetShownSettings().Get(s);
  }

  IntSetting::ValueType Get(const IntSetting& s) override{
    return m_faintWindow.GetShownSettings().Get(s);
  }

  ColorSetting::ValueType Get(const ColorSetting& s) override{
    return m_faintWindow.GetShownSettings().Get(s);
  }

  FloatSetting::ValueType Get(const FloatSetting& s) override{
    return m_faintWindow.GetShownSettings().Get(s);
  }

  Canvas& GetActiveCanvas() override{
    return m_faintWindow.GetActiveCanvas();
  }

  Tool* GetActiveTool() override{
    return m_faintWindow.GetActiveTool();
  }

  Canvas& GetCanvas(const index_t& i) override{
    return m_faintWindow.GetCanvas(i);
  };

  index_t GetCanvasCount() const override{
    return m_faintWindow.GetCanvasCount();
  }

  Grid GetDefaultGrid() const override{
    return m_defaultGrid;
  }

  ImageInfo GetDefaultImageInfo() override{
    return ImageInfo(IntSize(640,480), Color(255,255,255), create_bitmap(true));
  }

  DialogContext& GetDialogContext() override{
    return m_dialogContext;
  }

  std::vector<Format*> GetFormats() override{
    return m_faintWindow.GetFileFormats();
  }

  ResizeDialogOptions GetDefaultResizeDialogOptions() const override{
    return m_defaultResizeSettings;
  }

  Layer GetLayerType() const override{
    return m_faintWindow.GetLayerType();
  }

  IntPoint GetMousePos() override{
    return to_faint(wxGetMousePosition());
  }

  StatusInterface& GetStatusInfo(){ // Non virtual
    return m_statusbar;
  }

  ToolId GetToolId() const override{
    return m_faintWindow.GetToolId();
  }

  Settings GetToolSettings() const override{
    return m_faintWindow.GetShownSettings();
  }

  const TransparencyStyle& GetTransparencyStyle() const override{
    return m_transparencyStyle;
  }

  bool IsFullScreen() const override{
    return m_faintWindow.IsFullScreen();
  }

  Canvas* Load(const FilePath& filePath, const change_tab& changeTab) override{
    return m_faintWindow.Open(filePath, changeTab);
  }

  void Load(const FileList& filePaths) override{
    return m_faintWindow.Open(filePaths);
  }

  Canvas* LoadAsFrames(const FileList& paths, const change_tab& changeTab) override{
    std::vector<ImageProps> props;
    const std::vector<Format*> formats = loading_file_formats(m_faintWindow.GetFileFormats());
    for (const FilePath& filePath : paths){
      FileExtension extension(filePath.Extension());
      bool loaded = false;
      for (Format* format : formats){
        if (format->Match(extension)){
          props.emplace_back();
          format->Load(filePath, props.back());
          if (!props.back().IsOk()){
            // Fixme: Commented for some reason
            // show_load_failed_error(m_faintWindow, filePath, props.back().GetError());
            return nullptr;
          }
          loaded = true;
          break;
        }
      }
      if (!loaded){
        // show_load_failed_error(m_faintWindow, filePath, "One path could not be loaded.");
        return nullptr;
      }
    }
    auto canvas = m_tabControl->NewDocument(std::move(props),
      changeTab, initially_dirty(true));
    Canvas* canvasInterface = &(canvas->GetInterface());
    return canvasInterface;
  }

  void Maximize() override{
    m_faintWindow.Maximize(!m_faintWindow.IsMaximized());
  }

  void MaximizeInterpreter() override{
    m_interpreterFrame.Maximize(!m_interpreterFrame.IsMaximized());
  }

  bool ModalDialogShown() const override{
    return m_modalDialog;
  }

  Canvas& NewDocument(const ImageInfo& info) override{
    return m_faintWindow.NewDocument(info);
  }

  Canvas& NewDocument(ImageProps&& props) override{
    CanvasPanel* canvas = m_tabControl->NewDocument(std::move(props),
      change_tab(true), initially_dirty(true));
    return canvas->GetInterface();
  }

  void NextTab() override{
    m_faintWindow.NextTab();
  }

  void PreviousTab() override{
    m_faintWindow.PreviousTab();
  }

  void QueueLoad(const FileList& filenames) override{
    m_faintWindow.QueueLoad(filenames);
  }

  void Quit() override{
    m_faintWindow.Close(false); // False means don't force
  }

  void RaiseWindow() override{
    m_faintWindow.Raise();
  }

  bool Save(Canvas& canvas) override{
    return m_faintWindow.Save(canvas);
  }

  void SelectTool(ToolId id) override{
    m_faintWindow.SelectTool(id);
  }

  void Set(const BoolSetting& s, BoolSetting::ValueType v) override{
    change_setting(m_faintWindow, s, v, from_control(false));
  }

  void Set(const StrSetting& s, StrSetting::ValueType v) override{
    change_setting(m_faintWindow, s, v, from_control(false));
  }

  void Set(const IntSetting& s, IntSetting::ValueType v) override{
    change_setting(m_faintWindow, s, v, from_control(false));
  }

  void Set(const ColorSetting& s, ColorSetting::ValueType v) override{
    change_setting(m_faintWindow, s, v, from_control(false));
  }

  void Set(const FloatSetting& s, FloatSetting::ValueType v) override{
    change_setting(m_faintWindow, s, v, from_control(false));
  }

  void SetActiveCanvas(const CanvasId& id) override{
    return m_faintWindow.SetActiveCanvas(id);
  }

  void SetDefaultGrid(const Grid& grid) override{
    m_defaultGrid = grid;
  }

  void SetDefaultResizeDialogOptions(const ResizeDialogOptions& opts) override{
    m_defaultResizeSettings = opts;
  }

  void SetInterpreterBackground(const ColRGB& c) override{
    m_interpreterFrame.SetBackgroundColor(c);
  }

  void SetInterpreterTextColor(const ColRGB& c) override{
    m_interpreterFrame.SetTextColor(c);
  }

  void SetPalette(const PaintMap& paintMap) override{
    m_faintWindow.SetPalette(paintMap);
  }

  void SetTransparencyStyle(const TransparencyStyle& style) override{
    m_transparencyStyle = style;
    m_faintWindow.Refresh();
  }

  void SetLayer(Layer layer) override{
    m_faintWindow.SelectLayer(layer);
  }

  void ModalFull(const dialog_func& show_dialog) override{
    Canvas& canvas = m_faintWindow.GetActiveCanvas();
    DialogFeedbackImpl feedback(canvas);
    BeginModalDialog();
    Optional<Command*> maybeCmd = show_dialog(m_faintWindow.GetRawFrame(), feedback, canvas);
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
    auto maybeCmd = show_dialog(m_faintWindow.GetRawFrame(), feedback);
    EndModalDialog();

    maybeCmd.Visit(
      [&](BitmapCommand* cmd){
        canvas.RunCommand(context_targetted(cmd, canvas));
        canvas.Refresh();
      },
      [&](){
        canvas.Refresh();
      });
  }

  void ToggleHelpFrame() override{
    toggle_top_level_window(m_helpFrame);
  }

  void TogglePythonConsole() override{
    toggle_top_level_window(m_interpreterFrame);
  }

  void ShowColorPanel(bool show) override{
    m_faintWindow.ShowColorPanel(show);
  }

  void ShowPythonConsole() override{
    m_interpreterFrame.Show();
    m_interpreterFrame.Raise();
  }

  void ShowStatusbar(bool show) override{
    m_faintWindow.ShowStatusbar(show);
  }

  void ShowToolPanel(bool show) override{
    m_faintWindow.ShowToolPanel(show);
  }

  int TabletGetCursor() override{
    return m_tabletCursor;
  }

  // Note: Not an override, used directly by FaintWindow
  void TabletSetCursor(int tabletCursor){
    m_tabletCursor = tabletCursor;
  }

  void ToggleFullScreen(bool fullScreen) override{
    m_faintWindow.FullScreen(fullScreen);
  }

  void ToggleMaximize() override{
    m_faintWindow.Maximize(!m_faintWindow.IsMaximized());
  }

  void UpdateShownSettings() override{
    m_faintWindow.UpdateShownSettings();
  }

  void UpdateToolSettings(const Settings& s) override{
    m_faintWindow.UpdateToolSettings(s);
  }

  bool FloatingWindowFocused() const{ // Non-virtual <- Fixme: remove
    return m_helpFrame.HasFocus() ||
      m_interpreterFrame.HasFocus();
  }

  void SetTabCtrl(TabCtrl* tabControl){ // Non virtual
    // Fixme: Remove this method.
    // Added to make it possible to remove GetTabCtrl from FaintWindow,
    // (the initialization order of AppContext and panels
    // are a mess).
    m_tabControl.reset(tabControl);
  }

private:
  FaintDialogContext m_dialogContext;
  FaintWindow& m_faintWindow;
  HelpFrame& m_helpFrame;
  InterpreterFrame& m_interpreterFrame;
  bool m_modalDialog;
  SBInterface m_statusbar;
  Grid m_defaultGrid;
  ResizeDialogOptions m_defaultResizeSettings;
  TransparencyStyle m_transparencyStyle;
  int m_tabletCursor;
  dumb_ptr<TabCtrl> m_tabControl;
};

} // namespace

#endif

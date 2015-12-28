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

#ifndef FAINT_APP_CONTEXT_HH
#define FAINT_APP_CONTEXT_HH
#include <vector>
#include <functional>
#include "gui/command-dialog.hh"
#include "gui/resize-dialog-settings.hh"
#include "gui/transparency-style.hh"
#include "tools/tool-id.hh"
#include "util/common-fwd.hh"
#include "util/distinct.hh"
#include "util/id-types.hh"
#include "util/setting-id.hh"

namespace faint{
class AppContext;
typedef Distinct<bool, AppContext, 0> change_tab;
typedef Distinct<bool, AppContext, 1> bind_global;

class AppContext {
public:
  virtual ~AppContext(){}
  virtual void AddFormat(Format*) = 0;
  virtual void AddPaletteColor(const Color&) = 0;
  virtual void BeginModalDialog() = 0;
  virtual void BeginTextEntry() = 0;

  virtual void Bind(const KeyPress&, const bind_global&) = 0; // Fixme: Move to Python-interfacing class.
  virtual bool Bound(const KeyPress&) const = 0; // Fixme: Move to Python-interfacing class.
  virtual bool BoundGlobal(const KeyPress&) const = 0; // Fixme: Move to Python-interfacing class.
  virtual void Close(Canvas&) = 0;
  virtual void DialogOpenFile() = 0;
  virtual void DialogSaveAs(Canvas&) = 0;
  virtual void EndModalDialog() = 0;
  virtual void EndTextEntry() = 0;
  virtual bool Exists(const CanvasId&) = 0;
  virtual BoolSetting::ValueType Get(const BoolSetting&)=0;
  virtual StrSetting::ValueType Get(const StrSetting&)=0;
  virtual IntSetting::ValueType Get(const IntSetting&)=0;
  virtual ColorSetting::ValueType Get(const ColorSetting&)=0;
  virtual FloatSetting::ValueType Get(const FloatSetting&)=0;
  virtual Canvas& GetActiveCanvas() = 0;
  virtual Tool* GetActiveTool() = 0;
  virtual Canvas& GetCanvas(int) = 0;
  virtual int TabletGetCursor() = 0;
  virtual int GetCanvasCount() const = 0;
  virtual Grid GetDefaultGrid() const = 0;
  virtual ImageInfo GetDefaultImageInfo() = 0;
  virtual ResizeDialogSettings GetDefaultResizeDialogSettings() const = 0;
  virtual std::vector<Format*> GetFormats() = 0;
  virtual Layer GetLayerType() const = 0;
  virtual IntPoint GetMousePos() = 0;
  virtual ToolId GetToolId() = 0;
  virtual Settings GetToolSettings() const = 0;
  virtual const TransparencyStyle& GetTransparencyStyle() const = 0;
  virtual bool IsFullScreen() const = 0;
  virtual Canvas* Load(const FilePath&, const change_tab&) = 0;
  virtual void Load(const FileList&) = 0;
  virtual Canvas* LoadAsFrames(const FileList&, const change_tab&) = 0;
  virtual void Maximize() = 0;
  virtual void MaximizeInterpreter() = 0;
  virtual bool ModalDialogShown() const = 0;
  virtual Canvas& NewDocument(const ImageInfo&) = 0;
  virtual Canvas& NewDocument(ImageProps&&) = 0;
  virtual void NextTab() = 0;
  virtual void PreviousTab() = 0;

  // Adds ... to the interpreter
  virtual void PythonContinuation() = 0; // Fixme: Move to Python-interfacing class.

  // Signal that a read-eval-print or function evaluation in
  // python is complete, e.g. to perform refresh etc. after
  // batched drawing
  virtual void PythonDone() = 0; // Fixme: Move to Python-interfacing class.

  // Tell the interpreter to get a key from the user
  virtual void PythonGetKey() = 0;

  // Interface printing
  virtual void PythonIntFaintPrint(const utf8_string&) = 0;

  // Add a new prompt-line to the interpreter.
  virtual void PythonNewPrompt() = 0; // Fixme: Move to Python-interfacing class.

  // print e.g. for debugging
  virtual void PythonPrint(const utf8_string&) = 0;

  // Flag the canvas as requiring a refresh, but save it until PythonDone
  // Done automatically by PythonRunCommand, so this can be used for non-command
  // changes like selections.
  virtual void PythonQueueRefresh(Canvas*) = 0; // Fixme: Move to Python-interfacing class.

  // Run a Faint-command from a Python interface function
  virtual void PythonRunCommand(Canvas*, Command*) = 0; // Fixme: Move to Python-interfacing class.
  // Run a Faint-command from a Python interface function
  virtual void PythonRunCommand(Canvas*, Command*, const FrameId&) = 0; // Fixme: Move to Python-interfacing class.

  // Queues the files for loading, but returns immediately
  virtual void QueueLoad(const FileList&) = 0;
  virtual void Quit() = 0;
  virtual void RaiseFrame() = 0;
  virtual bool Save(Canvas&) = 0;
  virtual void SelectTool(ToolId) = 0;
  virtual void Set(const BoolSetting&, BoolSetting::ValueType)=0;
  virtual void Set(const StrSetting&, StrSetting::ValueType)=0;
  virtual void Set(const IntSetting&, IntSetting::ValueType)=0;
  virtual void Set(const ColorSetting&, ColorSetting::ValueType)=0;
  virtual void Set(const FloatSetting&, FloatSetting::ValueType)=0;
  virtual void SetActiveCanvas(const CanvasId&) = 0;
  virtual void SetDefaultGrid(const Grid&) = 0;
  virtual void SetDefaultResizeDialogSettings(const ResizeDialogSettings&) = 0;
  virtual void SetInterpreterBackground(const ColRGB&) = 0;
  virtual void SetInterpreterTextColor(const ColRGB&) = 0;
  virtual void SetLayer(Layer) = 0;
  virtual void SetPalette(const PaintMap&) = 0;
  virtual void SetTransparencyStyle(const TransparencyStyle&) = 0;
  virtual void ModalFull(const dialog_func&) = 0;
  virtual void Modal(const bmp_dialog_func&) = 0;
  virtual void ShowPythonConsole() = 0;

  virtual void ToggleColorbar(bool) = 0;
  virtual void ToggleFullScreen(bool) = 0;
  virtual void ToggleHelpFrame() = 0;
  virtual void ToggleMaximize() = 0;
  virtual void TogglePythonConsole() = 0;
  virtual void ToggleToolPanel(bool) = 0;
  virtual void ToggleStatusbar(bool) = 0;

  virtual void Unbind(const KeyPress&) = 0; // Fixme: Move to Python-interfacing class.

  // Shows the settings and values from the active tool in the UI
  // setting controls
  virtual void UpdateShownSettings() = 0;

  // Makes the given settings the application-wide settings
  virtual void UpdateToolSettings(const Settings&) = 0;
};

} // namespace
#endif

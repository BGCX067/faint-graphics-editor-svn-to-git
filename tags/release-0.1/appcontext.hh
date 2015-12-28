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

#ifndef FAINT_APPCONTEXT_HH
#define FAINT_APPCONTEXT_HH
#include <vector>
#include "toolid.hh"
#include "util/idtypes.hh"
#include "settings.hh"
#include "tools/settingid.hh"
#include "canvasinterface.hh"

class Format;
class Command;
class ToolBehavior;

class StatusInterface {
public:
  virtual void SetMainText( const std::string& ) = 0;
  virtual void SetText( const std::string& text, int field=0 ) = 0;
};

class AppContext {
public:
  virtual ~AppContext(){}
  virtual void DialogOpenFile() = 0;

  virtual CanvasInfo GetDefaultCanvasInfo() = 0;
  virtual CanvasInterface& NewDocument( size_t w, size_t h ) = 0; // Fixme: Pointers?
  virtual CanvasInterface& NewDocument( const CanvasInfo& ) = 0;
  virtual CanvasInterface* Load( const char*, bool changeTab=true ) = 0;

  virtual CanvasInterface& GetActiveCanvas() = 0;
  virtual void Close( CanvasInterface& ) = 0;

  // Shows the settings and values from the active tool in the UI
  // setting controls
  virtual void UpdateShownSettings() = 0;

  // Makes the given settings the application-wide settings
  virtual void UpdateToolSettings( const FaintSettings& ) = 0;  
  virtual void SelectTool( ToolId id ) = 0;
  virtual ToolId GetToolId() = 0;
  virtual ToolBehavior* GetActiveTool() = 0;
  virtual void Set( const BoolSetting&, BoolSetting::ValueType )=0;
  virtual void Set( const StrSetting&, StrSetting::ValueType)=0;
  virtual void Set( const IntSetting&, IntSetting::ValueType )=0;
  virtual void Set( const ColorSetting&, ColorSetting::ValueType )=0;
  virtual void Set( const FloatSetting&, FloatSetting::ValueType )=0;
  virtual BoolSetting::ValueType Get( const BoolSetting& )=0;
  virtual StrSetting::ValueType Get( const StrSetting&)=0;
  virtual IntSetting::ValueType Get( const IntSetting& )=0;
  virtual ColorSetting::ValueType Get( const ColorSetting& )=0;
  virtual FloatSetting::ValueType Get( const FloatSetting& )=0;
  virtual void SetLayer( Layer ) = 0;
  virtual Layer GetLayerType() const = 0;
  virtual const FaintSettings& GetToolSettings() const = 0;  
  virtual size_t GetCanvasCount() const = 0;
  virtual CanvasInterface& GetCanvas( size_t i ) = 0;
  virtual bool Exists( const CanvasId& ) = 0;
  virtual Point GetMousePos() = 0;
  virtual StatusInterface& GetStatusInfo() = 0;
  virtual void BeginTextEntry() = 0;
  virtual void EndTextEntry() = 0;
  virtual void Quit() = 0;
  virtual void OnDocumentChange() = 0;
  virtual void OnDocumentStateChange() = 0;
  virtual void OnZoomChange() = 0;
  virtual void AddFormat( Format* ) = 0;
  virtual std::vector<Format*> GetFormats() = 0;
  virtual void SetInterpreterBackground( const faint::Color& ) = 0;
  virtual void SetInterpreterTextColor( const faint::Color& ) = 0;
  virtual void MaximizeInterpreter() = 0;
  virtual void Maximize() = 0;
  virtual void AddPaletteColor( const faint::Color& ) = 0;

  // Fixme: Move the following to a Python-interfacing class.
  virtual bool Bound( int key, int modifiers ) const = 0;
  virtual void Bind( int key, int modifiers ) = 0;
  virtual void Unbind( int key, int modifiers ) = 0;

  // Run a Faint-command from a Python interface function
  virtual void PythonRunCommand( CanvasInterface*, Command* ) = 0;

  // Signal that a read-eval-print or function evaluation in
  // python is complete, e.g. to perform refresh etc. after
  // batched drawing
  virtual void PythonDone() = 0;

  // Add a new prompt-line to the interpreter.
  virtual void PythonNewPrompt() = 0;

  // Add ... to the interpreter
  virtual void PythonContinuation() = 0;

  // Tell the interpreter to get a key from the user
  virtual void PythonGetKey() = 0;

  // print e.g. for debugging
  virtual void PythonPrint( const std::string& ) = 0;

  // Interface printing
  virtual void PythonIntFaintPrint( const std::string& ) = 0;

  // Temporary hack while deleting MainFrame::m_activeTool in
  // MainFrame destructor causes a crash on XP
  virtual void DeleteOnClose( ToolBehavior* ) = 0;
};

#endif

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

#ifndef FAINT_COMMAND_DIALOG
#define FAINT_COMMAND_DIALOG
#include "commands/command.hh"

class wxWindow;
class Command;

class DialogFeedback{
public:
  virtual ~DialogFeedback(){}
  // Get a bitmap for feedback
  virtual faint::Bitmap& GetBitmap() = 0;
  // Fixme: Preferably remove, pass the minimum required stuff to the
  // individual CommandDialog constructors
  virtual CanvasInterface& GetCanvas() = 0;

  // Notify that the bitmap has been changed
  virtual void Update() = 0;
};

class CommandDialog{
public:
  virtual ~CommandDialog(){}
  virtual Command* GetCommand() = 0;
  virtual bool ShowModal(wxWindow*, DialogFeedback&) = 0;
private:
};

#endif

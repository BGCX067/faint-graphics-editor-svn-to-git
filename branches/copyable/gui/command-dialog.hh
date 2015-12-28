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

#ifndef FAINT_COMMAND_DIALOG_HH
#define FAINT_COMMAND_DIALOG_HH
#include <functional>
#include "commands/bitmap-cmd.hh"
#include "commands/command.hh"
#include "util/template-fwd.hh"

class wxWindow;

namespace faint{

class DialogFeedback{
  // Context for letting dialogs show feedback on a Bitmap
public:
  virtual ~DialogFeedback(){}

  // Returns a bitmap for drawing feedback.
  virtual Bitmap& GetBitmap() = 0;

  virtual bool HasSelection() const = 0;

  // Notify that the bitmap has been changed
  virtual void Update() = 0;
};

class CommandDialog{
  // Base class for dialogs that produce a command.
public:
  virtual ~CommandDialog(){}
  virtual Command* GetCommand() = 0;
  virtual bool ShowModal(wxWindow*, DialogFeedback&) = 0;
private:
};

class BitmapCommandDialog{
  // Base class for more generalized command dialogs, which apply
  // functions/filters that only change the pixel content of bitmaps.
public:
  virtual ~BitmapCommandDialog(){}
  virtual BitmapCommand* GetCommand() = 0;
  virtual bool ShowModal(wxWindow*, DialogFeedback& ) = 0;
};

typedef std::function<Optional<BitmapCommand*>(wxWindow&, DialogFeedback&)> bmp_dialog_func;

typedef std::function<Optional<Command*>(wxWindow&, DialogFeedback&, Canvas&)> dialog_func;

} // namespace

#endif

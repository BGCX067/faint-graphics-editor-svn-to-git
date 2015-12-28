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

#ifndef FAINT_SET_BITMAP_CMD_HH
#define FAINT_SET_BITMAP_CMD_HH
#include "commands/command.hh"
#include "geo/tri.hh"
#include "objects/objraster.hh"

class SetObjectBitmapCommand : public Command {
public:
  SetObjectBitmapCommand(ObjRaster*, const faint::Bitmap&, const Tri&, const std::string& name );
  void Do( CommandContext& );
  std::string Name() const;
  void Undo( CommandContext& );
private:
  SetObjectBitmapCommand& operator=( const SetObjectBitmapCommand& );
  ObjRaster* m_object;
  faint::Bitmap m_bitmap;
  faint::Bitmap m_oldBitmap;
  const Tri m_tri;
  const Tri m_oldTri;
  std::string m_name;
};

class SetBitmapCommand : public Command {
public:
  // Sets the image bitmap and offsets any objects using topLeft.
  SetBitmapCommand(const faint::Bitmap&, const IntPoint& topLeft, const std::string& name);
  void Do( CommandContext& );
  void DoRaster( CommandContext& );
  std::string Name() const;
  void Undo( CommandContext& );
private:
  faint::Bitmap m_bitmap;
  std::string m_name;
  IntPoint m_topLeft;
};
#endif

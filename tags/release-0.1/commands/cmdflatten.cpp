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

#include "cmdflatten.hh"
#include "faintimage.hh"
#include "faintdc.hh"
#include "objects/object.hh"

CommandType FlattenCommand::commandType( CMD_TYPE_RASTER );

FlattenCommand::FlattenCommand( Object* object )
  : Command(FlattenCommand::commandType)
{
  m_object = object;
}

void FlattenCommand::Do( faint::Image& image ){
  FaintDC dc( image.GetBitmapRef() );
  m_object->Draw( dc );
}


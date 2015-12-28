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

#include "commandutil.hh"
#include "objects/objraster.hh"
#include "commands/addobjectcommand.hh"
#include "commands/tricommand.hh"
#include "commands/commandbunch.hh"
#include "commands/cmdflatten.hh"
#include "commands/delobjectcommand.hh"
#include "commands/cmdcropobject.hh"

Command* PerhapsBunch( CommandType type, std::vector<Command*> commands){
  assert( !commands.empty() );
  return ( commands.size() == 1 ) ?
    commands.front() :
    new CommandBunch( type, commands );
}

Command* PerhapsBunch( CommandType type, std::deque<Command*> commands){
  assert( !commands.empty() );
  return ( commands.size() == 1 ) ?
    commands.front() :
    new CommandBunch( type, std::vector<Command*>( commands.begin(), commands.end() ) );
}

Command* GetRotateCommand( const std::vector<Object*>& objects, const faint::radian angle, const Point& origin ){
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* object = objects[i];
    const Tri& tri = object->GetTri();
    commands.push_back( new TriCommand( object, Rotated( tri, angle, origin ), tri ) );
  }
  return PerhapsBunch( CMD_TYPE_OBJECT, commands );
}

Command* GetScaleCommand( const std::vector<Object*>& objects, faint::coord scale_x, faint::coord scale_y, const Point& origin ){
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* obj = objects[i];
    const Tri& tri = obj->GetTri();
    commands.push_back( new TriCommand( obj,
        Scaled( tri, scale_x, scale_y, origin ), tri ) );
  }
  return PerhapsBunch( CMD_TYPE_OBJECT, commands );
}

Command* GetFlattenCommand( const std::vector<Object*>& objects, const faint::Image& image ){
  std::deque<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* obj = objects[i];
    commands.push_back( new FlattenCommand( obj ) );
    commands.push_front( new DelObjectCommand( obj, image.GetObjectZ( obj ) ) );
  }
  return new CommandBunch( CMD_TYPE_HYBRID, std::vector<Command*>(commands.begin(), commands.end() ) );
}

Command* GetCropCommand( std::vector<Object*>& objects ){
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Command* cmd = CropOneObject( objects[i] );
    if ( cmd != 0 ){
      commands.push_back( cmd );
    }
  }

  if ( commands.empty() ){
    // No object supported autocrop
    return 0;
  }
  return PerhapsBunch( CMD_TYPE_OBJECT, commands );
}

Command* GetAddObjectCommand( Object* object ){
  return new AddObjectCommand( object );
}

Command* GetAddObjectsCommand( const std::vector<Object*>& objects ){
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    commands.push_back( new AddObjectCommand( objects[i] ) );
  }
  return PerhapsBunch( CMD_TYPE_OBJECT, commands );
}

Command* GetDeleteObjectsCommand( const std::vector<Object*>& objects, const faint::Image& image ){
  std::vector<Command*> commands;

  // Note the reverse iteration, objects must be deleted in reverse
  // order so that the Z-order is preserved
  for ( int i = objects.size() - 1; i >= 0; i-- ){
    Object* object = objects[i];
    commands.push_back( new DelObjectCommand( object, image.GetObjectZ( object ) ) );
  }
  return PerhapsBunch( CMD_TYPE_OBJECT, commands );
}

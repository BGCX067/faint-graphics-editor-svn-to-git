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

#ifndef FAINT_COMMANDUTIL_HH
#define FAINT_COMMANDUTIL_HH
#include <vector>
#include <deque>
#include "geo/geotypes.hh"
#include "commands/command.hh"
#include "faintimage.hh"

Command* PerhapsBunch( CommandType, std::vector<Command*> );
Command* PerhapsBunch( CommandType, std::deque<Command*> );
Command* GetRotateCommand( const std::vector<Object*>&, const faint::radian angle, const Point& origin );
Command* GetScaleCommand( const std::vector<Object*>&, faint::coord scale_x, faint::coord scale_y, const Point& origin );
Command* GetFlattenCommand( const std::vector<Object*>&, const faint::Image& );
Command* GetAddObjectCommand( Object* );
Command* GetAddObjectsCommand( const std::vector<Object*>& );
Command* GetDeleteObjectsCommand( const std::vector<Object*>&, const faint::Image& ); 

// Returns a command which crops any croppable objects passed in or 0
// if no object could be cropped.
Command* GetCropCommand( std::vector<Object*>& );

#endif

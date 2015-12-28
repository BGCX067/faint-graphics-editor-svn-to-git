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

#ifndef FAINT_DELETECOMMAND_HH
#define FAINT_DELETECOMMAND_HH
#include "command.hh"

class DeleteCommand : public Command{
public:
  DeleteCommand( const IntRect&, const faint::Color& );  
  void Do( faint::Image& );
private:
  IntRect m_rect;
  faint::Color m_color;
};

#endif

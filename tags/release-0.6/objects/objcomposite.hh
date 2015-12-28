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

#ifndef FAINT_OBJCOMPOSITE_HH
#define FAINT_OBJCOMPOSITE_HH
#include "object.hh"

enum Ownership{ OWNER, // ObjComposite should delete contained objects on destruction
  LOANER // ...should not
};

class ObjComposite : public Object{
public:
  ObjComposite( const objects_t&, Ownership );
  ~ObjComposite();
  Object* Clone() const;
  void Draw( FaintDC& );
  void DrawMask( FaintDC& );
  std::vector<Point> GetSnappingPoints() const;
  std::vector<Point> GetAttachPoints() const;
  Object* GetObject( int );
  const Object* GetObject( int ) const;
  size_t GetObjectCount() const;
  IntRect GetRefreshRect();
private:
  ObjComposite( const ObjComposite& ); // For Clone
  objects_t m_objects;
  Tri m_origTri;
  std::vector<Tri> m_objTris;
  Ownership m_ownership;
};

#endif

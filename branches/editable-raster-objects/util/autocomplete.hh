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

#ifndef FAINT_AUTOCOMPLETE_HH
#define FAINT_AUTOCOMPLETE_HH
#include <string>
#include <vector>

namespace faint {
class WordsImpl;

class Words{
public:
  Words();
  Words( const Words& other );
  ~Words();
  Words& operator=( const Words& other );
  size_t size() const;
  std::string get( size_t i ) const;
  void clear();

private:
  Words( WordsImpl* m_impl );
  WordsImpl* m_impl;
  friend class AutoComplete;
};

class ACNode;
class AutoComplete{
public:
  AutoComplete();
  ~AutoComplete();
  void add( const std::string& word );
  Words match( const std::string& partial );
private:
  // Prevent assignment and copy
  AutoComplete( const AutoComplete& );
  AutoComplete& operator=( const AutoComplete& );
  std::vector< ACNode* > m_nodes;
};

} // Namespace

#endif

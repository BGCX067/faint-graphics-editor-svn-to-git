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

#ifndef FAINT_AUTO_COMPLETE_HH
#define FAINT_AUTO_COMPLETE_HH
#include <string>
#include <vector>

namespace faint {
class WordsImpl;

class Words{
  // A list of words retrieved from an AutoComplete object by matching
  // a partial word.
public:
  Words();
  Words(const Words&);
  ~Words();
  Words& operator=(const Words&);
  void clear();
  std::string get(size_t) const;
  size_t size() const;
private:
  Words( WordsImpl* m_impl );
  WordsImpl* m_impl;
  friend class AutoComplete;
};

class ACNode;
class AutoComplete{
  // Dictionary for auto-completion of words.
public:
  AutoComplete();
  ~AutoComplete();

  // Add a word to recognize in match()
  void add( const std::string& word );

  // Get all words matching the string
  Words match( const std::string& );

  AutoComplete( const AutoComplete& ) = delete;
  AutoComplete& operator=( const AutoComplete& ) = delete;
private:
  std::vector<ACNode*> m_nodes;
};

} // Namespace

#endif

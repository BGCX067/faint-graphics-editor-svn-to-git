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

#include "autocomplete.hh"
#include <algorithm>
#include <cassert>

namespace faint {
  class ACNode;
  struct ACNodeLess{
    bool operator()( const ACNode* lhs, const ACNode* rhs ) const;
  };

  class WordsImpl{
  public:
    ACNode* m_node;
    std::string m_base;
  };


  /* ACNode implementation */
  class ACNode {
  public:
    ACNode( char c ){
      m_char = c;
    }

    ~ACNode(){
      for ( size_t i = 0; i != m_children.size(); i++ ){
        delete m_children[i];
      }
    }

    void extend( const std::string& word ){
      if ( !word.empty() ){
        ACNode* node = add( word[0] );
        node->extend( word.substr( 1, word.size() - 1 ) );
      }
      else {
        add( '.' );
      }
    }

    ACNode* add( char c ){
      for ( size_t i = 0; i != m_children.size(); i++ ){
        if ( m_children[i]->m_char == c ){
          return m_children[i];
        }
      }
      ACNode* node = new ACNode( c );
      m_children.push_back( node );
      sort( m_children.begin(), m_children.end(), ACNodeLess() );
      return node;
    }

    ACNode* find( const std::string& str ){
      if ( str.size() == 1 && str[0] == m_char ){
        return this;
      }
      if ( str.size() > 1 ){
        if ( m_char != str[0] ){
          return 0;
        }
        for ( size_t i = 0; i != m_children.size(); i++ ){
          ACNode* node = m_children[i]->find( str.substr(1, str.size() - 1 ) );
          if ( node != 0 ){
            return node;
          }
        }
      }
      return 0;
    }

    void all_words( std::vector<std::string>& collect, const std::string& prepend ){
      if ( m_children.empty() ){
        collect.push_back( prepend );
        return;
      }
      else {
        for ( size_t i = 0; i != m_children.size(); i++ ){
          std::vector<std::string> subwords;
          m_children[i]->all_words( subwords, prepend + m_char );
          for ( size_t i = 0; i != subwords.size(); i++ ){
            collect.push_back( subwords[i] );
          }
        }
      }
    }

    bool operator<( const ACNode& other ) const {
      return other.m_char < m_char;
    }

    char m_char;
    std::vector<ACNode*> m_children;
  };


  /* AutoComplete implementation */
  AutoComplete::AutoComplete(){
  }

  AutoComplete::~AutoComplete(){
    for ( size_t i = 0; i != m_nodes.size(); i++ ){
      delete m_nodes[i];
    }
  }

  void AutoComplete::add( const std::string& word ){
    for ( size_t i = 0; i != m_nodes.size(); i++ ){
      if ( m_nodes[i]->m_char == word[0] ){
        m_nodes[i]->extend( word.substr( 1, word.size() - 1 ) );
        return;
      }
    }

    ACNode* node = new ACNode( word[0] );
    m_nodes.push_back( node );
    node->extend( word.substr(1, word.size() - 1 ) );
  }

  Words AutoComplete::match( const std::string& str ){
    WordsImpl* w = new WordsImpl();
    w->m_node = 0;
    for ( size_t i = 0; i != m_nodes.size(); i++ ){
      ACNode* node = m_nodes[i];
      if ( node->m_char == str[0] ){
        ACNode* found = node->find(str );
        if ( found != 0 ){
          w->m_node = found;
          w->m_base = str;
          break;
        }
      }
    }
    return Words( w );
  }

  bool ACNodeLess::operator()( const ACNode* lhs, const ACNode* rhs ) const {
    return *lhs < *rhs;
  }

  Words::Words(){
    m_impl = new WordsImpl();
    m_impl->m_node = 0;
  }

  /* Words implementation */
  Words::Words( WordsImpl* impl )
    : m_impl( impl )
  {}

  Words::~Words(){
    delete m_impl;
  }

  Words::Words( const Words& other ){
    m_impl = new WordsImpl();
    m_impl->m_node = other.m_impl->m_node;
    m_impl->m_base = other.m_impl->m_base;
  }

  Words& Words::operator=( const Words& other ){
    if ( &other == this ){
      return *this;
    }
    m_impl = new WordsImpl();
    m_impl->m_node = other.m_impl->m_node;
    m_impl->m_base = other.m_impl->m_base;
    return *this;

  }

  size_t Words::size() const{
    if ( m_impl->m_node == 0 ){
      return 0;
    }
    std::vector<std::string> words;
    const std::string& base = m_impl->m_base;
    m_impl->m_node->all_words( words, base.substr(0, base.size() - 1 ) );
    return words.size();
  }

  std::string Words::get( size_t i ) const{
    assert( m_impl->m_node != 0 );
    std::vector<std::string> words;
    const std::string& base = m_impl->m_base;
    m_impl->m_node->all_words( words, base.substr(0, base.size() - 1 ) );
    return words[i];
  }

  void Words::clear(){
    m_impl->m_base = "";
    m_impl->m_node = 0;
  }

} // Namespace

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

#include <algorithm>
#include <string>
#include "geo/size.hh"
#include "rendering/faintdc.hh"
#include "util/split_string.hh"

inline faint::coord getw( FaintDC& dc, const faint::utf8_string& str, const Settings& settings ){
  Size sz(dc.TextExtents( str, settings ) );
  return sz.w;
}

#ifdef DEBUG_SPLIT_STR
faint::utf8_char wordBrk('>');
faint::utf8_char hardBrk(faint::pilcrow_sign);
faint::utf8_char softBrk(faint::downwards_arrow_with_tip_leftwards); // Fixme: Typically not available in simpler fonts
#else
faint::utf8_char wordBrk(faint::space);
faint::utf8_char hardBrk(faint::space);
faint::utf8_char softBrk(faint::space);
#endif

std::pair<bool, faint::utf8_string> hard_break( const faint::utf8_string& str ){
  return std::make_pair( true, str );
}

std::pair<bool, faint::utf8_string> soft_break( const faint::utf8_string& str ){
  return std::make_pair( false, str );
}

faint::utf8_string split_word( const faint::utf8_string& string, text_lines_t& result ){
  // wxArrayInt widths;
  // GetPartialTextExtents broken for gcdc?
  // (All fields set to 0)
  /* bool ok = dc.GetPartialTextExtents( string, widths );
  assert( widths.size() > 0 );
  assert( ok );*/

  // <- The old Commented stuff apparently used wxGCDC, and did not
  // work. We use a FaintDC now, so something similar to the above
  // should be done for that.  Also some hyphenization could be
  // cool. BUT: Just break the word in two for now!
  result.push_back( soft_break( string.substr(0, string.size() / 2) + wordBrk ) );
  return string.substr( string.size() / 2, string.size() - string.size() / 2 );
}

void split_line( FaintDC& dc, const faint::utf8_string& string, faint::coord maxW, text_lines_t& result, const Settings& settings ){
  size_t wordStart = 0;
  size_t wordEnd = 0;

  faint::utf8_string line;
  do {
    wordEnd = string.find( faint::utf8_char(" "), wordStart );
    if ( wordEnd == std::string::npos ){
      wordEnd = string.size();
    }
    faint::utf8_string word = string.substr( wordStart, wordEnd - wordStart  );

    if ( !line.empty() && getw( dc, line + faint::space + word, settings ) > maxW ){
      result.push_back(soft_break( line + softBrk ));
      line.clear();
    }

    if ( getw( dc, word, settings ) > maxW ) {
      word = split_word( word, result );
    }

    if ( !line.empty() ){
      line += faint::space;
    }

    line += word;
    wordStart = wordEnd + 1;
  } while ( wordEnd != string.size() );

  if ( line.size() > 1 ){
    result.push_back( soft_break(line + softBrk) );
  }
}

text_lines_t split_str( FaintDC& dc, const faint::utf8_string& string, faint::coord maxW, const Settings& settings ){
  size_t lineEnd = 0;
  size_t lineStart = 0;

  text_lines_t result;
  do {
    lineEnd = string.find( faint::eol, lineStart );
    bool softBreak = lineEnd == std::string::npos;
    if ( softBreak ){
      lineEnd = string.size();
    }

    if ( getw( dc, string.substr( lineStart, lineEnd - lineStart ), settings ) < maxW ){
      if ( softBreak ){
        result.push_back( soft_break( string.substr( lineStart, lineEnd - lineStart ) + hardBrk ) );
      }
      else {
        result.push_back( hard_break( string.substr( lineStart, lineEnd - lineStart ) + hardBrk ) );
      }
    }
    else {
      split_line( dc, string.substr( lineStart, lineEnd - lineStart ), maxW, result, settings );
    }

    lineStart = lineEnd + 1;

  } while ( lineEnd != string.size() );

  return result;
}

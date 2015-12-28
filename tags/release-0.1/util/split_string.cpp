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

#include <string>
#include <algorithm>
#include "split_string.hh"
#include "faintdc.hh"

inline unsigned int getw( FaintDC& dc, const wxString& str, const FaintSettings& settings ){
  Size sz = dc.TextExtents( std::string( str.ToUTF8() ), settings );
  return sz.w;
}

#ifdef DEBUG_SPLIT_STR
char wordBrk = '>';
char hardBrk = '^';
char softBrk = '|';
#else
char wordBrk = ' ';
char hardBrk = ' ';
char softBrk = ' ';
#endif

std::pair<bool, wxString> hard_break( const wxString& str ){
  return std::make_pair( true, str );
}

std::pair<bool, wxString> soft_break( const wxString& str ){
  return std::make_pair( false, str );
}

wxString split_word( const wxString& string, text_lines_t& result ){
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

void split_line( FaintDC& dc, const wxString& string, size_t maxW, text_lines_t& result, const FaintSettings& settings ){
  size_t wordStart = 0;
  size_t wordEnd = 0;

  wxString line;
  do {
    wordEnd = string.find( " ", wordStart );
    if ( wordEnd == wxString::npos ){
      wordEnd = string.size();
    }
    wxString word = string.substr( wordStart, wordEnd - wordStart  );

    if ( !line.empty() && getw( dc, line + " " + word, settings ) > maxW ){
      result.push_back(soft_break( line + softBrk ));
      line.clear();
    }

    if ( getw( dc, word, settings ) > maxW ) {
      word = split_word( word, result );
    }

    if ( !line.empty() ){
      line += " ";
    }

    line += word;
    wordStart = wordEnd + 1;
  } while ( wordEnd != string.size() );

  if ( line.size() > 1 ){
    result.push_back( soft_break(line + softBrk) );
  }
}

text_lines_t split_str( FaintDC& dc, const wxString& string, unsigned int maxW, const FaintSettings& settings ){
  size_t lineEnd = 0;
  size_t lineStart = 0;

  text_lines_t result;
  do {
    lineEnd = string.find( "\n", lineStart );
    bool softBreak = lineEnd == wxString::npos;
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

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

#include <vector>
#include <stack>
#include <sstream>
#include "geo/basic.hh"
#include "util/parse-math-string.hh"

using std::string;

const string operators("%+-*/ ");
int precedence( const string& c ){
  if ( c == "-" || c == "+" ){
    return 1;
  }
  if ( c == "*" || c == "/" ){
    return 2;
  }
  return 0;
}

// Split the string into components of operators and operands
std::vector<string> tokenize( const string& s ){
  std::vector<string> tokens;
  string::size_type prevPos = 0;

  while ( prevPos != string::npos && prevPos < s.size() ){
    string::size_type pos = s.find_first_of(operators, prevPos);
    if ( pos == string::npos ){
      // No operator left before the end of the string.
      // Push the remainder of the string as a single token
      tokens.push_back( s.substr(prevPos, s.size() - prevPos) );
    }
    else {
      // Push the operand preceding the found operator, if any
      if ( pos - prevPos > 0 ){
        tokens.push_back( s.substr( prevPos, pos - prevPos ) );
      }
      if ( s[pos] != ' ' ){
        // Push the operator
        tokens.push_back(s.substr(pos, 1));
      }
    }
    prevPos = pos;
    if ( prevPos != string::npos ){
      prevPos += 1;
    }
  }
  return tokens;
}

std::vector<string> infix_to_postfix( const std::vector<string>& infix ){
  std::vector<string> postfix;
  std::stack<string> stack;

  for ( const std::string& token : infix ){
    if ( operators.find( token ) == string::npos ){
      postfix.push_back(token);
    }
    else if ( stack.empty() || precedence(stack.top()) < precedence(token) ){
      stack.push(token);
    }
    else {
      postfix.push_back(stack.top());
      stack.pop();
      stack.push(token);
    }
  }

  while (!stack.empty()){
    postfix.push_back( stack.top() );
    stack.pop();
  }
  return postfix;
}

double my_stof( const string& s ){
  std::stringstream ss(s);
  double d;
  ss >> d;
  return d;
}

double compute( std::vector<string> postfix ){
  std::stack<double> operands;
  for ( const std::string& token : postfix ){
    if ( operators.find(token) == string::npos ){
      operands.push(my_stof(token));
    }
    else {
      if ( operands.size() < 2 ){
        // Error - crap stack
        return -1;
      }
      double rhs = operands.top();
      operands.pop();
      double lhs = operands.top();
      operands.pop();
      if ( token == "-" ){
        operands.push( lhs - rhs );
      }
      if ( token == "+" ){
        operands.push( lhs + rhs );
      }
      if ( token == "/" ){
        operands.push( lhs / rhs );
      }
      if ( token == "*" ){
        operands.push( lhs * rhs );
      }
    }
  }
  if ( !operands.empty() ){
    return operands.top();
  }
  return 0;
}

int parse_math_string( const string& s, int originalValue ){
  std::vector<string> tokens = tokenize(s);
  bool previousWasOperator = false;
  for ( size_t i = 0; i != tokens.size(); i++ ){
    if ( tokens[i] == "%" && i != tokens.size() - 1 ){
      // Error: % inside equation
      return -1;
    }
    else if ( operators.find(tokens[i]) != string::npos ){
      if ( previousWasOperator ){
        // Error: Two operators in a row
        return -1;
      }
      previousWasOperator = true;
    }
    else {
      previousWasOperator = false;
    }
  }

  bool percentage = !tokens.empty() && tokens.back() == "%";
  if ( percentage ){
    tokens.pop_back();
  }

  std::vector<string> postfix = infix_to_postfix( tokens );
  double result = compute(postfix);
  if ( percentage ){
    return rounded((originalValue * result ) / 100);
  }
  return rounded(result);
}

#ifdef TEST_PARSE_VALUE
#include <iostream>

int main(){
  string eq1 = "1+21+255 + 7 - 12";
  std::cout << eq1 << std::endl;
  parse_math_string(eq1, 7, 12);

  std::cout << std::endl;
  string eq2 = "+21";
  std::cout << eq2 << std::endl;
  parse_math_string(eq2, 7, 12);

  std::cout << std::endl;
  string eq3 = "21%";
  std::cout << eq3 << std::endl;
  parse_math_string(eq3, 7, 12);

  std::cout << std::endl;
  string eq4 = "21";
  std::cout << eq4 << std::endl;
  parse_math_string(eq4, 7, 12);

  std::cout << std::endl;
  string eq5 = "2";
  std::cout << eq5 << std::endl;
  parse_math_string(eq5, 7, 12);

  std::cout << std::endl;
  string eq6 = "2 ++ 7";
  std::cout << eq6 << std::endl;
  parse_math_string(eq6, 7, 12);

  std::cout << std::endl;
  string eq7 = "2 % 7";
  std::cout << eq7 << std::endl;
  parse_math_string(eq7, 7, 12);

  std::cout << std::endl;
  string eq8 = "14 / 5";
  std::cout << eq8 << std::endl;
  parse_math_string(eq8, 7, 12);
}

#endif

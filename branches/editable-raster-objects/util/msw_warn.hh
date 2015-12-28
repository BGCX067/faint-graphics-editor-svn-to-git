#ifndef _WIN32
#error msw_warn.hh included on non-VC compiler
#endif

// Enables off-by-default compiler warnings in Visual Studio
// Included with /FI by compile_msw.py
#pragma warning(default:4061) // enumerator 'identifier' in a switch of enum 'enumeration' is not explicitly handled by a case label
#pragma warning(default:4062) // enumerator 'identifier' in a switch of enum 'enumeration' is not handled
#pragma warning(default:4191) // unsafe conversion from 'type of expression' to 'type required'
#pragma warning(default:4254) // unsafe conversion from 'type of expression' to 'type required'
#pragma warning(default:4263) // member function does not override any base class virtual member function
#pragma warning(default:4264) // no override available for virtual member function from base 'class'; function is hidden
#pragma warning(default:4265) // class has virtual functions, but destructor is not virtual
#pragma warning(default:4266) // no override available for virtual member function from base 'type'; function is hidden
#pragma warning(default:4302) // truncation from 'type 1' to 'type 2'
#pragma warning(default:4826) // conversion from 'type1' to 'type2' is sign-extended. This may cause unexpected runtime behavior
#pragma warning(default:4905) // wide string literal cast to 'LPSTR'
#pragma warning(default:4906) // string literal cast to 'LPWSTR'
#pragma warning(default:4928) // illegal copy-initialization; more than one user-defined conversion has been implicitly applied

// Produced too many warnings from wxWidgets (some old rev though)
// #pragma warning(default:4242) // conversion from 'type1' to 'type2', possible loss of data

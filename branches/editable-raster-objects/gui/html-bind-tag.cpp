#include "wx/wx.h"
#include "wx/html/m_templ.h"
#include "wx/html/htmlcell.h"
#include "python/pyinterface.hh"
#include "util/keycode.hh"
#include "util/pathutil.hh"

std::string get_bound_key( const std::string& function ){
  std::vector<faint::BindInfo> binds = faint::list_binds();
  for ( size_t i = 0; i != binds.size(); i++ ){
    if ( binds[i].function == function ){
      const faint::BindInfo& b(binds[i]);
      return key::as_text(b.key, b.modifiers);
    }
  }
  return "";
}

// This adds the tag <BIND> to wxHtml.
// When parsed, I look up if the content in <bind> matches
// a Python-function bound to a key, e.g. <bind>zoom_in</bind>,
// and if so, list the key.
TAG_HANDLER_BEGIN(BIND, "BIND")
  TAG_HANDLER_CONSTR(BIND) { }

  TAG_HANDLER_PROC(tag){
    wxString str = m_WParser->GetInnerSource(tag);
    std::string key = get_bound_key(std::string(str));
    if ( !key.empty() ){
      m_WParser->GetContainer()->InsertCell( new wxHtmlWordCell("(Key: "+key+ ")", wxScreenDC()));
    }
    return true; // Eat inner content
  }
TAG_HANDLER_END(BIND)

// The <BINDLIST> tag
TAG_HANDLER_BEGIN(BINDLIST, "BINDLIST")
  TAG_HANDLER_CONSTR(BINDLIST){}

  TAG_HANDLER_PROC(tag){
    std::vector<faint::BindInfo> binds = faint::list_binds();
    m_WParser->GetInnerSource(tag); // Just to silence unused parameter warning for "tag".
    // Create a table of binds
    for ( size_t i = 0; i != binds.size(); i++ ){
      const faint::BindInfo& b(binds[i]);

      // A container for the key-text
      wxHtmlContainerCell* cell = m_WParser->OpenContainer();
      // Use 20% of the width for the key
      cell->SetWidthFloat(20, wxHTML_UNITS_PERCENT);
      cell->InsertCell( new wxHtmlWordCell(key::as_text(b.key, b.modifiers), wxScreenDC()) );
      m_WParser->CloseContainer();

      // A container for the function name
      cell = m_WParser->OpenContainer();
      // Use 80% of the width for the function name
      cell->SetWidthFloat(80, wxHTML_UNITS_PERCENT);
      cell->InsertCell( new wxHtmlWordCell(wxString(b.function), wxScreenDC()) );
      m_WParser->CloseContainer();
    }

    return true; // Eat inner content
  }
TAG_HANDLER_END(BINDLIST)

// The <FAINTCONFIGPATH> tag
TAG_HANDLER_BEGIN(CONFIG_PATH_TAG, "FAINTCONFIGPATH")
  TAG_HANDLER_CONSTR(CONFIG_PATH_TAG){}

  TAG_HANDLER_PROC(tag){
    std::string iniPath = get_ini_file_path();
    m_WParser->GetContainer()->InsertCell( new wxHtmlWordCell(iniPath, wxScreenDC() ) );
    m_WParser->GetInnerSource(tag); // Just to silence unused parameter warning for "tag".
    return true; // Eat inner content
  }
TAG_HANDLER_END(CONFIG_PATH_TAG)


// Add the custom Faint tags as a tag-module
TAGS_MODULE_BEGIN(HFAINT)
TAGS_MODULE_ADD(BIND)
TAGS_MODULE_ADD(BINDLIST)
TAGS_MODULE_ADD(CONFIG_PATH_TAG)
TAGS_MODULE_END(HFAINT)

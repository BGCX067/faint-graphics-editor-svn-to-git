#include "textentrycommand.hh"
#include "objects/objtext.hh"

TextEntryCommand::TextEntryCommand( ObjText* textObj, const NewText& newText, const OldText& oldText )
  : Command( CMD_TYPE_OBJECT ),
    m_textObj( textObj ),
    m_old( oldText.Get() ),
    m_new( newText.Get() )
{}

void TextEntryCommand::Do( CommandContext& ){
  *(m_textObj->GetTextBuffer()) = TextBuffer( m_new );
}

std::string TextEntryCommand::Name() const{
  return "Edit Text";
}

void TextEntryCommand::Undo( CommandContext& ){
  *(m_textObj->GetTextBuffer()) = TextBuffer( m_old );
}

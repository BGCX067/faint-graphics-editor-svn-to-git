#define MAKE_FAINT_COMMAND_EVENT(NAME)const wxEventType NAME = wxNewEventType(); \
const wxEventTypeTag<wxCommandEvent> EVT_##NAME(NAME)

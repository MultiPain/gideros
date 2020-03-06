#include "event.h"
#include "eventvisitor.h"

Event::Type Event::ENTER_FRAME("enterFrame");
Event::Type Event::EXIT_FRAME("exitFrame");
Event::Type Event::SOUND_COMPLETE("soundComplete");
Event::Type Event::ADDED_TO_STAGE("addedToStage");
Event::Type Event::REMOVED_FROM_STAGE("removedFromStage");
//Event::Type Event::APPLICATION_DID_FINISH_LAUNCHING("applicationDidFinishLaunching");
//Event::Type Event::APPLICATION_WILL_TERMINATE("applicationWillTerminate");
Event::Type Event::MEMORY_WARNING("memoryWarning");
Event::Type Event::APPLICATION_START("applicationStart");
Event::Type Event::APPLICATION_EXIT("applicationExit");
Event::Type Event::APPLICATION_SUSPEND("applicationSuspend");
Event::Type Event::APPLICATION_RESUME("applicationResume");
Event::Type Event::APPLICATION_BACKGROUND("applicationBackground");
Event::Type Event::APPLICATION_FOREGROUND("applicationForeground");
Event::Type Event::APPLICATION_RESIZE("applicationResize");

void Event::apply(EventVisitor* v)
{
	v->visit(this);
}

int Event::s_uniqueid_ = 0;

OpenUrlEvent::Type OpenUrlEvent::OPEN_URL("openUrl");
void OpenUrlEvent::apply(EventVisitor* v)
{
	v->visit(this);
}

TextInputEvent::Type TextInputEvent::TEXT_INPUT("textInput");
void TextInputEvent::apply(EventVisitor* v)
{
	v->visit(this);
}

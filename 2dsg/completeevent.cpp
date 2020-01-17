#include "completeevent.h"
#include "eventvisitor.h"

CompleteEvent::Type CompleteEvent::COMPLETE("complete");

void CompleteEvent::apply(EventVisitor* v)
{
    v->visit(this);
}

HeaderEvent::Type HeaderEvent::HEADER("header");

void HeaderEvent::apply(EventVisitor* v)
{
    v->visit(this);
}

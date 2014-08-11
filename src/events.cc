#include "entities.h"
#include "events.h"

#define UNINITIALIZED_TIME -1

using std::string;
using std::vector;

// TODO everything here is a small method, move to header file?

Event::Event() : time_(UNINITIALIZED_TIME), affected_entities_() {}

Event::Event(Time t, Entity* e) : time_(t), affected_entities_() {
  affected_entities_.push_back(e);
}

Event::Event(Time t, Entity* e1, Entity* e2) : time_(t), affected_entities_() {
  affected_entities_.push_back(e1);
  affected_entities_.push_back(e2);
}

Time Event::time() const { return time_; }

vector<Entity*>::iterator Event::AffectedEntitiesBegin() {
  return affected_entities_.begin();
}

vector<Entity*>::iterator Event::AffectedEntitiesEnd() {
  return affected_entities_.end();
}

void Event::Handle(Entity* e) { e->Handle(this); }

string Event::Description() { return "base event"; }

RouterUpEvent::RouterUpEvent(Time t, Router* r) : Event(t, r) {}

void RouterUpEvent::Handle(Entity* e) { e->Handle(this); }

string RouterUpEvent::Description() { return "router up event"; }

RouterDownEvent::RouterDownEvent(Time t, Router* r) : Event(t, r) {}

void RouterDownEvent::Handle(Entity* e) { e->Handle(this); }

string RouterDownEvent::Description() { return "router down event"; }

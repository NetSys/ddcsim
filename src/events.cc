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

SwitchUp::SwitchUp(Time t, Switch* r) : Event(t, r) {}

void SwitchUp::Handle(Entity* e) { e->Handle(this); }

string SwitchUp::Description() { return "switch up"; }

SwitchDown::SwitchDown(Time t, Switch* r) : Event(t, r) {}

void SwitchDown::Handle(Entity* e) { e->Handle(this); }

string SwitchDown::Description() { return "switch down"; }

void Broadcast::SetInPort(Port p) { in_port_ = p; }

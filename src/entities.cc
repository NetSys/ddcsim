#include <iostream>

#include "entities.h"

using std::cout;
using std::endl;
using std::make_pair;

Entity::Ports::Ports() : port_to_entity_(), is_link_up_() {}

Entity::Entity() : ports_() {}

// TODO remove cout's after debugging

void Entity::Handle(Event* e) {
  cout << "Entity received event " << e->Description() << endl;
}

Switch::Switch() : is_up_(true) {}

void Switch::Handle(Event* e) {
  cout << "Switch received event " << e->Description() << endl;
}

void Switch::Handle(SwitchUp* e) { is_up_ = true; }

void Switch::Handle(SwitchDown* e) { is_up_ = false; }

BroadcastSwitch::BroadcastSwitch() : seen() {}

void BroadcastSwitch::Handle(Broadcast* b) {
  if (HasBeenSeen(b)) return;

  // TODO forward broadcast to each of my neighbors

  MarkAsSeen(b);
}

void BroadcastSwitch::MarkAsSeen(const Broadcast* b) {
  // TODO this can throw an exception if seen's allocator fails.  Should I just
  // ignore this possibility?
  seen.insert(MakeMsgId(b));
}

bool BroadcastSwitch::HasBeenSeen(const Broadcast* b) const {
  return seen.count(MakeMsgId(b)) > 0;
}

MsgId BroadcastSwitch::MakeMsgId(const Broadcast* b) {
  return make_pair(b->sn(), b->src());
}

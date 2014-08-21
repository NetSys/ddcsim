#include "entities.h"
#include "events.h"

#include <iostream>

using std::cout;
using std::endl;
using std::vector;

Links::Links() : port_nums_(), port_to_link_() {}

vector<Port>::const_iterator Links::PortsBegin() { return port_nums_.cbegin(); }

vector<Port>::const_iterator Links::PortsEnd() { return port_nums_.cend(); }

bool Links::IsLinkUp(Port p) { return port_to_link_[p].first; }

Entity* Links::GetEndpoint(Port p) { return port_to_link_[p].second; }

std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator
Links::LinksBegin() {
  return port_to_link_.cbegin();
}

std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator
Links::LinksEnd() {
  return port_to_link_.cend();
}

Entity::Entity(Scheduler& s) : links_(), scheduler_(s) {}

void Entity::Handle(Event* e) {
  cout << "Entity received event " << e->Description() << endl;
}

void Entity::Handle(Broadcast* b) {
  cout << "Entity received event " << b->Description() << endl;
}

Links& Entity::links() {
  return links_;
}

Switch::Switch(Scheduler& s) : Entity(s), is_up_(true) {}

void Switch::Handle(Event* e) {
  cout << "Switch received event " << e->Description() << endl;
}

void Switch::Handle(SwitchUp* e) { is_up_ = true; }

void Switch::Handle(SwitchDown* e) { is_up_ = false; }

BroadcastSwitch::BroadcastSwitch(Scheduler& s) : Switch(s), seen() {}

void BroadcastSwitch::Handle(Broadcast* b) {
  cout << "BroadcastSwitch " << this << " received Broadcast event";

  if (HasBeenSeen(b)) {
    cout << "\tseen" << endl;
    return;
  }
  cout << endl;

  for(auto it = links_.PortsBegin(); it != links_.PortsEnd(); ++it)
    if(b->in_port() != *it)
      scheduler_.Forward(this, b, *it);

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
  return {b->sn(), b->src()};
}

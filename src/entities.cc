#include "entities.h"
#include "events.h"

#include <iostream>

using std::cout;
using std::endl;
using std::vector;

// TODO log handler methods

Links::Links() : port_nums_(), port_to_link_() {}

vector<Port>::const_iterator Links::PortsBegin() { return port_nums_.cbegin(); }

vector<Port>::const_iterator Links::PortsEnd() { return port_nums_.cend(); }

void Links::SetLinkUp(Port p) { port_to_link_[p].first = true; }

void Links::SetLinkDown(Port p) { port_to_link_[p].first = false; }

bool Links::IsLinkUp(Port p) { return port_to_link_[p].first; }

Entity* Links::GetEndpoint(Port p) { return port_to_link_[p].second; }

Port Links::GetPortTo(Entity* endpoint) {
  for(auto it = LinksBegin(); it != LinksEnd(); ++it)
    if(it->second.second == endpoint)
      return it->first;
  return PORT_NOT_FOUND;
}

std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator
Links::LinksBegin() { return port_to_link_.cbegin(); }

std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator
Links::LinksEnd() { return port_to_link_.cend(); }

Entity::Entity(Scheduler& s) : links_(), scheduler_(s), is_up_(true),
                               id_(NONE_ID) {}

Entity::Entity(Scheduler& s, Id id) : links_(), scheduler_(s), is_up_(true),
                                      id_(id) {}

void Entity::Handle(Event* e) {
  cout << "Entity received event " << e->Description() << endl;
}

void Entity::Handle(Up* u) { is_up_ = true; }

void Entity::Handle(Down* d) { is_up_ = false; }

void Entity::Handle(Broadcast* b) {
  cout << "Entity received event " << b->Description() << endl;
}

void Entity::Handle(Heartbeat* h) {
  cout << "Entity received event " << h->Description() << endl;
}

void Entity::Handle(LinkUp* lu) { links_.SetLinkUp(lu->broken); }

void Entity::Handle(LinkDown* ld) { links_.SetLinkDown(ld->broken); }

Links& Entity::links() { return links_; }

Id Entity::id() const { return id_; }

Switch::Switch(Scheduler& s) : Entity(s), seen() {}

Switch::Switch(Scheduler& s, Id id) : Entity(s, id), seen() {}

void Switch::Handle(Heartbeat* b) {
  cout << "Switch " << id_ << " received Broadcast event";

  if(!is_up_) {
    cout << "\tdropped" << endl;
    return;
  }

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

void Switch::Handle(LinkUp* lu) {
  Entity::Handle(lu);

  // TODO broadcast out a Failure message
}

void Switch::Handle(LinkDown* ld) {
  Entity::Handle(ld);

  // TODO broadcast out a Failure message
}

void Switch::MarkAsSeen(const Heartbeat* b) {
  // TODO this can throw an exception if seen's allocator fails.  Should I just
  // ignore this possibility?
  seen.insert(MakeMsgId(b));
}

bool Switch::HasBeenSeen(const Heartbeat* b) const {
  return seen.count(MakeMsgId(b)) > 0;
}

MsgId Switch::MakeMsgId(const Heartbeat* b) {
  return {b->sn(), b->src()};
}

#include "entities.h"
#include "events.h"

#include <iostream>

using std::cout;
using std::endl;
using std::vector;
using std::pair;

// TODO log handler methods

HeartbeatHistory::HeartbeatHistory() : seen_() {}

void HeartbeatHistory::MarkAsSeen(const Heartbeat* b) {
  // TODO this can throw an exception if seen's allocator fails.  Should I just
  // ignore this possibility?
  seen_.insert(MakeHeartbeatId(b));
}

bool HeartbeatHistory::HasBeenSeen(const Heartbeat* b) const {
  return seen_.count(MakeHeartbeatId(b)) > 0;
}

HeartbeatHistory::HeartbeatId HeartbeatHistory::MakeHeartbeatId(const Heartbeat* b) {
  return {b->sn(), b->src()};
}

LinkFailureHistory::LinkFailureHistory() : failures_() {}

bool LinkFailureHistory::LinkIsUp(const LinkAlert* l) const {
  return failures_.count(MakeLinkId(l)) <= 0;
}

void LinkFailureHistory::MarkAsDown(const LinkAlert* l) {
  failures_.insert(MakeLinkId(l));
}

bool LinkFailureHistory::MarkAsUp(const LinkAlert* l) {
  failures_.erase(MakeLinkId(l));
}

LinkFailureHistory::LinkId LinkFailureHistory::MakeLinkId(const LinkAlert* l) {
  return {l->out_, l->src_};
}

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

Port Links::FindInPort(Entity* sender) {
  for(auto it = LinksBegin(); it != LinksEnd(); ++it)
    if(sender == it->second.second)
      return it->first;
  return PORT_NOT_FOUND;
}

std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator
Links::LinksBegin() { return port_to_link_.cbegin(); }

std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator
Links::LinksEnd() { return port_to_link_.cend(); }

Entity::Entity(Scheduler& s) : links_(), scheduler_(s), is_up_(true),
                               id_(NONE_ID), heart_history_() {}

Entity::Entity(Scheduler& s, Id id) : links_(), scheduler_(s), is_up_(true),
                                      id_(id), heart_history_() {}

void Entity::Handle(Event* e) {
  cout << "Entity received event " << e->Description() << endl;
}

void Entity::Handle(Up* u) { is_up_ = true; }

void Entity::Handle(Down* d) { is_up_ = false; }

void Entity::Handle(Broadcast* b) {
  cout << "Entity received event " << b->Description() << endl;
}

void Entity::Handle(Heartbeat* h) {
  if(!is_up_ || heart_history_.HasBeenSeen(h)) return;

  // TODO how to factor out to Entity::Handle?
  for(auto it = links_.PortsBegin(); it != links_.PortsEnd(); ++it)
    if(h->in_port() != *it)
      scheduler_.Forward(this, h, *it);

  heart_history_.MarkAsSeen(h);
}

void Entity::Handle(LinkUp* lu) { links_.SetLinkUp(lu->out_); }

void Entity::Handle(LinkDown* ld) { links_.SetLinkDown(ld->out_); }

void Entity::Handle(LinkAlert* alert) {
  cout << "Entity received event " << alert->Description() << endl;
}

void Entity::Handle(LinkUp* lu) { links_.SetLinkUp(lu->broken); }

void Entity::Handle(LinkDown* ld) { links_.SetLinkDown(ld->broken); }

Links& Entity::links() { return links_; }

Id Entity::id() const { return id_; }

Switch::Switch(Scheduler& s) : Entity(s), link_history_() {}

Switch::Switch(Scheduler& s, Id id) : Entity(s, id), link_history_() {}

void Switch::Handle(LinkAlert* alert) {
  if(!is_up_) return;

  if(! (alert->is_up_ ^ link_history_.LinkIsUp(alert))) return;

  for(auto it = links_.PortsBegin(); it != links_.PortsEnd(); ++it)
    if(alert->in_port() != *it)
      scheduler_.Forward(this, alert, *it);

  if(alert->is_up_)
    link_history_.MarkAsUp(alert);
  else
    link_history_.MarkAsDown(alert);
}

Controller::Controller(Scheduler& s) : Entity(s) {}

Controller::Controller(Scheduler& s, Id id) : Entity(s, id) {}

void Controller::Handle(LinkAlert* alert) {
}

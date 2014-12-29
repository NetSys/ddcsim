#include "entities.h"
#include "events.h"

#include <glog/logging.h>

#define UNINITIALIZED_TIME -1

using std::array;
using std::string;
using std::to_string;
using std::shared_ptr;
using std::vector;

namespace std {
string to_string(const vector<int>& ints) {
  string rtn = "";

  for(int i : ints)
    rtn += to_string(i) + " ";

  return rtn;
}
string to_string(const array<Id, 13>& ints) {
  string rtn = "";

  for(int i : ints)
    rtn += to_string(i) + " ";

  return rtn;
}
};

Event::Event() {}

Event::Event(Time t, Entity* e) : time_(t), affected_entities_({e}) {}

Event::Event(Time t, Entity* e1, Entity* e2) : time_(t),
                                               affected_entities_({e1, e2}) {}

Event::~Event() { return; }

void Event::Handle(Entity* e) { e->Handle(this); }

string Event::Description() const {
  string ids = "";

  for(auto it = affected_entities_.begin(); it != affected_entities_.end(); ++it)
    ids += to_string((*it)->id()) + " ";

  return "time_=" + to_string(time_) + " affected_entities=" + ids;
}

string Event::Name() const { return "Event"; }

Up::Up(Time t, Entity* e) : Event(t, e) {}

void Up::Handle(Entity* e) { e->Handle(this); }

string Up::Description() const { return Event::Description(); }

string Up::Name() const { return "Up"; }

Down::Down(Time t, Entity* e) : Event(t, e) {}

void Down::Handle(Entity* e) { e->Handle(this); }

string Down::Description() const { return Event::Description(); }

string Down::Name() const { return "Down"; }

LinkUp::LinkUp(Time t, Entity* e, Port p) : Event(t, e), out_(p) {}

void LinkUp::Handle(Entity* e) { e->Handle(this); }

string LinkUp::Description() const {
  return Event::Description() + " out_=" + to_string(out_);
}

string LinkUp::Name() const { return "Link Up"; }

LinkDown::LinkDown(Time t, Entity* e, Port p) : Event(t, e), out_(p) {}

void LinkDown::Handle(Entity* e) { e->Handle(this); }

string LinkDown::Description() const {
  return Event::Description() + " out_=" + to_string(out_);
}

string LinkDown::Name() const { return "Link Down"; }

Broadcast::Broadcast() {}

Broadcast::Broadcast(Time t, Entity* affected_entity, Port in) :
    Event(t, affected_entity), in_port_(in) {}

void Broadcast::Handle(Entity* e) { e->Handle(this); }

string Broadcast::Description() const {
  return Event::Description() + " in_port_=" + to_string(in_port_);
}

string Broadcast::Name() const { return "Broadcast"; }

LinkStateUpdate::LinkStateUpdate(Time t, Entity* e, Port i, Entity* src,
                                 SequenceNum sn, Time expiration,
                                 array<Id, 13> up_links, Id src_id)
    : Broadcast(t, e, i), src_(src), sn_(sn), expiration_(expiration),
      up_links_(up_links), src_id_(src_id) {}

void LinkStateUpdate::Handle(Entity* e) { e->Handle(this); }

string LinkStateUpdate::Description() const {
   return Broadcast::Description()  +
       " sn_=" + to_string(sn_) +
       " src_id=" + to_string(src_id_) +
       " up_links_=" + to_string(up_links_) +
       " expiration_=" + to_string(expiration_);
}

string LinkStateUpdate::Name() const { return "Link State Update"; }

InitiateLinkState::InitiateLinkState(Time t, Entity* e) : Event(t, e) {}

void InitiateLinkState::Handle(Entity* e) { e->Handle(this); }

string InitiateLinkState::Description() const { return Event::Description(); }

string InitiateLinkState::Name() const { return "Initiate Link State Update"; }

RoutingUpdate::RoutingUpdate(Time t, Entity* e, Port in, Entity* src,
                             SequenceNum sn,
                             shared_ptr<vector<Id> > dst_to_neighbor, Id dst, Id src_id) :
    //Table dst_to_neighbor, Id dst, Id src_id) :
    Broadcast(t, e, in), sn_(sn), src_(src),
    dst_to_neighbor_(dst_to_neighbor), dst_(dst), src_id_(src_id) {}

RoutingUpdate::~RoutingUpdate() { dst_to_neighbor_ = nullptr; }

void RoutingUpdate::Handle(Entity* e) { e->Handle(this); }

string RoutingUpdate::Description() const {
  string rtn = Broadcast::Description() + " sn_=" + to_string(sn_) +
      " src_=" + to_string(src_->id()) + " dst_=" + to_string(dst_) +
      " dst_to_neighbor_=";

  if(dst_to_neighbor_ == nullptr) {
    rtn += "null";
  } else {
    for(int i : *dst_to_neighbor_)
      rtn += to_string(i) + " ";
  }

  return rtn;
}

string RoutingUpdate::Name() const { return "Routing Update"; }

LinkStateRequest::LinkStateRequest(Time t, Entity* e, Port in, Entity* src,
                                   SequenceNum sn, Id id) :
    Broadcast(t, e, in), src_(src), sn_(sn), src_id_(id) {}

void LinkStateRequest::Handle(Entity* e) { e->Handle(this); }

string LinkStateRequest::Description() const {
  return Broadcast::Description()  +
      " sn_=" + to_string(sn_) +
      " src_=" + to_string(src_->id());
}

string LinkStateRequest::Name() const { return "Link State Request"; }

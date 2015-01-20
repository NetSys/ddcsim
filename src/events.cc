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

Event::Event() : time_(INVALID_TIME), affected_entities_() {}

Event::Event(Time t, Entity* e) : time_(t), affected_entities_({e}) {}

Event::Event(Time t, Entity* e1, Entity* e2) : time_(t),
                                               affected_entities_({e1, e2}) {}

Event::~Event() { return; }

void Event::Handle(Entity* e) {  e->Handle(this); }

string Event::Description() const {
  string ids = "";

  for(auto it = affected_entities_.begin(); it != affected_entities_.end(); ++it)
    ids += to_string((*it)->id()) + " ";

  return "time_=" + to_string(time_) + " affected_entities=" + ids;
}

string Event::Name() const { return "Event"; }

//unsigned int Event::size() const { return 0; }

unsigned int Event::size() const { return -1; }

Up::Up() : Event() {}

Up::Up(Time t, Entity* e) : Event(t, e) {}

unsigned int Up::count_ = 0;

void Up::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string Up::Description() const { return Event::Description(); }

string Up::Name() const { return "Up"; }

Down::Down() : Event() {}

Down::Down(Time t, Entity* e) : Event(t, e) {}

unsigned int Down::count_ = 0;

void Down::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string Down::Description() const { return Event::Description(); }

string Down::Name() const { return "Down"; }

LinkUp::LinkUp() : Event(), out_(PORT_NOT_FOUND) {}

LinkUp::LinkUp(Time t, Entity* e, Port p) : Event(t, e), out_(p) {}

unsigned int LinkUp::count_ = 0;

void LinkUp::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string LinkUp::Description() const {
  return Event::Description() + " out_=" + to_string(out_);
}

string LinkUp::Name() const { return "Link Up"; }

LinkDown::LinkDown() : Event(), out_(PORT_NOT_FOUND) {}

LinkDown::LinkDown(Time t, Entity* e, Port p) : Event(t, e), out_(p) {}

unsigned int LinkDown::count_ = 0;

void LinkDown::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string LinkDown::Description() const {
  return Event::Description() + " out_=" + to_string(out_);
}

string LinkDown::Name() const { return "Link Down"; }

Broadcast::Broadcast() : Event(), in_port_(PORT_NOT_FOUND) {}

Broadcast::Broadcast(Time t, Entity* affected_entity, Port in) :
    Event(t, affected_entity), in_port_(in) {}

void Broadcast::Handle(Entity* e) {  e->Handle(this); }

string Broadcast::Description() const {
  return Event::Description() + " in_port_=" + to_string(in_port_);
}

string Broadcast::Name() const { return "Broadcast"; }

//unsigned int Broadcast::size() const { return 20; }

unsigned int Broadcast::size() const { return -1; }

LinkStateUpdate::LinkStateUpdate() : Broadcast(), src_(nullptr), sn_(NONE_SEQNUM),
                                     expiration_(INVALID_TIME), up_links_(), src_id_(NONE_ID) {}

LinkStateUpdate::LinkStateUpdate(Time t, Entity* e, Port i, Entity* src,
                                 SequenceNum sn, Time expiration,
                                 array<Id, 13> up_links, Id src_id)
    : Broadcast(t, e, i), src_(src), sn_(sn), expiration_(expiration),
      up_links_(up_links), src_id_(src_id) {}

unsigned int LinkStateUpdate::count_ = 0;

void LinkStateUpdate::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string LinkStateUpdate::Description() const {
   return Broadcast::Description()  +
       " sn_=" + to_string(sn_) +
       " src_id=" + to_string(src_id_) +
       " up_links_=" + to_string(up_links_) +
       " expiration_=" + to_string(expiration_);
}

string LinkStateUpdate::Name() const { return "Link State Update"; }

// unsigned int LinkStateUpdate::size() const {
//   return Broadcast::size() + 4 + 6 + 13 * 6 + 8;
// }

unsigned int LinkStateUpdate::size() const { return 0; }

InitiateLinkState::InitiateLinkState() : Event() {}

InitiateLinkState::InitiateLinkState(Time t, Entity* e) : Event(t, e) {}

unsigned int InitiateLinkState::count_ = 0;

void InitiateLinkState::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string InitiateLinkState::Description() const { return Event::Description(); }

string InitiateLinkState::Name() const { return "Initiate Link State Update"; }

RoutingUpdate::RoutingUpdate() : Broadcast(), sn_(NONE_SEQNUM), src_(nullptr),
                                 dst_to_neighbor_(nullptr), dst_(NONE_ID),
                                 src_id_(NONE_ID) {}

RoutingUpdate::RoutingUpdate(Time t, Entity* e, Port in, Entity* src,
                             SequenceNum sn,
                             shared_ptr<vector<Id> > dst_to_neighbor,
                             Id dst, Id src_id) :
    Broadcast(t, e, in), sn_(sn), src_(src),
    dst_to_neighbor_(dst_to_neighbor), dst_(dst), src_id_(src_id) {}

RoutingUpdate::~RoutingUpdate() { dst_to_neighbor_ = nullptr; }

unsigned int RoutingUpdate::count_ = 0;

void RoutingUpdate::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string RoutingUpdate::Description() const {
  string rtn = Broadcast::Description() + " sn_=" + to_string(sn_) +
      " src_=" + to_string(src_->id()) + " dst_=" + to_string(dst_) +
      " dst_to_neighbor_=";

  if(dst_to_neighbor_) {
    for(int i : *dst_to_neighbor_)
      rtn += to_string(i) + " ";
  } else {
    rtn += "null";
  }

  return rtn;
}

string RoutingUpdate::Name() const { return "Routing Update"; }

// unsigned int RoutingUpdate::size() const {
//   //return Broadcast::size() + 4 + 6 +  2000*6 + 6;
//   return Broadcast::size() + 4 + 6 +  2*6 + 6;
// }

unsigned int RoutingUpdate::size() const { return 1; }

LinkStateRequest::LinkStateRequest() : Broadcast(), src_(nullptr), sn_(NONE_ID),
                                       src_id_(NONE_ID) {}

LinkStateRequest::LinkStateRequest(Time t, Entity* e, Port in, Entity* src,
                                   SequenceNum sn, Id id) :
    Broadcast(t, e, in), src_(src), sn_(sn), src_id_(id) {}

unsigned int LinkStateRequest::count_ = 0;

void LinkStateRequest::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string LinkStateRequest::Description() const {
  return Broadcast::Description()  +
      " sn_=" + to_string(sn_) +
      " src_=" + to_string(src_->id());
}

string LinkStateRequest::Name() const { return "Link State Request"; }

//unsigned int LinkStateRequest::size() const { return Broadcast::size() + 4 + 6; }

unsigned int LinkStateRequest::size() const { return 2; }

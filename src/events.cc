#include "bv.h"
#include "entities.h"
#include "events.h"

#include <glog/logging.h>

#include <cmath>

#define UNINITIALIZED_TIME -1

using std::ceil;
using std::string;
using std::to_string;
using std::ostream;
using std::vector;

namespace std {
string to_string(vector<bool> vb) {
  string rtn = "";

  for(int i = 0; i < vb.size(); ++i)
    rtn += vb[i] ? "1" : "0";

  return rtn;
}
}

Event::Event(Time t, Entity* e) : time_(t), affected_entities_({e}) {}

Event::Event(Time t, Entity* e1, Entity* e2) : time_(t),
                                               affected_entities_({e1, e2}) {}

void Event::Handle(Entity* e) { e->Handle(this); }

string Event::Description() const {
  string ids = "";

  for(auto it = affected_entities_.begin(); it != affected_entities_.end(); ++it)
    ids += to_string((*it)->id()) + " ";

  return "time_=" + to_string(time_) + " affected_entities=" + ids;
}

string Event::Name() const { return "Event"; }

Size Event::size() const { return 0; }

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

InitiateHeartbeat::InitiateHeartbeat(Time t, Entity* affected_entity) :
    Event(t, affected_entity) {}

void InitiateHeartbeat::Handle(Entity* e) { return e->Handle(this); }

string InitiateHeartbeat::Description() const { return Event::Description(); }

string InitiateHeartbeat::Name() const { return "Initiate Heartbeat"; }

Broadcast::Broadcast(Time t, Entity* affected_entity, Port in) :
    Event(t, affected_entity), in_port_(in) {}

void Broadcast::Handle(Entity* e) { e->Handle(this); }

string Broadcast::Description() const {
  return Event::Description() + " in_port_=" + to_string(in_port_);
}

string Broadcast::Name() const { return "Broadcast"; }

Size Broadcast::size() const {
  /* 20 bytes for a header with no options */
  return 20;
}

Heartbeat::Heartbeat(Time t, const Entity* src, Entity* affected_entity,
                     Port in, SequenceNum sn, BV r) :
    Broadcast(t, affected_entity, in), src_(src), sn_(sn), recently_seen_(r),
    leader_(NONE_ID), current_partition_(0) {
  ++(*recently_seen_.ref_count_);
}

Heartbeat::~Heartbeat() {
  --(*(recently_seen_.ref_count_));

  if(*(recently_seen_.ref_count_) == 0) {
    delete recently_seen_.bv_;
    delete recently_seen_.ref_count_;
  }
}

void Heartbeat::Handle(Entity* e) { e->Handle(this); }

string Heartbeat::Description() const {
  return Broadcast::Description() +
    " sn_=" + to_string(sn_) +
    " src_=" + to_string(src_->id()) +
    " current_parition_=" + to_string(current_partition_) +
      " leader_=" + to_string(leader_);
    // " recently_seen_=" + to_string(*recently_seen_.bv_);
}

string Heartbeat::Name() const { return "Heartbeat"; }

Size Heartbeat::size() const {
  // TODO how to automate this?
  return Broadcast::size() + sizeof(sn_) + sizeof(src_);
      //      ceil(recently_seen_.bv_->size() / 8.0) + sizeof(leader_) + sizeof(current_partition_);
}

LinkStateUpdate::LinkStateUpdate(Time t, Entity* e, Port i, const Entity* s,
                                 SequenceNum sn, const vector<Id> v, Time exp)
    : Broadcast(t, e, i), src_(s), sn_(sn), neighbors_(v), expiration_(exp) {}

void LinkStateUpdate::Handle(Entity* e) { e->Handle(this); }

string LinkStateUpdate::Description() const {
  return Broadcast::Description()  + "...";
}

string LinkStateUpdate::Name() const { return "Link State Update"; }

Size LinkStateUpdate::size() const { return Broadcast::size() + 50; }

InitiateLinkState::InitiateLinkState(Time t, Entity* e) : Event(t, e) {}

void InitiateLinkState::Handle(Entity* e) { e->Handle(this); }

string InitiateLinkState::Description() const { return Event::Description(); }

string InitiateLinkState::Name() const { return "Initiate Link State Update"; }

OVERLOAD_EVENT_OSTREAM_IMPL(Event)
OVERLOAD_EVENT_OSTREAM_IMPL(Up)
OVERLOAD_EVENT_OSTREAM_IMPL(Down)
OVERLOAD_EVENT_OSTREAM_IMPL(LinkUp)
OVERLOAD_EVENT_OSTREAM_IMPL(LinkDown)
OVERLOAD_EVENT_OSTREAM_IMPL(InitiateHeartbeat)
OVERLOAD_EVENT_OSTREAM_IMPL(Broadcast)
OVERLOAD_EVENT_OSTREAM_IMPL(Heartbeat)
OVERLOAD_EVENT_OSTREAM_IMPL(LinkStateUpdate)
OVERLOAD_EVENT_OSTREAM_IMPL(InitiateLinkState)

#include <iostream>

#include "entities.h"
#include "events.h"

#define UNINITIALIZED_TIME -1

using std::string;
using std::vector;
using std::cout;
using std::endl;

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

Up::Up(Time t, Entity* e) : Event(t, e) {}

void Up::Handle(Entity* e) { e->Handle(this); }

string Up::Description() { return "up"; }

Down::Down(Time t, Entity* e) : Event(t, e) {}

void Down::Handle(Entity* e) { e->Handle(this); }

string Down::Description() { return "down"; }

LinkUp::LinkUp(Time t, Entity* e, Port p) : Event(t, e), out_(p) {}

void LinkUp::Handle(Entity* e) { e->Handle(this); }

string LinkUp::Description() { return "link up"; }

LinkDown::LinkDown(Time t, Entity* e, Port p) : Event(t, e), out_(p) {}

void LinkDown::Handle(Entity* e) { e->Handle(this); }

string LinkDown::Description() { return "link down"; }

InitiateHeartbeat::InitiateHeartbeat(Time t, Entity* affected_entity) :
    Event(t, affected_entity) {}

void InitiateHeartbeat::Handle(Entity* e) { return e->Handle(this); }

string InitiateHeartbeat::Description() { return "initiate heartbeat"; }

Broadcast::Broadcast(Time t, Entity* affected_entity, Port in) :
    Event(t, affected_entity), in_port_(in) {}

Port Broadcast::in_port() const { return in_port_; }

void Broadcast::Handle(Entity* e) { e->Handle(this); }

string Broadcast::Description() { return "broadcast"; }

Heartbeat::Heartbeat(Time t, const Entity* src, Entity* affected_entity,
                     Port in, SequenceNum sn, vector<bool> r) :
    Broadcast(t, affected_entity, in), src_(src), sn_(sn), recently_seen_(r),
    leader_(NONE_ID), current_partition_(0) {}

SequenceNum Heartbeat::sn() const { return sn_; }

const Entity* Heartbeat::src() const { return src_; }

vector<bool> Heartbeat::recently_seen() const { return recently_seen_; }

void Heartbeat::Handle(Entity* e) { e->Handle(this); }

string Heartbeat::Description() { return "heartbeat"; }

LinkAlert::LinkAlert(Time t, Entity* e, Port i, const Entity* s, Port p, bool b)
    : Broadcast(t, e, i), src_(s), out_(p), is_up_(b) {}

void LinkAlert::Handle(Entity* e) { e->Handle(this); }

string LinkAlert::Description() { return "link alert"; }

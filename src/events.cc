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

Up::Up(Time t, Entity* r) : Event(t, r) {}

void Up::Handle(Entity* e) { e->Handle(this); }

string Up::Description() { return "up"; }

Down::Down(Time t, Entity* r) : Event(t, r) {}

void Down::Handle(Entity* e) { e->Handle(this); }

string Down::Description() { return "down"; }

Broadcast::Broadcast(Time t, Entity* affected_entity, Port in):
    Event(t, affected_entity), in_port_(in) {}

Port Broadcast::in_port() const { return in_port_; }

void Broadcast::Handle(Entity* e) { e->Handle(this); }

string Broadcast::Description() { return "broadcast"; }

Heartbeat::Heartbeat(Time t, const Switch* src, Entity* affected_entity, Port in,
                     SequenceNum sn) : Broadcast(t, affected_entity, in),
                                       src_(src), sn_(sn) {}

SequenceNum Heartbeat::sn() const { return sn_; }

const Switch* Heartbeat::src() const { return src_; }

void Heartbeat::Handle(Entity* e) {
  cout << "Broadcast Event: sn=" << sn_;
  cout << " src=" << src_->id();
  cout << " in_port=" << in_port_;
  cout << " t=" << time_ << endl;
  e->Handle(this);
}

string Heartbeat::Description() { return "heartbeat"; }

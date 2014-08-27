#include <assert.h>
#include <iostream>
#include <utility>

#include "entities.h"
#include "events.h"
#include "scheduler.h"

using std::vector;

const Time Scheduler::kLinkLatency = 0.1;
const Time Scheduler::kDefaultHeartbeatPeriod = 3;
const Time Scheduler::kDefaultEndTime = 60;

Scheduler::Scheduler(Time end_time) : event_queue_(),
                                      end_time_(end_time) {}

void Scheduler::AddEvent(Event* e) {
  event_queue_.push(e);
}

bool Scheduler::HasNextEvent() {
  return ! event_queue_.empty();
}

Event* Scheduler::NextEvent() {
  Event* next = event_queue_.top();
  event_queue_.pop();
  return next;
}

bool Scheduler::Comparator::operator() (const Event* const lhs,
                                        const Event* const rhs) const {
  return lhs->time() > rhs->time();
}

// TODO why isn't partial specialization of methods allowed?
template<> Broadcast* Scheduler::Schedule(Broadcast* broadcast_in,
                                          Entity* receiver, Port in) {
  return new Broadcast(broadcast_in->time() + kLinkLatency,
                       broadcast_in->src(), receiver, in, broadcast_in->sn());
}

template<class E, class M> void Scheduler::Forward(E* sender, M* msg_in, Port out) {
  Links& l = sender->links();

  if(! l.IsLinkUp(out)) {
    std::cout << "Broadcast packet dropped due to down link" << std::endl;
    return;
  }

  Entity* receiver = l.GetEndpoint(out);

  Port in = FindInPort(sender, receiver);
  assert(in != PORT_NOT_FOUND);

  M* new_event = Schedule(msg_in, receiver, in);
  AddEvent(new_event);
}

void Scheduler::StartSimulation() {
  Time cur_time = 0;

  while(HasNextEvent() && cur_time < end_time_) {
    Event* ev = NextEvent();

    cur_time = ev->time();

    for (vector<Entity*>::iterator it = ev->AffectedEntitiesBegin();
         it != ev->AffectedEntitiesEnd(); ++it) {
      ev->Handle(*it);
      delete ev;
    }
  }
}

Time Scheduler::end_time() { return end_time_; }

template<class E> Port Scheduler::FindInPort(E* sender, Entity* receiver) {
  Links& l = receiver->links();
  for(auto it = l.LinksBegin(); it != l.LinksEnd(); ++it)
    if(sender == it->second.second)
      return it->first;
  return PORT_NOT_FOUND;
}

/* TODO explain why we need to oblige the compiler to instantiate these
 * here methods
 */
template void Scheduler::Forward<BroadcastSwitch, Broadcast>(BroadcastSwitch*,
                                                             Broadcast*,
                                                             Port);
template Port Scheduler::FindInPort<BroadcastSwitch>(BroadcastSwitch*,
                                                     Entity*);

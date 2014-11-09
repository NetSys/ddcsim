#include <string>
#include <utility>

#include <glog/logging.h>

#include "entities.h"
#include "events.h"
#include "scheduler.h"
#include "statistics.h"

using std::default_random_engine;
using std::discrete_distribution;
using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;

/* The type-specific parts of Scheduler::Forward are deferred to this class.
 * This functionality is implemented as a class rather than as a generic
 * method (with appropriate specializations) so that we can leverage partial
 * specialization, which is forbidden for methods but not classes.
 */
template<class E, class M> class Schedule {
 public:
  Event* operator()(E* sender, M* msg_in, Entity* reciever, Port in);
};

// TODO should the scheduler create messages?
template<> class Schedule<Entity, Heartbeat> {
 public:
  Event* operator()(Entity* sender, Heartbeat* heartbeat_in,
                    Entity* receiver, Port in) {
    return new Heartbeat(heartbeat_in->time() + Scheduler::Delay(),
                         heartbeat_in->src(), receiver, in, heartbeat_in->sn(),
                         heartbeat_in->recently_seen());
  }
};

template<> class Schedule<Entity, InitiateHeartbeat> {
 public:
  Event* operator()(Entity* sender, InitiateHeartbeat* init,
                    Entity* receiver, Port in) {
    return new Heartbeat(init->time() + Scheduler::Delay(), sender,
                         receiver, in, sender->NextHeartbeatSeqNum(),
                         sender->ComputeRecentlySeen());
  }
};

template<class E> class Schedule<E, LinkAlert> {
 public:
  Event* operator()(E* sender, LinkAlert* alert_in, Entity* receiver, Port in) {
    return new LinkAlert(alert_in->time() + Scheduler::Delay(), receiver,
                         in, alert_in->src_, alert_in->out_, alert_in->is_up_);
  }
};

const Time Scheduler::kComputationDelay = 0.00001; /* 10 micros */
const Time Scheduler::kTransDelay = 0.001;         /* 1 ms */
const Time Scheduler::kPropDelay = 0.01;           /* 10 ms */
const Time Scheduler::kDefaultHeartbeatPeriod = 3;
const Time Scheduler::kDefaultEndTime = 60;
const int Scheduler::kNoMaxEntities = -1;
int Scheduler::kMaxEntities = 0;

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
template<class E, class M> void Scheduler::Forward(E* sender, M* msg_in, Port out,
                                                   Statistics& stats) {
  Links& l = sender->links();

  if(! l.IsLinkUp(out)) return;

  Entity* receiver = l.GetEndpoint(out);

  Port in = receiver->links().GetPortTo(sender);
  CHECK_NE(in, PORT_NOT_FOUND);

  Schedule<E, M> s;
  Event* new_event = s(sender, msg_in, receiver, in);

  // TODO clean this up
  // Size sz = new_event->size();
  // BandwidthMeter& m = l.port_to_link_[out].meter;

  // if(m.CanSend(sz)) {
  //   m.Send(sz);
  // TODO move this before allocation of new_event?
  if(new_event->time() <= end_time_) {
    AddEvent(new_event);
    stats.RecordSend(new_event);
  } else {
    delete new_event;
  }
  // } else {
  //   // TODO better drop message
  //   //LOG(INFO) << "Packet dropped due to insufficient link capacity";
  //   LOG(ERROR) << "Packet dropped due to insufficient link capacity";
  //   delete new_event;
  // }
}

// TODO do a better job of sharing the id_to_entity_ mapping between reader
void Scheduler::StartSimulation(unordered_map<Id, Entity*>& id_to_entity) {
  Time last_time, next_milestone, milestone_granularity;

  last_time = cur_time_ = START_TIME;
  next_milestone = milestone_granularity = 0.05;

  while(HasNextEvent() && cur_time_ < end_time_) {
    Event* ev = NextEvent();

    last_time = cur_time_;
    cur_time_ = ev->time();
    CHECK_GE(cur_time_, last_time);

    if (cur_time_ / end_time_ > next_milestone) {
      LOG(WARNING) << "Progress: " << (next_milestone * 100) << "%";
      next_milestone += milestone_granularity;
    }

    // TODO put this functionality somewhere else?

    // for(auto it = id_to_entity.begin(); it != id_to_entity.end(); ++it)
    //   it->second->UpdateLinkCapacities(cur_time_ - last_time);

     for (auto it = ev->AffectedEntitiesBegin(); it != ev->AffectedEntitiesEnd();
         ++it)
       ev->Handle(*it);

    delete ev;
  }
}

Time Scheduler::cur_time() { return cur_time_; }

Time Scheduler::end_time() { return end_time_; }

Time Scheduler::Delay() {
  // TODO make these functions of link bandwidth and length
  return kComputationDelay + kTransDelay + kPropDelay;
}

/* TODO explain why we need to oblige the compiler to instantiate this templated
* method explicity
*/
template void Scheduler::Forward<Entity, Heartbeat>(Entity*, Heartbeat*, Port,
                                                    Statistics&);
template void Scheduler::Forward<Switch, LinkAlert>(Switch*, LinkAlert*, Port,
                                                    Statistics&);
template void Scheduler::Forward<Entity, InitiateHeartbeat>(Entity*,
                                                            InitiateHeartbeat*,
                                                            Port, Statistics&);

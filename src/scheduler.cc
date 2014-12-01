#include <string>

#include <glog/logging.h>

#include "entities.h"
#include "events.h"
#include "scheduler.h"
#include "statistics.h"

#include <iostream>
#include <random>

using std::default_random_engine;
using std::discrete_distribution;
using std::uniform_real_distribution;
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
  Event* operator()(E* sender, M* msg_in, Entity* reciever, Port in) {};
};

// TODO should the scheduler create messages?
template<> class Schedule<Entity, Heartbeat> {
 public:
  Event* operator()(Entity* sender, Heartbeat* heartbeat_in, Entity* receiver,
                    Port in) {
    return new Heartbeat(heartbeat_in->time_ + Scheduler::Delay(),
                         heartbeat_in->src_,
                         receiver,
                         in,
                         heartbeat_in->sn_,
                         heartbeat_in->recently_seen_);
  }
};

template<> class Schedule<Entity, InitiateHeartbeat> {
 public:
  Event* operator()(Entity* sender, InitiateHeartbeat* init, Entity* receiver,
                    Port in) {
    return new Heartbeat(init->time_ + Scheduler::Delay(),
                         sender,
                         receiver,
                         in,
                         sender->NextHeartbeatSeqNum(),
                         sender->ComputeRecentlySeen());
  }
};

template<> class Schedule<Switch, LinkStateUpdate> {
 public:
  Event* operator()(Entity* sender, LinkStateUpdate* ls, Entity* receiver,
                    Port in) {
    return new LinkStateUpdate(ls->time_ + Scheduler::Delay(),
                               receiver,
                               in,
                               ls->src_,
                               ls->sn_,
                               ls->neighbors_,
                               ls->expiration_);
  }
};

template<> class Schedule<Switch, InitiateLinkState> {
 public:
  Event* operator()(Switch* sender, InitiateLinkState* ls, Entity* receiver,
                    Port in) {
    return new LinkStateUpdate(ls->time_ + Scheduler::Delay(),
                               receiver,
                               in,
                               sender,
                               sender->NextLSSeqNum(),
                               sender->ComputeUpNeighbors(),
                               ls->time_ +
                               Scheduler::kComputationDelay +
                               Scheduler::kExpireDelta);
    // TODO use either the encapsulated message time or the global scheduler time
  }
};

const Time Scheduler::kComputationDelay = 0.00001; /* 10 micros */
const Time Scheduler::kTransDelay = 0.001;         /* 1 ms */
const Time Scheduler::kPropDelay = 0.01;           /* 10 ms */
const Time Scheduler::kDefaultHeartbeatPeriod = 3;
const Time Scheduler::kDefaultLSUpdatePeriod = 3;
const Time Scheduler::kDefaultEndTime = 60;
const Time Scheduler::kDefaultHelloDelay = 0.001;  /* 1 ms */
// TODO this depends on topology and should probably be set according to each input
const Time Scheduler::kExpireDelta = 5;

Scheduler::Scheduler(Time end_time, unsigned int num_entities) :
    event_queue_(), end_time_(end_time),  num_entities_(num_entities) {}

void Scheduler::AddEvent(Event* e) {  event_queue_.emplace(e->time_, e); }

bool Scheduler::HasNextEvent() { return ! event_queue_.empty(); }

Event* Scheduler::NextEvent() {
  std::pair<Time, Event*> next = event_queue_.top();
  event_queue_.pop();
  return next.second;
}

bool Scheduler::Comparator::operator() (const std::pair<Time, const Event* const> lhs,
                                        const std::pair<Time, const Event* const> rhs) const {
  return lhs.first > rhs.first;
}

// TODO why isn't partial specialization of methods allowed?
// TODO don't pass in stats
template<class E, class M> void Scheduler::Forward(E* sender, M* msg_in, Port out,
                                                   Statistics& stats) {
  Links& l = sender->links();

  if(! l.IsLinkUp(out)) return;

  Entity* receiver = l.GetEndpoint(out);

  Port in = receiver->links().GetPortTo(sender);
  CHECK_NE(in, PORT_NOT_FOUND);

  Schedule<E, M> s;
  Event* new_event = s(sender, msg_in, receiver, in);

  // TODO move this before allocation of new_event?
  if(new_event->time_ <= end_time_) {
    AddEvent(new_event);
    stats.RecordSend(new_event);
  } else {
    delete new_event;
  }
}

// TODO generalize
// TODO feed default_random_engine a seed to make it deterministic
void Scheduler::SchedulePeriodicEvents(unordered_map<Id, Entity*>& id_to_entity,
                                       Time heartbeat_period,
                                       Time ls_update_period) {
  default_random_engine entropy_src;

  Time half_hrtbt = heartbeat_period / 2;
  uniform_real_distribution<Time> hrtbt_init_dist(0, half_hrtbt);
  uniform_real_distribution<Time> hrtbt_dist(-1 * half_hrtbt, half_hrtbt);

  // TODO verify that it's okay to use entropy_src for both init_dist and dist
  for(auto it : id_to_entity)
    AddEvent(new InitiateHeartbeat(hrtbt_init_dist(entropy_src),
                                         it.second));

  // TODO verify semantics of end_time
  for (Time t = heartbeat_period; t <= end_time_; t += heartbeat_period)
    for(auto it : id_to_entity)
      AddEvent(new InitiateHeartbeat(t + hrtbt_dist(entropy_src),
                                           it.second));

  Time half_ls = ls_update_period / 2;
  uniform_real_distribution<Time> ls_init_dist(0, half_ls);
  uniform_real_distribution<Time> ls_dist(-1 * half_ls, half_ls);

  for(auto it : id_to_entity)
    AddEvent(new InitiateLinkState(ls_init_dist(entropy_src),
                                         it.second));

  for (Time t = ls_update_period; t <= end_time_; t += ls_update_period)
    for(auto it : id_to_entity)
      AddEvent(new InitiateLinkState(t + ls_dist(entropy_src),
                                           it.second));
}

// TODO do a better job of sharing the id_to_entity_ mapping between reader
void Scheduler::StartSimulation(unordered_map<Id, Entity*>& id_to_entity) {
  Time last_time, next_milestone, milestone_granularity;

  last_time = cur_time_ = START_TIME;
  next_milestone = milestone_granularity = 0.05;

  while(HasNextEvent() && cur_time_ < end_time_) {
    Event* ev = NextEvent();

    last_time = cur_time_;
    cur_time_ = ev->time_;
    CHECK_GE(cur_time_, last_time);

    if (cur_time_ / end_time_ > next_milestone) {
      LOG(WARNING) << "Progress: " << (next_milestone * 100) << "%";
      next_milestone += milestone_granularity;
    }

    for (Entity* e : ev->affected_entities_)
      ev->Handle(e);

    delete ev;
  }
}

Time Scheduler::cur_time() { return cur_time_; }

Time Scheduler::end_time() { return end_time_; }

unsigned int Scheduler::num_entities() { return num_entities_; }

Time Scheduler::Delay() {
  // TODO make these functions of link bandwidth and length
  return kComputationDelay + kTransDelay + kPropDelay;
}

/* TODO explain why we need to oblige the compiler to instantiate this templated
* method explicity
*/
template void Scheduler::Forward<Entity, Heartbeat>(Entity*, Heartbeat*, Port,
                                                    Statistics&);
template void Scheduler::Forward<Entity, InitiateHeartbeat>(Entity*,
                                                            InitiateHeartbeat*,
                                                            Port, Statistics&);
template void Scheduler::Forward<Switch, LinkStateUpdate>(Switch*,
                                                          LinkStateUpdate*,
                                                          Port,
                                                          Statistics&);
template void Scheduler::Forward<Switch, InitiateLinkState>(Switch*,
                                                            InitiateLinkState*,
                                                            Port,
                                                            Statistics&);

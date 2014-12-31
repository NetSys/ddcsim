#include "entities.h"
#include "events.h"
#include "scheduler.h"
#include "statistics.h"

// #include <random>

// using std::default_random_engine;
// using std::discrete_distribution;
// using std::uniform_real_distribution;
using std::vector;

/* The type-specific parts of Scheduler::Forward are deferred to this class.
 * This functionality is implemented as a class rather than as a generic
 * method (with appropriate specializations) so that we can leverage partial
 * specialization, which is forbidden for methods but not classes.
 */
template<class E, class In, class Out> class Schedule {
 public:
  void operator()(E* sender, In* msg_in, Out* msg_out, Entity* reciever, Port in) {};
};

template<> class Schedule<Switch, Up, LinkStateUpdate> {
 public:
  void operator()(Switch* sender, Up* u, LinkStateUpdate* lsu, Entity* receiver,
                  Port in) {
    lsu->time_ = u->time_ + Scheduler::Delay();
    lsu->affected_entities_ = {receiver};
    lsu->in_port_ = in;
  }
};

template<> class Schedule<Switch, LinkUp, LinkStateUpdate> {
 public:
  void operator()(Switch* sender, LinkUp* lu, LinkStateUpdate* lsu,
                  Entity* receiver, Port in) {
    lsu->time_ = lu->time_ + Scheduler::Delay() + Scheduler::kDefaultHelloDelay;
    lsu->affected_entities_ = {receiver};
    lsu->in_port_ = in;
  }
};

template<> class Schedule<Switch, LinkDown, LinkStateUpdate> {
 public:
  void operator()(Switch* sender, LinkDown* ld, LinkStateUpdate* lsu,
                  Entity* receiver, Port in) {
    lsu->time_ = ld->time_ + Scheduler::Delay() + Scheduler::kDefaultHelloDelay;
    lsu->affected_entities_ = {receiver};
    lsu->in_port_ = in;
  }
};

template<> class Schedule<Switch, LinkStateUpdate, LinkStateUpdate> {
 public:
  void operator()(Switch* sender, LinkStateUpdate* lsu_in, LinkStateUpdate* lsu_out,
                  Entity* receiver, Port in) {
    lsu_out->time_ = lsu_in->time_ + Scheduler::Delay();
    lsu_out->affected_entities_ = {receiver};
    lsu_out->in_port_ = in;
  }
};

template<> class Schedule<Switch, InitiateLinkState, LinkStateUpdate> {
 public:
  void operator()(Switch* sender, InitiateLinkState* init, LinkStateUpdate* lsu,
                  Entity* receiver, Port in) {
    lsu->time_ = init->time_ + Scheduler::Delay();
    lsu->affected_entities_ = {receiver};
    lsu->in_port_ = in;
  }
};

template<> class Schedule<Controller, LinkStateUpdate, RoutingUpdate> {
 public:
  void operator()(Controller* sender, LinkStateUpdate* lsu, RoutingUpdate* ru,
                  Entity* receiver, Port in) {
    ru->time_ = lsu->time_ + Scheduler::Delay();
    ru->affected_entities_ = {receiver};
    ru->in_port_ = in;
  }
};

template<> class Schedule<Switch, RoutingUpdate, RoutingUpdate> {
 public:
  void operator()(Switch* sender, RoutingUpdate* ru_in, RoutingUpdate* ru_out,
                  Entity* receiver, Port in) {
    ru_out->time_ = ru_in->time_ + Scheduler::Delay();
    ru_out->affected_entities_ = {receiver};
    ru_out->in_port_ = in;
  }
};

template<> class Schedule<Controller, LinkStateUpdate, LinkStateRequest> {
 public:
  void operator()(Controller* sender, LinkStateUpdate* lsu_in,
                  LinkStateRequest* lsr_out, Entity* receiver, Port in) {
    lsr_out->time_ = lsu_in->time_ + Scheduler::Delay();
    lsr_out->affected_entities_ = {receiver};
    lsr_out->in_port_ = in;
  }
};

template<> class Schedule<Switch, LinkStateRequest, LinkStateRequest> {
 public:
  void operator()(Switch* sender, LinkStateRequest* lsr_in,
                  LinkStateRequest* lsr_out, Entity* receiver, Port in) {
    lsr_out->time_ = lsr_in->time_ + Scheduler::Delay();
    lsr_out->affected_entities_ = {receiver};
    lsr_out->in_port_ = in;
  }
};

template<> class Schedule<Switch, LinkStateRequest, LinkStateUpdate> {
 public:
  void operator()(Switch* sender, LinkStateRequest* lsr_in,
                  LinkStateUpdate* lsu_out, Entity* receiver, Port in) {
    lsu_out->time_ = lsr_in->time_ + Scheduler::Delay();
    lsu_out->affected_entities_ = {receiver};
    lsu_out->in_port_ = in;
  }
};

const Time Scheduler::kComputationDelay = 0.00001; /* 10 micros */
const Time Scheduler::kTransDelay = 0.001;         /* 1 ms */
const Time Scheduler::kPropDelay = 0.01;           /* 10 ms */
const Time Scheduler::kDefaultHelloDelay = 0.001;  /* 1 ms */

const Time Scheduler::kDefaultHeartbeatPeriod = 3;
const Time Scheduler::kDefaultLSUpdatePeriod = 3;
const Time Scheduler::kDefaultEndTime = 60;

Scheduler::Scheduler(Time end_time, size_t switch_count,
                     size_t controller_count, size_t host_count)
    : event_queue_(switch_count + controller_count + host_count),
      end_time_(end_time), kSwitchCount(switch_count),
      kControllerCount(controller_count), kHostCount(host_count) {}

void Scheduler::AddEvent(Time t, Event* e) { event_queue_.Push(t, e); }

bool Scheduler::HasNextEvent() { return ! event_queue_.Empty(); }

Event* Scheduler::NextEvent() { return event_queue_.Pop(); }

// TODO why isn't partial specialization of methods allowed?
template<class E, class In, class Out>
void Scheduler::Forward(E* sender, In* msg_in, Out* msg_out, Port out) {
  Links& l = sender->links();

  if(! l.IsLinkUp(out)) {
    delete msg_out;
    return;
  }

  Entity* receiver = CHECK_NOTNULL(l.GetEndpoint(out));

  Port in_port = receiver->links().GetPortTo(sender);
  CHECK_NE(in_port, PORT_NOT_FOUND);

  // TODO what the fuck compiler error...
  Schedule<E, In, Out> s;
  s(sender, msg_in, msg_out, receiver, in_port);

  if(msg_out->time_ <= end_time_)
    AddEvent(msg_out->time_, msg_out);
  else
    delete msg_out;
}

// TODO generalize
// TODO feed default_random_engine a seed to make it deterministic
void Scheduler::SchedulePeriodicEvents(vector<Switch*> switches,
                                       Time heartbeat_period,
                                       Time ls_update_period) {
  // default_random_engine entropy_src;

  // Time half_hrtbt = heartbeat_period / 2;
  // uniform_real_distribution<Time> hrtbt_init_dist(0, half_hrtbt);
  // uniform_real_distribution<Time> hrtbt_dist(-1 * half_hrtbt, half_hrtbt);

  // // TODO verify that it's okay to use entropy_src for both init_dist and dist
  // for(auto it : id_to_entity)
  //   AddEvent(new InitiateHeartbeat(hrtbt_init_dist(entropy_src),
  //                                        it.second));

  // // TODO verify semantics of end_time
  // for (Time t = heartbeat_period; t <= end_time_; t += heartbeat_period)
  //   for(auto it : id_to_entity)
  //     AddEvent(new InitiateHeartbeat(t + hrtbt_dist(entropy_src),
  //                                          it.second));

  // Time half_ls = ls_update_period / 2;
  // uniform_real_distribution<Time> ls_init_dist(0, half_ls);
  // uniform_real_distribution<Time> ls_dist(-1 * half_ls, half_ls);

  // for(auto it : id_to_entity)
  //   AddEvent(new InitiateLinkState(ls_init_dist(entropy_src), it.second));

  // for (Time t = ls_update_period; t <= end_time_; t += ls_update_period)
  //   for(auto it : id_to_entity)
  //     AddEvent(new InitiateLinkState(t, it.second));

  for(Switch* s : switches)
    AddEvent(START_TIME, new InitiateLinkState(START_TIME, s));
}

void Scheduler::StartSimulation(Statistics& statistics, vector<Switch*>& switches) {
  Time last_time = cur_time_ = START_TIME;

  // TODO move into statistics
  unsigned int cur_bucket_size = 0;
  Time cur_bucket_time = -1;

  while (HasNextEvent() && cur_time_ < end_time_) {
    Event* ev = NextEvent();

    last_time = cur_time_;
    cur_time_ = ev->time_;
    CHECK_GE(cur_time_, last_time);

    if (cur_time_ > cur_bucket_time) {
      LOG(WARNING) << cur_bucket_time << " had " << cur_bucket_size << " events";
      cur_bucket_time = cur_time_;
      cur_bucket_size = 1;
      statistics.RecordEventCounts();
      LOG(WARNING) << "\n";
    } else {
      cur_bucket_size++;
    }

    for (Entity* e : ev->affected_entities_) {
      DLOG(INFO) << e->Name() << " " << e->id() << " received event " << ev->Name() << ":" << ev->Description();
      ev->Handle(e);
      DLOG(INFO) << e->Description();
    }

    delete ev;
  }
}

Time Scheduler::cur_time() { return cur_time_; }

Time Scheduler::end_time() { return end_time_; }

Time Scheduler::Delay() {
  // TODO make these functions of link bandwidth and length
  return kComputationDelay + kTransDelay + kPropDelay;
}

bool Scheduler::IsHost(Id id) {
  return  kSwitchCount + kControllerCount <= id &&
      id < kSwitchCount + kControllerCount + kHostCount;
}

bool Scheduler::IsController(Id id) {
  return kSwitchCount <= id && id < kSwitchCount + kControllerCount;
}

bool Scheduler::IsSwitch(Id id) { return 0 <= id && id < kSwitchCount; }

// TODO how to force generic instantiations to respect subtype polymorphism?
/* TODO explain why we need to oblige the compiler to instantiate this templated
 * method explicity
 */
template void
Scheduler::Forward<Switch, Up, LinkStateUpdate>(Switch*,
                                                Up*,
                                                LinkStateUpdate*,
                                                Port);
template void
Scheduler::Forward<Switch, LinkUp, LinkStateUpdate>(Switch*,
                                                    LinkUp*,
                                                    LinkStateUpdate*,
                                                    Port);
template void
Scheduler::Forward<Switch, LinkDown, LinkStateUpdate>(Switch*,
                                                      LinkDown*,
                                                      LinkStateUpdate*,
                                                      Port);
template void
Scheduler::Forward<Switch, LinkStateUpdate, LinkStateUpdate>(Switch*,
                                                             LinkStateUpdate*,
                                                             LinkStateUpdate*,
                                                             Port);
template void
Scheduler::Forward<Switch, InitiateLinkState, LinkStateUpdate>(Switch*,
                                                               InitiateLinkState*,
                                                               LinkStateUpdate*,
                                                               Port);
template void
Scheduler::Forward<Controller, LinkStateUpdate, RoutingUpdate>(Controller*,
                                                               LinkStateUpdate*,
                                                               RoutingUpdate*,
                                                               Port);
template void
Scheduler::Forward<Switch, RoutingUpdate, RoutingUpdate>(Switch*,
                                                         RoutingUpdate*,
                                                         RoutingUpdate*,
                                                         Port);
template void
Scheduler::Forward<Controller, LinkStateUpdate, LinkStateRequest>(Controller*,
                                                                  LinkStateUpdate*,
                                                                  LinkStateRequest*,
                                                                  Port);
template void
Scheduler::Forward<Switch, LinkStateRequest, LinkStateRequest>(Switch*,
                                                               LinkStateRequest*,
                                                               LinkStateRequest*,
                                                               Port);
template void
Scheduler::Forward<Switch, LinkStateRequest, LinkStateUpdate>(Switch*,
                                                              LinkStateRequest*,
                                                              LinkStateUpdate*,
                                                              Port);

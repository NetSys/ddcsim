#include "entities.h"
#include "events.h"
#include "statistics.h"

#include <glog/logging.h>

using std::pair;
using std::vector;

#define LOG_HANDLE(level, type, var)                                    \
  LOG(level) << #type << " " << id_ << " received event " << var->Name() << ":"; \
  LOG(level) << var->Description();

HeartbeatHistory::HeartbeatHistory() : seen_() {}

void HeartbeatHistory::MarkAsSeen(const Heartbeat* b, Time time_seen) {
  // TODO this can throw an exception if seen's allocator fails.  Should I just
  // ignore this possibility?
  seen_.insert(MakeHeartbeatId(b));
  last_seen_.insert({b->src()->id(), time_seen});
}

bool HeartbeatHistory::HasBeenSeen(const Heartbeat* b) const {
  return seen_.count(MakeHeartbeatId(b)) > 0;
}

bool HeartbeatHistory::HasBeenSeen(Id id) const {
  return last_seen_.count(id) > 0;
}

Time HeartbeatHistory::LastSeen(Id id) const {
  return last_seen_.at(id);
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

Entity::Entity(Scheduler& sc, Id id, Statistics& st) : links_(), scheduler_(sc),
                                                       is_up_(true), id_(id),
                                                       heart_history_(),
                                                       next_heartbeat_(0),
                                                       stats_(st) {}

void Entity::Handle(Up* u) { is_up_ = true; }

void Entity::Handle(Down* d) { is_up_ = false; }

void Entity::Handle(Heartbeat* h) {
  stats_.Record(h);

  if(!is_up_ || heart_history_.HasBeenSeen(h)) return;

  for(Port p = 0; p < links_.PortCount(); ++p)
    if(h->in_port() != p)
      scheduler_.Forward(this, h, p);

  heart_history_.MarkAsSeen(h, scheduler_.cur_time());
}

void Entity::Handle(LinkUp* lu) { links_.SetLinkUp(lu->out_); }

void Entity::Handle(LinkDown* ld) { links_.SetLinkDown(ld->out_); }

void Entity::Handle(InitiateHeartbeat* init) {
  if(!is_up_) return;

  for(Port p = 0; p < links_.PortCount(); ++p)
    scheduler_.Forward(this, init, p);

  next_heartbeat_++;
}

Links& Entity::links() { return links_; }

Id Entity::id() const { return id_; }

SequenceNum Entity::NextHeartbeatSeqNum() const { return next_heartbeat_; }

// TODO how to avoid copying here?
vector<bool> Entity::ComputeRecentlySeen() const {
  vector<bool> recently_seen(Scheduler::kMaxEntities, false);

  for(Id id = 0; id < Scheduler::kMaxEntities; ++id)
    if(heart_history_.HasBeenSeen(id))
      recently_seen[id] =
          scheduler_.cur_time() - heart_history_.LastSeen(id) < kMaxRecent;

  return recently_seen;
}

void Entity::UpdateLinkCapacities(Time passed) {
  links_.UpdateCapacities(passed);
}

const Time Entity::kMaxRecent = 5;

Switch::Switch(Scheduler& sc, Id id, Statistics& st) : Entity(sc, id, st),
                                                       link_history_() {}

void Switch::Handle(Event* e) { LOG_HANDLE(ERROR, Switch, e) }

void Switch::Handle(Up* u) {
  LOG_HANDLE(INFO, Switch, u)

  Entity::Handle(u);
}

void Switch::Handle(Down* d) {
  LOG_HANDLE(INFO, Switch, d)

  Entity::Handle(d);
}

void Switch::Handle(Broadcast* b) { LOG_HANDLE(ERROR, Switch, b); }

void Switch::Handle(Heartbeat* h) {
  LOG_HANDLE(INFO, Switch, h)

  Entity::Handle(h);
}

void Switch::Handle(LinkUp* lu) {
  LOG_HANDLE(INFO, Switch, lu)

  Entity::Handle(lu);
}

void Switch::Handle(LinkDown* ld) {
  LOG_HANDLE(INFO, Switch, ld)

  Entity::Handle(ld);
}

void Switch::Handle(LinkAlert* alert) {
  LOG_HANDLE(INFO, Switch, alert)

  if(!is_up_) return;

  if(! (alert->is_up_ ^ link_history_.LinkIsUp(alert))) return;

  for(Port p = 0; p < links_.PortCount(); ++p)
    if(alert->in_port() != p)
      scheduler_.Forward(this, alert, p);

  if(alert->is_up_)
    link_history_.MarkAsUp(alert);
  else
    link_history_.MarkAsDown(alert);
}

void Switch::Handle(InitiateHeartbeat* init) {
  LOG_HANDLE(INFO, Switch, init)

  Entity::Handle(init);
}

Controller::Controller(Scheduler& sc, Id id, Statistics& st) : Entity(sc, id, st) {}

void Controller::Handle(Event* e) { LOG_HANDLE(ERROR, Controller, e) }

void Controller::Handle(Up* u) {
  LOG_HANDLE(INFO, Controller, u)

  Entity::Handle(u);
}

void Controller::Handle(Down* d) {
  LOG_HANDLE(INFO, Controller, d)

  Entity::Handle(d);
}

void Controller::Handle(Broadcast* b) { LOG_HANDLE(ERROR, Controller, b) }

void Controller::Handle(Heartbeat* h) {
  LOG_HANDLE(INFO, Controller, h)

  Entity::Handle(h);
}

void Controller::Handle(LinkUp* lu) {
  LOG_HANDLE(INFO, Controller, lu)

  Entity::Handle(lu);
}

void Controller::Handle(LinkDown* ld) {
  LOG_HANDLE(INFO, Controller, ld)

  Entity::Handle(ld);
}

void Controller::Handle(LinkAlert* alert) {
  LOG_HANDLE(INFO, Controller, alert);
}

void Controller::Handle(InitiateHeartbeat* init) {
  LOG_HANDLE(INFO, Controller, init)

  Entity::Handle(init);
}

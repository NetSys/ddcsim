#include "entities.h"
#include "events.h"

#include <glog/logging.h>

#include <algorithm>

using std::min;
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

// TODO how to push initilinks into constructor
Links::Links() : port_nums_(), port_to_link_(), port_to_size_(),
                 bucket_capacity(UNINIT_SIZE), fill_rate(UNINIT_RATE) {}

vector<Port>::const_iterator Links::PortsBegin() { return port_nums_.cbegin(); }

vector<Port>::const_iterator Links::PortsEnd() { return port_nums_.cend(); }

void Links::SetLinkUp(Port p) { port_to_link_[p].first = true; }

void Links::SetLinkDown(Port p) { port_to_link_[p].first = false; }

void Links::UpdateCapacities(Time passed) {
  /* Token buckets fill up at a rate of fill_rate bytes/sec so for each
   * link, we need to add fill_rate * (cur_time_ - last_time) bytes to its
   * bucket UNLESS it is already full.  Thus, we update each token bucket by
   * size = min{ size + fill_rate * (cur_time_ - last_time), bucket_capacity }
   */

  for(auto it = port_to_size_.begin(); it != port_to_size_.end(); ++it)
    // TODO having to cast to a double is bullshit
    // why the fuck did I alias the type in the first place
    it->second = min(static_cast<double>(bucket_capacity),
                     static_cast<double>(it->second + fill_rate * passed));
}

const Size Links::kDefaultCapacity = 1 << 31;

const Rate Links::kDefaultRate = 1 << 31;

bool Links::IsLinkUp(Port p) { return port_to_link_[p].first; }

Entity* Links::GetEndpoint(Port p) { return port_to_link_[p].second; }

Port Links::GetPortTo(Entity* endpoint) {
  for(auto it = LinksBegin(); it != LinksEnd(); ++it)
    if(it->second.second == endpoint)
      return it->first;
  return PORT_NOT_FOUND;
}

Port Links::FindInPort(Entity* sender) {
  for(auto it = LinksBegin(); it != LinksEnd(); ++it)
    if(sender == it->second.second)
      return it->first;
  return PORT_NOT_FOUND;
}

std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator
Links::LinksBegin() { return port_to_link_.cbegin(); }

std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator
Links::LinksEnd() { return port_to_link_.cend(); }

Entity::Entity(Scheduler& s) : links_(), scheduler_(s), is_up_(true),
                               id_(NONE_ID), heart_history_(),
                               next_heartbeat_(0) {}

Entity::Entity(Scheduler& s, Id id) : links_(), scheduler_(s), is_up_(true),
                                      id_(id), heart_history_(),
                                      next_heartbeat_(0) {}

void Entity::Handle(Up* u) { is_up_ = true; }

void Entity::Handle(Down* d) { is_up_ = false; }

void Entity::Handle(Heartbeat* h) {
  if(!is_up_ || heart_history_.HasBeenSeen(h)) return;

  for(auto it = links_.PortsBegin(); it != links_.PortsEnd(); ++it)
    if(h->in_port() != *it)
      scheduler_.Forward(this, h, *it);

  heart_history_.MarkAsSeen(h, scheduler_.cur_time());
}

void Entity::Handle(LinkUp* lu) { links_.SetLinkUp(lu->out_); }

void Entity::Handle(LinkDown* ld) { links_.SetLinkDown(ld->out_); }

void Entity::Handle(InitiateHeartbeat* init) {
  if(!is_up_) return;

  for(auto it = links_.PortsBegin(); it != links_.PortsEnd(); ++it)
    scheduler_.Forward(this, init, *it);

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

Switch::Switch(Scheduler& s) : Entity(s), link_history_() {}

Switch::Switch(Scheduler& s, Id id) : Entity(s, id), link_history_() {}

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

  for(auto it = links_.PortsBegin(); it != links_.PortsEnd(); ++it)
    if(alert->in_port() != *it)
      scheduler_.Forward(this, alert, *it);

  if(alert->is_up_)
    link_history_.MarkAsUp(alert);
  else
    link_history_.MarkAsDown(alert);
}

void Switch::Handle(InitiateHeartbeat* init) {
  LOG_HANDLE(INFO, Switch, init)

  Entity::Handle(init);
}

Controller::Controller(Scheduler& s) : Entity(s) {}

Controller::Controller(Scheduler& s, Id id) : Entity(s, id) {}

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

#include "entities.h"
#include "events.h"
#include "scheduler.h"
#include "statistics.h"

#include <array>
#include <limits>

using std::array;
using std::default_random_engine;
using std::discrete_distribution;
using std::numeric_limits;
using std::string;
using std::to_string;
using std::vector;

Entity::Entity(Scheduler& sc, Id id, Statistics& st) : links_(), scheduler_(sc),
                                                       is_up_(true), id_(id),
                                                       stats_(st) {
                                                       // entropy_src_(),
                                                       // dist_{1, 999} {
  CHECK_GE(kMinTimes, 1);
}

string Entity::Description() const { return "..."; }

string Entity::Name() const { return "Entity"; }

void Entity::Handle(Up* u) {
  CHECK(!is_up_);
  is_up_ = true;
  stats_.EntityUp(id_);
}

void Entity::Handle(Down* d) {
  CHECK(is_up_);
  is_up_ = false;
  stats_.EntityDown(id_);
}

void Entity::Handle(LinkUp* lu) {
  CHECK(!links_.IsLinkUp(lu->out_));
  links_.SetLinkUp(lu->out_);
  stats_.LinkUp(id_, links_.GetEndpoint(lu->out_)->id());
}

void Entity::Handle(LinkDown* ld) {
  CHECK(links_.IsLinkUp(ld->out_));
  links_.SetLinkDown(ld->out_);
  stats_.LinkDown(id_, links_.GetEndpoint(ld->out_)->id());
}

Links& Entity::links() { return links_; }

Id Entity::id() const { return id_; }

// void Entity::UpdateLinkCapacities(Time passed) {
//   links_.UpdateCapacities(passed);
// }

const Time Entity::kMaxRecent = 3;
const unsigned int Entity::kMinTimes = 2;

RoutingUpdateHistory::RoutingUpdateHistory() : last_seen_(), dst_to_next_sn_() {}

string RoutingUpdateHistory::Description() const { return "TODO"; }

void RoutingUpdateHistory::MarkAsSeen(const RoutingUpdate* e) {
  last_seen_[{e->src_id_, e->dst_}] = e->sn_;
}

bool RoutingUpdateHistory::HasBeenSeen(const RoutingUpdate* e) const {
  Id src = e->src_id_;
  Id dst = e->dst_;
  if(last_seen_.count({src,dst}) == 0)
    return false;
  else
    return last_seen_.at({src,dst}) >= e->sn_;
}

SequenceNum RoutingUpdateHistory::NextSeqNum(Id dst) {
  if(dst_to_next_sn_.count(dst) == 0)
    dst_to_next_sn_[dst] = 0;

  SequenceNum& sn = dst_to_next_sn_[dst];
  return sn++;
}

Switch::Switch(Scheduler& sc, Id id, Statistics& st) :
    Entity(sc, id, st), ls_(sc), ru_history_(),
    lsr_history_(sc.kControllerCount, NONE_SEQNUM),
    cv_history_(sc.kControllerCount, NONE_SEQNUM),
    dst_to_neighbor_(nullptr) {}

const Time Switch::kLSExpireDelta = numeric_limits<Time>::max();

string Switch::Description() const {
  string rtn = Entity::Description() + " ls_=" + ls_.Description() +
      " dst_to_neighbor_=";

  if (dst_to_neighbor_) {
    for(int i : *dst_to_neighbor_)
      rtn += to_string(i) + " ";
  } else {
    rtn += "null";
  }

  return rtn;
}

string Switch::Name() const { return "Switch"; }

void Switch::Handle(Event* e) {
  LOG(FATAL) << "Switch received a naked Event";
}

void Switch::Handle(Up* u) {
  Entity::Handle(u);

  LinkStateUpdate* lsu;
  SequenceNum cur_sn = ls_.NextSeqNum();
  array<Id, 13> up_links = links_.UpNeighbors();
  Time exp = u->time_ + kLSExpireDelta;
  bool update_self = true;

  for(Port out_port = 0; out_port < links_.PortCount(); ++out_port) {
    CHECK_EQ(u->time_, scheduler_.cur_time());
    if(! scheduler_.IsHost(links_.GetEndpointId(out_port))) {
      lsu = new LinkStateUpdate(START_TIME,
                                NULL,
                                PORT_NOT_FOUND,
                                this,
                                cur_sn,
                                exp,
                                up_links,
                                id_);
      if(update_self) {
          ls_.Update(lsu);
          update_self = false;
      }

      scheduler_.Forward(this, u, lsu, out_port);
    }
  }
}

void Switch::Handle(Down* d) { Entity::Handle(d); }

void Switch::Handle(Broadcast* b) {
  LOG(FATAL) << "Switch received a naked Broadcast";
}

void Switch::Handle(LinkUp* lu) {
  CHECK(dst_to_neighbor_);
  Entity::Handle(lu);

  if(!is_up_) {
    DLOG(INFO) << "Switch is down";
    return;
  }

  LinkStateUpdate* lsu;
  SequenceNum cur_sn = ls_.NextSeqNum();
  array<Id, 13> up_links = links_.UpNeighbors();
  Time exp = lu->time_ + kLSExpireDelta + Scheduler::kDefaultHelloDelay;
  bool update_self = true;

  for(Port p = 0; p < links_.PortCount(); ++p) {
    // TODO should be taking kComputationDelay into account with expiration...
    CHECK_EQ(lu->time_, scheduler_.cur_time());
    if(! scheduler_.IsHost(links_.GetEndpointId(p))) {
      lsu = new LinkStateUpdate(START_TIME,
                                NULL,
                                PORT_NOT_FOUND,
                                this,
                                cur_sn,
                                exp,
                                up_links,
                                id_);

      if(update_self) {
        ls_.Update(lsu);
        update_self = false;
      }

      scheduler_.Forward(this, lu, lsu, p);
    }
  }
}

void Switch::Handle(LinkDown* ld) {
  CHECK(dst_to_neighbor_);
  Entity::Handle(ld);

  if(!is_up_) {
    DLOG(INFO) << "Switch is down";
    return;
  }

  LinkStateUpdate* lsu;
  SequenceNum cur_sn = ls_.NextSeqNum();
  array<Id, 13> up_links = links_.UpNeighbors();
  Time exp = ld->time_ + kLSExpireDelta + Scheduler::kDefaultHelloDelay;
  bool update_self = true;

  for(Port p = 0; p < links_.PortCount(); ++p) {
    // TODO should be taking kComputationDelay into account with expiration...
    CHECK_EQ(ld->time_, scheduler_.cur_time());
    if(! scheduler_.IsHost(links_.GetEndpointId(p))) {
      lsu = new LinkStateUpdate(START_TIME,
                                NULL,
                                PORT_NOT_FOUND,
                                this,
                                cur_sn,
                                exp,
                                up_links,
                                id_);

      if(update_self) {
        ls_.Update(lsu);
        update_self = false;
      }

      scheduler_.Forward(this, ld, lsu, p);
    }
  }
}

void Switch::Handle(LinkStateUpdate* lsu) {
  if(!is_up_) {
    DLOG(INFO) << "Switch is down";
    return;
  }

  if(scheduler_.cur_time() > lsu->expiration_) {
    DLOG(INFO) << "Link state update died of old age";
    return;
  }

  //  ls_.Refresh(scheduler_.cur_time());

  if(ls_.IsStaleUpdate(lsu)) {
    DLOG(INFO) << "Link state update is stale";
    if(ls_.HaveNewUpdate(lsu)) {
      scheduler_.Forward(this,
                         lsu,
                         ls_.CurrentLinkState(lsu->src_, lsu->src_id_),
                         lsu->in_port_);
    }
    return;
  }

  ls_.Update(lsu);

  LinkStateUpdate* out_lsu;
  Time t = lsu->time_ + Scheduler::Delay();

  for (Port p = 0, in = lsu->in_port_; p < links_.PortCount(); ++p) {
    if (in != p && ! scheduler_.IsHost(links_.GetEndpointId(p))) {
        out_lsu = new LinkStateUpdate(START_TIME,
                                      NULL,
                                      PORT_NOT_FOUND,
                                      lsu->src_,
                                      lsu->sn_,
                                      lsu->expiration_,
                                      lsu->up_links_,
                                      lsu->src_id_,
                                      lsu->is_from_lsr_);
        scheduler_.Forward(this, lsu, out_lsu, p);
    }
  }
}

void Switch::Handle(InitiateLinkState* init) {
  if(!is_up_) {
    DLOG(INFO) << "Switch is down";
    return;
  }

  LinkStateUpdate* lsu;
  SequenceNum cur_sn = ls_.NextSeqNum();
  array<Id, 13> up_links = links_.UpNeighbors();
  Time exp = init->time_ + kLSExpireDelta;
  bool update_self = true;

  for(Port out_port = 0; out_port < links_.PortCount(); ++out_port) {
    if(! scheduler_.IsHost(links_.GetEndpointId(out_port))) {
      lsu = new LinkStateUpdate(START_TIME,
                                NULL,
                                PORT_NOT_FOUND,
                                this,
                                cur_sn,
                                exp,
                                up_links,
                                id_);

      if(update_self) {
        ls_.Update(lsu);
        update_self = false;
      }

      scheduler_.Forward(this, init, lsu, out_port);
    }
  }
}

void Switch::Handle(RoutingUpdate* ru) {
  if(!is_up_) {
    DLOG(INFO) << "Switch is down";
    return;
  }

  if(ru_history_.HasBeenSeen(ru)) {
    DLOG(INFO) << "Routing update has already been seen";
    return;
  }

  ru_history_.MarkAsSeen(ru);

  if(ru->dst_ == id_) {
    DLOG(INFO) << "Routing update was directed at me";
    dst_to_neighbor_ = ru->dst_to_neighbor_;
  } else {
    RoutingUpdate* ru_out;
    for(Port p = 0, in = ru->in_port_; p < links_.PortCount(); ++p) {
      if(p != in && scheduler_.IsSwitch(links_.GetEndpointId(p))) {
        ru_out = new RoutingUpdate(START_TIME,
                                   NULL,
                                   PORT_NOT_FOUND,
                                   ru->src_,
                                   ru->sn_,
                                   ru->dst_to_neighbor_,
                                   ru->dst_,
                                   ru->src_id_);
        scheduler_.Forward(this, ru, ru_out, p);
      }
    }
  }
}

void Switch::Handle(LinkStateRequest* lsr_in) {
  if(!is_up_) {
    DLOG(INFO) << "Switch is down";
    return;
  }

  if(lsr_history_[lsr_in->src_id_ - scheduler_.kSwitchCount] >= lsr_in->sn_) {
    DLOG(INFO) << "LinkStateRequest has already been seen";
    return;
  }

  lsr_history_[lsr_in->src_id_ - scheduler_.kSwitchCount] = lsr_in->sn_;

  LinkStateUpdate* lsu;
  SequenceNum cur_sn = ls_.NextSeqNum();
  array<Id, 13> up_links = links_.UpNeighbors();
  Time exp = lsr_in->time_ + kLSExpireDelta;
  bool update_self = true;

  for(Port out_port = 0; out_port < links_.PortCount(); ++out_port) {
    if(! scheduler_.IsHost(links_.GetEndpointId(out_port))) {
      lsu = new LinkStateUpdate(START_TIME,
                                NULL,
                                PORT_NOT_FOUND,
                                this,
                                cur_sn,
                                exp,
                                up_links,
                                id_,
                                true);

      if(update_self) {
        ls_.Update(lsu);
        update_self = false;
      }

      scheduler_.Forward(this, lsr_in, lsu, out_port);
    }
  }

  LinkStateRequest* lsr_out;
  for(Port out_port = 0; out_port < links_.PortCount(); ++out_port) {
    if(out_port != lsr_in->in_port_ &&
       scheduler_.IsSwitch(links_.GetEndpointId(out_port))) {
      lsr_out = new LinkStateRequest(START_TIME,
                                     NULL,
                                     PORT_NOT_FOUND,
                                     lsr_in->src_,
                                     lsr_in->sn_,
                                     lsr_in->src_id_);
      scheduler_.Forward(this, lsr_in, lsr_out, out_port);
    }
  }
}

void Switch::Handle(ControllerView* cv) {
  if(!is_up_) {
    DLOG(INFO) << "Switch is down";
    return;
  }

  if(cv_history_[cv->src_id_ - scheduler_.kSwitchCount] >= cv->sn_) {
    DLOG(INFO) << "ControllerView has already been seen";
    return;
  }

  cv_history_[cv->src_id_ - scheduler_.kSwitchCount] = cv->sn_;

  for(Port p = 0; p < links_.PortCount(); ++p) {
    if(p != cv->in_port_ && !scheduler_.IsHost(links_.GetEndpointId(p))) {
      scheduler_.Forward(this,
                         cv,
                         new ControllerView(START_TIME,
                                            NULL,
                                            PORT_NOT_FOUND,
                                            cv->src_,
                                            cv->sn_,
                                            cv->src_id_,
                                            cv->topology_,
                                            cv->id_to_last_),
                         p);
    }
  }
}

void Switch::Handle(InitiateRoutingUpdate* iru) {
  LOG(FATAL) << "Switch received initiate routing update";
}

Controller::Controller(Scheduler& sc, Id id, Statistics& st)
    : Entity(sc, id, st), ls_(sc), switch_to_next_sn_(sc.kSwitchCount, 0),
      next_lsr_(0), cv_history_(sc.kControllerCount, NONE_SEQNUM) {
  cv_history_[id_ - sc.kSwitchCount] = 0;
}

string Controller::Description() const {
  return Entity::Description() + " " + ls_.Description();
}

string Controller::Name() const { return "Controller"; }

void Controller::Handle(Event* e) {
  LOG(FATAL) << "Controller received a naked Event";
}

void Controller::Handle(Up* u) { Entity::Handle(u); }

void Controller::Handle(Down* d) { Entity::Handle(d); }

void Controller::Handle(Broadcast* b) {
  LOG(FATAL) << "Controller received a naked Broadcast";
}

void Controller::Handle(LinkUp* lu) { Entity::Handle(lu); }

void Controller::Handle(LinkDown* ld) { Entity::Handle(ld); }

void Controller::Handle(LinkStateUpdate* lsu) {
  if(!is_up_) {
    DLOG(INFO) << "Controller is down";
    return;
  }

  if(scheduler_.cur_time() > lsu->expiration_) {
    DLOG(INFO) << "Link state update died of old age";
    return;
  }

  if(ls_.IsStaleUpdate(lsu)) {
    DLOG(INFO) << "Link state update is stale";
    return;
  }

  bool heals_partition = ls_.HealsPartition(id_, lsu);

  bool changed = ls_.Update(lsu);

  if(!changed)
    return;

  SequenceNum& sn = cv_history_[id_ - scheduler_.kSwitchCount];

  if(lsu->time_ > stats_.topo_diameter() * Scheduler::Delay()) {
    for(Port p = 0; p < links_.PortCount(); ++p) {
      scheduler_.Forward(this,
                         lsu,
                         new ControllerView(START_TIME,
                                            NULL,
                                            PORT_NOT_FOUND,
                                            this,
                                            sn,
                                            id_,
                                            ls_.topology(),
                                            ls_.id_to_last()),
                         p);
    }
    ++sn;
  }

  if(lsu->time_ > stats_.topo_diameter() * Scheduler::Delay()) {
    ls_.ComputePartitions();
    auto switches = ls_.SwitchesInParition(id_);
    for(auto it = switches.begin(); it != switches.end(); ++it) {
      Id cur = *it;
      auto table = ls_.ComputeRoutingTable(cur);
      SequenceNum sn = switch_to_next_sn_[cur];
      RoutingUpdate* ru;
      for(Port p = 0; p < links_.PortCount(); ++p) {
        if(scheduler_.IsSwitch(links_.GetEndpointId(p))) {
          ru = new RoutingUpdate(START_TIME,
                                 NULL,
                                 PORT_NOT_FOUND,
                                 this,
                                 sn,
                                 table,
                                 cur,
                                 id_);
          scheduler_.Forward(this, lsu, ru, p);
        }
      }

      switch_to_next_sn_[cur]++;
    }
  }

  if(heals_partition && lsu->time_ >
     stats_.topo_diameter() * Scheduler::Delay()) {
    DLOG(INFO) << "Switch was partitioned";

    LinkStateRequest* lsr;
    for(Port p = 0; p < links_.PortCount(); ++p) {
      lsr = new LinkStateRequest(START_TIME,
                                 NULL,
                                 PORT_NOT_FOUND,
                                 this,
                                 next_lsr_,
                                 id_);
      scheduler_.Forward(this, lsu, lsr, p);
    }

    next_lsr_++;
  }
}

void Controller::Handle(InitiateLinkState* init) {
  LOG(FATAL) << "Controller received InitiateLinkState";
}

void Controller::Handle(RoutingUpdate* ru) {
  LOG(FATAL) << "Controller received routing update";
}

void Controller::Handle(LinkStateRequest* ru) {
  LOG(FATAL) << "Controller received LinkStateRequest";
}

void Controller::Handle(ControllerView* cv) {
  if(!is_up_) {
    DLOG(INFO) << "Controller is down";
    return;
  }

  if(cv_history_[cv->src_id_ - scheduler_.kSwitchCount] >= cv->sn_) {
    DLOG(INFO) << "ControllerView is stale";
    return;
  }

  cv_history_[cv->src_id_ - scheduler_.kSwitchCount] = cv->sn_;

  ls_.Update(cv);
}

void Controller::Handle(InitiateRoutingUpdate* iru) {
  ls_.ComputePartitions();

  auto switches = ls_.SwitchesInParition(id_);
  for(auto it = switches.begin(); it != switches.end(); ++it) {
    Id cur = *it;
    auto table = ls_.ComputeRoutingTable(cur);
    SequenceNum sn = switch_to_next_sn_[cur];
    RoutingUpdate* ru;
    for(Port p = 0; p < links_.PortCount(); ++p) {
      if(scheduler_.IsSwitch(links_.GetEndpointId(p))) {
        ru = new RoutingUpdate(START_TIME,
                               NULL,
                               PORT_NOT_FOUND,
                               this,
                               sn,
                               table,
                               cur,
                               id_);
        scheduler_.Forward(this, iru, ru, p);
      }
    }

    switch_to_next_sn_[cur]++;
  }
}

Host::Host(Scheduler& sched, Id id, Statistics& stats) : Entity(sched, id, stats) {}

string Host::Description() const { return "..."; }

string Host::Name() const { return "Host"; }

void Host::Handle(Event* e) {
  LOG(FATAL) << "Host received a naked Event";
}

void Host::Handle(Up* u) { Entity::Handle(u); }

void Host::Handle(Down* d) { Entity::Handle(d); }

void Host::Handle(Broadcast* b) {
  LOG(FATAL) << "Broadcast received a naked Broadcast";
}

void Host::Handle(LinkUp* lu) { Entity::Handle(lu); }

void Host::Handle(LinkDown* ld) { Entity::Handle(ld); }

void Host::Handle(LinkStateUpdate* lsu) {
  DLOG(INFO) << "Disregarding link state update";
}

void Host::Handle(InitiateLinkState* init) {
  LOG(FATAL) << "Host received an InitiateLinkState";
}

void Host::Handle(RoutingUpdate* ru) {
  LOG(FATAL) << "Host received routing update";
}

void Host::Handle(LinkStateRequest* lsr) {
  LOG(FATAL) << "Host received link state request";
}

void Host::Handle(ControllerView*) {
  LOG(FATAL) << "Host received controller view";
}

void Host::Handle(InitiateRoutingUpdate* iru) {
  LOG(FATAL) << "Host received initiate routing update";
}

Id Host::EdgeSwitch() {
  /* Assume hosts are singly homed, so their next hop is always the one switch
     they are connected to */
  Port p;
  Id rtn;

  for(p = 0; p < links_.PortCount(); ++p) {
    rtn = links_.GetEndpointId(p);
    if(rtn < scheduler_.kSwitchCount)
      break;
  }

  ++p;

  for(; p < links_.PortCount(); ++p)
    CHECK(links_.GetEndpointId(p) >= scheduler_.kSwitchCount);

  return rtn;
}

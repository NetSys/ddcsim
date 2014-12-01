#include "bv.h"
#include "entities.h"
#include "events.h"
#include "scheduler.h"
#include "statistics.h"

#include <glog/logging.h>

#include <iostream>
#include <tuple>

using boost::circular_buffer;

using boost::add_edge;
using boost::clear_vertex;
using boost::out_edges;
using boost::remove_edge;
using boost::remove_vertex;

using std::default_random_engine;
using std::discrete_distribution;
using std::pair;
using std::tie;
using std::unordered_map;
using std::vector;

#define LOG_HANDLE(level, type, var)                                    \
  if (level >= FLAGS_minloglevel) {                                     \
    LOG(level) << #type << " " << id_ << " received event " << var->Name() << ":" << var->Description(); \
  }

HeartbeatHistory::HeartbeatHistory() : seen_(), last_seen_(),
                                       id_to_recently_seen_() {}

void HeartbeatHistory::MarkAsSeen(const Heartbeat* b, Time time_seen) {
  // TODO this can throw an exception if seen's allocator fails.  Should I just
  // ignore this possibility?
  seen_.insert(MakeHeartbeatId(b));

  Id id = b->src_->id();

  if(!HasBeenSeen(id))
    last_seen_.insert({id, circular_buffer<Time>(Entity::kMinTimes)});
  circular_buffer<Time>& times = last_seen_[id];
  times.push_back(time_seen);

  id_to_recently_seen_.erase(id);
  //  id_to_recently_seen_.insert({id, b->recently_seen()});
}

bool HeartbeatHistory::HasBeenSeen(const Heartbeat* b) const {
  return seen_.count(MakeHeartbeatId(b)) > 0;
}

bool HeartbeatHistory::HasBeenSeen(Id id) const {
  return last_seen_.count(id) > 0;
}

circular_buffer<Time> HeartbeatHistory::LastSeen(Id id) const {
  return last_seen_.at(id);
}

unordered_map<Id, vector<bool> > HeartbeatHistory::id_to_recently_seen() const {
  return id_to_recently_seen_;
}

HeartbeatHistory::HeartbeatId HeartbeatHistory::MakeHeartbeatId(const Heartbeat* b) {
  return {b->sn_, b->src_};
}

LinkState::LinkState(unsigned int num_entities)
    : id_to_last_seq_num_(num_entities, NONE_SEQNUM),
      id_to_exp_(num_entities, 0),
      topology_() {}

bool LinkState::IsStaleUpdate(LinkStateUpdate* ls) {
  return id_to_last_seq_num_[ls->src_->id()] < ls->sn_;
}

void LinkState::Update(LinkStateUpdate* ls) {
  Id src = ls->src_->id();

  // TODO how to remove edges with a single function call
  OutEdgeIter ei, ei_end;
  for (tie(ei, ei_end) = out_edges(src, topology_); ei != ei_end; ++ei)
    remove_edge(*ei, topology_);

  for(auto dst : ls->neighbors_)
    add_edge(src, dst, topology_);

  id_to_last_seq_num_[src] = ls->sn_;
  id_to_exp_[src] = ls->expiration_;
}

void LinkState::Refresh(Time cur_time) {
  for(int id = 0; id < id_to_exp_.size(); ++id) {
    if(id_to_last_seq_num_[id] != NONE_SEQNUM && cur_time > id_to_exp_[id]) {
      id_to_last_seq_num_[id] = NONE_SEQNUM;
      clear_vertex(id, topology_);
      remove_vertex(id, topology_);
    }
  }
}

Entity::Entity(Scheduler& sc, Id id, Statistics& st) : links_(), scheduler_(sc),
                                                       is_up_(true), id_(id),
                                                       heart_history_(),
                                                       next_heartbeat_(0),
                                                       stats_(st),
                                                       entropy_src_(),
                                                       cached_bv_(nullptr, nullptr),
                                                       is_cache_valid_(false),
                                                       dist_{1, 999} {
  CHECK_GE(kMinTimes, 1);
}

void Entity::Handle(Up* u) { is_up_ = true; }

void Entity::Handle(Down* d) { is_up_ = false; }

void Entity::Handle(Heartbeat* h) {
  if(dist_(entropy_src_) == 0) {
    LOG(INFO) << "Packet dropped randomly";
    return;
  }

  if(!is_up_) {
    LOG(INFO) << "Entity is down";
    return;
  }

  if(heart_history_.HasBeenSeen(h)) {
    LOG(INFO) << "Heartbeat has already been seen";
    return;
  }

  is_cache_valid_ = false;

  for(Port p = 0, in = h->in_port_; p < links_.PortCount(); ++p)
    if(in != p)
      scheduler_.Forward(this, h, p, stats_);

  heart_history_.MarkAsSeen(h, scheduler_.cur_time());
}

void Entity::Handle(LinkUp* lu) { links_.SetLinkUp(lu->out_); }

void Entity::Handle(LinkDown* ld) { links_.SetLinkDown(ld->out_); }

void Entity::Handle(InitiateHeartbeat* init) {
  if(!is_up_) {
    LOG(INFO) << "Entity is down";
    return;
  }

  for(Port p = 0; p < links_.PortCount(); ++p)
    scheduler_.Forward(this, init, p, stats_);

  next_heartbeat_++;
}

Links& Entity::links() { return links_; }

Id Entity::id() const { return id_; }

SequenceNum Entity::NextHeartbeatSeqNum() const { return next_heartbeat_; }

BV Entity::ComputeRecentlySeen() {
  if(!is_cache_valid_) {
    vector<bool>* rs = new vector<bool>(scheduler_.num_entities(), false);
    vector<bool>& recently_seen = *rs;

    for(Id id = 0; id < scheduler_.num_entities(); ++id) {
      if(heart_history_.HasBeenSeen(id)) {
        circular_buffer<Time> times_seen = heart_history_.LastSeen(id);
        recently_seen[id] = true;
        for(auto t = times_seen.begin(); t != times_seen.end(); ++t)
          recently_seen[id] = recently_seen[id] &&
              scheduler_.cur_time() - *t < kMaxRecent;
      }
    }

    if(cached_bv_.ref_count_ != NULL || cached_bv_.bv_ != NULL) {
      CHECK_NOTNULL(cached_bv_.ref_count_);
      CHECK_NOTNULL(cached_bv_.bv_);
      --(*(cached_bv_.ref_count_));
      if(*(cached_bv_.ref_count_) == 0) {
        delete cached_bv_.bv_;
        delete cached_bv_.ref_count_;
      }
    }

    cached_bv_ = BV(rs, new unsigned int(1));
    is_cache_valid_ = true;
  }

  return cached_bv_;
}

class Visitor {
 public:
  // TODO fix this style of abstract base class
  Visitor() {}
  virtual void previsit(Id) {}
  virtual void postvisit(Id) {}
  virtual void preexplore(Id) {}
  virtual void postexplore(Id) {}
 private:
  DISALLOW_COPY_AND_ASSIGN(Visitor);
};

class PostOrder : public Visitor {
 public:
  PostOrder() : ascending_post_order_() {}
  void postvisit(Id id) { ascending_post_order_.push_back(id); }
  vector<Id> ascending_post_order() { return ascending_post_order_; }
 private:
  vector<Id> ascending_post_order_;
  DISALLOW_COPY_AND_ASSIGN(PostOrder);
};

class ResolveSCC : public Visitor {
 public:
  ResolveSCC(unsigned int num_entities) : cur_scc_(0), id_to_scc_(num_entities) {}
  void previsit(Id i) { id_to_scc_[i] = cur_scc_; }
  void postexplore(Id i) { ++cur_scc_; }
  vector<unsigned int> id_to_scc() { return id_to_scc_; }
 private:
  unsigned int cur_scc_;
  vector<unsigned int> id_to_scc_;
  DISALLOW_COPY_AND_ASSIGN(ResolveSCC);
};

void dfs(Id root, unordered_map<Id, vector<bool> >& graph, unsigned int num_entities,
         vector<bool>& seen, Visitor& v, bool rev) {
  seen[root] = true;

  v.previsit(root);

  for (Id i = 0; i < num_entities; ++i) {
    bool edge_exists = rev ? graph.count(i) > 0 && graph[i][root] :
        graph.count(root) > 0 && graph[root][i];
    if(edge_exists && !seen[i]) {
      dfs(i, graph, num_entities, seen, v, rev);
    }
  }

  v.postvisit(root);
}

void explore1(unordered_map<Id, vector<bool> >& graph, unsigned int num_entities,
              Visitor& v) {
  vector<bool> seen(num_entities, false);

  for(Id i = 0; i < num_entities; ++i) {
    if(!seen[i]) {
      v.preexplore(i);
      dfs(i, graph, num_entities, seen, v, true);
      v.postexplore(i);
    }
  }
}

template<class Iter>
void explore2(unordered_map<Id, vector<bool> >& graph, Visitor& v, unsigned int num_entities,
              Iter next, Iter beyond) {
  vector<bool> seen(num_entities, false);

  for( ; next < beyond; ++next) {
    Id i = *next;
    if(!seen[i]) {
      v.preexplore(i);
      dfs(i, graph, num_entities, seen, v, false);
      v.postexplore(i);
    }
  }
}

vector<unsigned int> Entity::ComputePartitions() const {
  unordered_map<Id, vector<bool> > recently_seen =
      heart_history_.id_to_recently_seen();

  unsigned int num_entities = scheduler_.num_entities();

  // TODO change to explicit stack
  // TODO how to avoid iterating through bitvectors?
  // TODO combine explore1 and explore2
  PostOrder po;
  explore1(recently_seen, num_entities, po);
  auto visit_order = po.ascending_post_order();

  ResolveSCC rscc(num_entities);
  explore2(recently_seen, rscc, num_entities, visit_order.rbegin(), visit_order.rend());

  return rscc.id_to_scc();
}

void Entity::UpdateLinkCapacities(Time passed) {
  links_.UpdateCapacities(passed);
}

const Time Entity::kMaxRecent = 3;
const unsigned int Entity::kMinTimes = 2;

Switch::Switch(Scheduler& sc, Id id, Statistics& st) : Entity(sc, id, st),
                                                       next_link_state_(0),
                                                       link_state_(sc.num_entities()) {}

void Switch::Handle(Event* e) { LOG_HANDLE(ERROR, Switch, e) }

void Switch::Handle(Up* u) {
  LOG_HANDLE(INFO, Switch, u)

  scheduler_.AddEvent(new InitiateLinkState(u->time_, this));

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

  // TODO fix hack with factories
  // TODO allow setting in commandline?
  scheduler_.AddEvent(
      new InitiateLinkState(lu->time_ + Scheduler::kDefaultHelloDelay, this));
}

void Switch::Handle(LinkDown* ld) {
  LOG_HANDLE(INFO, Switch, ld)

  Entity::Handle(ld);

  scheduler_.AddEvent(
      new InitiateLinkState(ld->time_ + Scheduler::kDefaultHelloDelay, this));
}

void Switch::Handle(InitiateHeartbeat* init) {
  LOG_HANDLE(INFO, Switch, init)

  Entity::Handle(init);
}

void Switch::Handle(LinkStateUpdate* ls) {
  LOG_HANDLE(INFO, Switch, ls);

  if(!is_up_) {
    LOG(INFO) << "Entity is down";
    return;
  }

  if(scheduler_.cur_time() > ls->expiration_) {
    LOG(INFO) << "Link state update died of old age";
    return;
  }

  link_state_.Refresh(scheduler_.cur_time());

  if(link_state_.IsStaleUpdate(ls)) {
    // TODO forward newer entry
    LOG(INFO) << "Link state update has already been seen";
    return;
  }

  for(Port p = 0, in = ls->in_port_; p < links_.PortCount(); ++p)
    if(in != p)
      scheduler_.Forward(this, ls, p, stats_);

  link_state_.Update(ls);
}

void Switch::Handle(InitiateLinkState* ls) {
  LOG_HANDLE(INFO, Switch, ls);

  if(!is_up_) {
    LOG(INFO) << "Switch is down";
    return;
  }

  for(Port p = 0; p < links_.PortCount(); ++p)
    scheduler_.Forward(this, ls, p, stats_);

  next_link_state_++;
}

SequenceNum Switch::NextLSSeqNum() const { return next_link_state_; }

vector<Id> Switch::ComputeUpNeighbors() const {
  vector<Id> up;

  for(Port p = 0; p < links_.PortCount(); ++p)
    if(links_.IsLinkUp(p))
      up.push_back(links_.GetEndpoint(p)->id());

  return up;
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

void Controller::Handle(InitiateHeartbeat* init) {
  LOG_HANDLE(INFO, Controller, init)

  Entity::Handle(init);
}

void Controller::Handle(LinkStateUpdate* ls) { LOG_HANDLE(ERROR, Controller, ls) }

void Controller::Handle(InitiateLinkState* ls) { LOG_HANDLE(ERROR, Controller, ls) }

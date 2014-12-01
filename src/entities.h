#ifndef DDCSIM_ROUTERS_H_
#define DDCSIM_ROUTERS_H_

#include <boost/circular_buffer.hpp>
#include <iterator>
#include <inttypes.h>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "bv.h"
#include "common.h"
#include "links.h"

class Event;
class Up;
class Down;
class LinkUp;
class LinkDown;
class Broadcast;
class Heartbeat;
class InitiateHeartbeat;
class LinkStateUpdate;
class InitiateLinkState;
class Statistics;

enum lvl_to_num : int {INFO = 0, WARNING = 1, ERROR = 2, FATAL = 3};

/* Specialize the standard hash function for HeartbeatId's and LinkId's*/
namespace std {
template<> struct hash<pair<int, const Entity*>> {
  size_t operator()(const pair<int, const Entity*>& id) const {
    return hash<int>()(id.first) ^
        hash<uint64_t>()(reinterpret_cast<uint64_t>(id.second));
  }
};
}

// TODO move into entity?
class HeartbeatHistory {
 public:
  HeartbeatHistory();
  void MarkAsSeen(const Heartbeat*, Time);
  bool HasBeenSeen(const Heartbeat*) const;
  boost::circular_buffer<Time> LastSeen(Id) const;
  bool HasBeenSeen(Id) const;
  std::unordered_map<Id, std::vector<bool> > id_to_recently_seen() const;

 private:
  // TODO combine the set and map?
  // TODO make into a pair of sequence number and id?
  typedef std::pair<SequenceNum, const Entity*> HeartbeatId;
  // TODO what does the style guide say about static methods?
  static HeartbeatId MakeHeartbeatId(const Heartbeat* b);
  std::unordered_set<HeartbeatId> seen_;
  // TODO make into array-type mapping for better efficiency/style?
  std::unordered_map<Id, boost::circular_buffer<Time> > last_seen_;
  std::unordered_map<Id, std::vector<bool> > id_to_recently_seen_;
  DISALLOW_COPY_AND_ASSIGN(HeartbeatHistory);
};

class LinkState {
 public:
  LinkState(unsigned int);
  bool IsStaleUpdate(LinkStateUpdate*);
  void Update(LinkStateUpdate*);
  void Refresh(Time);

 private:
  std::vector<SequenceNum> id_to_last_seq_num_;
  std::vector<Time> id_to_exp_;
  // TODO intialize properly
  Topology topology_;
  DISALLOW_COPY_AND_ASSIGN(LinkState);
};

class Entity {
 public:
  Entity(Scheduler&, Id, Statistics&);
  template<class Iterator> void InitLinks(Iterator first, Iterator last,
                                          Size capacity, Rate rate) {
    links_.Init(first, last, capacity, rate);
  }
  virtual void Handle(Event*) = 0;
  virtual void Handle(Up*);
  virtual void Handle(Down*);
  virtual void Handle(Broadcast*) = 0;
  virtual void Handle(Heartbeat*);
  virtual void Handle(LinkUp*);
  virtual void Handle(LinkDown*);
  virtual void Handle(InitiateHeartbeat*);
  virtual void Handle(LinkStateUpdate*) = 0;
  virtual void Handle(InitiateLinkState*) = 0;
  Links& links(); // TODO didn't want to do it...
  Id id() const;
  SequenceNum NextHeartbeatSeqNum() const;
  BV ComputeRecentlySeen();
  std::vector<unsigned int> ComputePartitions() const;
  void UpdateLinkCapacities(Time);
  /* A switch is considered "recently seen" if its hearbeats have been seen
   * kMinTimes times in the last kMaxRecent seconds.
   */
  // TODO should these be command line args?
  static const Time kMaxRecent;
  static const unsigned int kMinTimes;

 protected:
  SequenceNum next_heartbeat_;
  HeartbeatHistory heart_history_;
  Links links_;
  Scheduler& scheduler_;
  Id id_;
  bool is_up_;
  Statistics& stats_;
  BV cached_bv_;
  bool is_cache_valid_;
  // TODO should be using the same entropy source as in sim.cc?
  // TODO clean this up
  std::default_random_engine entropy_src_;
  std::discrete_distribution<unsigned char> dist_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Entity);
};

class Switch : public Entity {
 public:
  Switch(Scheduler&, Id, Statistics&);
  void Handle(Event*);
  void Handle(Up*);
  void Handle(Down*);
  void Handle(Broadcast*);
  void Handle(Heartbeat*);
  void Handle(LinkUp*);
  void Handle(LinkDown*);
  void Handle(InitiateHeartbeat*);
  void Handle(LinkStateUpdate*);
  void Handle(InitiateLinkState*);
  SequenceNum NextLSSeqNum() const;
  std::vector<Id> ComputeUpNeighbors() const;

 private:
  SequenceNum next_link_state_;
  LinkState link_state_;
  DISALLOW_COPY_AND_ASSIGN(Switch);
};

class Controller : public Entity {
 public:
  Controller(Scheduler&, Id, Statistics&);
  void Handle(Event*);
  void Handle(Up*);
  void Handle(Down*);
  void Handle(Broadcast*);
  void Handle(Heartbeat*);
  void Handle(LinkUp*);
  void Handle(LinkDown*);
  void Handle(InitiateHeartbeat*);
  void Handle(LinkStateUpdate*);
  void Handle(InitiateLinkState*);

 private:
  DISALLOW_COPY_AND_ASSIGN(Controller);
};

#endif

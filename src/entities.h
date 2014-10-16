#ifndef DDCSIM_ROUTERS_H_
#define DDCSIM_ROUTERS_H_

#include <iterator>
#include <inttypes.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common.h"
#include "links.h"

class Event;
class Up;
class Down;
class LinkUp;
class LinkDown;
class Broadcast;
class Heartbeat;
class LinkAlert;
class InitiateHeartbeat;
class Statistics;

/* Specialize the standard hash function for HeartbeatId's and LinkId's*/
namespace std {
template<> struct hash<pair<unsigned int, const Entity*>> {
  size_t operator()(const pair<unsigned int, const Entity*>& id) const {
    return hash<unsigned int>()(id.first) ^
        hash<uint64_t>()(reinterpret_cast<uint64_t>(id.second));
  }
};
template<> struct hash<pair<int, const Entity*>> {
  size_t operator()(const pair<int, const Entity*>& id) const {
    return hash<unsigned int>()(id.first) ^
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
  Time LastSeen(Id) const;
  bool HasBeenSeen(Id) const;

 private:
  // TODO combine the set and map?
  // TODO make into a pair of sequence number and id?
  typedef std::pair<SequenceNum, const Entity*> HeartbeatId;
  // TODO what does the style guide say about static methods?
  static HeartbeatId MakeHeartbeatId(const Heartbeat* b);
  std::unordered_set<HeartbeatId> seen_;
  // TODO make into array-type mapping for better efficiency?
  std::unordered_map<Id, Time> last_seen_;
  DISALLOW_COPY_AND_ASSIGN(HeartbeatHistory);
};

// TODO combine HeartbeatHistory and LinkHistory

class LinkFailureHistory {
 public:
  LinkFailureHistory();
  bool LinkIsUp(const LinkAlert*) const;
  void MarkAsDown(const LinkAlert*);
  bool MarkAsUp(const LinkAlert*);

 private:
  typedef std::pair<Port, const Entity*> LinkId;
  // TODO what does the style guide say about static methods?
  static LinkId MakeLinkId(const LinkAlert* b);
  std::unordered_set<LinkId> failures_;
  DISALLOW_COPY_AND_ASSIGN(LinkFailureHistory);
};

class Entity {
 public:
  Entity(Scheduler&);
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
  virtual void Handle(LinkAlert*) = 0;
  virtual void Handle(InitiateHeartbeat*);
  Links& links(); // TODO didn't want to do it...
  Id id() const;
  SequenceNum NextHeartbeatSeqNum() const;
  std::vector<bool> ComputeRecentlySeen() const;
  void UpdateLinkCapacities(Time);
  static const Time kMaxRecent;  // TODO should be a commandline arg?

 protected:
  SequenceNum next_heartbeat_;
  HeartbeatHistory heart_history_;
  Links links_;
  Scheduler& scheduler_;
  Id id_;
  bool is_up_;
  Statistics& stats_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Entity);
};

class Switch : public Entity {
 public:
  Switch(Scheduler&);
  Switch(Scheduler&, Id, Statistics&);
  void Handle(Event*);
  void Handle(Up*);
  void Handle(Down*);
  void Handle(Broadcast*);
  void Handle(Heartbeat*);
  void Handle(LinkUp*);
  void Handle(LinkDown*);
  void Handle(LinkAlert*);
  void Handle(InitiateHeartbeat*);

 private:
  LinkFailureHistory link_history_;
  DISALLOW_COPY_AND_ASSIGN(Switch);
};

class Controller : public Entity {
 public:
  Controller(Scheduler&);
  Controller(Scheduler&, Id, Statistics&);
  void Handle(Event*);
  void Handle(Up*);
  void Handle(Down*);
  void Handle(Broadcast*);
  void Handle(Heartbeat*);
  void Handle(LinkUp*);
  void Handle(LinkDown*);
  void Handle(LinkAlert*);
  void Handle(InitiateHeartbeat*);

 private:
  DISALLOW_COPY_AND_ASSIGN(Controller);
};

#endif

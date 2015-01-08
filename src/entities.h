#ifndef DDCSIM_ENTITIES_H_
#define DDCSIM_ENTITIES_H_

#include <memory>
#include <random>
#include <string>

#include "common.h"
#include "links.h"
#include "link_state.h"

class Event;
class Up;
class Down;
class LinkUp;
class LinkDown;
class Broadcast;
class LinkStateRequest;
class LinkStateUpdate;
class InitiateLinkState;
class RoutingUpdate;
class Statistics;

class Entity {
 public:
  Entity(Scheduler&, Id, Statistics&);
  template<class Iterator> void InitLinks(Iterator first, Iterator last,
                                          Size capacity, Rate rate) {
    links_.Init(first, last, capacity, rate);
  }
  virtual std::string Description() const;
  virtual std::string Name() const;
  virtual void Handle(Event*) = 0;
  virtual void Handle(Up*);
  virtual void Handle(Down*);
  virtual void Handle(Broadcast*) = 0;
  virtual void Handle(LinkUp*);
  virtual void Handle(LinkDown*);
  virtual void Handle(InitiateLinkState*) = 0;
  virtual void Handle(LinkStateUpdate*) = 0;
  virtual void Handle(RoutingUpdate*) = 0;
  virtual void Handle(LinkStateRequest*) = 0;
  Links& links();
  Id id() const;
  //  void UpdateLinkCapacities(Time);
  /* An entity is considered "recently seen" if its hearbeats have been seen
   * kMinTimes times in the last kMaxRecent seconds.
   */
  // TODO should these be command line args?
  static const Time kMaxRecent;
  static const unsigned int kMinTimes;

 protected:
  Links links_;
  Scheduler& scheduler_;
  Id id_;
  bool is_up_;
  Statistics& stats_;
  // TODO should be using the same entropy source as in sim.cc?
  // TODO clean this up
  //  std::default_random_engine entropy_src_;
  //  std::discrete_distribution<unsigned char> dist_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Entity);
};

namespace std {
template<> struct hash<pair<int, int>> {
  size_t operator()(const pair<int, int>& p) const {
    return hash<unsigned int>()(p.first) ^
        hash<unsigned int>()(p.second);
  }
};
};

class RoutingUpdateHistory {
 public:
  RoutingUpdateHistory();
  std::string Description() const;
  void MarkAsSeen(const RoutingUpdate*);
  bool HasBeenSeen(const RoutingUpdate*) const;
  SequenceNum NextSeqNum(Id);

 private:
  std::unordered_map<std::pair<Id,Id>, SequenceNum> last_seen_;
  std::unordered_map<Id, SequenceNum> dst_to_next_sn_;
  DISALLOW_COPY_AND_ASSIGN(RoutingUpdateHistory);
};

class Switch : public Entity {
 public:
  Switch(Scheduler&, Id, Statistics&);
  std::string Description() const;
  std::string Name() const;
  void Handle(Event*);
  void Handle(Up*);
  void Handle(Down*);
  void Handle(Broadcast*);
  void Handle(LinkUp*);
  void Handle(LinkDown*);
  void Handle(LinkStateUpdate*);
  void Handle(InitiateLinkState*);
  void Handle(RoutingUpdate*);
  void Handle(LinkStateRequest*);
  static const Time kLSExpireDelta;
  // TODO make private again
  std::shared_ptr<std::vector<Id> > dst_to_neighbor_;

 private:
  LinkState ls_;
  RoutingUpdateHistory ru_history_;
  std::vector<SequenceNum> lsr_history_;
  DISALLOW_COPY_AND_ASSIGN(Switch);
};

class Controller : public Entity {
 public:
  Controller(Scheduler&, Id, Statistics&);
  std::string Description() const;
  std::string Name() const;
  void Handle(Event*);
  void Handle(Up*);
  void Handle(Down*);
  void Handle(Broadcast*);
  void Handle(LinkUp*);
  void Handle(LinkDown*);
  void Handle(LinkStateUpdate*);
  void Handle(InitiateLinkState*);
  void Handle(RoutingUpdate*);
  void Handle(LinkStateRequest*);

 private:
  LinkStateControl ls_;
  std::vector<SequenceNum> switch_to_next_sn_;
  SequenceNum next_lsr_;
  DISALLOW_COPY_AND_ASSIGN(Controller);
};

class Host : public Entity {
 public:
  Host(Scheduler&, Id, Statistics&);
  std::string Description() const;
  std::string Name() const;
  void Handle(Event*);
  void Handle(Up*);
  void Handle(Down*);
  void Handle(Broadcast*);
  void Handle(LinkUp*);
  void Handle(LinkDown*);
  void Handle(LinkStateUpdate*);
  void Handle(InitiateLinkState*);
  void Handle(RoutingUpdate*);
  void Handle(LinkStateRequest*);
  Id EdgeSwitch();

 private:
  DISALLOW_COPY_AND_ASSIGN(Host);
};

#endif

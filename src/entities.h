#ifndef DDCSIM_ROUTERS_H_
#define DDCSIM_ROUTERS_H_

#include <iterator>
#include <inttypes.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common.h"
#include "reader.h"
#include "scheduler.h"

class Entity; /* For Links */
class Event;
class Up;
class Down;
class LinkUp;
class LinkDown;
class Broadcast;
class Heartbeat;
class LinkAlert;
class InitiateHeartbeat;

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

class Links {
 public:
  Links();
  // TODO what does the style guide say about a newline after the template?
  template<class Iterator> void Init(Iterator neighbors_begin,
                                     Iterator neighbors_end,
                                     Size capacity, Rate drain) {
    for(Port p = 0; neighbors_begin != neighbors_end; ++neighbors_begin, ++p) {
      port_nums_.push_back(p);
      port_to_link_.insert({p, {true, *neighbors_begin}});
      port_to_size_.insert({p, bucket_capacity});
      bucket_capacity = capacity;
      drain_rate = drain;
    }
  }
  // TODO return generic iterator rather than an interator to a vector
  std::vector<Port>::const_iterator PortsBegin();
  std::vector<Port>::const_iterator PortsEnd();
  void SetLinkUp(Port);
  void SetLinkDown(Port);
  void UpdateCapacities(Time);
  Size bucket_capacity;
  Rate drain_rate;
  static const Size kDefaultCapacity;
  static const Rate kDefaultRate;
  template<class E, class M> friend void Scheduler::Forward(E* sender, M* msg_in, Port out);
  friend bool Reader::ParseEvents();

 private:
  bool IsLinkUp(Port);
  Entity* GetEndpoint(Port);
  Port GetPortTo(Entity*);
  Port FindInPort(Entity*);
  std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator LinksBegin();
  std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator LinksEnd();
  // TODO use Boost's iterator transformers to return an iterator to only keys
  std::vector<Port> port_nums_;
  std::unordered_map<Port, std::pair<bool,Entity*> > port_to_link_;
  // TODO combine port mappings
  std::unordered_map<Port, Size> port_to_size_;
  DISALLOW_COPY_AND_ASSIGN(Links);
};

class Entity {
 public:
  Entity(Scheduler&);
  Entity(Scheduler&, Id);
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

 private:
  DISALLOW_COPY_AND_ASSIGN(Entity);
};

class Switch : public Entity {
 public:
  Switch(Scheduler&);
  Switch(Scheduler&, Id);
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
  Controller(Scheduler&, Id);
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

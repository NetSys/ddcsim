#ifndef DDCSIM_ROUTERS_H_
#define DDCSIM_ROUTERS_H_

#include <iterator>
#include <inttypes.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common.h"
#include "scheduler.h"

class Entity; /* For Links */
class Switch; /* For MsgId's */
class Event;
class Up;
class Down;
class Broadcast;
class Heartbeat;

// TODO move into Switch
// TODO represent as class instead?
typedef std::pair<SequenceNum, const Switch*> MsgId;

/* Specialize the standard hash function for MsgId's */
namespace std {
template<> struct hash<MsgId> {
  size_t operator()(const MsgId& id) const {
    return hash<unsigned int>()(id.first) ^
        hash<uint64_t>()(reinterpret_cast<uint64_t>(id.second));
  }
};
}

class Links {
 public:
  Links();
  // TODO what does the style guide say about a newline after the template?
  template<class Iterator> void Init(Iterator neighbors_begin,
                                     Iterator neighbors_end) {
    for(Port p = 0; neighbors_begin != neighbors_end; ++neighbors_begin, ++p) {
      port_nums_.push_back(p);
      port_to_link_.insert({p, {true, *neighbors_begin}});
    }
  }
  // TODO return generic iterator rather than an interator to a vector
  std::vector<Port>::const_iterator PortsBegin();
  std::vector<Port>::const_iterator PortsEnd();
  // TODO Is using friend functions considered good style?
  template<class E, class M> friend void Scheduler::Forward(E* sender, M* msg_in, Port out);
  template<class E> friend Port Scheduler::FindInPort(E* sender, Entity* receiver);

 private:
  bool IsLinkUp(Port);
  Entity* GetEndpoint(Port);
  std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator LinksBegin();
  std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator LinksEnd();
  // TODO use Boost's iterator transformers to return an iterator to only keys
  std::vector<Port> port_nums_;
  std::unordered_map<Port, std::pair<bool,Entity*> > port_to_link_;
  DISALLOW_COPY_AND_ASSIGN(Links);
};

class Entity {
 public:
  Entity(Scheduler&);
  Entity(Scheduler&, Id);
  template<class Iterator> void InitLinks(Iterator first, Iterator last) {
    links_.Init(first, last);
  }
  virtual void Handle(Event*);
  virtual void Handle(Up*);
  virtual void Handle(Down*);
  virtual void Handle(Broadcast*);
  virtual void Handle(Heartbeat*);
  Links& links(); // TODO didn't want to do it...
  Id id() const;

 protected:
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
  virtual void Handle(Heartbeat*);

 protected:
  void MarkAsSeen(const Heartbeat*);
  bool HasBeenSeen(const Heartbeat*) const;
  // TODO what does the style guide say about static methods?
  static MsgId MakeMsgId(const Heartbeat*);
  std::unordered_set<MsgId> seen;

 private:
  DISALLOW_COPY_AND_ASSIGN(Switch);
};

#endif

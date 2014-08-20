#ifndef DDCSIM_ENTITIES_H_
#define DDCSIM_ENTITIES_H_

#include <unordered_set>
#include <inttypes.h>

#include "common.h"
#include "events.h"

// TODO make it local to Broadcast switch
typedef std::pair<SequenceNum, const Switch*> MsgId;

// TODO put in a class
// TODO redefined somewhere else?
/*
bool operator==(const MsgId& lhs, const MsgId& rhs) {
  return lhs.first == rhs.first && lhs.second == rhs.second;
}
*/

/* Specialize the standard hash function for MsgId's */
namespace std {
  template<> struct hash<MsgId> {
    size_t operator()(const MsgId& id) const {
      return hash<unsigned int>()(id.first) ^
          hash<uint64_t>()(reinterpret_cast<uint64_t>(id.second));
    }
  };
}

class Entity {
  class Ports {
   public:
    Ports();
    // TODO what does the style guide say about a newline after the template?
    template<class Iterator> void Init(Iterator neighbors_begin,
                                       Iterator neighbors_end) {
      for( ; neighbors_begin != neighbors_end ; ++neighbors_begin) {
        port_to_entity_.push_back(*neighbors_begin);
        is_link_up_.push_back(true);
      }
    }

   private:
    std::vector<Entity*> port_to_entity_;
    std::vector<bool> is_link_up_;
  };
 public:
  Entity();
  template<class Iterator> void InitPorts(Iterator first, Iterator last) {
    ports_.Init(first, last);
  }
  virtual void Handle(Event*);

 private:
  Ports ports_;
  DISALLOW_COPY_AND_ASSIGN(Entity);
};

class Switch : public Entity {
 public:
  Switch();
  virtual void Handle(Event*);
  virtual void Handle(SwitchUp*);
  virtual void Handle(SwitchDown*);

 private:
  bool is_up_;
  DISALLOW_COPY_AND_ASSIGN(Switch);
};

class BroadcastSwitch : public Switch {
 public:
  // TODO how does this handle UPs?
  BroadcastSwitch();
  virtual void Handle(Broadcast*);

 private:
  void MarkAsSeen(const Broadcast*);
  bool HasBeenSeen(const Broadcast*) const;
  // TODO what does the style guide say about static methods?
  static MsgId MakeMsgId(const Broadcast*);
  std::unordered_set<MsgId> seen;
  DISALLOW_COPY_AND_ASSIGN(BroadcastSwitch);
};

#endif

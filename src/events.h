#ifndef DDCSIM_EVENTS_H_
#define DDCSIM_EVENTS_H_

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "common.h"

class Entity;

class Event {
 public:
  Event();
  Event(Time, Entity*);
  Event(Time, Entity*, Entity*);
  virtual ~Event();
  /* The following Handle method, and the Handle methods in all descendants
   * of Event, implement the pattern outlined here:
   * http://en.wikipedia.org/wiki/Double_dispatch#Double_dispatch_in_C.2B.2B
   * in order to implement double dispatch.
   */
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  virtual unsigned int size() const;
  Time time_;
  std::vector<Entity*> affected_entities_;
};

class Up : public Event {
 public:
  Up();
  Up(Time, Entity*);
  virtual void Handle(Entity*);
  // TODO is virtual here redundant?
  virtual std::string Description() const;
  virtual std::string Name() const;
  static unsigned int count_;
};

class Down : public Event {
 public:
  Down();
  Down(Time, Entity*);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  static unsigned int count_;
};

class LinkUp : public Event {
 public:
  LinkUp();
  LinkUp(Time, Entity*, Port);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  Port out_;
  static unsigned int count_;
};

class LinkDown : public Event {
 public:
  LinkDown();
  LinkDown(Time, Entity*, Port);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  Port out_;
  static unsigned int count_;
};

class Broadcast : public Event {
 public:
  Broadcast();
  Broadcast(Time, Entity*, Port);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  virtual unsigned int size() const;
  Port in_port_;
};

class LinkStateUpdate : public Broadcast {
 public:
  LinkStateUpdate();
  LinkStateUpdate(Time, Entity*, Port, Entity*, SequenceNum, Time,
                  std::array<Id, 13>, Id);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  virtual unsigned int size() const;
  SequenceNum sn_;
  Entity* src_;
  std::array<Id, 13> up_links_;
  Time expiration_;
  Id src_id_;
  static unsigned int count_;
};

class InitiateLinkState : public Event {
 public:
  InitiateLinkState();
  InitiateLinkState(Time, Entity*);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  static unsigned int count_;
};

class RoutingUpdate : public Broadcast {
 public:
  RoutingUpdate();
  RoutingUpdate(Time, Entity*, Port, Entity*, SequenceNum,
                std::shared_ptr<std::vector<Id> >, Id, Id);
  ~RoutingUpdate();
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  virtual unsigned int size() const;
  SequenceNum sn_;
  Entity* src_;
  std::shared_ptr<std::vector<Id> > dst_to_neighbor_;
  Id dst_;
  Id src_id_;
  static unsigned int count_;
};

class LinkStateRequest : public Broadcast {
 public:
  LinkStateRequest();
  LinkStateRequest(Time, Entity*, Port, Entity*, SequenceNum, Id);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  virtual unsigned int size() const;
  SequenceNum sn_;
  Entity* src_;
  Id src_id_;
  static unsigned int count_;
};

#endif

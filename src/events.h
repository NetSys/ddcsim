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

 private:
  DISALLOW_COPY_AND_ASSIGN(Event);
};

class Up : public Event {
 public:
  Up(Time, Entity*);
  virtual void Handle(Entity*);
  // TODO is virtual here redundant?
  virtual std::string Description() const;
  virtual std::string Name() const;
  static unsigned int count_;

 private:
  // TODO need disallow in derived classes?
  DISALLOW_COPY_AND_ASSIGN(Up);
};

class Down : public Event {
 public:
  Down(Time, Entity*);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  static unsigned int count_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Down);
};

class LinkUp : public Event {
 public:
  LinkUp(Time, Entity*, Port);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  const Port out_;
  static unsigned int count_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LinkUp);
};

class LinkDown : public Event {
 public:
  LinkDown(Time, Entity*, Port);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  const Port out_;
  static unsigned int count_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LinkDown);
};

class Broadcast : public Event {
 public:
  Broadcast(Time, Entity*, Port);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  virtual unsigned int size() const;
  Port in_port_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Broadcast);
};

class LinkStateUpdate : public Broadcast {
 public:
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

 private:
  DISALLOW_COPY_AND_ASSIGN(LinkStateUpdate);
};

class InitiateLinkState : public Event {
 public:
  InitiateLinkState(Time, Entity*);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  static unsigned int count_;

 private:
  DISALLOW_COPY_AND_ASSIGN(InitiateLinkState);
};

class RoutingUpdate : public Broadcast {
 public:
  RoutingUpdate(Time, Entity*, Port, Entity*, SequenceNum,
                std::shared_ptr<std::vector<Id> >, Id, Id);
  ~RoutingUpdate();
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  virtual unsigned int size() const;
  const SequenceNum sn_;
  Entity* src_;
  std::shared_ptr<std::vector<Id> > dst_to_neighbor_;
  const Id dst_;
  const Id src_id_;
  static unsigned int count_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RoutingUpdate);
};

class LinkStateRequest : public Broadcast {
 public:
  LinkStateRequest(Time, Entity*, Port, Entity*, SequenceNum, Id);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  virtual unsigned int size() const;
  SequenceNum sn_;
  Entity* src_;
  Id src_id_;
  static unsigned int count_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LinkStateRequest);
};

#endif

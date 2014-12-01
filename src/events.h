#ifndef DDCSIM_EVENTS_H_
#define DDCSIM_EVENTS_H_

#include <iostream>
#include <string>
#include <vector>

#include "common.h"

// TODO do this with templates as we are essentially attempting to generate
// a family of (overloaded) functions
#define OVERLOAD_EVENT_OSTREAM_IMPL(event_type)                     \
  ostream& operator<<(ostream& s, const event_type &e) {            \
    return s << #event_type << " " << e.Description();              \
  }

#define OVERLOAD_EVENT_OSTREAM_DECL(event_type)                 \
  std::ostream& operator<<(std::ostream&, const event_type &);

class Entity;

class Event {
 public:
  Event(Time, Entity*);
  Event(Time, Entity*, Entity*);
  /* The following Handle method, and the Handle methods in all descendants
   * of Event, implement the pattern outlined here:
   * http://en.wikipedia.org/wiki/Double_dispatch#Double_dispatch_in_C.2B.2B
   * in order to implement double dispatch.
   */
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  // TODO remove this eventually and factor into a packettx superclass
  virtual Size size() const;
  const Time time_;
  const std::vector<Entity*> affected_entities_;

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

 private:
  DISALLOW_COPY_AND_ASSIGN(LinkDown);
};

class InitiateHeartbeat : public Event {
 public:
  InitiateHeartbeat(Time, Entity*);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;

 private:
  DISALLOW_COPY_AND_ASSIGN(InitiateHeartbeat);
};

class Broadcast : public Event {
 public:
  Broadcast(Time, Entity*, Port);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  virtual Size size() const;
  const Port in_port_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Broadcast);
};

class Heartbeat : public Broadcast {
 public:
  Heartbeat(Time, const Entity*, Entity*, Port, SequenceNum, BV);
  ~Heartbeat();
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  virtual Size size() const;
  const SequenceNum sn_;
  const Entity* src_; // TODO better to change to a ref?
  const BV recently_seen_;
  const unsigned int current_partition_;
  const Id leader_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Heartbeat);
};

class LinkStateUpdate : public Broadcast {
 public:
  LinkStateUpdate(Time, Entity*, Port, const Entity*, SequenceNum,
                  std::vector<Id>, Time);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;
  virtual Size size() const;
  const SequenceNum sn_;
  const Entity* src_;
  const std::vector<Id> neighbors_;
  const Time expiration_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LinkStateUpdate);
};

class InitiateLinkState : public Event {
 public:
  InitiateLinkState(Time, Entity*);
  virtual void Handle(Entity*);
  virtual std::string Description() const;
  virtual std::string Name() const;

 private:
  DISALLOW_COPY_AND_ASSIGN(InitiateLinkState);
};

OVERLOAD_EVENT_OSTREAM_DECL(Event)
OVERLOAD_EVENT_OSTREAM_DECL(Up)
OVERLOAD_EVENT_OSTREAM_DECL(Down)
OVERLOAD_EVENT_OSTREAM_DECL(LinkUp)
OVERLOAD_EVENT_OSTREAM_DECL(LinkDown)
OVERLOAD_EVENT_OSTREAM_DECL(InitiateHeartbeat)
OVERLOAD_EVENT_OSTREAM_DECL(Broadcast)
OVERLOAD_EVENT_OSTREAM_DECL(Heartbeat)
OVERLOAD_EVENT_OSTREAM_DECL(LinkStateUpdate)
OVERLOAD_EVENT_OSTREAM_DECL(InitiateLinkState)

#endif

#ifndef DDCSIM_EVENTS_H_
#define DDCSIM_EVENTS_H_

#include <vector>
#include <string>

#include "common.h"

class Entity;

class Event {
 public:
  Event();
  Event(Time, Entity*);
  Event(Time, Entity*, Entity*);
  // TODO what does the google stye guide say about using templates?
  // TODO change last to beyond?
  template<class Iterator> Event(Time t, Iterator first, Iterator last) :
      time_(t), affected_entities_(first, last) {}

  // TODO are the contents of Entity considered part of this class with respect
  // to const-ness?
  Time time() const;
  // TODO return generic iterator rather than vector's
  std::vector<Entity*>::iterator AffectedEntitiesBegin();
  std::vector<Entity*>::iterator AffectedEntitiesEnd();

  /* The following Handle method, and the Handle methods in all descendants
   * of Event, implement the pattern outlined here:
   * http://en.wikipedia.org/wiki/Double_dispatch#Double_dispatch_in_C.2B.2B
   * in order to implement double dispatch.
   */
  virtual void Handle(Entity*);
  virtual std::string Description();

 protected:
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
  virtual std::string Description();

 private:
  // TODO need disallow in derived classes?
  DISALLOW_COPY_AND_ASSIGN(Up);
};

class Down : public Event {
 public:
  Down(Time, Entity*);
  virtual void Handle(Entity*);
  virtual std::string Description();

 private:
  DISALLOW_COPY_AND_ASSIGN(Down);
};

class LinkUp : public Event {
 public:
  LinkUp(Time, Entity*, Port);
  virtual void Handle(Entity*);
  virtual std::string Description();
  const Port out_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LinkUp);
};

class LinkDown : public Event {
 public:
  LinkDown(Time, Entity*, Port);
  virtual void Handle(Entity*);
  virtual std::string Description();
  const Port out_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LinkDown);
};

class Broadcast : public Event {
 public:
  Broadcast(Time, Entity*, Port);
  Port in_port() const;
  virtual void Handle(Entity*);
  virtual std::string Description();

 protected:
  const Port in_port_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Broadcast);
};

class Heartbeat : public Broadcast {
 public:
  Heartbeat(Time, const Entity*, Entity*, Port, SequenceNum);
  SequenceNum sn() const;
  const Entity* src() const;
  virtual void Handle(Entity*);
  virtual std::string Description();

 protected:
  const SequenceNum sn_;
  const Entity* src_; // TODO better to change to a ref?

 private:
  DISALLOW_COPY_AND_ASSIGN(Heartbeat);
};

class LinkAlert : public Broadcast {
 public:
  LinkAlert(Time, Entity*, Port, const Entity*, Port, bool);
  virtual void Handle(Entity*);
  virtual std::string Description();
  const Entity* src_;
  const Port out_;
  const bool is_up_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LinkAlert);
};

#endif

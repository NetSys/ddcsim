#ifndef DDCSIM_EVENTS_H_
#define DDCSIM_EVENTS_H_

#include <vector>
#include <string>

#include "common.h"

class Entity;
class Switch;

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

 private:
  Time time_;
  std::vector<Entity*> affected_entities_;
  DISALLOW_COPY_AND_ASSIGN(Event);
};

class SwitchUp : public Event {
 public:
  SwitchUp(Time, Switch*);
  virtual void Handle(Entity*);
  // TODO is virtual here redundant?
  virtual std::string Description();

 private:
  // TODO need disallow in derived classes?
  DISALLOW_COPY_AND_ASSIGN(SwitchUp);
};

class SwitchDown : public Event {
 public:
  SwitchDown(Time, Switch*);
  virtual void Handle(Entity*);
  virtual std::string Description();

 private:
  DISALLOW_COPY_AND_ASSIGN(SwitchDown);
};

// TODO make friend of broadcast switch?
// TODO what are the proper practices with friend relationships
class Broadcast : public Event {
 public:
  // TODO correct spacing on constructor?
  template<class Iterator> Broadcast(Time t, Switch* s, Iterator begin,
                                     Iterator end, Port in) :
      Event(t, begin, end), src_(s), in_port_(in) {}
  void SetInPort(Port);

 private:
  const SequenceNum sn_;
  // TODO advantages of making r a ref?
  const Switch* src_;
  Port in_port_;
  DISALLOW_COPY_AND_ASSIGN(Broadcast);
};

#endif

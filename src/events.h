#ifndef DDCSIM_EVENTS_H_
#define DDCSIM_EVENTS_H_

#include <vector>
#include <string>

#include "common.h"

class Entity;
class Router;

class Event {
 public:
  Event();
  /* At this point, an event will either affect 1 or 2 entities.  Thus, we hard
   * code this into the constructor.  Later on, if the need arises, we can add
   * range constructors and what not.
   */
  Event(Time, Entity*);
  Event(Time, Entity*, Entity*);

  // TODO are the contents of Entity considered part of this class with respect
  // to const-ness?
  Time time() const;
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

class RouterUpEvent : public Event {
 public:
  RouterUpEvent(Time, Router*);
  virtual void Handle(Entity*);
  // TODO is virtual here redundant?
  virtual std::string Description();

 private:
  // TODO need disallow in derived classes?
  DISALLOW_COPY_AND_ASSIGN(RouterUpEvent);
};

class RouterDownEvent : public Event {
 public:
  RouterDownEvent(Time, Router*);
  virtual void Handle(Entity*);
  virtual std::string Description();

 private:
  DISALLOW_COPY_AND_ASSIGN(RouterDownEvent);
};

#endif

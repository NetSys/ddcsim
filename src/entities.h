#ifndef DDCSIM_ROUTERS_H_
#define DDCSIM_ROUTERS_H_

#include "common.h"
#include "events.h"

class Entity {
 public:
  Entity();
  virtual void Handle(Event*);

 private:
  DISALLOW_COPY_AND_ASSIGN(Entity);
};

class Router : public Entity {
 public:
  Router();
  virtual void Handle(Event*);
  virtual void Handle(RouterUpEvent*);
  virtual void Handle(RouterDownEvent*);

 private:
  DISALLOW_COPY_AND_ASSIGN(Router);
};

#endif

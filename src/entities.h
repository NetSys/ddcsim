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

class Switch : public Entity {
 public:
  Switch();
  virtual void Handle(Event*);
  virtual void Handle(SwitchUp*);
  virtual void Handle(SwitchDown*);

 private:
  DISALLOW_COPY_AND_ASSIGN(Switch);
};

#endif

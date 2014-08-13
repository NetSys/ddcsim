#ifndef DDCSIM_ROUTERS_H_
#define DDCSIM_ROUTERS_H_

#include "common.h"
#include "events.h"

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

#endif

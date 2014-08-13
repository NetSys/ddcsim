#include "common.h"
#include "entities.h"
#include "events.h"
#include "scheduler.h"

using std::vector;

int main(int argc, char* argv[]) {
  // TODO remove hard-coded simulation instance below after simulation input
  // are implemented

  // TODO why did adding a constructor that the compiler should have already
  // inserted fix the issue?  Something to do with move constructors?

  /* Construct entities */
  vector<Entity*> ents({new Entity, new Switch, new Switch});

  /* Construct topology */
  ents[0]->InitPorts(++(ents.begin()), ents.end());
  ents[1]->InitPorts(ents.begin(), ++(ents.begin()));
  ents[2]->InitPorts(ents.begin(), ++(ents.begin()));

  /* An "abstract" event. */
  Event* abs = new Event(3, ents[0]);

  /* Two "concrete" events. */
  // TODO better solution than casts?
  SwitchDown* down = new SwitchDown(1, static_cast<Switch*>(ents[1]));
  SwitchUp* up = new SwitchUp(2, static_cast<Switch*>(ents[1]));

  Scheduler sched;
  sched.AddEvent(abs);
  sched.AddEvent(up);
  sched.AddEvent(down);

  Time cur_time;
  while(sched.HasNextEvent()) {
    Event* ev = sched.NextEvent();

    cur_time = ev->time();

    for (vector<Entity*>::iterator it = ev->AffectedEntitiesBegin();
         it != ev->AffectedEntitiesEnd(); ++it) {
      ev->Handle(*it);
      delete ev;
    }
  }

  for(auto it = ents.begin(); it != ents.end(); ++it) {
    delete *it;
  }

  return 0;
}

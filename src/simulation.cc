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

  /* An "abstract" event. */
  Event* abs = new Event(3, new Entity);

  /* Two "concrete" events. */
  Switch* r = new Switch;
  SwitchUp* up = new SwitchUp(2, r);
  SwitchDown* down = new SwitchDown(1, r);

  Scheduler sched;
  sched.AddEvent(abs);
  sched.AddEvent(up);
  sched.AddEvent(down);

  Time cur_time;
  while(sched.HasNextEvent()) {
    Event* e = sched.NextEvent();

    cur_time = e->time();

    for (vector<Entity*>::iterator it = e->AffectedEntitiesBegin();
         it != e->AffectedEntitiesEnd(); ++it) {
      e->Handle(*it);
      delete e;
    }
  }

  return 0;
}

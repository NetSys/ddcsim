#include "../src/common.h"
#include "../src/entities.h"
#include "../src/events.h"
#include "../src/scheduler.h"
// TODO I don't believe the Google style guide allows for relative addressing
// in header paths

using std::vector;

int main(int argc, char* argv[]) {
  // TODO remove hard-coded simulation instance below after simulation input
  // are implemented

  Scheduler sched;

  // TODO why did adding a constructor that the compiler should have already
  // inserted fix the issue?  Something to do with move constructors?

  /* Construct entities */
  vector<Entity*> ents({new Entity(sched), new Switch(sched),
          new Switch(sched)});

  /* Construct topology */
  ents[0]->InitLinks(++(ents.begin()), ents.end());
  ents[1]->InitLinks(ents.begin(), ++(ents.begin()));
  ents[2]->InitLinks(ents.begin(), ++(ents.begin()));

  /* An "abstract" event. */
  Event* abs = new Event(3, ents[0]);

  /* Two "concrete" events. */
  // TODO better solution than casts?
  SwitchDown* down = new SwitchDown(1, static_cast<Switch*>(ents[1]));
  SwitchUp* up = new SwitchUp(2, static_cast<Switch*>(ents[1]));

  sched.AddEvent(abs);
  sched.AddEvent(up);
  sched.AddEvent(down);

  sched.StartSimulation();

  for(auto it = ents.begin(); it != ents.end(); ++it) {
    delete *it;
  }

  return 0;
}

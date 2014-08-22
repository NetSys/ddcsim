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
  vector<Entity*> ents;
  for(int i = 0; i < 4; i++)
    ents.push_back(new BroadcastSwitch(sched));

  /* Construct topology */
  vector<Entity*> zero_neighbors  = {ents[1], ents[3]};
  vector<Entity*> one_neighbors   = {ents[0], ents[2]};
  vector<Entity*> two_neighbors   = {ents[1], ents[3]};
  vector<Entity*> three_neighbors = {ents[0], ents[2]};
  ents[0]->InitLinks(zero_neighbors.begin(), zero_neighbors.end());
  ents[1]->InitLinks(one_neighbors.begin(), one_neighbors.end());
  ents[2]->InitLinks(two_neighbors.begin(), two_neighbors.end());
  ents[3]->InitLinks(three_neighbors.begin(), three_neighbors.end());

  Broadcast* b = new Broadcast(static_cast<Time>(0),
                               static_cast<BroadcastSwitch*>(ents[0]),
                               ents[0], INITIATING_EVENT,
                               static_cast<SequenceNum>(1));

  sched.AddEvent(b);

  sched.StartSimulation();

  for(auto it = ents.begin(); it != ents.end(); ++it) {
    delete *it;
  }

  return 0;
}

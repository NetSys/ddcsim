#ifndef DDCSIM_SCHEDULER_H_
#define DDCSIM_SCHEDULER_H_

#include <queue>
#include <unordered_map>
#include <vector>

#include "common.h"

class Entity;
class Event;

class Scheduler {
  /* When an inner class is not part of a class's interface, as is the case
   * here, the Google style guide recommends forward-declaring the nested
   * class in the header file and placing both its declaration and
   * implementation in the associated .cc file.  Applying this recommendation
   * naively to Comparator below results in compilation errors, presumably
   * because it is used in the template for priority_queue.  This CAN be overcome
   * by adding a dummy template parameter to the entire Scheduler class to enforce
   * a different template intitialization order, as depicted by this SO post:
   * http://stackoverflow.com/questions/23963594/templated-class-member-vs-nested-class-forward-declaration
   * but I forewent the temptation (after wasting half a day working on it) as
   * it obfuscates the code.
   */
  class Comparator {
   public:
    bool operator() (const Event* const, const Event* const) const;
  };

 public:
  Scheduler(Time);
  void AddEvent(Event*);
  // TODO more descriptive template type names? what is the convention?
  template<class E, class M> void Forward(E* sender, M* msg_in, Port out);
  void StartSimulation(std::unordered_map<Id, Entity*>&);
  Time cur_time();
  Time end_time();
  static int kMaxEntities;
  static const Time kLinkLatency;
  static const Time kDefaultHeartbeatPeriod;
  static const Time kDefaultEndTime;
  static const int kNoMaxEntities;

 private:
  bool HasNextEvent();
  Event* NextEvent();
  Time cur_time_;
  Time end_time_;
  std::priority_queue<Event*, std::vector<Event*>, Comparator> event_queue_;
  DISALLOW_COPY_AND_ASSIGN(Scheduler);
};

#endif

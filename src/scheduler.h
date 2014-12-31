#ifndef DDCSIM_SCHEDULER_H_
#define DDCSIM_SCHEDULER_H_

#include <vector>

#include "common.h"
#include "frontier_queue.h"

class Entity;
class Event;
class Statistics;
class Switch;

class Scheduler {
 public:
  Scheduler(Time, size_t, size_t, size_t);
  void AddEvent(Time, Event*);
  template<class E, class In, class Out> void Forward(E*, In*, Out*, Port);
  void SchedulePeriodicEvents(std::vector<Switch*>, Time, Time);
  void StartSimulation(Statistics&);
  Time cur_time();
  Time end_time();
  static Time Delay();
  bool IsHost(Id);
  bool IsController(Id);
  bool IsSwitch(Id);
  static const Time kComputationDelay;
  static const Time kTransDelay;
  static const Time kPropDelay;
  static const Time kDefaultHeartbeatPeriod;
  static const Time kDefaultLSUpdatePeriod;
  static const Time kDefaultEndTime;
  static const Time kDefaultHelloDelay;
  const size_t kSwitchCount;
  const size_t kControllerCount;
  const size_t kHostCount;

 private:
  bool HasNextEvent();
  Event* NextEvent();
  Time cur_time_;
  Time end_time_;
  FrontierQueue event_queue_;
  DISALLOW_COPY_AND_ASSIGN(Scheduler);
};

#endif

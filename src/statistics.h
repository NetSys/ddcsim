#ifndef DDCSIM_STATISTICS_H_
#define DDCSIM_STATISTICS_H_

#include "common.h"

#include <string>
#include <unordered_map>
#include <memory>

class Scheduler;
class Entity;
class Event;

class Statistics {
 public:
  Statistics(std::string, Scheduler&);
  void Init(Topology);
  void EntityUp(Id);
  void EntityDown(Id);
  void LinkUp(Id, Id);
  void LinkDown(Id, Id);
  std::string Reachability();
  std::vector<Entity*>& id_to_entity();
  void RecordEventCounts();
  void RecordSend(Event*);
  //int MaxPathLength();
  static const std::string SEPARATOR;
  static const Time WINDOW_SIZE;
  const int NO_PATH = 0;

 private:
  void InitComponents();
  int ComputePhysReachable();
  int ComputeVirtReachable();
  Topology physical_;
  std::string out_prefix_;
  std::vector<int> id_to_component_;
  Scheduler& scheduler_;
  std::vector<Entity*> id_to_entity_;
  std::vector<std::shared_ptr<std::vector<Id> > > switch_to_table_;
  std::vector<Id> host_to_edge_switch_;
  size_t begin_host_;
  size_t beyond_host_;
  Time window_left_;
  Time window_right_;
  unsigned int cur_window_count_;
  DISALLOW_COPY_AND_ASSIGN(Statistics);
};

#endif

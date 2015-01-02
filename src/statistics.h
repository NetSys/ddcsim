#ifndef DDCSIM_STATISTICS_H_
#define DDCSIM_STATISTICS_H_

#include "common.h"

#include <fstream>
#include <string>
#include <unordered_map>
#include <memory>

class Scheduler;
class Entity;

class Statistics {
 public:
  Statistics(std::string, Scheduler&);
  ~Statistics();
  void Init(Topology);
  void EntityUp(Id);
  void EntityDown(Id);
  void LinkUp(Id, Id);
  void LinkDown(Id, Id);
  void RecordReachability();
  std::vector<Entity*>& id_to_entity();
  void RecordEventCounts();
  //int MaxPathLength();
  static const std::string REACHABILITY_LOG_NAME;
  const int NO_PATH = 0;

 private:
  void InitComponents();
  int ComputePhysReachable();
  int ComputeVirtReachable();
  /* Having to store pointers to ofstream's rather than the object itself is an
   * unfortunate consequence of ofstreams not being copyable.  I do not know of a
   * better solution.
   */
  std::ofstream reachability_log_;
  Topology physical_;
  std::string out_prefix_;
  std::vector<int> id_to_component_;
  Scheduler& scheduler_;
  std::vector<Entity*> id_to_entity_;
  std::vector<std::shared_ptr<std::vector<Id> > > switch_to_table_;
  std::vector<Id> host_to_edge_switch_;
  size_t begin_host_;
  size_t beyond_host_;
  DISALLOW_COPY_AND_ASSIGN(Statistics);
};

#endif

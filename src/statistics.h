#ifndef DDCSIM_STATISTICS_H_
#define DDCSIM_STATISTICS_H_

#include "common.h"

#include <fstream>
#include <string>
#include <unordered_map>

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
  std::unordered_map<Id, Entity*>& id_to_entity();
  //int MaxPathLength();
  static const std::string REACHABILITY_LOG_NAME;
  static const int NO_PATH = 0;

 private:
  /* Having to store pointers to ofstream's rather than the object itself is an
   * unfortunate consequence of ofstreams not being copyable.  I do not know of a
   * better solution.
   */
  std::ofstream reachability_log_;
  Topology physical_;
  std::string out_prefix_;
  // std::vector<int> dst_to_distance_;
  //std::vector<boost::graph_traits<Topology>::vertices_size_type> dst_to_distance_;
  //  std::vector<boost::graph_traits<Topology>::vertices_size_type> d;//[num_vertices(physical_)];
  Scheduler& scheduler_;
  std::unordered_map<Id, Entity*> id_to_entity_;
  DISALLOW_COPY_AND_ASSIGN(Statistics);
};

#endif

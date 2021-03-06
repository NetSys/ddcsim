#ifndef DDCSIM_STATISTICS_H_
#define DDCSIM_STATISTICS_H_

#include "common.h"

#include <fstream>
#include <string>
#include <vector>

class Event;
class Heartbeat;
class Scheduler;

class Statistics {
 public:
  Statistics(Scheduler&);
  ~Statistics();
  void Init(std::string, Topology);
  void RecordSend(Event*);
  static const std::string USAGE_LOG_NAME;
  static const std::string SEPARATOR;
  static const Time WINDOW_SIZE;

 private:
  Scheduler& scheduler_;
  /* Having to store pointers to ofstream's rather than the object itself is an
   * unfortunate consequence of ofstreams not being copyable.  I do not know of a
   * better solution.
   */
  std::ofstream bandwidth_usage_log_;
  Time window_left_;
  Time window_right_;
  Size cur_window_count_;
  Topology physical_;
  DISALLOW_COPY_AND_ASSIGN(Statistics);
};

#endif

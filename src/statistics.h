#ifndef DDCSIM_STATISTICS_H_
#define DDCSIM_STATISTICS_H_

#include "common.h"

#include <string>
#include <vector>

class Heartbeat;
class Scheduler;

class Statistics {
 public:
  Statistics(Scheduler&);
  ~Statistics();
  void Init();
  void Record(Heartbeat*);
  // TODO allow the user to set the prefix and suffix
  static const std::string LOG_PREFIX;
  static const std::string LOG_SUFFIX;

 private:
  Scheduler& scheduler_;
  /* Having to store pointers to ofstream's rather than the object itself is an
   * unfortunate consequence of ofstreams not being copyable.  I do not know of a
   * better solution.
   */
  std::vector<std::ofstream*> id_to_log_;
  DISALLOW_COPY_AND_ASSIGN(Statistics);
};

#endif

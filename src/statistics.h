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
  void Init();
  void Record(Heartbeat*);
  void RecordSend(Event*);
  // TODO allow the user to set the prefix and suffix
  static const std::string LOG_PREFIX;
  static const std::string LOG_SUFFIX;
  static const std::string USAGE_LOG_NAME;
  static const std::string SEPARATOR;

 private:
  Scheduler& scheduler_;
  /* Having to store pointers to ofstream's rather than the object itself is an
   * unfortunate consequence of ofstreams not being copyable.  I do not know of a
   * better solution.
   */
  // TODO more descriptive name
  //  std::vector<std::ofstream*> id_to_log_;
  std::ofstream bandwidth_usage_log_;
  DISALLOW_COPY_AND_ASSIGN(Statistics);
};

#endif

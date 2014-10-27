#include "statistics.h"
#include "entities.h"
#include "events.h"
#include "scheduler.h"

#include <assert.h>
#include <iostream>

using std::string;
using std::to_string;
using std::ofstream;

const string Statistics::LOG_PREFIX = "log_";
const string Statistics::LOG_SUFFIX = ".txt";
const string Statistics::USAGE_LOG_NAME = "network_usage.txt";
const string Statistics::SEPARATOR = ",";

Statistics::Statistics(Scheduler& s) : scheduler_(s), id_to_log_(),
                                       bandwidth_usage_log_() {}

Statistics::~Statistics() {
  for(auto it = id_to_log_.begin(); it != id_to_log_.end(); ++it) {
    (*it)->close();
    delete(*it);
  }

  bandwidth_usage_log_.close();
}

void Statistics::Init() {
  for(Id id = 0; id < scheduler_.kMaxEntities; ++id) {
    id_to_log_.push_back(new ofstream);
    id_to_log_[id]->open(LOG_PREFIX + to_string(id) + LOG_SUFFIX);
  }

  bandwidth_usage_log_.open(USAGE_LOG_NAME);
}

void Statistics::Record(Heartbeat* h) {
  assert(++(h->AffectedEntitiesBegin()) == h->AffectedEntitiesEnd());
  Id id = (*(h->AffectedEntitiesBegin()))->id();
  *(id_to_log_[id]) << h->time() << SEPARATOR << h->size() << "\n";
}

void Statistics::RecordSend(Event* e) {
  Time put_on_link = e->time() + Scheduler::kComputationDelay;
  bandwidth_usage_log_ << put_on_link << SEPARATOR << e->size() << "\n";
}

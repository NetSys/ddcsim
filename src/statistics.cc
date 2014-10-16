#include "statistics.h"
#include "entities.h"
#include "events.h"
#include "scheduler.h"

#include <assert.h>
#include <fstream>
#include <iostream>

using std::string;
using std::to_string;
using std::ofstream;

const string Statistics::LOG_PREFIX = "log_";
const string Statistics::LOG_SUFFIX = ".txt";

Statistics::Statistics(Scheduler& s) : scheduler_(s), id_to_log_() {}

Statistics::~Statistics() {
  for(auto it = id_to_log_.begin(); it != id_to_log_.end(); ++it) {
    (*it)->close();
    delete(*it);
  }
}

void Statistics::Init() {
  for(Id id = 0; id < scheduler_.kMaxEntities; ++id) {
    id_to_log_.push_back(new ofstream);
    id_to_log_[id]->open(LOG_PREFIX + to_string(id) + LOG_SUFFIX);
  }
}

void Statistics::Record(Heartbeat* h) {
  assert(++(h->AffectedEntitiesBegin()) == h->AffectedEntitiesEnd());
  Id id = (*(h->AffectedEntitiesBegin()))->id();
  *(id_to_log_[id]) << h->time() << "," << h->size() << "\n";
}

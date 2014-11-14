#include "statistics.h"
#include "entities.h"
#include "events.h"
#include "scheduler.h"

#include <glog/logging.h>
#include <cmath>
#include <iostream>

using std::string;
using std::to_string;
using std::ofstream;

const string Statistics::LOG_PREFIX = "log_";
const string Statistics::LOG_SUFFIX = ".txt";
const string Statistics::USAGE_LOG_NAME = "/mnt/sam/tmp/r1/network_usage.txt";
const string Statistics::SEPARATOR = ",";
const Time Statistics::WINDOW_SIZE = 0.05; /* 50 ms */

//Statistics::Statistics(Scheduler& s) : scheduler_(s), id_to_log_(),
Statistics::Statistics(Scheduler& s) : scheduler_(s), bandwidth_usage_log_(),
                                       window_left_(START_TIME),
                                       window_right_(WINDOW_SIZE),
                                       cur_window_count_(0),
				       to_write_out_(253671416) {}

Statistics::~Statistics() {
  // for(auto it = id_to_log_.begin(); it != id_to_log_.end(); ++it) {
  //   (*it)->close();
  //   delete(*it);
  // }

  bandwidth_usage_log_.close();
}

void Statistics::Init() {
  // for(Id id = 0; id < scheduler_.kMaxEntities; ++id) {
  //   id_to_log_.push_back(new ofstream);
  //   id_to_log_[id]->open(LOG_PREFIX + to_string(id) + LOG_SUFFIX);
  // }

  bandwidth_usage_log_.open(USAGE_LOG_NAME);
}

void Statistics::Record(Heartbeat* h) {
  // TODO why does glog CHECK_EQ throw compiler errors?
  // CHECK(h->AffectedEntitiesBegin() + 1 == h->AffectedEntitiesEnd());
  // Id id = (*(h->AffectedEntitiesBegin()))->id();
  // *(id_to_log_[id]) << h->time() << SEPARATOR << h->size() << "\n";
}

void Statistics::RecordSend(Event* e) {
  Time put_on_link = e->time() + Scheduler::kComputationDelay;

  if (! (window_left_ <= put_on_link && put_on_link < window_right_)) {
    //bandwidth_usage_log_ << put_on_link << SEPARATOR << cur_window_count_ << "\n";
    OutPair op = {put_on_link, cur_window_count_};
    to_write_out_.push_back(op);

    cur_window_count_ = 0;
    window_left_ = floor(put_on_link / WINDOW_SIZE);
    window_right_ = window_left_ + WINDOW_SIZE;
  }

  cur_window_count_ += e->size();
}

void Statistics::WriteOut() {
  for(auto it = to_write_out_.begin(); it != to_write_out_.end(); ++it)
    bandwidth_usage_log_ << it->put_on_link << SEPARATOR << it->size << "\n";
}

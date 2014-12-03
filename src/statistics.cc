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

const string Statistics::USAGE_LOG_NAME = "network_usage.txt";
const string Statistics::SEPARATOR = ",";
const Time Statistics::WINDOW_SIZE = 0.05; /* 50 ms */

Statistics::Statistics(Scheduler& s) : scheduler_(s), bandwidth_usage_log_(),
                                       window_left_(START_TIME),
                                       window_right_(WINDOW_SIZE),
                                       cur_window_count_(0) {}

Statistics::~Statistics() { bandwidth_usage_log_.close(); }

void Statistics::Init(string out_prefix, Topology physical) {
  bandwidth_usage_log_.open(out_prefix + USAGE_LOG_NAME,
                            ofstream::out | ofstream::app);

  physical_ = physical;
}

void Statistics::RecordSend(Event* e) {
  Time put_on_link = e->time_ + Scheduler::kComputationDelay;

  if (! (window_left_ <= put_on_link && put_on_link < window_right_)) {
    bandwidth_usage_log_ << window_left_ << SEPARATOR << cur_window_count_ << "\n";

    cur_window_count_ = 0;
    window_left_ = floor(put_on_link / WINDOW_SIZE) * WINDOW_SIZE;
    window_right_ = window_left_ + WINDOW_SIZE;
  }

  cur_window_count_ += e->size();
}

#include "frontier_queue.h"
#include "events.h"

#define INVALID_FRONTIER -1

#include <algorithm>

using std::find;
using std::pair;
using std::vector;
using std::to_string;
using std::string;

FrontierQueue::Frontier::Frontier(Time t)
//    : next_free_(0), time_(t), events_(new vector<Event*>(kEventCapacity)) {}
    : next_free_(0), time_(t), events_() {}//kEventCapacity, NULL) {}

#ifndef NDEBUG
FrontierQueue::Frontier::~Frontier() {
  for(auto e : events_)
    delete e;
}
#endif

// FrontierQueue::Frontier::Frontier(Time t, vector<Event*>* v)
//     : next_free_(0), time_(t), events_(v) {}

void FrontierQueue::Frontier::Push(Event* e) {
  CHECK_EQ(e->time_, time_);

  //  if (next_free_ < events_->size())
  //    (*events_)[next_free_] = e;
  //  else
  //  events_->push_back(e);
  events_.push_back(e);

    //  ++next_free_;
}

Event* FrontierQueue::Frontier::Pop() {
  //  CHECK_GT(next_free_, 0);
  //  --next_free_;
  //  return (*events_)[next_free_];
  auto rtn = events_.back();
  events_.pop_back();
  return rtn;
}

//bool FrontierQueue::Frontier::Empty() const { return next_free_ == 0; }

bool FrontierQueue::Frontier::Empty() const { return events_.size() == 0; }

string FrontierQueue::Frontier::Description() const {
  // return "time_=" + to_string(time_) +
  //     " next_free_=" + to_string(next_free_) +
  //     " events_=" + to_string(reinterpret_cast<long>(events_));
  return "";
}

//vector<Event*>* FrontierQueue::Frontier::events() const { return events_; }

Time FrontierQueue::Frontier::time() const { return time_; }

FrontierQueue::FrontierQueue() : frontiers_(), event_queue_() {}//, unused_() {}

#ifndef NDEBUG
FrontierQueue::~FrontierQueue() {
  for(auto p : frontiers_)
    delete p.second;
}
#endif

void FrontierQueue::Push(Time t, Event* e) {
  if(frontiers_.count(t) == 1) {
    frontiers_[t]->Push(e);
    return;
  }

  Frontier* f;

  // if(unused_.size() > 0) {
  //   auto it = unused_.begin();
  //   auto cur_max = unused_.begin();

  //   ++it;

  //   for( ; it != unused_.end(); ++it)
  //     if((*it)->size() > (*cur_max)->size())
  //       cur_max =  it;

  //   unused_.erase(cur_max);

  //   f = new Frontier(t, *cur_max);
  // } else {
    f = new Frontier(t);
    //  }

  frontiers_.insert({t, f});
  event_queue_.push(f);

  f->Push(e);
}

Event* FrontierQueue::Pop() {
  CHECK(!Empty());

  Frontier* cur = event_queue_.top();

  Event* rtn = cur->Pop();

  if(cur->Empty()) {
    Time t = cur->time();
    frontiers_.erase(t);
    //    unused_.push_back(cur->events());
    event_queue_.pop();
    delete cur;
  }

  return rtn;
}

bool FrontierQueue::Empty() {
  return event_queue_.size() == 0;
}

bool FrontierQueue::Comparator::operator() (const Frontier* const lhs,
                                            const Frontier* const rhs) const {

  return lhs->time() > rhs->time();
}

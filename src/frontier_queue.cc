#include "frontier_queue.h"
#include "events.h"
#include "entities.h"

#define INVALID_FRONTIER -1

//#include <algorithm>
#include <limits>

using std::find;
using std::pair;
using std::vector;
using std::to_string;
using std::string;
using std::numeric_limits;
using std::min;

FrontierQueue::Frontier::Frontier(Time t, size_t entity_count) :
    time_(t), id_to_events_(entity_count), cur_(numeric_limits<Id>::max()) {}

#ifndef NDEBUG
FrontierQueue::Frontier::~Frontier() {
  //  for(auto e : events_)
  //    delete e;
}
#endif

void FrontierQueue::Frontier::Push(Event* e) {
  CHECK_EQ(e->time_, time_);
  CHECK(e->affected_entities_.size() == 1);
  Id id = e->affected_entities_[0]->id();
  id_to_events_[id].push_back(e);
  cur_ = min(cur_, id);
}

void FrontierQueue::Frontier::Update() {
  for( ; cur_ < id_to_events_.size(); ++cur_)
    if(! id_to_events_[cur_].empty())
      break;
}

Event* FrontierQueue::Frontier::Pop() {
  vector<Event*>& cur_events = id_to_events_[cur_];

  auto rtn = cur_events.back();
  cur_events.pop_back();

  Update();

  return rtn;
}

bool FrontierQueue::Frontier::Empty() const {
  return cur_ >= id_to_events_.size();
}

string FrontierQueue::Frontier::Description() const {
  return "time_=" + to_string(time_);
      //      " events_=" + to_string(reinterpret_cast<long>(events_));
}

Time FrontierQueue::Frontier::time() const { return time_; }

FrontierQueue::FrontierQueue(size_t entity_count) :
    frontiers_(), event_queue_(), entity_count_(entity_count) {}

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

  Frontier* f = new Frontier(t, entity_count_);

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

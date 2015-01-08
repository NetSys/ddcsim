#include "frontier_queue.h"
#include "entities.h"

#define INVALID_FRONTIER -1

#include <limits>

using std::find;
using std::pair;
using std::vector;
using std::to_string;
using std::string;
using std::numeric_limits;

FrontierQueue::Frontier::Events::Events() :
    lsrs_(), rus_(), ils_(), lsus_(), us_(), ds_(), lus_(), lds_(),
    lsrs_next_(-1), rus_next_(-1), ils_next_(-1), lsus_next_(-1),
    us_next_(-1), ds_next_(-1), lus_next_(-1), lds_next_(-1) {}

void FrontierQueue::Frontier::Events::Push(LinkStateRequest e) {
  lsrs_.push_back(e);
  ++lsrs_next_;
}

void FrontierQueue::Frontier::Events::Push(RoutingUpdate e) {
  rus_.push_back(e);
  ++rus_next_;
}

void FrontierQueue::Frontier::Events::Push(InitiateLinkState e) {
  ils_.push_back(e);
  ++ils_next_;
}

void FrontierQueue::Frontier::Events::Push(LinkStateUpdate e) {
  lsus_.push_back(e);
  ++lsus_next_;
}

void FrontierQueue::Frontier::Events::Push(Up e) {
  us_.push_back(e);
  ++us_next_;
}

void FrontierQueue::Frontier::Events::Push(Down e) {
  ds_.push_back(e);
  ++ds_next_;
}

void FrontierQueue::Frontier::Events::Push(LinkUp e) {
  lus_.push_back(e);
  ++lus_next_;
}

void FrontierQueue::Frontier::Events::Push(LinkDown e) {
  lds_.push_back(e);
  ++lds_next_;
}

Event* FrontierQueue::Frontier::Events::Pop() {
  if(lsrs_next_ != -1) {
    return &lsrs_[lsrs_next_--];
  }
  if(rus_next_ != -1) {
    return &rus_[rus_next_--];
  }
  if(ils_next_ != -1) {
    return &ils_[ils_next_--];
  }
  if(lsus_next_ != -1) {
    return &lsus_[lsus_next_--];
  }
  if(us_next_ != -1) {
    return &us_[us_next_--];
  }
  if(ds_next_ != -1) {
    return &ds_[ds_next_--];
  }
  if(lus_next_ != -1) {
    return &lus_[lus_next_--];
  }
  if(lds_next_ != -1) {
    return &lds_[lds_next_--];
  }
  CHECK(false);
}

bool FrontierQueue::Frontier::Events::Empty() {
  return lsrs_next_ == -1 && rus_next_ == -1 && ils_next_ == -1 && lsus_next_ == -1 &&
      us_next_ == -1 && ds_next_ == -1 && lus_next_ == -1 && lds_next_ == -1;
}

FrontierQueue::Frontier::Frontier(Time t, size_t entity_count) :
    time_(t), id_to_events_(entity_count), cur_(numeric_limits<Id>::max()) {}

void FrontierQueue::Frontier::Update() {
  for( ; cur_ < id_to_events_.size(); ++cur_)
    if(! id_to_events_[cur_].Empty())
      break;
}

Event* FrontierQueue::Frontier::Pop() {
  Events& cur_events = id_to_events_[cur_];

  auto rtn = cur_events.Pop();

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
    frontiers_(), event_queue_(), entity_count_(entity_count),
    to_be_deleted_(nullptr) {}

#ifndef NDEBUG
FrontierQueue::~FrontierQueue() {
  for(auto p : frontiers_)
    delete p.second;
  delete to_be_deleted_;
}
#endif

Event* FrontierQueue::Pop() {
  CHECK(!Empty());

  if(to_be_deleted_) {
    delete to_be_deleted_;
    to_be_deleted_ = nullptr;
  }

  Frontier* cur = event_queue_.top();

  Event* rtn = cur->Pop();

  if(cur->Empty()) {
    Time t = cur->time();
    frontiers_.erase(t);
    event_queue_.pop();
    to_be_deleted_ = cur;
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

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

EventMemPool::EventMemPool() : event_pool_() {
  for(int i = 0; i < palloc_mul_; ++i) {
    EventPool cur = {new vector<LinkStateUpdate>(max_lsu_hint_),
                     new vector<RoutingUpdate>(max_ru_hint_),
                     new vector<LinkStateRequest>(max_lsr_hint_)};
    event_pool_.push_back(cur);
  }
}

#ifndef NDEBUG
EventMemPool::~EventMemPool() {
  for(auto ep : event_pool_) {
    delete ep.lsus;
    delete ep.rus;
    delete ep.lsrs;
  }
}
#endif

bool EventMemPool::IsEmpty() { return event_pool_.empty(); }

EventPool EventMemPool::Retrieve() {
  auto rtn = event_pool_.back();
  event_pool_.pop_back();
  return rtn;
}

void EventMemPool::Free(EventPool ep) {
  event_pool_.push_back(ep);
}

FrontierQueue::Frontier::Events::Events() :
    lsrs_(nullptr),
    rus_(nullptr),
    ils_(new vector<InitiateLinkState>()),
    lsus_(nullptr),
    us_(new vector<Up>()),
    ds_(new vector<Down>()),
    lus_(new vector<LinkUp>()),
    lds_(new vector<LinkDown>()),
    lsrs_next_(0), rus_next_(0), ils_next_(0), lsus_next_(0),
    us_next_(0), ds_next_(0), lus_next_(0), lds_next_(0) {
  if(pool_.IsEmpty()) {
    rus_ = new vector<RoutingUpdate>();
    lsus_ = new vector<LinkStateUpdate>();
    lsrs_ = new vector<LinkStateRequest>();
  } else {
    auto ep = pool_.Retrieve();
    rus_ = ep.rus;
    lsus_ = ep.lsus;
    lsrs_ = ep.lsrs;
  }
}

EventMemPool FrontierQueue::Frontier::Events::pool_;

#ifndef NDEBUG
FrontierQueue::Frontier::Events::~Events() {
  delete ils_;
  delete us_;
  delete ds_;
  delete lus_;
  delete lds_;
  EventPool ep = {lsus_, rus_, lsrs_};
  pool_.Free(ep);
}
#endif

void FrontierQueue::Frontier::Events::Push(LinkStateRequest e) {
  if(lsrs_next_ >= lsrs_->size()) {
    lsrs_->push_back(e);
    ++lsrs_next_;
  } else {
    (*lsrs_)[lsrs_next_++] = e;
  }
}

void FrontierQueue::Frontier::Events::Push(RoutingUpdate e) {
  if(rus_next_ >= rus_->size()) {
    rus_->push_back(e);
    ++rus_next_;
  } else {
    (*rus_)[rus_next_++] = e;
  }
}

void FrontierQueue::Frontier::Events::Push(InitiateLinkState e) {
  if(ils_next_ >= ils_->size()) {
    ils_->push_back(e);
    ++ils_next_;
  } else {
    (*ils_)[ils_next_++] = e;
  }
}

void FrontierQueue::Frontier::Events::Push(LinkStateUpdate e) {
  if(lsus_next_ >= lsus_->size()) {
    lsus_->push_back(e);
    ++lsus_next_;
  } else {
    (*lsus_)[lsus_next_++] = e;
  }
}

void FrontierQueue::Frontier::Events::Push(Up e) {
  if(us_next_ >= us_->size()) {
    us_->push_back(e);
    ++us_next_;
  } else {
    (*us_)[us_next_++] = e;
  }
}

void FrontierQueue::Frontier::Events::Push(Down e) {
  if(ds_next_ >= ds_->size()) {
    ds_->push_back(e);
    ++ds_next_;
  } else {
    (*ds_)[ds_next_++] = e;
  }
}

void FrontierQueue::Frontier::Events::Push(LinkUp e) {
  if(lus_next_ >= lus_->size()) {
    lus_->push_back(e);
    ++lus_next_;
  } else {
    (*lus_)[lus_next_++] = e;
  }
}

void FrontierQueue::Frontier::Events::Push(LinkDown e) {
  if(lds_next_ >= lds_->size()) {
    lds_->push_back(e);
    ++lds_next_;
  } else {
    (*lds_)[lds_next_++] = e;
  }
}

Event* FrontierQueue::Frontier::Events::Pop() {
  if(lsrs_next_ != 0) {
    return &((*lsrs_)[--lsrs_next_]);
  }
  if(rus_next_ != 0) {
    return &((*rus_)[--rus_next_]);
  }
  if(ils_next_ != 0) {
    return &((*ils_)[--ils_next_]);
  }
  if(lsus_next_ != 0) {
    return &((*lsus_)[--lsus_next_]);
  }
  if(us_next_ != 0) {
    return &((*us_)[--us_next_]);
  }
  if(ds_next_ != 0) {
    return &((*ds_)[--ds_next_]);
  }
  if(lus_next_ != 0) {
    return &((*lus_)[--lus_next_]);
  }
  if(lds_next_ != 0) {
    return &((*lds_)[--lds_next_]);
  }
  CHECK(false);
}

bool FrontierQueue::Frontier::Events::Empty() {
  return lsrs_next_ == 0 && rus_next_ == 0 && ils_next_ == 0 && lsus_next_ == 0 &&
      us_next_ == 0 && ds_next_ == 0 && lus_next_ == 0 && lds_next_ == 0;
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

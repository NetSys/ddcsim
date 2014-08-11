#include "scheduler.h"

using std::priority_queue;
using std::vector;

Scheduler::Scheduler() : event_queue_() {}

void Scheduler::AddEvent(Event* e) {
  event_queue_.push(e);
}

bool Scheduler::HasNextEvent() {
  return ! event_queue_.empty();
}

Event* Scheduler::NextEvent() {
  Event* next = event_queue_.top();
  event_queue_.pop();
  return next;
}

bool Scheduler::Comparator::operator() (const Event* const lhs,
                                        const Event* const rhs) const {
  return lhs->time() > rhs->time();
}

#ifndef DDCSIM_FRONTIER_QUEUE_H_
#define DDCSIM_FRONTIER_QUEUE_H_

#include "common.h"

#include <queue>
#include <vector>
#include <string>

class Event;

class FrontierQueue {
  // TODO can you only forward declare?
  class Frontier {
   public:
    Frontier(Time, size_t);
#ifndef NDEBUG
    ~Frontier();
#endif
    void Push(Event*);
    Event* Pop();
    bool Empty() const;
    Time time() const;
    std::string Description() const;

   private:
    void Update();
    Time time_;
    Id cur_;
    std::vector<std::vector<Event*> > id_to_events_;
    static const size_t kEventCapacity = 1000000;
  };

  class Comparator {
   public:
    bool operator() (const Frontier* const, const Frontier* const) const;
  };

 public:
  FrontierQueue(size_t);
#ifndef NDEBUG
  ~FrontierQueue();
#endif
  void Push(Time, Event*);
  Event* Pop();
  bool Empty();

 private:
  // TODO check if having a pointer to Frontier is dangerous
  std::unordered_map<Time, Frontier*> frontiers_;
  std::priority_queue<Frontier*, std::vector<Frontier*>, Comparator> event_queue_;
  size_t entity_count_;
  DISALLOW_COPY_AND_ASSIGN(FrontierQueue);
};


#endif

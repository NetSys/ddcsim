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
    Frontier(Time);
#ifndef NDEBUG
    ~Frontier();
#endif
    //    Frontier(Time, std::vector<Event*>*);
    void Push(Event*);
    Event* Pop();
    bool Empty() const;
    //    std::vector<Event*>* events() const;
    Time time() const;
    std::string Description() const;

   private:
    Time time_;
    int next_free_;
    std::vector<Event*> events_;
    static const size_t kEventCapacity = 1000000;
  };

  class Comparator {
   public:
    bool operator() (const Frontier* const, const Frontier* const) const;
  };

 public:
  FrontierQueue();
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
  //  std::vector<std::vector<Event*>*> unused_;
  DISALLOW_COPY_AND_ASSIGN(FrontierQueue);
};


#endif

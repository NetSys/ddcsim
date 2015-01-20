#ifndef DDCSIM_FRONTIER_QUEUE_H_
#define DDCSIM_FRONTIER_QUEUE_H_

#include "common.h"
#include "events.h"

#include <algorithm>
#include <queue>
#include <vector>
#include <string>

typedef struct event_pool {
  std::vector<LinkStateUpdate>* lsus;
  std::vector<RoutingUpdate>* rus;
  std::vector<LinkStateRequest>* lsrs;
} EventPool;

class EventMemPool {
 public:
  EventMemPool();
#ifndef NDEBUG
  ~EventMemPool();
#endif
  bool IsEmpty();
  EventPool Retrieve();
  void Free(EventPool);

 private:
  std::vector<EventPool> event_pool_;
  // TODO find these out
  static const size_t max_lsu_hint_ = 1000;
  static const size_t max_lsr_hint_ = 1000;
  static const size_t max_ru_hint_ = 1000;
  static const unsigned int palloc_mul_ = 2;
  DISALLOW_COPY_AND_ASSIGN(EventMemPool);
};

class FrontierQueue {
  // TODO can you only forward declare?
  class Frontier {
    class Events {
     public:
      Events();
#ifndef NDEBUG
      ~Events();
#endif
      void Push(LinkStateRequest);
      void Push(RoutingUpdate);
      void Push(InitiateLinkState);
      void Push(LinkStateUpdate);
      void Push(Up);
      void Push(Down);
      void Push(LinkUp);
      void Push(LinkDown);
      Event* Pop();
      bool Empty();

     private:
      static EventMemPool pool_;
      std::vector<LinkStateRequest>* lsrs_;
      int lsrs_next_;
      std::vector<RoutingUpdate>* rus_;
      int rus_next_;
      std::vector<InitiateLinkState>* ils_;
      int ils_next_;
      std::vector<LinkStateUpdate>* lsus_;
      int lsus_next_;
      std::vector<Up>* us_;
      int us_next_;
      std::vector<Down>* ds_;
      int ds_next_;
      std::vector<LinkUp>* lus_;
      int lus_next_;
      std::vector<LinkDown>* lds_;
      int lds_next_;
    };

   public:
    Frontier(Time, size_t);
    template<class E> void Push(E e) {
      CHECK_EQ(e.time_, time_);
      CHECK(e.affected_entities_.size() == 1);
      Id id = e.affected_entities_[0]->id();
      id_to_events_[id].Push(e);
      cur_ = std::min(cur_, id);
    }
    Event* Pop();
    bool Empty() const;
    Time time() const;
    std::string Description() const;

   private:
    void Update();
    Time time_;
    Id cur_;
    std::vector<Events> id_to_events_;
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
  template<class E> void Push(Time t, E e) {
    if(frontiers_.count(t) == 1) {
      frontiers_[t]->Push(e);
      return;
    }

    Frontier* f = new Frontier(t, entity_count_);

    frontiers_.insert({t, f});
    event_queue_.push(f);

    f->Push(e);
  }
  Event* Pop();
  bool Empty();

 private:
  // TODO check if having a pointer to Frontier is dangerous
  std::unordered_map<Time, Frontier*> frontiers_;
  std::priority_queue<Frontier*, std::vector<Frontier*>, Comparator> event_queue_;
  size_t entity_count_;
  Frontier* to_be_deleted_;
  DISALLOW_COPY_AND_ASSIGN(FrontierQueue);
};


#endif

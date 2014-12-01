#ifndef DDCSIM_LINKS_H_
#define DDCSIM_LINKS_H_

#include <vector>
#include <unordered_map>

#include "common.h"
#include "reader.h"
#include "scheduler.h"

class Entity;
class Statistics;

// TODO Put into Links class?
class BandwidthMeter {
 public:
  BandwidthMeter(Size, Rate);
  bool CanSend(Size);
  void UpdateCapacity(Time);
  void Send(Size);
  /*
   * Unless the user specifies bandwidth restrictions, bandwidth is
   * "unlimited".
   */
  static const Size kDefaultCapacity;
  static const Rate kDefaultRate;

 private:
  Size max_capacity_;
  Size cur_capacity_;
  Rate fill_rate_;
  DISALLOW_ASSIGN(BandwidthMeter);
};

class Links {
  typedef struct link {
    bool is_up;
    Entity* endpoint;
    BandwidthMeter meter;
  } Link;

 public:
  Links();
  // TODO what does style guide say about template being on its own line?
  template<class Iterator>
  void Init(Iterator neighbors_begin, Iterator neighbors_end,
            Size capacity, Rate fill) {
    for ( ; neighbors_begin != neighbors_end; ++neighbors_begin)
      port_to_link_.push_back(
          {true, *neighbors_begin, BandwidthMeter(capacity, fill)});
  }
  void SetLinkUp(Port);
  void SetLinkDown(Port);
  void UpdateCapacities(Time);
  unsigned int PortCount() const;
  bool IsLinkUp(Port) const;
  Entity* GetEndpoint(Port) const;

  template<class E, class M> friend void Scheduler::Forward(E* sender, M* msg_in, Port out, Statistics&);
  friend bool Reader::ParseEvents();

 private:
  Port GetPortTo(Entity*);
  std::vector<Link> port_to_link_;
  DISALLOW_COPY_AND_ASSIGN(Links);
};

#endif

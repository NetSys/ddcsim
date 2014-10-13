#ifndef DDCSIM_LINKS_H_
#define DDCSIM_LINKS_H_

#include <vector>
#include <unordered_map>

#include "common.h"
#include "reader.h"
#include "scheduler.h"

class Entity;

class Links {
 public:
  Links();
  template<class Iterator>
  void Init(Iterator neighbors_begin,
            Iterator neighbors_end,
            Size capacity, Rate fill) {
    for(Port p = 0; neighbors_begin != neighbors_end; ++neighbors_begin, ++p) {
      bucket_capacity = capacity;
      fill_rate = fill;
      port_nums_.push_back(p);
      port_to_link_.insert({p, {true, *neighbors_begin}});
      port_to_size_.insert({p, bucket_capacity});
    }
  }
  // TODO what does the style guide say about a newline after the template?
  // TODO return generic iterator rather than an interator to a vector
  std::vector<Port>::const_iterator PortsBegin();
  std::vector<Port>::const_iterator PortsEnd();
  void SetLinkUp(Port);
  void SetLinkDown(Port);
  void UpdateCapacities(Time);
  Size bucket_capacity;
  Rate fill_rate;
  static const Size kDefaultCapacity;
  static const Rate kDefaultRate;
  template<class E, class M> friend void Scheduler::Forward(E* sender, M* msg_in, Port out);
  friend bool Reader::ParseEvents();

 private:
  bool IsLinkUp(Port);
  Entity* GetEndpoint(Port);
  Port GetPortTo(Entity*);
  Port FindInPort(Entity*);
  std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator LinksBegin();
  std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator LinksEnd();
  // TODO use Boost's iterator transformers to return an iterator to only keys
  std::vector<Port> port_nums_;
  std::unordered_map<Port, std::pair<bool,Entity*> > port_to_link_;
  // TODO combine port mappings
  std::unordered_map<Port, Size> port_to_size_;
  DISALLOW_COPY_AND_ASSIGN(Links);
};

#endif

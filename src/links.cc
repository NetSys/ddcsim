#include "links.h"
#include "entities.h"

#include <algorithm>

using std::min;
using std::vector;
using std::unordered_map;

// TODO how to push initilinks into constructor
Links::Links() : port_nums_(), port_to_link_(), port_to_size_(),
                 bucket_capacity(UNINIT_SIZE), fill_rate(UNINIT_RATE) {}

vector<Port>::const_iterator Links::PortsBegin() { return port_nums_.cbegin(); }

vector<Port>::const_iterator Links::PortsEnd() { return port_nums_.cend(); }

void Links::SetLinkUp(Port p) { port_to_link_[p].first = true; }

void Links::SetLinkDown(Port p) { port_to_link_[p].first = false; }

void Links::UpdateCapacities(Time passed) {
  /* Token buckets fill up at a rate of fill_rate bytes/sec so for each
   * link, we need to add fill_rate * (cur_time_ - last_time) bytes to its
   * bucket UNLESS it is already full.  Thus, we update each token bucket by
   * size = min{ size + fill_rate * (cur_time_ - last_time), bucket_capacity }
   */

  for(auto it = port_to_size_.begin(); it != port_to_size_.end(); ++it)
    // TODO having to cast to a double is bullshit
    // why the fuck did I alias the type in the first place
    it->second = min(static_cast<double>(bucket_capacity),
                     static_cast<double>(it->second + fill_rate * passed));
}

const Size Links::kDefaultCapacity = 1 << 31;

const Rate Links::kDefaultRate = 1 << 31;

bool Links::IsLinkUp(Port p) { return port_to_link_[p].first; }

Entity* Links::GetEndpoint(Port p) { return port_to_link_[p].second; }

Port Links::GetPortTo(Entity* endpoint) {
  for(auto it = LinksBegin(); it != LinksEnd(); ++it)
    if(it->second.second == endpoint)
      return it->first;
  return PORT_NOT_FOUND;
}

Port Links::FindInPort(Entity* sender) {
  for(auto it = LinksBegin(); it != LinksEnd(); ++it)
    if(sender == it->second.second)
      return it->first;
  return PORT_NOT_FOUND;
}

std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator
Links::LinksBegin() { return port_to_link_.cbegin(); }

std::unordered_map<Port, std::pair<bool,Entity*> >::const_iterator
Links::LinksEnd() { return port_to_link_.cend(); }

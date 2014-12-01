#include "links.h"
#include "entities.h"

#include <algorithm>
#include <limits>

using std::min;
using std::numeric_limits;
using std::vector;
using std::unordered_map;

BandwidthMeter::BandwidthMeter(Size s, Rate r) : max_capacity_(s),
                                                 cur_capacity_(s),
                                                 fill_rate_(r) {}

bool BandwidthMeter::CanSend(Size s) { return s <= cur_capacity_; }

void BandwidthMeter::UpdateCapacity(Time t) {
  /* Token buckets fill up at a rate of fill_rate bytes/sec so for each
   * link, we need to add fill_rate * (cur_time_ - last_time) bytes to its
   * bucket UNLESS it is already full.  Thus, we update each token bucket by
   * size = min{ size + fill_rate * (cur_time_ - last_time), bucket_capacity }
   */
  // TODO need to worry about overflow?
  cur_capacity_ = min(max_capacity_, cur_capacity_ + fill_rate_ * t);
}

void BandwidthMeter::Send(Size s) { cur_capacity_-= s; }

const Size BandwidthMeter::kDefaultCapacity = numeric_limits<Size>::max();

const Rate BandwidthMeter::kDefaultRate = numeric_limits<Rate>::max();

Links::Links() : port_to_link_() {}

void Links::SetLinkUp(Port p) { port_to_link_[p].is_up = true; }

void Links::SetLinkDown(Port p) { port_to_link_[p].is_up = false; }

void Links::UpdateCapacities(Time passed) {
  for(auto it = port_to_link_.begin(); it != port_to_link_.end(); ++it)
    it->meter.UpdateCapacity(passed);
}

bool Links::IsLinkUp(Port p) const { return port_to_link_[p].is_up; }

Entity* Links::GetEndpoint(Port p) const { return port_to_link_[p].endpoint; }

Port Links::GetPortTo(Entity* dst) {
  for(Port p = 0; p < PortCount(); ++p)
    if(port_to_link_[p].endpoint == dst)
      return p;
  return PORT_NOT_FOUND;
}

unsigned int Links::PortCount() const { return port_to_link_.size(); }

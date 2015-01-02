#include "common.h"
#include "statistics.h"
#include "scheduler.h"
#include "entities.h"
#include "events.h"

#include "boost/graph/connected_components.hpp"
#include "boost/tuple/tuple.hpp"

#include <bitset>

using std::bitset;
using std::string;
using std::to_string;
using std::vector;
using std::unordered_map;

using boost::add_edge;
using boost::edge;
using boost::remove_edge;
using boost::vertex;

const string Statistics::SEPARATOR = ",";
const Time Statistics::WINDOW_SIZE = 0.05; /* 50 ms */

Statistics::Statistics(string out_prefix, Scheduler& s) :
    scheduler_(s),
    out_prefix_(out_prefix),
    id_to_component_(s.kSwitchCount + s.kControllerCount + s.kHostCount),
    id_to_entity_(s.kSwitchCount + s.kControllerCount + s.kHostCount),
    switch_to_table_(s.kSwitchCount),
    begin_host_(s.kSwitchCount + s.kControllerCount),
    beyond_host_(s.kSwitchCount + s.kControllerCount + s.kHostCount),
    host_to_edge_switch_(s.kHostCount),
    window_left_(START_TIME),
    window_right_(WINDOW_SIZE),
    cur_window_count_(0) {}

vector<Entity*>& Statistics::id_to_entity() { return id_to_entity_; }

void Statistics::Init(Topology physical) {
  physical_ = physical;

  for(int i = 0; i < host_to_edge_switch_.size(); ++i)
    host_to_edge_switch_[i] =
        (static_cast<Host*>(id_to_entity_[i +
                                          scheduler_.kSwitchCount +
                                          scheduler_.kControllerCount]))->EdgeSwitch();

}

void Statistics::EntityUp(Id entity) {
  LOG(FATAL) << "entityup: readily implementable but hasn't been";
}

void Statistics::EntityDown(Id entity){
  LOG(FATAL) << "entitydown: readily implementable but hasn't been";
}

void Statistics::LinkUp(Id src, Id dst) {
  add_edge(vertex(src, physical_),
           vertex(dst, physical_),
           physical_);
}

void Statistics::LinkDown(Id src, Id dst) {
  remove_edge(vertex(src, physical_),
              vertex(dst, physical_),
              physical_);
}

string Statistics::Reachability() {
  int phys_reachable = ComputePhysReachable();
  int virt_reachable = ComputeVirtReachable();

  return to_string(virt_reachable) + "/" + to_string(phys_reachable);

  // LOG(WARNING) << "reachability @ " << scheduler_.cur_time() << " is " <<
  //     virt_reachable << "/" << phys_reachable;
}

// throw away function for finding the diameter of input graph
// int Statistics::MaxPathLength() {
//   int max = -1;

//   typedef boost::graph_traits<Topology>::vertex_descriptor Vertex;

//   boost::graph_traits<Topology>::vertices_size_type d[num_vertices(physical_)];

//   for(Vertex i = 0; i < num_vertices(physical_); ++i) {
//     std::fill_n(d, num_vertices(physical_), 0);

//     boost::breadth_first_search(physical_, i,
//                                 boost::visitor(boost::make_bfs_visitor(
//                                     boost::record_distances(d, boost::on_tree_edge()))));
//     for(int dist : d)
//       if(dist > max)
//         max = dist;
//   }

//   return max;
// }

void Statistics::RecordEventCounts() {
  LOG(WARNING) << "Up=" << Up::count_;
  LOG(WARNING) << "Down=" << Down::count_;
  LOG(WARNING) << "LinkUp=" << LinkUp::count_;
  LOG(WARNING) << "LinkDown=" << LinkDown::count_;
  LOG(WARNING) << "LinkStateRequest=" << LinkStateRequest::count_;
  LOG(WARNING) << "LinkStateUpdate=" << LinkStateUpdate::count_;
  LOG(WARNING) << "InitiateLinkState=" << InitiateLinkState::count_;
  LOG(WARNING) << "RoutingUpdate=" << RoutingUpdate::count_;

  Up::count_ = Down::count_ = LinkUp::count_ = LinkDown::count_ =
      LinkStateRequest::count_ = LinkStateUpdate::count_ =
      InitiateLinkState::count_ = RoutingUpdate::count_ = 0;
}

void Statistics::InitComponents() {
  for(int i = 0; i < id_to_component_.size(); ++i)
    id_to_component_[i] = i;
}

int Statistics::ComputePhysReachable() {
  // TODO is this necessary?
  InitComponents();

  connected_components(physical_, &id_to_component_[0]);

  int reachable = 0;

  for(Id src = begin_host_; src < beyond_host_; ++src)
    for(Id dst = begin_host_; dst < beyond_host_; ++dst)
      if(src != dst && id_to_component_[src] == id_to_component_[dst])
        ++reachable;

  return reachable;
}

int Statistics::ComputeVirtReachable() {
  for(int i = 0; i < scheduler_.kSwitchCount; ++i)
    switch_to_table_[i] = (static_cast<Switch*>(id_to_entity_[i]))->dst_to_neighbor_;

  int reachable = 0;
  Id cur, prev;
  bool has_edge;
  static bitset<10000> visited(scheduler_.kSwitchCount);

  for(Id src = begin_host_; src < beyond_host_; ++src) {
    for(Id dst = begin_host_; dst < beyond_host_; ++dst) {
      if(src == dst) continue;

      prev = src;
      cur = host_to_edge_switch_[src - scheduler_.kSwitchCount
                                 - scheduler_.kControllerCount];

      visited.reset();
      for( ; cur != DROP && cur != dst && !visited[cur]; ) {
        has_edge = edge(vertex(prev, physical_),
                        vertex(cur, physical_),
                        physical_).second;

        if(! has_edge) break;

        visited[cur] = true;
        prev = cur;

        if(switch_to_table_[cur])
          cur = (*switch_to_table_[cur])
              [dst - scheduler_.kSwitchCount - scheduler_.kControllerCount];
        else
          cur = DROP;
      }

      if(cur == dst) {
        has_edge = edge(vertex(prev, physical_),
                        vertex(cur, physical_),
                        physical_).second;
        if(has_edge)
          ++reachable;
      }
    }
  }

  return reachable;
}

void Statistics::RecordSend(Event* e) {
  Time put_on_link = e->time_ + Scheduler::kComputationDelay;

  if (! (window_left_ <= put_on_link && put_on_link < window_right_)) {
    LOG(WARNING) << "bw in [" << window_left_ << "," << window_right_
                 << ") is " << cur_window_count_ << " bytes";
    cur_window_count_ = 0;
    window_left_ = floor(put_on_link / WINDOW_SIZE) * WINDOW_SIZE;
    window_right_ = window_left_ + WINDOW_SIZE;
  }

  cur_window_count_ += e->size();
}

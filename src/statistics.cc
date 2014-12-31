#include "common.h"
#include "statistics.h"
#include "scheduler.h"
#include "entities.h"
#include "events.h"

#include <algorithm>
#include "boost/graph/breadth_first_search.hpp"
#include "boost/tuple/tuple.hpp"

using std::fill;
using std::string;
using std::to_string;
using std::ofstream;
using std::vector;
using std::unordered_map;

using boost::add_edge;
using boost::breadth_first_search;
using boost::edge;
using boost::record_distances;
using boost::remove_edge;
using boost::make_bfs_visitor;
using boost::on_tree_edge;
using boost::vertex;
using boost::visitor;

const string Statistics::REACHABILITY_LOG_NAME = "reachability.txt";

Statistics::Statistics(string out_prefix, Scheduler& s) :
    scheduler_(s),
    out_prefix_(out_prefix),
    reachability_log_(),
    d(s.kSwitchCount + s.kControllerCount + s.kHostCount),
    id_to_entity_() {}

unordered_map<Id, Entity*>& Statistics::id_to_entity() { return id_to_entity_; }

Statistics::~Statistics() { reachability_log_.close(); }

void Statistics::Init(Topology physical) {
  reachability_log_.open(out_prefix_ + REACHABILITY_LOG_NAME,
                         ofstream::out | ofstream::app);
  physical_ = physical;
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

void Statistics::RecordReachability() {
  Id begin_host = scheduler_.kSwitchCount + scheduler_.kControllerCount;
  Id beyond_host = scheduler_.kSwitchCount +
      scheduler_.kControllerCount +
      scheduler_.kHostCount;

  int phys_reachable = 0;
  for(Id src = begin_host; src < beyond_host; ++src) {
    fill(d.begin(), d.end(), NO_PATH);

    breadth_first_search(physical_,
                         vertex(src, physical_),
                         visitor(make_bfs_visitor(record_distances(&d[0],
                                                                   on_tree_edge()))));

    for(Id dst = begin_host; dst < beyond_host; ++dst)
      if(src != dst && d[dst] > NO_PATH)
        ++phys_reachable;
  }

  int virt_reachable = 0;
  for(Id src = begin_host; src < beyond_host; ++src) {
    for(Id dst = begin_host; dst < beyond_host; ++dst) {
      Id cur;

      if(src == dst)
        continue;

      Id next;
      bool has_edge;
      for(cur = src; cur != DROP && cur != dst; ) {
        next = id_to_entity_[cur]->NextHop(dst);

        has_edge = edge(vertex(cur, physical_),
                        vertex(next, physical_),
                        physical_).second;
        if(!has_edge) break;

        cur = next;
      }

      if(cur == dst)
        ++virt_reachable;
    }
  }

  reachability_log_ << scheduler_.cur_time() << "," <<
      virt_reachable << "/" << phys_reachable << "\n";
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

#include "common.h"
#include "statistics.h"
#include "scheduler.h"
#include "entities.h"

#include <algorithm>
#include "boost/graph/breadth_first_search.hpp"
//#include "boost/graph/johnson_all_pairs_shortest.hpp"
#include "boost/tuple/tuple.hpp"

using std::string;
using std::ofstream;
using std::vector;
using std::unordered_map;

const string Statistics::REACHABILITY_LOG_NAME = "reachability.txt";

using boost::vertex;
using boost::edge;
using boost::add_edge;
//using boost::johnson_all_pairs_shortest_paths;

Statistics::Statistics(string out_prefix, Scheduler& s) :
    scheduler_(s),
    out_prefix_(out_prefix), //physical_(num_entities),
    reachability_log_(),
    //    d(s.kSwitchCount + s.kControllerCount + s.kHostCount),
    //    dst_to_distance_(
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
  // Edge e;
  // bool exists;
  // boost::tie(e, exists) = edge(vertex(src, physical_),
  //                              vertex(dst, physical_),
  //                              physical_h);
  // if(!exists)
  //   add_edge(vertex(src, physical_), vertex(dst, physical_), physical_);
}

void Statistics::LinkDown(Id src, Id dst) {
  //  LOG(FATAL) << "linkdown";
}

void Statistics::RecordReachability() {
  int virt_reachable = 0;

  Id begin_host = scheduler_.kSwitchCount + scheduler_.kControllerCount;
  Id beyond_host = scheduler_.kSwitchCount +
      scheduler_.kControllerCount +
      scheduler_.kHostCount;

  for(Id src = begin_host; src < beyond_host; ++src) {
    for(Id dst = begin_host; dst < beyond_host; ++dst) {
      Id cur;

      for(cur = src; cur != DROP && cur != dst; ) {
        cur = (static_cast<Switch*>(id_to_entity_[cur]))->NextHop(dst);
      }

      if(cur == dst)
        ++virt_reachable;
    }
  }

  // int phys_reachable = 0;

  // //  boost::graph_traits<Topology>::vertices_size_type d[num_vertices(physical_)];

  // for(Id src = begin_host; src < beyond_host; ++src) {
  //   std::fill_n(d, num_vertices(physical_), 0);

  //   boost::breadth_first_search(physical_, vertex(src, physical_),
  //                               boost::visitor(
  //                                   boost::make_bfs_visitor(
  //                                       boost::record_distances(&d,
  //                                                               boost::on_tree_edge()))));

  //   for(int dist : d)
  //     if(dist > NO_PATH)
  //       ++phys_reachable;
  // }

  // for(Id src = begin_host; src < beyond_host; ++src)
  //   for(Id dst = begin_host; dst < beyond_host; ++dst)
  //     if(distance_matrix_[src][dst] != NO_PATH)
  //       ++phys_reachable;

  // write time, percentage reachable to file
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

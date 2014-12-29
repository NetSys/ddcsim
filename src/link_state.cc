#include "events.h"
#include "entities.h"
#include "link_state.h"
#include "scheduler.h"

#include "boost/tuple/tuple.hpp"
#include "boost/graph/breadth_first_search.hpp"
#include "boost/graph/connected_components.hpp"
#include <limits>
#include <unordered_set>

using std::array;
using std::min;
using std::string;
using std::shared_ptr;
using std::to_string;
using std::vector;
using std::make_shared;
using std::numeric_limits;
using std::unordered_set;

using boost::target;
using boost::add_edge;
using boost::breadth_first_search;
using boost::connected_components;
using boost::edge;
using boost::edges;
using boost::graph_traits;
using boost::make_bfs_visitor;
using boost::on_tree_edge;
using boost::record_predecessors;
using boost::tie;
using boost::vertex;
using boost::property_map;
using boost::vertex_index_t;
using boost::vertex_index;

typedef graph_traits<Topology>::vertex_iterator VertexIter;
typedef graph_traits<Topology>::edge_iterator EdgeIter;
typedef graph_traits<Topology>::out_edge_iterator OutEdgeIter;

LinkState::LinkState(Scheduler& s) : scheduler_(s),
                                     topology_(s.kSwitchCount +
                                               s.kControllerCount +
                                               s.kHostCount),
                                     next_(0),
                                     id_to_exp_(s.kSwitchCount),
                                     id_to_last_sn_(s.kSwitchCount, NONE_SEQNUM) {}

bool LinkState::IsStaleUpdate(LinkStateUpdate* ls) const {
  return id_to_last_sn_[ls->src_id_] >= ls->sn_;
}

bool LinkStateControl::Update(LinkStateUpdate* ls) {
  Id id = ls->src_id_;
  Vertex src = vertex(id, topology_);

  unordered_set<Id> new_links(ls->up_links_.begin(), ls->up_links_.end());
  unordered_set<Id> old_links;

  OutEdgeIter ei, ei_end;
  for(tie(ei, ei_end) = out_edges(src, topology_); ei != ei_end; ++ei)
    old_links.insert(target(*ei, topology_));

  if(new_links == old_links) {
      id_to_last_sn_[id] = ls->sn_;
      id_to_exp_[id] = ls->expiration_;
      return false;
  }

  clear_vertex(src, topology_);

  for(auto it = ls->up_links_.begin(); it != ls->up_links_.end() &&
          *it != NONE_ID; ++it) {
    add_edge(src, vertex(*it, topology_), topology_);
  }

  id_to_last_sn_[id] = ls->sn_;
  id_to_exp_[id] = ls->expiration_;

  return true;
}

bool LinkState::Update(LinkStateUpdate* ls) {
  Id id = ls->src_id_;
  Vertex src = vertex(id, topology_);

  clear_vertex(src, topology_);
  for(auto it = ls->up_links_.begin(); it != ls->up_links_.end() &&
          *it != NONE_ID; ++it) {
    add_edge(src, vertex(*it, topology_), topology_);
  }

  id_to_last_sn_[id] = ls->sn_;
  id_to_exp_[id] = ls->expiration_;

  return true;
}

bool LinkStateControl::ArePartitioned(Id id1, Id id2) const {
  return id_to_component_[id1] != id_to_component_[id2];
}

// TODO make computation more lazy, don't remove all edges until you have to operate on graph
void LinkState::Refresh(Time cur_time) {
  for(int i = 0; i < id_to_exp_.size(); ++i) {
    if(cur_time > id_to_exp_[i]) {
      id_to_last_sn_[i] = NONE_SEQNUM;
      clear_vertex(vertex(i, topology_), topology_);
    }
  }
}

void LinkStateControl::InitComponents() {
  for(int i = 0; i < id_to_component_.size(); ++i)
    id_to_component_[i] = i;
}

void LinkStateControl::ComputePartitions() {
  InitComponents();
  connected_components(topology_, &id_to_component_[0]);
}

Id LinkStateControl::LowestController(Id src) const {
  int my_component = id_to_component_[src];
  Id min_controller = numeric_limits<Id>::max();

  for(Id i = 0; i < id_to_component_.size(); ++i)
    if(id_to_component_[i] == my_component && scheduler_.IsController(i))
      min_controller = min(min_controller, i);

  return min_controller;
}

SequenceNum LinkState::NextSeqNum() { return next_++; }

string LinkState::Description() const {
  string rtn = "";
  VertexIter ui,ui_end;
  OutEdgeIter ei, ei_end;
  property_map<Topology, vertex_index_t>::type v_index =
      get(vertex_index, topology_);

  for (tie(ui,ui_end) = vertices(topology_); ui != ui_end; ++ui) {
    tie(ei,ei_end) = out_edges(*ui, topology_);
    if(ei != ei_end) {
      rtn += to_string(v_index[*ui]) + " <--> ";
      for(; ei != ei_end; ++ei)
        rtn += to_string(v_index[target(*ei, topology_)]) + " ";
      if(ui + 1 != ui_end)
        rtn += ",";
    }
  }

  return rtn;
}

//TODO ensure no type collisions between Id and Vertex
Id LinkStateControl::NextHop(Id src, Id dst, vector<Vertex>& pred) {
  Id cur;

  for(cur = dst; pred[cur] != cur && pred[cur] != src; cur = pred[cur])  ;

  return pred[cur] == cur ? NONE_ID : cur;
}

// TODO save on stack allocation
shared_ptr<vector<Id> > LinkStateControl::ComputeRoutingTable(Id src) {
  shared_ptr<vector<Id> > dst_to_neighbor = make_shared<vector<Id> >
      (scheduler_.kHostCount, DROP);

  for(int i = 0; i < pred_.size(); ++i)
    pred_[i] = i;

  // TODO use another shortest-path algorithm?
  breadth_first_search(topology_, static_cast<Vertex>(src),
                       visitor(make_bfs_visitor(
                           record_predecessors(&pred_[0],
                                               on_tree_edge()))));

  VertexIter ui,ui_end;
  for(tie(ui, ui_end) = vertices(topology_); ui != ui_end; ++ui)
    if(scheduler_.IsHost(*ui))
      (*dst_to_neighbor)[*ui - scheduler_.kSwitchCount - scheduler_.kControllerCount] =
          NextHop(src, *ui, pred_);

  return dst_to_neighbor;
}

// TODO pass back iterators?
vector<Id> LinkStateControl::SwitchesInParition(Id src) {
  vector<Id> switches;

  int my_component = id_to_component_[src];

  for(Id i = 0; i < id_to_component_.size(); ++i)
    if(id_to_component_[i] == my_component && scheduler_.IsSwitch(i))
      switches.push_back(i);

  return switches;
}

LinkStateControl::LinkStateControl(Scheduler& s)
    : LinkState(s),
      id_to_component_(s.kSwitchCount + s.kControllerCount +  s.kHostCount),
      pred_(s.kSwitchCount + s.kControllerCount + s.kHostCount) {
  InitComponents();
}

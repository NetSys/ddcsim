#include "events.h"
#include "entities.h"
#include "link_state.h"
#include "scheduler.h"

#include "boost/tuple/tuple.hpp"
#include "boost/graph/breadth_first_search.hpp"

#include <algorithm>
#include <limits>
#include <set>

using std::array;
using std::set_difference;
using std::min;
using std::string;
using std::shared_ptr;
using std::to_string;
using std::vector;
using std::make_shared;
using std::numeric_limits;
using std::set;

using boost::target;
using boost::add_edge;
using boost::adjacent_vertices;
using boost::breadth_first_search;
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
                                     topology_(
                                         new Topology(s.kSwitchCount +
                                                      s.kControllerCount +
                                                      s.kHostCount)),
                                     next_(0),
                                     id_to_last_
                                     (new vector<seen>(
                                         s.kSwitchCount, {NONE_SEQNUM, -1})) {}

bool LinkState::IsStaleUpdate(LinkStateUpdate* ls) const {
  return *id_to_last_[ls->src_id_].sn >= ls->sn_;
}

bool LinkState::HaveNewUpdate(LinkStateUpdate* ls) const {
  return *id_to_last_[ls->src_id_].sn > ls->sn_;
}

LinkStateUpdate* LinkState::CurrentLinkState(Entity* src, Id src_id) {
  array<Id, 13> up;
  int i = 0;

  Vertex vsrc = vertex(src_id, *topology_);
  // TODO replace with adjacent iter
  OutEdgeIter ei, ei_end;
  for(tie(ei, ei_end) = out_edges(vsrc, *topology_); ei != ei_end; ++ei) {
    up[i] = target(*ei, *topology_);
    ++i;
  }

  for(; i < 13; ++i)
    up[i] = NONE_ID;

  auto p = *id_to_last_[src_id];
  return new LinkStateUpdate(START_TIME,
                             NULL,
                             PORT_NOT_FOUND,
                             src,
                             p.sn,
                             p.exp,
                             up,
                             src_id);
}

std::shared_ptr<Topology> LinkStateControl::topology() { return topology_; }

std::shared_ptr<std::vector<seen> > LinkStateControl::id_to_last() {
  return id_to_last_;
}

// TODO simplify
bool LinkStateControl::Update(LinkStateUpdate* ls) {
  Vertex src = vertex(ls->src_id_, *topology_);

  graph_traits <Topology>::adjacency_iterator vi, vi_end;
  tie(vi, vi_end) = adjacent_vertices(src, *topology_);
  set<Id> old_links(vi, vi_end);

  array<Id, 13>::iterator ai;
  for(ai = ls->up_links_.begin();
      ai != ls->up_links_.end() && *ai != NONE_ID; ++ai) ;
  set<Id> new_links(ls->up_links_.begin(), ai);

  vector<Id> to_be_removed(old_links.size());
  auto it = set_difference(old_links.begin(),
                           old_links.end(),
                           new_links.begin(),
                           new_links.end(),
                           to_be_removed.begin());
  to_be_removed.resize(it - to_be_removed.begin());

  vector<Id> to_be_added(new_links.size());
  auto jt = set_difference(new_links.begin(),
                           new_links.end(),
                           old_links.begin(),
                           old_links.end(),
                           to_be_added.begin());
  to_be_added.resize(jt - to_be_added.begin());

  if(to_be_removed.empty() && to_be_added.empty()) {
    *id_to_last_[ls->src_id_] = {ls->sn_, ls->expiration_};
    return false;
  }

  if(!to_be_removed.empty()) {
    for(auto id : to_be_removed)
      remove_edge(src, vertex(id, *topology_), *topology_);
    did_remove_ = true;
  }

  if(did_remove_) {
    for(auto id : to_be_added)
      add_edge(src, vertex(id, *topology_), *topology_);
  } else {
    for(auto id : to_be_added) {
      add_edge(src, vertex(id, *topology_), *topology_);
      ds_.union_set(ls->src_id_, id);
    }
  }

  *id_to_last_[ls->src_id_] = {ls->sn_, ls->expiration_};

  return true;
}

bool LinkState::Update(LinkStateUpdate* ls) {
  Id id = ls->src_id_;
  Vertex src = vertex(id, *topology_);

  clear_vertex(src, *topology_);
  for(auto it = ls->up_links_.begin(); it != ls->up_links_.end(); ++it)
    if(*it != NONE_ID)
      add_edge(src, vertex(*it, *topology_), *topology_);

  *id_to_last_[id] = {ls->sn_, ls->expiration_};

  return true;
}

bool LinkStateControl::ArePartitioned(Id id1, Id id2) {
  return !same_component(vertex(id1, *topology_),
                         vertex(id2, *topology_),
                         ds_);
}

bool LinkStateControl::HealsPartition(Id self, LinkStateUpdate* lsu) {
  for(auto it = lsu->up_links_.begin(); it != lsu->up_links_.end(); ++it)
    if(*it != NONE_ID && ArePartitioned(self, *it))
      return true;

  return false;
}

// TODO make computation more lazy, don't remove all edges until you have to operate on graph
void LinkState::Refresh(Time cur_time) {
  for(int i = 0; i < *id_to_last_.size(); ++i) {
    if(*id_to_last_[i].sn != NONE_SEQNUM && cur_time > *id_to_last_[i].exp) {
      *id_to_last_[i].sn = NONE_SEQNUM;
      clear_vertex(vertex(i, *topology_), *topology_);
    }
  }
}

void LinkStateControl::ComputePartitions() {
  if(did_remove_) {
    initialize_incremental_components(*topology_, ds_);
    incremental_components(*topology_, ds_);
    did_remove_ = false;
  }
}

Id LinkStateControl::LowestController(Id src) {
  Vertex v = vertex(src, *topology_);
  Id min_controller = numeric_limits<Id>::max();

  for(Id c = scheduler_.kSwitchCount;
      c < scheduler_.kSwitchCount + scheduler_.kControllerCount; ++c) {
    if(same_component(v, vertex(c, *topology_), ds_))
      min_controller = min(min_controller, c);
  }

  return min_controller;
}

SequenceNum LinkState::NextSeqNum() { return next_++; }

string LinkState::Description() const {
  string rtn = "";
  VertexIter ui,ui_end;
  OutEdgeIter ei, ei_end;
  property_map<Topology, vertex_index_t>::type v_index =
      get(vertex_index, *topology_);

  for (tie(ui,ui_end) = vertices(*topology_); ui != ui_end; ++ui) {
    tie(ei,ei_end) = out_edges(*ui, *topology_);
    if(ei != ei_end) {
      rtn += to_string(v_index[*ui]) + " <--> ";
      for(; ei != ei_end; ++ei)
        rtn += to_string(v_index[target(*ei, *topology_)]) + " ";
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

shared_ptr<vector<Id> > LinkStateControl::ComputeRoutingTable(Id src) {
  // TODO fix inlined size
  static vector<Vertex> pred_(11020);

  shared_ptr<vector<Id> > dst_to_neighbor = make_shared<vector<Id> >
      (scheduler_.kHostCount, DROP);

  // TODO will pred by initialized by bfs?
  for(int i = 0; i < pred_.size(); ++i)
    pred_[i] = i;

  // TODO use another shortest-path algorithm?
  breadth_first_search(*topology_, static_cast<Vertex>(src),
                       visitor(make_bfs_visitor(
                           record_predecessors(&pred_[0],
                                               on_tree_edge()))));

  VertexIter ui,ui_end;
  for(tie(ui, ui_end) = vertices(*topology_); ui != ui_end; ++ui)
    if(scheduler_.IsHost(*ui))
      (*dst_to_neighbor)[*ui - scheduler_.kSwitchCount - scheduler_.kControllerCount] =
          NextHop(src, *ui, pred_);

  return dst_to_neighbor;
}

// TODO pass back iterators?
vector<Id> LinkStateControl::SwitchesInParition(Id src) {
  vector<Id> switches;
  Vertex v = vertex(src, *topology_);

  for(Id s = 0; s < scheduler_.kSwitchCount; ++s)
    if(same_component(v, vertex(s, *topology_), ds_))
      switches.push_back(s);

  return switches;
}

LinkStateControl::LinkStateControl(Scheduler& s)
    : LinkState(s),
      rank_(num_vertices(*topology_)),
      parent_(num_vertices(*topology_)),
      ds_(&rank_[0], &parent_[0]),
      did_remove_(true) {
  initialize_incremental_components(*topology_, ds_);
}

void LinkStateControler::Update(ControllerView* cv) {
  vector<seen>& vs = *(cv->id_to_last_);

  for(Id s = 0; s < scheduler_.kSwitchCount; ++s) {
    if(vs[s].sn > id_to_last[s].sn) {
      clear_vertex(vertex(s, *topology_), *topology_);
      for(
      (*id_to_last_)[s] = vs[s];
    }
  }
}

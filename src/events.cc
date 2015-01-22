#include "entities.h"
#include "events.h"

#include <glog/logging.h>
#include "boost/tuple/tuple.hpp"

#define UNINITIALIZED_TIME -1

using std::array;
using std::string;
using std::to_string;
using std::shared_ptr;
using std::vector;

using boost::property_map;
using boost::vertex_index_t;
using boost::get;
using boost::vertex_index;
using boost::tie;

typedef boost::graph_traits<Topology>::out_edge_iterator OutEdgeIter;
typedef boost::graph_traits<Topology>::vertex_iterator VertexIter;

namespace std {
string to_string(const vector<int>& ints) {
  string rtn = "";

  for(int i : ints)
    rtn += to_string(i) + " ";

  return rtn;
}
string to_string(const array<Id, 13>& ints) {
  string rtn = "";

  for(int i : ints)
    rtn += to_string(i) + " ";

  return rtn;
}
};

Event::Event(Time t, Entity* e) : time_(t), affected_entities_({e}) {}

Event::Event(Time t, Entity* e1, Entity* e2) : time_(t),
                                               affected_entities_({e1, e2}) {}

Event::~Event() { return; }

void Event::Handle(Entity* e) {  e->Handle(this); }

string Event::Description() const {
  string ids = "";

  for(auto it = affected_entities_.begin(); it != affected_entities_.end(); ++it)
    ids += to_string((*it)->id()) + " ";

  return "time_=" + to_string(time_) + " affected_entities=" + ids;
}

string Event::Name() const { return "Event"; }

//unsigned int Event::size() const { return 0; }

unsigned int Event::size() const { return -1; }

Up::Up(Time t, Entity* e) : Event(t, e) {}

unsigned int Up::count_ = 0;

void Up::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string Up::Description() const { return Event::Description(); }

string Up::Name() const { return "Up"; }

Down::Down(Time t, Entity* e) : Event(t, e) {}

unsigned int Down::count_ = 0;

void Down::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string Down::Description() const { return Event::Description(); }

string Down::Name() const { return "Down"; }

LinkUp::LinkUp(Time t, Entity* e, Port p) : Event(t, e), out_(p) {}

unsigned int LinkUp::count_ = 0;

void LinkUp::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string LinkUp::Description() const {
  return Event::Description() + " out_=" + to_string(out_);
}

string LinkUp::Name() const { return "Link Up"; }

LinkDown::LinkDown(Time t, Entity* e, Port p) : Event(t, e), out_(p) {}

unsigned int LinkDown::count_ = 0;

void LinkDown::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string LinkDown::Description() const {
  return Event::Description() + " out_=" + to_string(out_);
}

string LinkDown::Name() const { return "Link Down"; }

Broadcast::Broadcast(Time t, Entity* affected_entity, Port in) :
    Event(t, affected_entity), in_port_(in) {}

void Broadcast::Handle(Entity* e) {  e->Handle(this); }

string Broadcast::Description() const {
  return Event::Description() + " in_port_=" + to_string(in_port_);
}

string Broadcast::Name() const { return "Broadcast"; }

//unsigned int Broadcast::size() const { return 20; }

unsigned int Broadcast::size() const { return -1; }

LinkStateUpdate::LinkStateUpdate(Time t, Entity* e, Port i, Entity* src,
                                 SequenceNum sn, Time expiration,
                                 array<Id, 13> up_links, Id src_id,
                                 bool is_from_lsr)
    : Broadcast(t, e, i), src_(src), sn_(sn), expiration_(expiration),
      up_links_(up_links), src_id_(src_id), is_from_lsr_(is_from_lsr) {}

unsigned int LinkStateUpdate::count_ = 0;

void LinkStateUpdate::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string LinkStateUpdate::Description() const {
   return Broadcast::Description()  +
       " sn_=" + to_string(sn_) +
       " src_id=" + to_string(src_id_) +
       " up_links_=" + to_string(up_links_);
       //       " expiration_=" + to_string(expiration_);
}

string LinkStateUpdate::Name() const { return "Link State Update"; }

// unsigned int LinkStateUpdate::size() const {
//   return Broadcast::size() + 4 + 6 + 13 * 6 + 8;
// }

unsigned int LinkStateUpdate::size() const { return 0; }

InitiateLinkState::InitiateLinkState(Time t, Entity* e) : Event(t, e) {}

unsigned int InitiateLinkState::count_ = 0;

void InitiateLinkState::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string InitiateLinkState::Description() const { return Event::Description(); }

string InitiateLinkState::Name() const { return "Initiate Link State Update"; }

RoutingUpdate::RoutingUpdate(Time t, Entity* e, Port in, Entity* src,
                             SequenceNum sn,
                             shared_ptr<vector<Id> > dst_to_neighbor,
                             Id dst, Id src_id) :
    Broadcast(t, e, in), sn_(sn), src_(src),
    dst_to_neighbor_(dst_to_neighbor), dst_(dst), src_id_(src_id) {}

RoutingUpdate::~RoutingUpdate() { dst_to_neighbor_ = nullptr; }

unsigned int RoutingUpdate::count_ = 0;

void RoutingUpdate::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string RoutingUpdate::Description() const {
  string rtn = Broadcast::Description() + " sn_=" + to_string(sn_) +
      " src_=" + to_string(src_->id()) + " dst_=" + to_string(dst_) +
      " dst_to_neighbor_=";

  if(dst_to_neighbor_) {
    for(int i : *dst_to_neighbor_)
      rtn += to_string(i) + " ";
  } else {
    rtn += "null";
  }

  if(dst_to_neighbor_) {
    Id* ids = dst_to_neighbor_->data();
    vector<Id>::size_type cap = dst_to_neighbor_->capacity();
    for(int i = 0; i < cap; ++i)
      LOG(INFO) << i;
  }

  return rtn;
}

string RoutingUpdate::Name() const { return "Routing Update"; }

// unsigned int RoutingUpdate::size() const {
//   //return Broadcast::size() + 4 + 6 +  2000*6 + 6;
//   return Broadcast::size() + 4 + 6 +  2*6 + 6;
// }

unsigned int RoutingUpdate::size() const { return 1; }

LinkStateRequest::LinkStateRequest(Time t, Entity* e, Port in, Entity* src,
                                   SequenceNum sn, Id id) :
    Broadcast(t, e, in), src_(src), sn_(sn), src_id_(id) {}

unsigned int LinkStateRequest::count_ = 0;

void LinkStateRequest::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string LinkStateRequest::Description() const {
  return Broadcast::Description()  +
      " sn_=" + to_string(sn_) +
      " src_=" + to_string(src_->id());
}

string LinkStateRequest::Name() const { return "Link State Request"; }

//unsigned int LinkStateRequest::size() const { return Broadcast::size() + 4 + 6; }

unsigned int LinkStateRequest::size() const { return 2; }

ControllerView::ControllerView(Time t, Entity* e, Port p, Entity* src,
                               SequenceNum sn, Id src_id,
                               std::shared_ptr<Topology> topology,
                               std::shared_ptr<std::vector<seen> > id_to_last)
    : Broadcast(t, e, p), src_(src), sn_(sn), src_id_(src_id),
      topology_(topology), id_to_last_(id_to_last) {}

unsigned int ControllerView::count_ = 0;

void ControllerView::Handle(Entity* e) {
  ++count_;
  e->Handle(this);
}

string ControllerView::Description() const {
  string id_to_last = "";
  for(auto p : *id_to_last_)
    id_to_last += to_string(p.sn);

  string topology = "";
  auto topo = *topology_;
  VertexIter ui,ui_end;
  OutEdgeIter ei, ei_end;
  property_map<Topology, vertex_index_t>::type v_index = get(vertex_index, topo);

  for (tie(ui,ui_end) = vertices(topo); ui != ui_end; ++ui) {
    tie(ei,ei_end) = out_edges(*ui, topo);
    if(ei != ei_end) {
      topology += to_string(v_index[*ui]) + " <--> ";
      for(; ei != ei_end; ++ei)
        topology += to_string(v_index[target(*ei, topo)]) + " ";
      if(ui + 1 != ui_end)
        topology += ",";
    }
  }

  return Broadcast::Description() +
      " sn_=" + to_string(sn_) +
      " src_id_=" + to_string(src_id_) +
      " topology_=" + topology +
      " id_to_last_=" + id_to_last;
}

string ControllerView::Name() const { return "Controller View"; }

unsigned int ControllerView::size() const { return 3; }

ControllerView::~ControllerView() {
  topology_ = nullptr;
  id_to_last_ = nullptr;
}

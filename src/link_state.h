#ifndef DDCSIM_LINK_STATE_H_
#define DDCSIM_LINK_STATE_H_

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <boost/pending/disjoint_sets.hpp>
#include <boost/graph/incremental_components.hpp>

#include "common.h"

class LinkStateUpdate;
class Scheduler;

class LinkState {
 public:
  LinkState(Scheduler&);
  std::string Description() const;
  bool IsStaleUpdate(LinkStateUpdate*) const;
  bool HaveNewUpdate(LinkStateUpdate*) const;
  virtual bool Update(LinkStateUpdate*);
  void Refresh(Time);
  SequenceNum NextSeqNum();
  LinkStateUpdate CurrentLinkState(Entity*, Id);

 protected:
  typedef struct { SequenceNum sn; Time exp; } pair;
  Topology topology_;
  Scheduler& scheduler_;
  SequenceNum next_;
  std::vector<pair> id_to_last_;
  DISALLOW_COPY_AND_ASSIGN(LinkState);
};

class LinkStateControl : public LinkState {
 public:
  LinkStateControl(Scheduler&);
  void ComputePartitions();
  bool ArePartitioned(Id, Id);
  bool HealsPartition(Id, LinkStateUpdate*);
  Id LowestController(Id);
  std::shared_ptr<std::vector<Id> > ComputeRoutingTable(Id);
  std::vector<Id> SwitchesInParition(Id);
  virtual bool Update(LinkStateUpdate*);

 private:
  Id NextHop(Id, Id, std::vector<boost::graph_traits<Topology>::vertex_descriptor>&);
  std::vector<VertexIndex> rank_;
  std::vector<Vertex> parent_;
  boost::disjoint_sets<VertexIndex*, Vertex*> ds_;
  bool did_remove_;
  DISALLOW_COPY_AND_ASSIGN(LinkStateControl);
};

#endif

#ifndef DDCSIM_LINK_STATE_H_
#define DDCSIM_LINK_STATE_H_

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "common.h"

class LinkStateUpdate;
class Scheduler;

class LinkState {
 public:
  LinkState(Scheduler&);
  std::string Description() const;
  bool IsStaleUpdate(LinkStateUpdate*) const;
  bool Update(LinkStateUpdate*);
  void Refresh(Time);
  SequenceNum NextSeqNum();

 protected:
  Topology topology_;
  Scheduler& scheduler_;
  SequenceNum next_;
  std::vector<SequenceNum> id_to_last_sn_;
  std::vector<Time> id_to_exp_;
  DISALLOW_COPY_AND_ASSIGN(LinkState);
};

class LinkStateControl : public LinkState {
 public:
  LinkStateControl(Scheduler&);
  void ComputePartitions();
  bool ArePartitioned(Id, Id) const;
  Id LowestController(Id) const;
  std::shared_ptr<std::vector<Id> > ComputeRoutingTable(Id);
  std::vector<Id> SwitchesInParition(Id);
  bool Update(LinkStateUpdate*);

 private:
  void InitComponents();
  Id NextHop(Id, Id, std::vector<boost::graph_traits<Topology>::vertex_descriptor>&);
  std::vector<int> id_to_component_;
  std::vector<Vertex> pred_;
  DISALLOW_COPY_AND_ASSIGN(LinkStateControl);
};

#endif

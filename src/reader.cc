#include <string>
#include <vector>

#include "common.h"
#include "scheduler.h"
#include "reader.h"
#include "entities.h"
#include "events.h"
#include "statistics.h"

#include "boost/tuple/tuple.hpp"

using std::string;
using std::unordered_map;
using std::vector;

using boost::tie;
using boost::add_edge;
using boost::add_vertex;

using namespace YAML;

Reader::Reader(string topo_file_path, string event_file_path, Size bucket_capacity,
               Rate fill_rate, Statistics& stats, Scheduler& sched,
               vector<Switch*>& switches, vector<Controller*>& controllers,
               vector<Host*>& hosts, Topology& physical, unordered_map<Id, Entity*>&
               id_to_entity)
    : topo_file_path_(topo_file_path), event_file_path_(event_file_path),
      bucket_capacity_(bucket_capacity), fill_rate_(fill_rate), statistics_(stats),
      scheduler_(sched), switches_(switches), controllers_(controllers),
  hosts_(hosts), id_to_entity_(id_to_entity), physical_(physical) {}

bool Reader::IsGenericEntity(Node n) {
  return !n["type"].as<string>().compare("entity");
}

bool Reader::IsSwitch(Node n) {
  return !n["type"].as<string>().compare("switch");
}

bool Reader::IsController(Node n) {
  return !n["type"].as<string>().compare("controller");
}

bool Reader::IsHost(Node n) {
  return !n["type"].as<string>().compare("host");
}

bool Reader::ParseEntities(Node raw_entities) {
  Controller* c;
  Switch* s;
  Host* h;
  Id id;

  for(Node n : raw_entities) {
    id = n["id"].as<Id>();
    if(IsController(n)) {
      c = new Controller(scheduler_, id, statistics_);
      controllers_.push_back(c);
      id_to_entity_.insert({id, c});
    } else if(IsSwitch(n)) {
      s = new Switch(scheduler_, id, statistics_);
      switches_.push_back(s);
      id_to_entity_.insert({id, s});
    } else if(IsHost(n)) {
      h = new Host(scheduler_, id, statistics_);
      hosts_.push_back(h);
      id_to_entity_.insert({id, h});
    } else if(IsGenericEntity(n)) {
      LOG(ERROR) << "Construction of generic entities is disallowed";
      return false;
    } else {
      LOG(ERROR) << "Iterated over unrecognizable entity type";
      return false;
    }
    add_vertex(physical_);
  }

  return true;
}

bool Reader::ParseLinks(Node&& raw_links) {
  Id src_id;
  Entity* src_ent;
  vector<Id> dst_ids;

  for(auto it = raw_links.begin(); it != raw_links.end(); ++it) {
    src_id = it->first.as<Id>();
    src_ent = id_to_entity_[src_id];
    dst_ids = it->second.as<std::vector<Id>>();
    // TODO this is verbose.  Could use a functional map...

    vector<Entity*> dst_ents;
    Edge e;
    bool added;
    Vertex src, dst;

    src = vertex(src_id, physical_);
    for(auto jt = dst_ids.begin(); jt != dst_ids.end(); ++jt) {
      dst_ents.push_back(id_to_entity_[*jt]);
      dst = vertex(*jt, physical_);

      if(! edge(src, dst, physical_).second) {
        tie(e, added) = add_edge(src, dst, physical_);
        CHECK(added);
      }
    }

    src_ent->InitLinks(dst_ents.begin(), dst_ents.end(),
                       bucket_capacity_, fill_rate_);
  }
  return true;
}

bool Reader::ParseTopology() {
  Node raw_topo(LoadFile(topo_file_path_));

  if(!raw_topo.IsMap()) {
    LOG(FATAL) << "Expected the top level structure of topology file to be a map";
    return false;
  }

  bool valid_entities = ParseEntities(raw_topo["entities"]);

  if(!valid_entities) return false;

  bool valid_links = ParseLinks(raw_topo["links"]);

  if(!valid_links) return false;

  return true;
}

bool Reader::IsUp(Node n) {
  return !n["type"].as<string>().compare("up");
}

bool Reader::IsDown(Node n) {
  return !n["type"].as<string>().compare("down");
}

bool Reader::IsLinkUp(Node n) {
  return !n["type"].as<string>().compare("linkup");
}

bool Reader::IsLinkDown(Node n) {
  return !n["type"].as<string>().compare("linkdown");
}

bool Reader::IsGenericEvent(Node n) {
  return !n["type"].as<string>().compare("event");
}

bool Reader::IsInitiateHeartbeat(Node n) {
  return !n["type"].as<string>().compare("initiateheartbeat");
}

bool Reader::IsBroadcast(Node n) {
  return !n["type"].as<string>().compare("broadcast");
}

bool Reader::IsHeartbeat(Node n) {
  return !n["type"].as<string>().compare("heartbeat");
}

bool Reader::IsLinkStateUpdate(Node n) {
  return !n["type"].as<string>().compare("linkstateupdate");
}

bool Reader::IsInitiateLinkState(Node n) {
  return !n["type"].as<string>().compare("initiatelinkstate");
}

bool Reader::ParseEvents() {
  // TODO verify there are no double down's/up's or at least log
  if(event_file_path_ == NO_EVENT_FILE) return true;

  Node raw_events(LoadFile(event_file_path_));

  Time t;
  Node ev;
  Id affected_id;

  for(auto it = raw_events.begin(); it != raw_events.end(); ++it) {
    t = it->first.as<Time>();
    ev = it->second;

    if(IsUp(ev)) {
      affected_id = it->second["id"].as<Id>();
      scheduler_.AddEvent(t, new Up(t, id_to_entity_[affected_id]));
    } else if(IsDown(ev)) {
      affected_id = it->second["id"].as<Id>();
      scheduler_.AddEvent(t, new Down(t, id_to_entity_[affected_id]));
    } else if(IsLinkUp(ev)) {
      affected_id = it->second["src_id"].as<Id>();
      Entity* src = id_to_entity_[affected_id];
      Entity* dst = id_to_entity_[it->second["dst_id"].as<Id>()];
      Port p = src->links().GetPortTo(dst);
      CHECK_NE(p, PORT_NOT_FOUND);
      scheduler_.AddEvent(t, new LinkUp(t, src, p));
    } else if(IsLinkDown(ev)) {
      affected_id = it->second["src_id"].as<Id>();
      Entity* src = id_to_entity_[affected_id];
      Entity* dst = id_to_entity_[it->second["dst_id"].as<Id>()];
      Port p = src->links().GetPortTo(dst);
      CHECK_NE(p, PORT_NOT_FOUND);
      scheduler_.AddEvent(t, new LinkDown(t, src, p));
    } else if(IsGenericEvent(ev)) {
      LOG(ERROR) << "Construction of generic events is disallowed";
      return false;
    } else if(IsInitiateHeartbeat(ev)) {
      LOG(ERROR) << "Not supported currently";
      return false;
    } else if(IsBroadcast(ev)) {
      LOG(ERROR) << "Construction of generic broadcasts is disallowed";
      return false;
    } else if(IsHeartbeat(ev)) {
      LOG(ERROR) << "To explicitly initiate a heartbeat, "
          "use the InitiateHeartbeat event";
      return false;
    } else if(IsLinkStateUpdate(ev)) {
      LOG(ERROR) << "To explicitly initiate a link state update, "
          "use the InitiateLinkState event";
      return false;
    } else if(IsInitiateLinkState(ev)) {
      LOG(ERROR) << "Not supported currently";
      return false;
    } else {
      LOG(ERROR) << "Iterated over unrecognizable event type";
      return false;
    }
  }

  return true;
}

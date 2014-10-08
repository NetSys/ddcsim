#include <string>
#include <utility>
#include <vector>

#include <glog/logging.h>

#include "scheduler.h"
#include "reader.h"
#include "entities.h"
#include "events.h"

using std::string;
using std::vector;
using namespace YAML;

// TODO complicated initialization on allowed in constructor?
// TODO is yaml constructor considered complicated?
Reader::Reader(std::string topo_file_path, std::string event_file_path,
               Scheduler& s) : topo_file_path_(topo_file_path),
                               event_file_path_(event_file_path),
                               scheduler_(s), id_to_entity_(),
                               num_entities_(0) {}

bool Reader::IsGenericEntity(Node n) {
  return !n["type"].as<string>().compare("entity");
}

bool Reader::IsSwitch(Node n) {
  return !n["type"].as<string>().compare("switch");
}

bool Reader::IsController(Node n) {
  return !n["type"].as<string>().compare("controller");
}

bool Reader::ParseEntities(Node raw_entities) {
  // TODO error handling
  // TODO hoist raw_entities.end out of loop?
  for(auto it = raw_entities.begin(); it != raw_entities.end(); ++it) {
    Node n = *it;
    Id id = n["id"].as<Id>();

    if(IsController(n)) {
      id_to_entity_.insert({id, new Controller(scheduler_, id)});
    } else if(IsSwitch(n)) {
      id_to_entity_.insert({id, new Switch(scheduler_, id)});
    } else if(IsGenericEntity(n)) {
      LOG(ERROR) << "Construction of generic entities is disallowed";
      return false;
    } else {
      LOG(ERROR) << "Iterated over unrecognizable entity type";
      return false;
    }

    num_entities_++;
  }

  return true;
}

bool Reader::ParseLinks(Node&& raw_links, Size bucket_capacity,
                        Rate drain_rate) {
  Id src_id;
  Entity* src_ent;
  vector<Id> dst_ids;

  // TODO error handling
  for(auto it = raw_links.begin(); it != raw_links.end(); ++it) {
    src_id = it->first.as<Id>();
    src_ent = id_to_entity_[src_id];
    dst_ids = it->second.as<std::vector<Id>>();
    // TODO this is verbose.  Could use a functional map...
    vector<Entity*> dst_ents;
    for(auto jt = dst_ids.begin(); jt != dst_ids.end(); ++jt) {
      dst_ents.push_back(id_to_entity_[*jt]);
    }
    src_ent->InitLinks(dst_ents.begin(), dst_ents.end(),
                       bucket_capacity, drain_rate);
  }
  return true;
}

bool Reader::ParseTopology(Size bucket_capacity, Rate drain_rate) {
  Node raw_topo(LoadFile(topo_file_path_));

  if(!raw_topo.IsMap()) {
    LOG(FATAL) << "Expected the top level structure of topology file to be a map";
    return false;
  }

  // TODO error handling
  bool valid_entities = ParseEntities(raw_topo["entities"]);

  if(!valid_entities) return false;

  bool valid_links = ParseLinks(raw_topo["links"], bucket_capacity, drain_rate);

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

bool Reader::IsLinkAlert(Node n) {
  return !n["type"].as<string>().compare("linkalert");
}

bool Reader::ParseEvents() {
  // TODO verify there are no double down's/up's or at least log
  if(event_file_path_ == NO_EVENT_FILE) return true;

  Node raw_events(LoadFile(event_file_path_));

  Time t;
  Node ev;
  Id affected_id;

  // TODO error handling
  // TODO change to take list of affected entities
  for(auto it = raw_events.begin(); it != raw_events.end(); ++it) {
    t = it->first.as<Time>();
    ev = it->second;

    if(IsUp(ev)) {
      affected_id = it->second["id"].as<Id>();
      // TODO how to cleanly remove static cast
      scheduler_.AddEvent(new Up(t,
                                 static_cast<Switch*>
                                 (id_to_entity_[affected_id])));
    } else if(IsDown(ev)) {
      affected_id = it->second["id"].as<Id>();
      scheduler_.AddEvent(new Down(t,
                                   static_cast<Switch*>
                                   (id_to_entity_[affected_id])));
    } else if(IsLinkUp(ev)) {
      affected_id = it->second["src_id"].as<Id>();
      Entity* src = id_to_entity_[affected_id];
      Entity* dst = id_to_entity_[it->second["dst_id"].as<Id>()];
      Port p = src->links().GetPortTo(dst);
      assert(p != PORT_NOT_FOUND);
      scheduler_.AddEvent(new LinkUp(t, src, p));
    } else if(IsLinkDown(ev)) {
      affected_id = it->second["src_id"].as<Id>();
      Entity* src = id_to_entity_[affected_id];
      Entity* dst = id_to_entity_[it->second["dst_id"].as<Id>()];
      Port p = src->links().GetPortTo(dst);
      assert(p != PORT_NOT_FOUND);
      scheduler_.AddEvent(new LinkDown(t, src, p));
    } else if(IsGenericEvent(ev)) {
      LOG(ERROR) << "Construction of generic events is disallowed";
      return false;
    } else if(IsInitiateHeartbeat(ev)) {
      // TODO
    } else if(IsBroadcast(ev)) {
      LOG(ERROR) << "Construction of generic broadcasts is disallowed";
      return false;
    } else if(IsHeartbeat(ev)) {
      LOG(ERROR) << "To explicitly initiate a heartbeat, "
          "use the InitiateHeartbeat event";
      return false;
    } else if(IsLinkAlert(ev)) {
      LOG(ERROR) << "Expliciti construction of link up/down events is disallowed"
          ".  To inject link up/down events, use the events input.";
      return false;
    } else {
      LOG(ERROR) << "Iterated over unrecognizable event type";
      return false;
    }
  }

  return true;
}

std::unordered_map<Id, Entity*>& Reader::id_to_entity() {
  return id_to_entity_;
}

int Reader::num_entities() { return num_entities_; }

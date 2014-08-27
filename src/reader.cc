#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "scheduler.h"
#include "reader.h"
#include "entities.h"
#include "events.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;
/*
using YAML::Node;
using YAML::LoadFile;
using YAML::as;
*/
using namespace YAML;
// TODO complicated initialization on allowed in constructor?
// TODO is yaml constructor considered complicated?
Reader::Reader(std::string topo_file_path, std::string event_file_path,
               Scheduler& s) : topo_file_path_(topo_file_path),
                               event_file_path_(event_file_path),
                               scheduler_(s), id_to_entity_() {}

bool Reader::IsEntity(Node n) {
  return false; //TODO
}

bool Reader::IsSwitch(Node n) {
  return false; //TODO
}

bool Reader::IsBroadcastSwitch(Node n) {
  return true; //TODO
}

bool Reader::ParseEntities(Node raw_entities) {
  Id id;
  Node typ;

  // TODO error handling
  // TODO hoist raw_entities.end out of loop?
  for(auto it = raw_entities.begin(); it != raw_entities.end(); ++it) {

    typ = ((*it)["type"]);
    id = ((*it)["id"]).as<Id>();

    if(IsBroadcastSwitch(typ)) {
      id_to_entity_.insert({id, new BroadcastSwitch(scheduler_, id)});
    } else if(IsSwitch(typ)) {
      id_to_entity_.insert({id, new Switch(scheduler_, id)});
    } else if(IsEntity(typ)) {
      id_to_entity_.insert({id, new Entity(scheduler_, id)});
    } else {
      /* Error out quickly */
      cout << "[error]Reader::ParseEntities: Iterated over unrecognizable entity";
      cout << " type" << endl;
      return false;
    }
  }

  return true;
}


bool Reader::ParseLinks(Node raw_links) {
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
    src_ent->InitLinks(dst_ents.begin(), dst_ents.end());
  }
  return true;
}

bool Reader::ParseTopology() {
  Node raw_topo(LoadFile(topo_file_path_));

  if(!raw_topo.IsMap()) {
    cout << "[error]Reader::ParseTopology: Expected the top level structure";
    cout << " of topology file to be a map" << endl;
    return false;
  }

  // TODO error handling
  bool valid_entities = ParseEntities(raw_topo["entities"]);

  if(!valid_entities) return false;

  bool valid_links = ParseLinks(raw_topo["links"]);

  if(!valid_links) return false;

  return true;
}

bool Reader::IsSwitchUp(YAML::Node n) {
  return !n["type"].as<string>().compare("switchup");
}

bool Reader::IsSwitchDown(YAML::Node n) {
  return !n["type"].as<string>().compare("switchdown");
}

bool Reader::IsLinkUp(YAML::Node n) {
  return false; // TODO
}

bool Reader::IsLinkDown(YAML::Node n) {
  return false; //TODO
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

    if(IsSwitchUp(ev)) {
      affected_id = it->second["id"].as<Id>();
      // TODO how to cleanly remove static cast
      scheduler_.AddEvent(new SwitchUp(t,
                                       static_cast<Switch*>
                                       (id_to_entity_[affected_id])));
    } else if(IsSwitchDown(ev)) {
      affected_id = it->second["id"].as<Id>();
      scheduler_.AddEvent(new SwitchDown(t,
                                         static_cast<Switch*>
                                         (id_to_entity_[affected_id])));
    } else if(IsLinkUp(ev)) {
    } else if(IsLinkDown(ev)) {
    } else {
      /* Error out quickly */
      cout << "[error]Reader::ParseEvents: Iterated over unrecognizable event";
      cout << " type" << endl;
      return false;
    }
  }

  return true;
}

std::unordered_map<Id, Entity*>& Reader::id_to_entity() {
  return id_to_entity_;
}

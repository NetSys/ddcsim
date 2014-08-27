#ifndef DDCSIM_READER_H_
#define DDCSIM_READER_H_

// TODO how does the compiler know where to find yaml-cpp/?
#include "yaml-cpp/yaml.h"

#include <string>
#include <unordered_map>

#include "common.h"

#define NO_EVENT_FILE ""

class Entity;
class Scheduler;

class Reader {
public:
  Reader(std::string, std::string, Scheduler&);
  bool ParseTopology();
  bool ParseEvents();
  // TODO take out type of iterator
  // TODO just make id_to_entity_ public?
  std::unordered_map<Id, Entity*>& id_to_entity();

private:
  bool IsEntity(YAML::Node);
  bool IsSwitch(YAML::Node);
  bool IsBroadcastSwitch(YAML::Node);
  bool ParseEntities(YAML::Node); // TODO why isn't ref allowed?
  bool IsSwitchUp(YAML::Node);
  bool IsSwitchDown(YAML::Node);
  bool IsLinkUp(YAML::Node);
  bool IsLinkDown(YAML::Node);
  bool ParseLinks(YAML::Node);
  std::string topo_file_path_;
  std::string event_file_path_;
  Scheduler& scheduler_;
  std::unordered_map<Id, Entity*> id_to_entity_;
  DISALLOW_COPY_AND_ASSIGN(Reader);
};

#endif
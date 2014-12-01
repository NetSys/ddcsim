#ifndef DDCSIM_READER_H_
#define DDCSIM_READER_H_

#include "yaml-cpp/yaml.h"

#include <string>
#include <unordered_map>

#include "common.h"

#define NO_EVENT_FILE ""

class Entity;
class Scheduler;
class Statistics;

class Reader {
public:
  Reader(std::string, std::string, Scheduler&);
  bool ParseTopology(Size, Rate, Statistics&);
  bool ParseEvents();
  // TODO take out type of iterator
  // TODO just make id_to_entity_ public?
  std::unordered_map<Id, Entity*>& id_to_entity();

private:
  bool IsGenericEntity(YAML::Node);
  bool IsSwitch(YAML::Node);
  bool IsController(YAML::Node);
  bool ParseEntities(YAML::Node, Statistics&); // TODO why isn't ref allowed?
  bool IsUp(YAML::Node);
  bool IsDown(YAML::Node);
  bool IsLinkUp(YAML::Node);
  bool IsLinkDown(YAML::Node);
  bool IsGenericEvent(YAML::Node);
  bool IsInitiateHeartbeat(YAML::Node);
  bool IsBroadcast(YAML::Node);
  bool IsHeartbeat(YAML::Node);
  bool ParseLinks(YAML::Node&&, Size, Rate);
  std::string topo_file_path_;
  std::string event_file_path_;
  Scheduler& scheduler_;
  std::unordered_map<Id, Entity*> id_to_entity_;
  DISALLOW_COPY_AND_ASSIGN(Reader);
};

#endif

#ifndef DDCSIM_READER_H_
#define DDCSIM_READER_H_

#include "yaml-cpp/yaml.h"

#include <string>
#include <unordered_map>

#include "common.h"

#define NO_EVENT_FILE ""

class Entity;
class Switch;
class Controller;
class Host;
class Scheduler;
class Statistics;

// TODO better error handling in parsing

class Reader {
public:
  Reader(std::string, std::string, Size, Rate, Statistics&, Scheduler&,
         std::vector<Switch*>&, std::vector<Controller*>&, std::vector<Host*>&,
         Topology&, std::unordered_map<Id, Entity*>&);
  bool ParseTopology();
  bool ParseEvents();

private:
  bool IsGenericEntity(YAML::Node);
  bool IsSwitch(YAML::Node);
  bool IsHost(YAML::Node);
  bool IsController(YAML::Node);
  bool IsUp(YAML::Node);
  bool IsDown(YAML::Node);
  bool IsLinkUp(YAML::Node);
  bool IsLinkDown(YAML::Node);
  bool IsGenericEvent(YAML::Node);
  bool IsInitiateHeartbeat(YAML::Node);
  bool IsBroadcast(YAML::Node);
  bool IsHeartbeat(YAML::Node);
  bool IsLinkStateUpdate(YAML::Node);
  bool IsInitiateLinkState(YAML::Node);
  bool ParseEntities(YAML::Node);
  bool ParseLinks(YAML::Node&&);
  std::string topo_file_path_;
  std::string event_file_path_;
  Scheduler& scheduler_;
  Statistics& statistics_;
  Size bucket_capacity_;
  Rate fill_rate_;
  std::vector<Switch*>& switches_;
  std::vector<Controller*>& controllers_;
  std::vector<Host*>& hosts_;
  Topology& physical_;
  std::unordered_map<Id, Entity*>& id_to_entity_;
  DISALLOW_COPY_AND_ASSIGN(Reader);
};

#endif

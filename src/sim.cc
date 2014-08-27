#include <boost/program_options.hpp>

#include <string>
#include <iostream>

#include "common.h"
#include "scheduler.h"
#include "reader.h"
#include "entities.h"
#include "events.h"

using std::string;
using std::cout;
using std::endl;
using std::unordered_map;
namespace po = boost::program_options;
using po::options_description;
using po::value;
using po::positional_options_description;
using po::variables_map;
using po::store;
using po::command_line_parser;
using po::notify;

// TODO does Google have special conventions for out params?
bool ParseArgs(int ac, char* av[], string& topo_file_path,
               string& event_file_path, Time& heartbeat_period,
               Time& end_time) {
  options_description desc("Allowed options");
  desc.add_options()
      ("help",
       "produce help message")
      ("topo,o",
       value<string>(&topo_file_path),
       "path to the YAML file describing the network topology")
      ("events,e",
       value<string>(&event_file_path)->default_value(NO_EVENT_FILE),
       "path to the YAML file describing events to inject into the simulation")
      ("hearbeat-period,h",
       value<Time>(&heartbeat_period)->default_value(
           Scheduler::kDefaultHeartbeatPeriod),
       "emit a heartbeat every heartbeat-period seconds")
      ("end-time,t",
       value<Time>(&end_time)->default_value(Scheduler::kDefaultEndTime),
       "stop the simulation after end-time seconds have passed");

  positional_options_description p;
  p.add("topo", 1);

  variables_map vm;
  store(command_line_parser(ac, av).options(desc).positional(p).run(),
            vm);
  notify(vm);

  if (!vm.count("topo")) {
    cout << "[error]sim.cc::ParseArgs: Path to file containing ";
    cout << "network topology not specified" << endl;
    return false;
  }

  return true;
}

void DeleteEntities(unordered_map<Id, Entity*>& id_to_entity) {
  for(auto it = id_to_entity.begin(); it != id_to_entity.end(); ++it)
    free(it->second);
}

int main(int ac, char* av[]) {
  string topo_file_path;
  string event_file_path;
  Time heartbeat_period;
  Time end_time;

  bool valid_args = ParseArgs(ac, av, topo_file_path, event_file_path,
                              heartbeat_period, end_time);

  if(!valid_args) return -1;

  Scheduler sched(end_time);

  Reader in(topo_file_path, event_file_path, sched);

  bool valid_topology = in.ParseTopology();

  if(!valid_topology) return -1;

  // TODO check that entities and links are correct by implementing print
  // functions for them
  bool valid_events = in.ParseEvents();

  if(!valid_events) return -1;

  // TODO take into account routers and links going down

  // TODO this is fucking disgusting and it will be removed...
  for (Time t = 0; t < sched.end_time(); t+= heartbeat_period) {
    for (auto it = in.id_to_entity().begin(); it != in.id_to_entity().end();
         ++it) {
      sched.AddEvent(new Broadcast(t,
                                   static_cast<BroadcastSwitch*>(it->second),
                                   it->second,
                                   INITIATING_EVENT,
                                   t / heartbeat_period));
    }
  }

  sched.StartSimulation();

  // TODO log remaining events in scheduler for post-morterm inspection

  DeleteEntities(in.id_to_entity());

  return 0;
}
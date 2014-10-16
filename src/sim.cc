#include <boost/program_options.hpp>
#include <glog/logging.h>

#include <string>

#include "common.h"
#include "entities.h"
#include "events.h"
#include "reader.h"
#include "scheduler.h"
#include "statistics.h"

using std::string;
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
               Time& end_time, int& max_entities,
               Size& bucket_capacity, Rate& fill_rate) {
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
       "stop the simulation after end-time seconds have passed")
      ("max-entities,m",
       value<int>(&max_entities)->default_value(Scheduler::kNoMaxEntities),
       "the maximum number of entities that can exist at any point")
      ("bucket-capacity,M",
       value<Size>(&bucket_capacity)->default_value(BandwidthMeter::kDefaultCapacity),
       "the size of the bucket in the token bucket scheme (in units of bytes)")
      ("fill-rate,R",
       value<Rate>(&fill_rate)->default_value(BandwidthMeter::kDefaultRate),
       "the rate at which the token bucket fills up (in units of bytes/sec)");

  // TODO better names for the variables R and M
  // TODO add an uncapped option

  positional_options_description p;
  p.add("topo", 1);

  variables_map vm;
  store(command_line_parser(ac, av).options(desc).positional(p).run(),
            vm);
  notify(vm);

  if (!vm.count("topo")) {
    LOG(FATAL) << "Path to file containing network topology ";
    return false;
  }

  return true;
}

void DeleteEntities(unordered_map<Id, Entity*>& id_to_entity) {
  for(auto it = id_to_entity.begin(); it != id_to_entity.end(); ++it)
    delete it->second;
}

void InitLogging(const char* argv0) {
  google::InitGoogleLogging(argv0);
  FLAGS_logtostderr = true;
  FLAGS_log_prefix = false;
}

int main(int ac, char* av[]) {
  string topo_file_path;
  string event_file_path;
  Time heartbeat_period;
  Time end_time;
  int max_entities;
  Size bucket_capacity;
  Rate fill_rate;

  InitLogging(av[0]);

  bool valid_args = ParseArgs(ac, av, topo_file_path, event_file_path,
                              heartbeat_period, end_time, max_entities,
                              bucket_capacity, fill_rate);

  if(!valid_args) return -1;

  Scheduler sched(end_time);

  Reader in(topo_file_path, event_file_path, sched);

  Statistics stats(sched);

  bool valid_topology = in.ParseTopology(bucket_capacity, fill_rate, stats);

  if(!valid_topology) return -1;

  sched.kMaxEntities =
      max_entities == Scheduler::kNoMaxEntities ?
      in.num_entities() : max_entities;

  // TODO check that entities and links are correct by implementing print
  // functions for them
  bool valid_events = in.ParseEvents();

  if(!valid_events) return -1;

  // TODO this will be generalized
  for (Time t = 0; t < sched.end_time(); t+= heartbeat_period)
    for (auto it = in.id_to_entity().begin(); it != in.id_to_entity().end();
         ++it)
      sched.AddEvent(new InitiateHeartbeat(t, it->second));

  stats.Init();

  sched.StartSimulation(in.id_to_entity());

  // TODO log remaining events in scheduler for post-morterm inspection

  DeleteEntities(in.id_to_entity());

  return 0;
}

#include <boost/program_options.hpp>
#include <glog/logging.h>

#include <iostream>
#include <string>

#include "common.h"
#include "entities.h"
#include "events.h"
#include "reader.h"
#include "scheduler.h"
#include "statistics.h"

using std::cerr;
using std::endl;
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
               Time& ls_update_period, Time& end_time, unsigned int& num_entities,
               Size& bucket_capacity, Rate& fill_rate, string& out_prefix) {
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
      ("ls-update-period,l",
       value<Time>(&ls_update_period)->default_value(
           Scheduler::kDefaultLSUpdatePeriod),
       "flood link state information every ls-update-period seconds")
      ("end-time,t",
       value<Time>(&end_time)->default_value(Scheduler::kDefaultEndTime),
       "stop the simulation after end-time seconds have passed")
      ("num-entities,n",
       value<unsigned int>(&num_entities),
       "the maximum number of entities that can exist at any point")
      ("bucket-capacity,M",
       value<Size>(&bucket_capacity)->default_value(BandwidthMeter::kDefaultCapacity),
       "the size of the bucket in the token bucket scheme (in units of bytes)")
      ("fill-rate,R",
       value<Rate>(&fill_rate)->default_value(BandwidthMeter::kDefaultRate),
       "the rate at which the token bucket fills up (in units of bytes/sec)")
      ("out-prefix,O",
       value<string>(&out_prefix)->default_value("./"),
       "directory to put out files");

  // TODO better names for the variables R and M
  // TODO add an uncapped option

  positional_options_description p;
  p.add("topo", 1);

  variables_map vm;
  store(command_line_parser(ac, av).options(desc).positional(p).run(),
	vm);
  notify(vm);

  if(!vm.count("help")) {
    if (!vm.count("topo")) {
      cerr << "Path to file containing network topology missing" << endl;
      return false;
    }

    if(!vm.count("num-entities")) {
      cerr << "The number of entities in the simulation needs to be specified" << endl;
      return false;
    }

  }

  return true;
}

void InitLogging(const char* argv0, string out_prefix) {
  google::InitGoogleLogging(argv0);
  FLAGS_stderrthreshold = 1;
  FLAGS_log_dir = out_prefix;
  FLAGS_log_prefix = false;
  FLAGS_minloglevel = 0;
  FLAGS_logbuflevel = 0;
}

int main(int ac, char* av[]) {
  string topo_file_path, event_file_path, out_prefix;
  Time heartbeat_period, ls_update_period, end_time;
  unsigned int num_entities;
  Size bucket_capacity;
  Rate fill_rate;

  bool valid_args = ParseArgs(ac, av, topo_file_path, event_file_path,
                              heartbeat_period, ls_update_period, end_time,
                              num_entities, bucket_capacity, fill_rate,
                              out_prefix);

  if(!valid_args) return -1;

  InitLogging(av[0], out_prefix);

  Scheduler sched(end_time, num_entities);

  Statistics stats(sched);

  Reader in(topo_file_path, event_file_path, sched);

  bool valid_topology = in.ParseTopology(bucket_capacity, fill_rate, stats);

  if(!valid_topology) return -1;

  // TODO check that entities and links are correct by implementing print
  // functions for them
  bool valid_events = in.ParseEvents();

  if(!valid_events) return -1;

  sched.SchedulePeriodicEvents(in.id_to_entity(),
                               heartbeat_period,
                               ls_update_period);

  stats.Init(out_prefix, in.physical_topo());

  sched.StartSimulation(in.id_to_entity());

  return 0;
}

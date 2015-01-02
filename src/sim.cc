#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

#include "common.h"
#include "entities.h"
#include "events.h"
#include "reader.h"
#include "scheduler.h"
#include "statistics.h"
#include "links.h"

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::unordered_map;

namespace po = boost::program_options;
using po::options_description;
using po::value;
using po::positional_options_description;
using po::variables_map;
using po::store;
using po::command_line_parser;
using po::notify;

using std::to_string;

// TODO does Google have special conventions for out params?
bool ParseArgs(int ac, char* av[], string& topo_file_path,
               string& event_file_path, Time& heartbeat_period,
               Time& ls_update_period, Time& end_time, Size& bucket_capacity,
               Rate& fill_rate, string& out_prefix,
               size_t& switch_count, size_t& host_count, size_t& controller_count) {
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
      // ("bucket-capacity,M",
      //  value<Size>(&bucket_capacity)->default_value(BandwidthMeter::kDefaultCapacity),
      //  "the size of the bucket in the token bucket scheme (in units of bytes)")
      // ("fill-rate,R",
      //  value<Rate>(&fill_rate)->default_value(BandwidthMeter::kDefaultRate),
      //  "the rate at which the token bucket fills up (in units of bytes/sec)")
      ("out-prefix,O",
       value<string>(&out_prefix)->default_value("./"),
       "directory to put out files")
      ("switch-count,S", value<size_t>(&switch_count), "number of switches")
      ("host-count,H", value<size_t>(&host_count), "number of hosts")
      ("controller-count,C", value<size_t>(&controller_count), "number of controllers");

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
    if (!vm.count("switch-count")) {
      cerr << "Switch count missing" << endl;
      return false;
    }
    if (!vm.count("controller-count")) {
      cerr << "Conotroller count missing" << endl;
      return false;
    }
    if (!vm.count("host-count")) {
      cerr << "Host count missing" << endl;
      return false;
    }
  }

  return true;
}

void InitLogging(const char* argv0, string out_prefix) {
  google::InitGoogleLogging(argv0);
  FLAGS_stderrthreshold = 1;
  FLAGS_log_dir = out_prefix;
#ifdef NDEBUG
  FLAGS_log_prefix = true;
#else
  FLAGS_log_prefix = false;
#endif
  FLAGS_logbuflevel = 0;
}
bool CreateWorld(string topo_file_path, string event_file_path,
                 Size bucket_capacity, Rate fill_rate,
                 Statistics& stats, Scheduler& sched, vector<Switch*>& switches,
                 vector<Controller*>& controllers, vector<Host*>& hosts,
                 Topology& physical, vector<Entity*>& id_to_entity) {
  Reader in(topo_file_path, event_file_path, bucket_capacity, fill_rate, stats,
            sched, switches, controllers, hosts, physical, id_to_entity);

  bool valid_topology = in.ParseTopology();

  if(!valid_topology) return false;

  bool valid_events = in.ParseEvents();

  if(!valid_events) return false;

  return true;
}

int main(int ac, char* av[]) {
  string topo_file_path, event_file_path, out_prefix;
  Time heartbeat_period, ls_update_period, end_time;
  Size bucket_capacity;
  Rate fill_rate;
  size_t switch_count, host_count, controller_count;

  bool valid_args = ParseArgs(ac, av, topo_file_path, event_file_path,
                              heartbeat_period, ls_update_period, end_time,
                              bucket_capacity, fill_rate, out_prefix,
                              switch_count, host_count, controller_count);

  if(!valid_args) return -1;

  InitLogging(av[0], out_prefix);

  Scheduler sched(end_time, switch_count, controller_count, host_count);

  Statistics stats(out_prefix, sched);

  // TODO allocate elsewhere?
  // TODO get rid of completely?
  vector<Switch*> switches;
  vector<Controller*> controllers;
  vector<Host*> hosts;

  Topology phys;

  bool valid_world = CreateWorld(topo_file_path, event_file_path,
                                 bucket_capacity, fill_rate, stats, sched,
                                 switches, controllers, hosts, phys,
                                 stats.id_to_entity());

  // for(auto s : switches)
  //   LOG(INFO) << to_string(s->id()) + ":" + to_string(reinterpret_cast<long>(s)) + "+" + to_string(sizeof(*s));
  // for(auto c : controllers)
  //   LOG(INFO) << to_string(c->id()) + ":" + to_string(reinterpret_cast<long>(c)) + "+" + to_string(sizeof(*c));
  // for(auto h : hosts)
  //   LOG(INFO) << to_string(h->id()) + ":" + to_string(reinterpret_cast<long>(h)) + "+" + to_string(sizeof(*h));

  if (!valid_world) return -1;

  stats.Init(phys);

  //std::cout << stats.MaxPathLength() << std::endl;

  sched.SchedulePeriodicEvents(switches, heartbeat_period, ls_update_period);

  sched.StartSimulation(stats);

#ifndef NDEBUG
  for(auto s : switches) delete s;
  for(auto c : controllers) delete c;
  for(auto h : hosts) delete h;
#endif

  return 0;
}

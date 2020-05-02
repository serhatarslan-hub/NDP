// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
#include "dumbbell_topology.h"
#include <vector>
#include "string.h"
#include <sstream>
#include <strstream>
#include <iostream>
#include "main.h"
#include "queue.h"
#include "switch.h"
#include "compositequeue.h"
#include "prioqueue.h"
#include "queue_lossless.h"
#include "queue_lossless_input.h"
#include "queue_lossless_output.h"
#include "ecnqueue.h"

extern uint32_t RTT;

string ntoa(double n);
string itoa(uint64_t n);

//extern int N;

DumbbellTopology::DumbbellTopology(int no_of_nodes, mem_b queuesize, Logfile* lg,
				 EventList* ev,FirstFit * fit,queue_type q){
    _queuesize = queuesize;
		_no_of_nodes = no_of_nodes;
    logfile = lg;
    eventlist = ev;
    ff = fit;
    qt = q;
    failed_links = 0;

    set_params(no_of_nodes);

    init_network();
}

DumbbellTopology::DumbbellTopology(int no_of_nodes, mem_b queuesize, Logfile* lg,
				 EventList* ev,FirstFit * fit, queue_type q, int fail){
    _queuesize = queuesize;
		_no_of_nodes = no_of_nodes;
    logfile = lg;
    eventlist = ev;
    ff = fit;
		qt = q;
    failed_links = fail;

    set_params(no_of_nodes);

    init_network();
}

void DumbbellTopology::set_params(int no_of_nodes) {
    cout << "Set params " << no_of_nodes << endl;
    cout << "_no_of_nodes " << _no_of_nodes << endl;
		cout << "RTT " << RTT << " (us)" << endl;
    cout << "Queue type " << qt << endl;

    // pipes_nc_nup.resize(NC, vector<Pipe*>(NK));
    // pipes_nup_nlp.resize(NK, vector<Pipe*>(NK));
    // pipes_nlp_ns.resize(NK, vector<Pipe*>(NSRV));
		pipes_nsw_nhost.resize(no_of_nodes);

    // queues_nc_nup.resize(NC, vector<Queue*>(NK));
    // queues_nup_nlp.resize(NK, vector<Queue*>(NK));
    // queues_nlp_ns.resize(NK, vector<Queue*>(NSRV));
		queues_nsw_nhost.resize(no_of_nodes);

    // pipes_nup_nc.resize(NK, vector<Pipe*>(NC));
    // pipes_nlp_nup.resize(NK, vector<Pipe*>(NK));
    // pipes_ns_nlp.resize(NSRV, vector<Pipe*>(NK));
		pipes_nhost_nsw.resize(no_of_nodes);

    // queues_nup_nc.resize(NK, vector<Queue*>(NC));
    // queues_nlp_nup.resize(NK, vector<Queue*>(NK));
    // queues_ns_nlp.resize(NSRV, vector<Queue*>(NK));
		queues_nhost_nsw.resize(no_of_nodes);
}

Queue* DumbbellTopology::alloc_src_queue(QueueLogger* queueLogger){
    return  new PriorityQueue(speedFromMbps((uint64_t)HOST_NIC), memFromPkt(FEEDER_BUFFER), *eventlist, queueLogger);
}

Queue* DumbbellTopology::alloc_queue(QueueLogger* queueLogger, mem_b queuesize){
    return alloc_queue(queueLogger, HOST_NIC, queuesize);
}

Queue* DumbbellTopology::alloc_queue(QueueLogger* queueLogger, uint64_t speed, mem_b queuesize){
    if (qt==RANDOM)
	return new RandomQueue(speedFromMbps(speed), memFromPkt(SWITCH_BUFFER + RANDOM_BUFFER), *eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
    else if (qt==COMPOSITE)
	return new CompositeQueue(speedFromMbps(speed), queuesize, *eventlist, queueLogger);
    else if (qt==CTRL_PRIO)
	return new CtrlPrioQueue(speedFromMbps(speed), queuesize, *eventlist, queueLogger);
    else if (qt==ECN)
	return new ECNQueue(speedFromMbps(speed), memFromPkt(2*SWITCH_BUFFER), *eventlist, queueLogger, memFromPkt(15));
    else if (qt==LOSSLESS)
	return new LosslessQueue(speedFromMbps(speed), memFromPkt(50), *eventlist, queueLogger, NULL);
    else if (qt==LOSSLESS_INPUT)
	return new LosslessOutputQueue(speedFromMbps(speed), memFromPkt(200), *eventlist, queueLogger);
    else if (qt==LOSSLESS_INPUT_ECN)
	return new LosslessOutputQueue(speedFromMbps(speed), memFromPkt(10000), *eventlist, queueLogger,1,memFromPkt(16));
    assert(0);
}

void DumbbellTopology::init_network(){
  QueueLoggerSampling* queueLogger;

  for (int j=0;j<_no_of_nodes;j++){
		pipes_nsw_nhost[j] = NULL;
		queues_nsw_nhost[j] = NULL;
		pipes_nhost_nsw[j] = NULL;
		queues_nhost_nsw[j] = NULL;
	}

  //create the switch if we have lossless operation
  if (qt==LOSSLESS)
		the_switch = new Switch("Switch_Dumbbell");

  // links from switch to servers
  for (int j = 0; j < _no_of_nodes; j++) {
		// Downlink
	  queueLogger = new QueueLoggerSampling(timeFromUs(double(100)), *eventlist);
	  logfile->addLogger(*queueLogger);

	  queues_nsw_nhost[j] = alloc_queue(queueLogger, _queuesize);
	  queues_nsw_nhost[j]->setName("Sw->DST" + ntoa(j));
	  logfile->writeName(*(queues_nsw_nhost[j]));

	  pipes_nsw_nhost[j] = new Pipe(timeFromUs(double(RTT)/4.0), *eventlist);
	  pipes_nsw_nhost[j]->setName("Pipe_Sw->DST" + ntoa(j));
	  logfile->writeName(*(pipes_nsw_nhost[j]));

	  // Uplink
	  queueLogger = new QueueLoggerSampling(timeFromUs(double(100)), *eventlist);
	  logfile->addLogger(*queueLogger);

	  queues_nhost_nsw[j] = alloc_src_queue(queueLogger);
	  queues_nhost_nsw[j]->setName("SRC" + ntoa(j) + "->Sw");
	  logfile->writeName(*(queues_nhost_nsw[j]));

		if (qt==LOSSLESS){
    	the_switch->addPort(queues_nsw_nhost[j]);
    	((LosslessQueue*)queues_nsw_nhost[j])->setRemoteEndpoint(queues_nhost_nsw[j]);
		}else if (qt==LOSSLESS_INPUT || qt == LOSSLESS_INPUT_ECN){
      //no virtual queue needed at server
      new LosslessInputQueue(*eventlist,queues_nhost_nsw[j]);
		}

	  pipes_nhost_nsw[j] = new Pipe(timeFromUs(double(RTT)/4.0), *eventlist);
	  pipes_nhost_nsw[j]->setName("Pipe_SRC" + ntoa(j) + "->Sw");
	  logfile->writeName(*(pipes_nhost_nsw[j]));

	  if (ff){
	    ff->add_queue(queues_nsw_nhost[j]);
	    ff->add_queue(queues_nhost_nsw[j]);
	  }
  }

    /*for (int i = 0;i<NSRV;i++){
	      for (int j = 0;j<NK;j++){
					printf("%p/%p ",queues_ns_nlp[i][j], queues_nlp_ns[j][i]);
	      }
  			printf("\n");
  		}*/

  //init thresholds for lossless operation
  if (qt==LOSSLESS)
		the_switch->configureLossless();
}

void DumbbellTopology::check_non_null(Route* rt){
  int fail = 0;
  for (unsigned int i=1;i<rt->size()-1;i+=2)
    if (rt->at(i)==NULL){
      fail = 1;
      break;
    }

  if (fail){
    cout << "Null queue in route" << endl;
    for (unsigned int i=1;i<rt->size()-1;i+=2)
      printf("%p ",rt->at(i));
    cout << endl;
    assert(0);
  }
}

vector<const Route*>* DumbbellTopology::get_paths(int src, int dest){
  vector<const Route*>* paths = new vector<const Route*>();

  route_t *routeout, *routeback;

  // forward path
  routeout = new Route();
  routeout->push_back(queues_nhost_nsw[src]);
  routeout->push_back(pipes_nhost_nsw[src]);

  if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
		routeout->push_back(queues_nhost_nsw[src]->getRemoteEndpoint());

  routeout->push_back(queues_nsw_nhost[dest]);
  routeout->push_back(pipes_nsw_nhost[dest]);

  // reverse path for RTS packets
  routeback = new Route();
  routeback->push_back(queues_nhost_nsw[dest]);
  routeback->push_back(pipes_nhost_nsw[dest]);

  if (qt==LOSSLESS_INPUT || qt==LOSSLESS_INPUT_ECN)
		routeback->push_back(queues_nhost_nsw[dest]->getRemoteEndpoint());

  routeback->push_back(queues_nsw_nhost[src]);
  routeback->push_back(pipes_nsw_nhost[src]);

  routeout->set_reverse(routeback);
  routeback->set_reverse(routeout);

  paths->push_back(routeout);

	check_non_null(routeout);
  return paths;
}

void DumbbellTopology::count_queue(Queue* queue){
  if (_link_usage.find(queue)==_link_usage.end()){
    _link_usage[queue] = 0;
  }

  _link_usage[queue] = _link_usage[queue] + 1;
}

int DumbbellTopology::find_destination(Queue* queue){
  //first check nlp_ns
  for (int i=0;i<_no_of_nodes;i++)
      if (queues_nsw_nhost[i]==queue)
				return i;

  return -1;
}

void DumbbellTopology::print_path(std::ofstream &paths,int src,const Route* route){
  paths << "SRC_" << src << " ";

  if (route->size()/2==2){
    paths << "Sw ";
    paths << "DST_" << find_destination((Queue*)route->at(3)) << " ";
  } else {
    paths << "Wrong hop count " << ntoa(route->size()/2);
  }

  paths << endl;
}

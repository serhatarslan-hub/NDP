#ifndef DUMBBELL
#define DUMBBELL
#include "main.h"
#include "randomqueue.h"
#include "pipe.h"
#include "config.h"
#include "loggers.h"
#include "network.h"
#include "firstfit.h"
#include "topology.h"
#include "logfile.h"
#include "eventlist.h"
#include "switch.h"
#include <ostream>

#ifndef QT
#define QT
typedef enum {RANDOM, ECN, COMPOSITE, CTRL_PRIO, LOSSLESS, LOSSLESS_INPUT, LOSSLESS_INPUT_ECN} queue_type;
#endif

class DumbbellTopology: public Topology{
 public:
  // vector <Switch*> switches_lp;
  // vector <Switch*> switches_up;
  // vector <Switch*> switches_c;
  Switch * the_switch;

  // vector< vector<Pipe*> > pipes_nc_nup;
  // vector< vector<Pipe*> > pipes_nup_nlp;
  // vector< vector<Pipe*> > pipes_nlp_ns;
  vector<Pipe*> pipes_nsw_nhost;

  // vector< vector<Queue*> > queues_nc_nup;
  // vector< vector<Queue*> > queues_nup_nlp;
  // vector< vector<Queue*> > queues_nlp_ns;
  vector<Queue*> queues_nsw_nhost;

  // vector< vector<Pipe*> > pipes_nup_nc;
  // vector< vector<Pipe*> > pipes_nlp_nup;
  // vector< vector<Pipe*> > pipes_ns_nlp;
  vector<Pipe*> pipes_nhost_nsw;

  // vector< vector<Queue*> > queues_nup_nc;
  // vector< vector<Queue*> > queues_nlp_nup;
  // vector< vector<Queue*> > queues_ns_nlp;
  vector<Queue*> queues_nhost_nsw;

  FirstFit* ff;
  Logfile* logfile;
  EventList* eventlist;
  int failed_links;
  queue_type qt;

  DumbbellTopology(int no_of_nodes, mem_b queuesize, Logfile* log,EventList* ev,FirstFit* f, queue_type q);
  DumbbellTopology(int no_of_nodes, mem_b queuesize, Logfile* log,EventList* ev,FirstFit* f, queue_type q, int fail);

  void init_network();
  virtual vector<const Route*>* get_paths(int src, int dest);

  Queue* alloc_src_queue(QueueLogger* q);
  Queue* alloc_queue(QueueLogger* q, mem_b queuesize);
  Queue* alloc_queue(QueueLogger* q, uint64_t speed, mem_b queuesize);

  void count_queue(Queue*);
  void print_path(std::ofstream& paths,int src,const Route* route);
  vector<int>* get_neighbours(int src) { return NULL;};
  int no_of_nodes() const {return _no_of_nodes;}
  void check_non_null(Route* rt);
 private:
  map<Queue*,int> _link_usage;
  // int find_lp_switch(Queue* queue);
  // int find_up_switch(Queue* queue);
  // int find_core_switch(Queue* queue);
  int find_destination(Queue* queue);
  void set_params(int no_of_nodes);
  // int K, NK, NC, NSRV;
  int _no_of_nodes;
  mem_b _queuesize;
};

#endif

#ifndef DDCSIM_COMMON_H_
#define DDCSIM_COMMON_H_

#include <glog/logging.h>

#include <boost/graph/adjacency_list.hpp>

typedef double Time; /* Time has units of seconds */
typedef int SequenceNum;
typedef int Port;
typedef int Id;
typedef double Size; /* Size has units of bytes */
typedef double Rate; /* Rate has units of bytes per sec*/

/* Special port numbers */
const Port PORT_NOT_FOUND = -1;
const Port DROP = -1;

/* Special Id's */
const Id NONE_ID = -1;

/* Special times */
const Time START_TIME = 0;
const Time INVALID_TIME = -1;

/* Special Sequence Numbers */
const SequenceNum NONE_SEQNUM = -1;

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  DISALLOW_COPY(TypeName); DISALLOW_ASSIGN(TypeName)

#define DISALLOW_ASSIGN(TypeName) void operator=(const TypeName&)

#define DISALLOW_COPY(TypeName) TypeName(const TypeName&)

typedef boost::adjacency_list< boost::vecS,
                               boost::vecS,
                               boost::undirectedS> Topology;

typedef boost::graph_traits<Topology>::edge_descriptor Edge;
typedef boost::graph_traits<Topology>::vertex_descriptor Vertex;
typedef boost::graph_traits<Topology>::vertices_size_type VertexIndex;

// TODO don't make pair globally visible
typedef struct { SequenceNum sn; Time exp; } seen;

#endif

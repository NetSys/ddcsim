#ifndef DDCSIM_COMMON_H_
#define DDCSIM_COMMON_H_

#include <vector>
#include <boost/graph/vector_as_graph.hpp>

typedef double Time; /* Time has units of seconds */
typedef int SequenceNum;
typedef int Port;
typedef int Id;
typedef double Size; /* Size has units of bytes */
typedef double Rate; /* Rate has units of bytes per sec*/

/* Special port numbers */
const Port PORT_NOT_FOUND = -1;

/* Special Id's */
const Id NONE_ID = -1;

/* Special times */
const Time START_TIME = 0;

/* Special Sequence Numbers */
const SequenceNum NONE_SEQNUM = -1;

typedef std::vector< std::vector<Id> > Topology;

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  DISALLOW_COPY(TypeName); DISALLOW_ASSIGN(TypeName)

#define DISALLOW_ASSIGN(TypeName) void operator=(const TypeName&)

#define DISALLOW_COPY(TypeName) TypeName(const TypeName&)

#endif

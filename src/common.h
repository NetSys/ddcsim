#ifndef DDCSIM_COMMON_H_
#define DDCSIM_COMMON_H_

typedef double Time; /* Time has units of seconds */
typedef unsigned int SequenceNum;
typedef int Port;
typedef int Id;

/* Special port numbers */
const Port PORT_NOT_FOUND = -1;

/* Special Id's */
const Id NONE_ID = -1;

/* Special times */
const Time START_TIME = 0;

#define DISALLOW_COPY_AND_ASSIGN(TypeName)      \
  TypeName(const TypeName&);                    \
  void operator=(const TypeName&)

#endif

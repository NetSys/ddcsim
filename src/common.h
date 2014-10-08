#ifndef DDCSIM_COMMON_H_
#define DDCSIM_COMMON_H_

typedef double Time; /* Time has units of seconds */
typedef unsigned int SequenceNum;
typedef int Port;
typedef int Id;
typedef unsigned int Size; /* Size has units of bytes */
typedef unsigned int Rate; /* Rate has units of bytes per sec*/

#define TEST 1

/* Special port numbers */
const Port PORT_NOT_FOUND = -1;

/* Special Id's */
const Id NONE_ID = -1;

/* Special times */
const Time START_TIME = 0;

/* Special sizes */
const Size UNINIT_SIZE = 0;

/* Special rates */
const Rate UNINIT_RATE = 0;

#define DISALLOW_COPY_AND_ASSIGN(TypeName)      \
  TypeName(const TypeName&);                    \
  void operator=(const TypeName&)

#endif

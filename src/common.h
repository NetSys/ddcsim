#ifndef DDCSIM_COMMON_H_
#define DDCSIM_COMMON_H_

typedef double Time; /* Time has units of seconds */
typedef unsigned int SequenceNum;
typedef int Port;
typedef std::string Id;

/* Special port numbers */
#define INITIATING_EVENT -1
#define PORT_NOT_FOUND -2

/* Special Id's */
#define NONE_ID ""

#define DISALLOW_COPY_AND_ASSIGN(TypeName)      \
  TypeName(const TypeName&);                    \
  void operator=(const TypeName&)

#endif

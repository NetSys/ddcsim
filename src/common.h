#ifndef DDCSIM_COMMON_H_
#define DDCSIM_COMMON_H_

#include <iostream>

typedef double Time;
typedef unsigned int SequenceNum;
typedef unsigned int Port;

#define DISALLOW_COPY_AND_ASSIGN(TypeName)      \
  TypeName(const TypeName&);                    \
  void operator=(const TypeName&)

#endif

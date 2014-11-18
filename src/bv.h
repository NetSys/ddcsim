#ifndef DDCSIM_BV_H
#define DDCSIM_BV_H

#include "common.h"

#include <vector>

class BV {
 public:
 //  BV(std::vector<bool>* bv);
 //  BV(const BV&);
 //  BV(BV&&);
 //  BV& operator=(const BV&);
 //  BV& operator=(BV&&);
 //  ~BV();
 // private:
  BV(std::vector<bool>*, unsigned int*);
  std::vector<bool>* bv_;
  unsigned int* ref_count_;
};

#endif

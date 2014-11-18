#include "bv.h"

#include <glog/logging.h>

using std::vector;

BV::BV(std::vector<bool>* bv, unsigned int* ref_count)
    : bv_(bv), ref_count_(ref_count) {}

// BV::BV(const BV& that) : bv_(new vector<bool>(*that.bv_)), ref_count_() {}
// BV::BV(const BV& that) : bv_(that.bv_), ref_count_(that.ref_count_) {
//   ++(*ref_count_);
// }

// BV::BV(BV&& that) : bv_(that.bv_), ref_count_(that.ref_count_) {}

// BV& BV::operator=(const BV& rhs) {
//   // this->bv_ = new vector<bool>(*rhs.bv_);
//   this->bv_ = rhs.bv_;
//   this->ref_count_ = rhs.ref_count_;
//   ++(*ref_count_);
//   return *this;
// }

// BV& BV::operator=(BV&& rhs) {
//   // delete this->bv_;
//   // this->bv_ = rhs.bv_;
//   // rhs.bv_ = nullptr;
//   if(*ref_count_ == 1){
//     delete bv_;
//     delete ref_count_;
//   } else {
//     --(*ref_count_);
//   }

//   this->bv_ = rhs.bv_;
//   this->ref_count_ = rhs.ref_count_;
//   ++(*ref_count_);
//   return *this;
// }

// BV::~BV() {
//   if(*ref_count_ == 1){
//     delete bv_;
//     delete ref_count_;
//   } else {
//     --(*ref_count_);
//   }
// }

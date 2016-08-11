/*
 * header.h
 *
 *  Created on: 2016年8月11日
 *      Author: zjc
 */

#ifndef KID_PUB_STEP_HEADER_H_
#define KID_PUB_STEP_HEADER_H_

#include "field_map.h"

namespace step {

class Header {
 public:
  Header(FieldMap& field_map);
  ~Header();
 public:

 protected:
  FieldMap& field_map_;
};

} /* namespace step */

#endif /* KID_PUB_STEP_HEADER_H_ */

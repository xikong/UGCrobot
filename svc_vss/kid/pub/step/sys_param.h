/*
 * sys_param.h
 *
 *  Created on: 2016年8月10日
 *      Author: zjc
 */

#ifndef KID_PUB_STEP_SYS_PARAM_H_
#define KID_PUB_STEP_SYS_PARAM_H_

#include <cstdlib>

#include "step.h"
#include "header.h"

namespace step {

class SysParam : public Header {
 public:
  SysParam(FieldMap& field_map);
  ~SysParam();
 public:
  static std::string MsgType() { return kMsgTypeSysParam; }
  int stock_num() const { return atoi(field_map_.get(TAG_STOCK_NUM)->c_str()); }
  int index_num() const { return atoi(field_map_.get(TAG_INDEX_NUM)->c_str()); }
  int set_num() const { return atoi(field_map_.get(TAG_SET_NUM)->c_str()); }
  int sys_open_time() const { return atoi(field_map_.get(TAG_SYS_OPEN_TIME)->c_str()); }
  int sys_close_time() const { return atoi(field_map_.get(TAG_SYS_CLOSE_TIME)->c_str()); }
};

} /* namespace step */

#endif /* KID_PUB_STEP_SYS_PARAM_H_ */

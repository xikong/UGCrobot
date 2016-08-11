/*
 * user_report.h
 *
 *  Created on: 2016年8月10日
 *      Author: zjc
 */

#ifndef KID_PUB_STEP_USER_REPORT_H_
#define KID_PUB_STEP_USER_REPORT_H_

#include "field_map.h"
#include "step.h"

namespace step {

class UserReport : public FieldMap {
 public:
  static const char kLive[];
  static const char kNetwork[];

  UserReport(const std::string& version_code, int user_num);
  ~UserReport();
 public:
  static std::string MsgType() { return kMsgTypeUserReport; }
  const std::string* data_time_stamp() const { return get(TAG_DATA_TIME_STAMP); }
};

} /* namespace step */

#endif /* KID_PUB_STEP_USER_REPORT_H_ */

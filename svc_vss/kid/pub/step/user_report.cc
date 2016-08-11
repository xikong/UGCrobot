/*
 * user_report.cc
 *
 *  Created on: 2016年8月10日
 *      Author: zjc
 */

#include <boost/date_time/posix_time/posix_time.hpp>

#include "user_report.h"
#include "glog/logging.h"

namespace step {

const char UserReport::kLive[] = "01";
const char UserReport::kNetwork[] = "02";

UserReport::UserReport(const std::string& version_code, int user_num) {
  if (kLive != version_code && kNetwork != version_code) {
    LOG(ERROR) << "unknown version code: " << version_code;
  }
  set(TAG_MSG_TYPE, kMsgTypeUserReport);
  set(TAG_VERSION_CODE, version_code);
  set(TAG_USER_NUM, user_num);

  std::string str_time =
      boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time());
  int pos = str_time.find('T');
  str_time.replace(pos,1,std::string("-"));
  str_time.replace(pos + 3,0,std::string(":"));
  str_time.replace(pos + 6,0,std::string(":"));
  set(TAG_DATA_TIME_STAMP, str_time);
}

UserReport::~UserReport() {
}

} /* namespace step */

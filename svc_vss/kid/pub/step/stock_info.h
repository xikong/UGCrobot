/*
 * stock_info.h
 *
 *  Created on: 2016年8月10日
 *      Author: zjc
 */

#ifndef KID_PUB_STEP_STOCK_INFO_H_
#define KID_PUB_STEP_STOCK_INFO_H_

#include "step.h"
#include "header.h"

namespace step {

class StockInfo : public Header {
 public:
  StockInfo();
  ~StockInfo();
 public:
  static std::string MsgType() { return kMsgTypeStockInfo; }
};

} /* namespace step */

#endif /* KID_PUB_STEP_STOCK_INFO_H_ */

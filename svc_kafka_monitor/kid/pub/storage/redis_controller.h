//  Copyright (c) 2015-2015 The autocrawler Authors. All rights reserved.
//  Created on: 2015年9月17日 Author: kerry

#ifndef KID_REDIS_CONTROLLER_
#define KID_REDIS_CONTROLLER_

#include <list>
#include "storage/storage_controller_engine.h"
#include "logic/base_values.h"

namespace base_logic {

class RedisController : public DataControllerEngine {
 public:
  RedisController() {
  }
  virtual ~RedisController() {
  }
 public:
  void Release();
  void InitParam(std::list<base::ConnAddr>* addrlist) {
  }
 public:
  bool WriteData(const int32 type, base_logic::Value* value);

  bool ReadData(const int32 type, base_logic::Value* value,
                void (*storage_get)(void*, base_logic::Value*));
 private:
  bool ReadHashData(base_logic::Value* value,
                    void (*storage_get)(void*, base_logic::Value*));

  bool ReadKeyValueData(base_logic::Value* value,
                        void (*storage_get)(void*, base_logic::Value*));
};
}  // namespace base_logic

#endif /* PLUGINS_CRAWLERSVC_STORAGE_REDIS_CRAWL_STORAGE_H_ */

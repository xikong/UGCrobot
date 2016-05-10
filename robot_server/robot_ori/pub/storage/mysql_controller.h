//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月17日 Author: kerry

#ifndef KID_MYSQL_CONTROLLER_
#define KID_MYSQL_CONTROLLER_

#include <string>
#include <list>
#include "storage/storage_controller_engine.h"
#include "logic/base_values.h"
#include "basic/scoped_ptr.h"

namespace base_logic {

class MysqlController:public DataControllerEngine {
 public:
    MysqlController() {}
    virtual ~MysqlController() {}
 public:
    void InitParam(std::list<base::ConnAddr>* addrlist);
    void Release();
 public:
    bool ReadData(const int32 type, base_logic::Value* value,
            void (*storage_get)(void*, base_logic::Value*));

    bool WriteData(const int32 type, base_logic::Value* value);
};
}  // namespace base_logic



#endif  // PUB_STORAGE_MYSQL_CONTROLLER_H_

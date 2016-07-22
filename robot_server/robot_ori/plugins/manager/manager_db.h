//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月17日 Author: kerry

#ifndef KID_MANAGER_DB_H_
#define KID_MANAGER_DB_H_

#include <string>

#include "../../pub/logic/auto_crawler_infos0505.h"
#include "storage/storage_controller_engine.h"
#include "basic/basictypes.h"
#include "logic/base_values.h"
#include "basic/scoped_ptr.h"

namespace manager_logic {

class ManagerDB {
 public:
    ManagerDB();
    virtual ~ManagerDB();
 public:
    bool CrawlerManagerLogin(void* data);

 public:
    static void CallBackManagerLogin(void* param,
            base_logic::Value* value);

 private:
    scoped_ptr<base_logic::DataControllerEngine> mysql_engine_;
};

}  // namespace manager_logic


#endif /* MANAGER_DB_H_ */

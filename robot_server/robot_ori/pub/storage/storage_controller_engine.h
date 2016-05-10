//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月17日 Author: kerry

#ifndef KID_STORAGE_STORAGE_CONTROLLER_ENGINE_H_
#define KID_STORAGE_STORAGE_CONTROLLER_ENGINE_H_

#include <list>
#include "basic/basictypes.h"
#include "logic/base_values.h"
#include "config/config.h"
#include "storage/storage.h"


enum STORAGETYPE{
    REIDS_TYPE = 0,
    MYSQL_TYPE = 1,
    MEM_TYPE = 2
};

enum MEMCACHE_TYPE{
    MEM_KEY_VALUE = 0,
    BATCH_KEY_VALUE = 1,
};

enum REDIS_TYPE{
    HASH_VALUE = 0,
    READIS_KEY_VALUE = 1,
};


namespace base_logic {

class DataControllerEngine {
 public:
    static DataControllerEngine* Create(int32 type);
    static void Init(config::FileConfig* config);
    static void Dest();
    virtual ~DataControllerEngine() {}
 public:
    virtual void Release() = 0;
    virtual void InitParam(std::list<base::ConnAddr>* addrlist) = 0;
 public:
    virtual bool ReadData(const int32 type, base_logic::Value* value,
    void (*storage_get)(void*, base_logic::Value*)) = 0;

    virtual bool WriteData(const int32 type, base_logic::Value* value) = 0;
};
}  // namespace base_logic


#endif /* PLUGINS_CRAWLERSVC_STORAGE_STORAGE_BASE_ENGINE_H_ */

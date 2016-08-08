//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月17日 Author: kerry

#ifndef KID_MEM_CONTROLLER_
#define KID_MEM_CONTROLLER_

#include <list>
#include "storage/storage_controller_engine.h"
#include "logic/base_values.h"

namespace base_logic {
class MemController:public DataControllerEngine {
 public:
    MemController();
    virtual ~MemController() {}
 public:
    void InitParam(std::list<base::ConnAddr>* addrlist);
    void Release();
 public:
    bool WriteData(const int32 type, base_logic::Value* value);

    bool ReadData(const int32 type, base_logic::Value* value,
            void (*storage_get)(void*, base_logic::Value*));
 private:
    bool WriteKeyValueData(base_logic::Value* value);
    bool ReadKeyValueData(base_logic::Value* value);
    bool ReadBatchKeyValue(base_logic::Value* value);
 private:
    base_storage::DictionaryStorageEngine* engine_;
};
}  // namespace base_logic



#endif /* MEM_DATA_STORAGE_H_ */

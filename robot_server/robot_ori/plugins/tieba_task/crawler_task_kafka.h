//  Copyright (c) 2015-2018 The KID Authors. All rights reserved.
//  Created on: 2016.2.18 Author: yangge

#ifndef KID_CRAWLER_TASK_KAFKA_H_
#define KID_CRAWLER_TASK_KAFKA_H_

#include <string>
#include <list>
#include <map>

#include "../../pub/logic/auto_crawler_infos0505.h"
#include "basic/basictypes.h"
#include "logic/base_values.h"
#include "basic/scoped_ptr.h"
#include "queue/kafka_consumer.h"

namespace tieba_task_logic {

class CrawlerTaskKafka {
 public:
	CrawlerTaskKafka();
    virtual ~CrawlerTaskKafka();

 public:

    bool FectchBatchTempTask(std::list<base_logic::TiebaTask> *list);

    void SetTaskInfo(base_logic::RobotTask &task_info, base_logic::DictionaryValue* task_info_dic);

 private:
    kafka_consumer kafka_consumer_;
};
}  // namespace crawler_task_logic


#endif /* TASK_DB_H_ */

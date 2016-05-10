//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月14日 Author: kerry
#include "auto_crawler_infos0505.h"

namespace base_logic {

void QZoneTask::GetDataFromKafka(base_logic::DictionaryValue *dict) {
	id_ = base::SysRadom::GetInstance()->GetRandomIntID();
	dict->GetString(L"topicId", &topic_id_);
	dict->GetString(L"hostUin", &host_uin_);
	dict->GetString(L"uin", &uin_);
	dict->GetString(L"content", &content_);
}

RobotTask* RobotTaskFactory::Create(RobotTask::TaskType type) {
	switch(type) {
	case RobotTask::TAOGUBA:
		return new Taoguba();
		break;
	default:
		return NULL;
	}
}

}  // namespace base_logic

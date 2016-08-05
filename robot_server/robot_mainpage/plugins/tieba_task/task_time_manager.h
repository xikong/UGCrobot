//  Copyright (c) 2015-2015 The kid Authors. All rights reserved.
//  Created on: 2015年10月12日 Author: kerry

#ifndef KID_CRAWLER_TASK_TIME_MANAGER_H_
#define KID_CRAWLER_TASK_TIME_MANAGER_H_

#include "crawler_task_db.h"
#include "crawler_task_kafka.h"
#include "task_schduler_engine.h"
#include "share/session_engine.h"

#define TIME_DISTRIBUTION_TASK       10000
#define TIME_FECTCH_TASK             10001
#define TIME_CLEAN_NO_EFFECTIVE      10005
#define TIME_RECYCLINGTASK           10006
#define TIME_FETCH_MAIN_TASK         10007
#define TIME_DISTRBUTION_TEMP_TASK   10008
#define TIME_UPDATE_EXEC_TASKS		   10009
#define TIME_FETCH_IP                10010
#define TIME_FETCH_BLACK_KW          10011

class ForgeryIPManager;

namespace tieba_task_logic {

class TaskTimeManager {
 public:
  explicit TaskTimeManager(tieba_task_logic::CrawlerTaskDB* task_db);
  virtual ~TaskTimeManager();
 public:
  void TaskTimeEvent(int opcode, int time);

  CrawlerTaskKafka& GetTaskKafka();

  void SetSessionMgr(plugin_share::SessionManager *session_mgr) {
    session_mgr_ = session_mgr;
  }
 private:
  void TimeFetchTask();

  void TimeCheckTask();

  void TimeFechMainTask();

  void CleanNoEffectCrawler();

  void UpdateExecTasks();

 private:
  tieba_task_logic::TaskSchdulerManager* schduler_mgr_;
  ForgeryIPManager* ip_mgr_;
  scoped_ptr<tieba_task_logic::CrawlerTaskDB> task_db_;
  CrawlerTaskKafka task_kafka_;
  plugin_share::SessionManager *session_mgr_;
};

}  //  namespace crawler_task_logic

#endif /* TASK_TIME_MANAGER_H_ */

//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月16日 Author: kerry

#include <list>
#include <string>
#include "core/common.h"
#include "net/errno.h"
#include "basic/native_library.h"
#include "logic/logic_comm.h"
#include "logic/logic_unit.h"
#include "basic/radom_in.h"
#include "crawler_task_logic.h"
#include "kw_blacklist.h"

#define DEFAULT_CONFIG_PATH     "./plugins/tieba_task/tieba_task_config.xml"

#define TIMER_SERVER_STARTUP	10002

namespace tieba_task_logic {

CrawlerTasklogic* CrawlerTasklogic::instance_ = NULL;
base_logic::RobotTask::TaskType CrawlerTasklogic::task_type_ =
    base_logic::RobotTask::TIEBA;

CrawlerTasklogic::CrawlerTasklogic()
    : session_mgr_(NULL) {
  if (!Init())
    assert(0);
}

CrawlerTasklogic::~CrawlerTasklogic() {
}

bool CrawlerTasklogic::Init() {
  bool r = false;
  router_schduler::SchdulerEngine* (*router_engine)(void);
  task_db_.reset(new tieba_task_logic::CrawlerTaskDB());
  task_time_mgr_.reset(new tieba_task_logic::TaskTimeManager(task_db_.get()));
  std::string path = DEFAULT_CONFIG_PATH;
  config::FileConfig* config = config::FileConfig::GetFileConfig();
  if (config == NULL)
    return false;
  r = config->LoadConfig(path);
  base_logic::DataControllerEngine::Init(config);

  config_ = Config::GetConfig();
  config_->Print();

  KwBlacklist::GetKwBlacklist()->FetchBlackKws(task_db_.get());

  std::string cralwer_library = "./crawler_schduler/crawler_schduler.so";
  std::string cralwer_func = "GetRouterSchdulerEngine";

  router_engine = (router_schduler::SchdulerEngine* (*)(void))
  logic::SomeUtils::GetLibraryFunction(
  cralwer_library, cralwer_func);

router_schduler_engine_  = (*router_engine)();
  if (router_schduler_engine_ == NULL)
    assert(0);

  tieba_task_logic::TaskSchdulerManager* schduler_mgr =
      tieba_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();

  InitTask(schduler_mgr);

  schduler_mgr->Init(router_schduler_engine_);
  schduler_mgr->InitDB(task_db_.get());

  tieba_task_logic::TaskSchdulerEngine* engine =
      tieba_task_logic::TaskSchdulerEngine::GetTaskSchdulerEngine();

  schduler_mgr->DistributionTask();
  base::SysRadom::GetInstance()->InitRandom();
  return true;
}

bool CrawlerTasklogic::Startup() {
  LOG_DEBUG("start up");
  struct server *pserver = logic::CoreSoUtils::GetSRV();
  assert(pserver);

  void **p_share = pserver->get_plugin_share_data(pserver, "manager");
  assert(p_share);
  session_mgr_ = *(plugin_share::SessionManager**) p_share;
  LOG_DEBUG2("session_mgr addr: %p", session_mgr_);
  assert(session_mgr_);

  TaskSchdulerManager *task_schedule_manager =
      TaskSchdulerEngine::GetTaskSchdulerManager();
  task_schedule_manager->InitManagerInfo(session_mgr_);
  task_time_mgr_.get()->SetSessionMgr(session_mgr_);
  return true;
}

void CrawlerTasklogic::InitTask(
    tieba_task_logic::TaskSchdulerManager* schduler_mgr) {
#if 0
  std::list<base_logic::WeiboTaskInfo> task_list;
  task_db_->FecthBatchTask(&task_list);
  schduler_mgr->FetchBatchTask(&task_list);
  base::SysRadom::GetInstance()->DeinitRandom();
#endif
}

CrawlerTasklogic*
CrawlerTasklogic::GetInstance() {
  if (instance_ == NULL)
    instance_ = new CrawlerTasklogic();
  return instance_;
}

void CrawlerTasklogic::FreeInstance() {
  if (NULL != instance_)
    delete instance_;
  instance_ = NULL;
}

bool CrawlerTasklogic::OnTaskConnect(struct server *srv, const int socket) {
  return HandleAllConnect(srv, socket);
}

bool CrawlerTasklogic::HandleAllMessage(struct server *srv, const int socket,
                                        const void *msg, const int len) {
  if (NULL == session_mgr_) {
    LOG_MSG("wait server start up");
    return false;
  }

  if (!session_mgr_->ServerIsValid()) {
    LOG_MSG("server has not registered success, don't process any request");
    return false;
  }

  if (!session_mgr_->IsValidSocket(socket)) {
    LOG_MSG2("the socket[%d] is invalid", socket);
    return false;
  }

  bool r = false;
  struct PacketHead* packet = NULL;
  if (srv == NULL || socket < 0 || msg == NULL || len < PACKET_HEAD_LENGTH)
    return false;

  if (!net::PacketProsess::UnpackStream(msg, len, &packet)) {
    LOG_ERROR2("UnpackStream Error socket %d", socket);
    net::PacketProsess::HexEncode(msg, len);
    return false;
  }
  std::string addr;
  logic::SomeUtils::GetAddressBySocket(socket, addr);
  LOG_DEBUG2("packet->operate_code=%d, socket = %d, addr = %s",
      (int)packet->operate_code, socket, addr.c_str());
  net::PacketProsess::DumpPacket(packet);
  switch (packet->operate_code) {
    case CRAWL_HBASE_STORAGE_INFO: {
      StorageMethod(srv, socket, packet, 1);
      break;
    }
    case REPLY_TASK_STATE: {
      ReplyTaskState(srv, socket, packet);
      break;
    }
    case TEMP_CRAWLER_OP: {
      TimeDistributionTask();
      break;
    }
    case ROUTER_STATUS: {
      OnRouterStatus(srv, socket, packet);
      break;
    }
    case ROBOT_TASK_STATE: {
      OnRobotTaskStatus(srv, socket, packet);
      break;
    }
    default:
      break;
  }
  net::PacketProsess::DeletePacket(msg, len, packet);
  return true;
}

void CrawlerTasklogic::StorageMethod(struct server* srv, int socket,
                                     struct PacketHead *packet, int32 type,
                                     const void *msg, int32 len) {
  struct CrawlStorageInfo* storage = (struct CrawlStorageInfo*) packet;
  task_time_mgr_->GetTaskKafka().AddStorageInfo(storage->storage_set, type);
}

bool CrawlerTasklogic::OnTaskMessage(struct server *srv, const int socket,
                                     const void *msg, const int len) {
  return HandleAllMessage(srv, socket, msg, len);
}

bool CrawlerTasklogic::OnTaskClose(struct server *srv, const int socket) {
  return HandleAllClose(srv, socket);
}

bool CrawlerTasklogic::HandleAllConnect(struct server *srv, const int socket) {
  std::string addr;
  logic::SomeUtils::GetAddressBySocket(socket, addr);
  LOG_DEBUG2("new connect, socket = %d, addr = %s", socket, addr.c_str());
  return true;
}

bool CrawlerTasklogic::OnBroadcastConnect(struct server *srv, const int socket,
                                          const void *msg, const int len) {
  return HandleAllConnect(srv, socket);
}

bool CrawlerTasklogic::OnBroadcastMessage(struct server *srv, const int socket,
                                          const void *msg, const int len) {
  return HandleAllMessage(srv, socket, msg, len);
}

bool CrawlerTasklogic::HandleAllClose(struct server *srv, const int socket) {
  time_t t = time(NULL);
  std::string addr;
  logic::SomeUtils::GetAddressBySocket(socket, addr);
  LOG_MSG2("[%s] connection closed, socket = %d, addr = %s", ctime(&t), socket, addr.c_str());
  return true;
}

bool CrawlerTasklogic::OnBroadcastClose(struct server *srv, const int socket) {
  return HandleAllClose(srv, socket);
}

bool CrawlerTasklogic::OnIniTimer(struct server *srv) {
  if (srv->add_time_task != NULL) {
    srv->add_time_task(srv, "tieba_task", TIMER_SERVER_STARTUP, 2, 1);

    srv->add_time_task(srv, "tieba_task", TIME_DISTRIBUTION_TASK,
                       config_->assign_task_tick, -1);

    srv->add_time_task(srv, "tieba_task", TIME_FECTCH_TASK,
                       config_->fetch_task_tick, -1);

    srv->add_time_task(srv, "tieba_task", TIME_RECYCLINGTASK,
                       config_->recycle_task_tick, -1);

    srv->add_time_task(srv, "tieba_task", TIME_FETCH_MAIN_TASK,
                       config_->fetch_main_task_tick, -1);

    srv->add_time_task(srv, "tieba_task", TIME_DISTRBUTION_TEMP_TASK, 10, -1);

    srv->add_time_task(srv, "tieba_task", TIME_CLEAN_NO_EFFECTIVE,
                       config_->clean_no_effective_client_tick, -1);

    srv->add_time_task(srv, "tieba_task", TIME_UPDATE_EXEC_TASKS,
                       config_->reply_self_state_tick, -1);

    srv->add_time_task(srv, "tieba_task", TIME_FETCH_IP, config_->fetch_ip_tick,
                       -1);

    srv->add_time_task(srv, "tieba_task", TIME_FETCH_BLACK_KW, config_->fetch_black_kw_tick,
                       -1);
  }
  return true;
}

bool CrawlerTasklogic::OnTimeout(struct server *srv, char *id, int opcode,
                                 int time) {
  switch (opcode) {
    case TIMER_SERVER_STARTUP:
      Startup();
      break;
    default:
      task_time_mgr_->TaskTimeEvent(opcode, time);
      break;
  }
  return true;
}

void CrawlerTasklogic::ReplyTaskState(struct server* srv, int socket,
                                      struct PacketHead *packet,
                                      const void *msg, int32 len) {
#if 0
  struct ReplyTaskState* task_state = (struct ReplyTaskState*) packet;
  tieba_task_logic::TaskSchdulerManager* schduler_mgr =
  tieba_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();
  schduler_mgr->AlterTaskState(task_state->jobid, task_state->state);
#endif
}

void CrawlerTasklogic::RelpyCrawlNum(struct server* srv, int socket,
                                     struct PacketHead *packet, const void *msg,
                                     int32 len) {
  struct ReplyCrawlContentNum* crawl_num = (struct ReplyCrawlContentNum*) packet;

  tieba_task_logic::TaskSchdulerManager* schduler_mgr =
      tieba_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();
  schduler_mgr->AlterCrawlNum(crawl_num->task_id, crawl_num->num);
}

void CrawlerTasklogic::TimeDistributionTask() {
  tieba_task_logic::TaskSchdulerManager* schduler_mgr =
      tieba_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();
  schduler_mgr->DistributionTask();
}

void CrawlerTasklogic::TimeFetchTask() {
#if 0
  tieba_task_logic::TaskSchdulerManager* schduler_mgr =
  tieba_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();
  std::list<base_logic::WeiboTaskInfo> list;
  task_db_->FecthBatchTask(&list, true);
  schduler_mgr->FetchBatchTask(&list, true);
#endif
}

bool CrawlerTasklogic::OnRouterStatus(struct server* srv, int socket,
                                      struct PacketHead* packet,
                                      const void *msg, int32 len) {
  bool r = true;
  struct RouterStatus *status = (struct RouterStatus *) packet;
  base_logic::RouterScheduler scheduler;
  r = router_schduler_engine_->GetRouterSchduler(status->router_id, &scheduler);
  if (!r) {
    LOG_MSG2("add new router scheduler, id: %d, socket: %d", status->router_id, socket);
    scheduler.set_id(status->router_id);
    scheduler.set_socket(socket);
    router_schduler_engine_->SetSchduler(status->router_id, &scheduler);
  }
  scheduler.set_router_status(status, socket);
  return r;
}

bool CrawlerTasklogic::OnRobotTaskStatus(struct server* srv, int socket,
                                         struct PacketHead* packet,
                                         const void *msg, int32 len) {
  bool r = true;
  struct RobotTaskStatus *status = (struct RobotTaskStatus *) packet;
  LOG_DEBUG2("receive robot task status: task_type = %d, task_id = %d, status = %d",
      status->task_type, status->task_id, status->is_success);
  if (base_logic::RobotTask::TaskType(status->task_type) != task_type_)
    return false;
  TASKSTAE task_state = TASK_SUCCESS;
  if (0 == status->is_success) {
    task_state = TASK_FAIL;
  }
  tieba_task_logic::TaskSchdulerManager* schduler_mgr =
      tieba_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();
  schduler_mgr->AlterTaskState(status->task_id, task_state);
  task_db_->UpdateRobotTaskState(status->task_id, (int) task_state);
}
}  // namespace crawler_task_logic


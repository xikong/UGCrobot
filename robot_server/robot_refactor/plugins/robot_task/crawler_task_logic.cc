//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月16日 Author: kerry

#include <list>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "core/common.h"
#include "net/errno.h"
#include "basic/native_library.h"
#include "logic/logic_comm.h"
#include "logic/logic_unit.h"
#include "basic/radom_in.h"
#include "crawler_task_logic.h"
#include "task_schduler_engine.h"

#define DEFAULT_CONFIG_PATH     "./plugins/robot_task/robot_task_config.xml"
#define ROBOT_CONFIG_FILE		"./config"
#define MAX_BUFFER_SIZE			4096

#define TIMER_SERVER_STARTUP	10002

namespace robot_task_logic {

CrawlerTasklogic* CrawlerTasklogic::instance_ = NULL;

CrawlerTasklogic::CrawlerTasklogic() :
		manager_info_(NULL) {
	if (!Init())
		assert(0);
}

CrawlerTasklogic::~CrawlerTasklogic() {
}

void Config::Deserialize(base_logic::DictionaryValue *dict) {
	int64 tmp;
	dict->GetBigInteger(L"assign_task_tick", &tmp);
	assign_task_tick = tmp;
	dict->GetBigInteger(L"fetch_task_tick", &tmp);
	fetch_task_tick = tmp;
	dict->GetBigInteger(L"recycle_task_tick", &tmp);
	recycle_task_tick = tmp;
	dict->GetBigInteger(L"clean_no_effective_client_tick", &tmp);
	clean_no_effective_client_tick = tmp;
	dict->GetBigInteger(L"fetch_ip_tick", &tmp);
	fetch_ip_tick = tmp;
	dict->GetBigInteger(L"fetch_cookie_tick", &tmp);
	fetch_cookie_tick = tmp;
	dict->GetBigInteger(L"fetch_content_tick", &tmp);
	fetch_content_tick = tmp;
	dict->GetBigInteger(L"cookie_use_tick", &tmp);
	cookie_use_tick = tmp;
}

void Config::Print() const {
	std::stringstream os;
	os << "\n--------------------- Config Begin ---------------------" << std::endl;
#define PRINT(v) \
	os << '\t' << #v << " = " << v << std::endl
	PRINT(assign_task_tick);
	PRINT(fetch_task_tick);
	PRINT(recycle_task_tick);
	PRINT(clean_no_effective_client_tick);
	PRINT(fetch_ip_tick);
	PRINT(fetch_cookie_tick);
	PRINT(fetch_content_tick);
	PRINT(cookie_use_tick);
	os << "--------------------- Config End  ---------------------" << std::endl;
	LOG_MSG2("%s", os.str().c_str());
#undef PRINT
}

bool CrawlerTasklogic::ReadConfig() {
	FILE *hFile = fopen(ROBOT_CONFIG_FILE, "r");
	if (NULL == hFile) {
		LOG_MSG2("open %s file error", ROBOT_CONFIG_FILE);
		return false;
	}
	struct stat  file_stat;
	if (0 != stat(ROBOT_CONFIG_FILE, &file_stat)) {
		LOG_MSG2("get %s stat error", ROBOT_CONFIG_FILE);
		return false;
	}
	off_t filesize = file_stat.st_size;
	char buf[MAX_BUFFER_SIZE];
	fread(buf, 1, filesize, hFile);
	std::string data(buf, filesize);
	LOG_DEBUG2("config content: %s", data.c_str());

	base_logic::ValueSerializer* engine =
			base_logic::ValueSerializer::Create(0, &data);
	int error_code = 0;
	std::string error_str;
	base_logic::Value* value = engine->Deserialize(&error_code, &error_str);
	if (0 != error_code || NULL == value) {
		LOG_MSG2("deserialize error, error_code = %d, error_str = %s",
				error_code, error_str);
		return false;
	}
	base_logic::DictionaryValue* config =
			(base_logic::DictionaryValue*) value;
	config_.Deserialize(config);
	delete config;
	base_logic::ValueSerializer::DeleteSerializer(0, engine);
	return true;
}

bool CrawlerTasklogic::Init() {
	bool r = false;
	router_schduler::SchdulerEngine* (*router_engine)(void);
	task_db_.reset(new robot_task_logic::CrawlerTaskDB());
	task_time_mgr_.reset(new robot_task_logic::TaskTimeManager(task_db_.get()));
	std::string path = DEFAULT_CONFIG_PATH;
	config::FileConfig* config = config::FileConfig::GetFileConfig();
	if (config == NULL)
		return false;
	r = config->LoadConfig(path);
	base_logic::DataControllerEngine::Init(config);

	ReadConfig();
	config_.Print();

	std::string cralwer_library = "./crawler_schduler/crawler_schduler.so";
	std::string cralwer_func = "GetRouterSchdulerEngine";

	router_engine = (router_schduler::SchdulerEngine* (*)(void))
	logic::SomeUtils::GetLibraryFunction(
	cralwer_library, cralwer_func);

	router_schduler_engine_	= (*router_engine)();
	if (router_schduler_engine_ == NULL)
		assert(0);

	robot_task_logic::TaskSchdulerManager* schduler_mgr =
			robot_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();

	schduler_mgr->Init(router_schduler_engine_, &config_);
	schduler_mgr->InitDB(task_db_.get());

	robot_task_logic::TaskSchdulerEngine* engine =
			robot_task_logic::TaskSchdulerEngine::GetTaskSchdulerEngine();

	base::SysRadom::GetInstance()->InitRandom();
	return true;
}

bool CrawlerTasklogic::Startup() {
	LOG_DEBUG("start up");
	struct server *pserver = logic::CoreSoUtils::GetSRV();
	assert(pserver);

	void **p_share = pserver->get_plugin_share_data(pserver, "manager");
	assert(p_share);

	manager_info_ = (plugin_share::ManagerInfo *) *p_share;
	assert(manager_info_);

	TaskSchdulerManager *task_schedule_manager =
			TaskSchdulerEngine::GetTaskSchdulerManager();
	task_schedule_manager->InitManagerInfo(manager_info_);
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
	if (NULL == manager_info_) {
		LOG_MSG("waiting for init complete");
		return false;
	}

	base_logic::MLockGd lk(manager_info_->lock_);
	if (manager_info_->router_map.end()
			== manager_info_->router_map.find(socket)) {
		LOG_MSG2("the connection is invalid, socket: %d", socket);
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
	case ROUTER_SCHEDULE_FAIL: {
		OnRouterScheduleFail(srv, socket, packet);
		break;
	}
	default:
		break;
	}
	net::PacketProsess::DeletePacket(msg, len, packet);
	return true;
}

bool CrawlerTasklogic::OnTaskMessage(struct server *srv, const int socket,
		const void *msg, const int len) {
	return HandleAllMessage(srv, socket, msg, len);
}

bool CrawlerTasklogic::OnTaskClose(struct server *srv, const int socket) {
	return HandleAllClose(srv, socket);
}

bool CrawlerTasklogic::HandleAllConnect(struct server *srv, const int socket) {
	time_t t = time(NULL);
	char *str_time = ctime(&t);
	str_time[strlen(str_time)-1] = '\0';
	std::string addr;
	logic::SomeUtils::GetAddressBySocket(socket, addr);
	LOG_MSG2("[%s] new connect, socket = %d, addr = %s", str_time, socket, addr.c_str());
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
		srv->add_time_task(srv, "robot_task", TIMER_SERVER_STARTUP, 1, 1);
		srv->add_time_task(srv, "robot_task", TIME_DISTRIBUTION_TASK, config_.assign_task_tick, -1);
		srv->add_time_task(srv, "robot_task", TIME_FECTCH_TASK, config_.fetch_task_tick, -1);
		srv->add_time_task(srv, "robot_task", TIME_RECYCLINGTASK, config_.recycle_task_tick, -1);
		srv->add_time_task(srv, "robot_task", TIME_FETCH_TEMP_TASK, 60, -1);
		srv->add_time_task(srv, "robot_task", TIME_DISTRBUTION_TEMP_TASK, 10,
				-1);
		srv->add_time_task(srv, "robot_task", TIME_CLEAN_NO_EFFECTIVE, config_.clean_no_effective_client_tick, -1);
		srv->add_time_task(srv, "robot_task", TIME_UPDATE_EXEC_TASKS, 10, -1);

		srv->add_time_task(srv, "robot_task", TIME_FETCH_IP, config_.fetch_ip_tick, -1);
		srv->add_time_task(srv, "robot_task", TIME_FETCH_COOKIE, config_.fetch_cookie_tick, -1);
		srv->add_time_task(srv, "robot_task", TIME_FETCH_CONTENT, config_.fetch_content_tick, -1);
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
		struct PacketHead *packet, const void *msg, int32 len) {
#if 0
	struct ReplyTaskState* task_state = (struct ReplyTaskState*) packet;
	robot_task_logic::TaskSchdulerManager* schduler_mgr =
	robot_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();
	schduler_mgr->AlterTaskState(task_state->jobid, task_state->state);
#endif
}

void CrawlerTasklogic::RelpyCrawlNum(struct server* srv, int socket,
		struct PacketHead *packet, const void *msg, int32 len) {
	struct ReplyCrawlContentNum* crawl_num =
			(struct ReplyCrawlContentNum*) packet;

	robot_task_logic::TaskSchdulerManager* schduler_mgr =
			robot_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();
	schduler_mgr->AlterCrawlNum(crawl_num->task_id, crawl_num->num);
}

void CrawlerTasklogic::TimeDistributionTask() {
	robot_task_logic::TaskSchdulerManager* schduler_mgr =
			robot_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();
	schduler_mgr->DistributionTask();
}

void CrawlerTasklogic::TimeFetchTask() {
#if 0
	robot_task_logic::TaskSchdulerManager* schduler_mgr =
	robot_task_logic::TaskSchdulerEngine::GetTaskSchdulerManager();
	std::list<base_logic::WeiboTaskInfo> list;
	task_db_->FecthBatchTask(&list, true);
	schduler_mgr->FetchBatchTask(&list, true);
#endif
}

bool CrawlerTasklogic::OnRouterStatus(struct server* srv, int socket,
		struct PacketHead* packet, const void *msg, int32 len) {
	bool r = true;
	struct RouterStatus *status = (struct RouterStatus *) packet;
	base_logic::RouterScheduler scheduler;
	r = router_schduler_engine_->GetRouterSchduler(status->router_id,
			&scheduler);
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
		struct PacketHead* packet, const void *msg, int32 len) {
	bool r = true;
	struct RobotTaskStatus *status = (struct RobotTaskStatus *) packet;
	TaskSchdulerManager *task_manager =
			TaskSchdulerEngine::GetTaskSchdulerManager();
	LOG_DEBUG2("receive robot task status: task_type = %d, task_id = %d, status = %d",
			status->task_type, status->task_id, status->is_success);
	if (TASK_FAIL == status->is_success) {
		task_db_->UpdateCookie(status->cookie_id, 0);
		task_manager->RemoveInvalidCookie(status->cookie_id);
		LOG_MSG2("task[%d] exec error, error code: %s",
				status->task_id, status->error_no.c_str());
	}
	task_db_->UpdateRobotTaskState(status->task_id, (int) status->is_success,
			status->error_no);
	task_manager->AlterTaskState(status->task_id, TASK_EXECUED);
}

bool CrawlerTasklogic::OnRouterScheduleFail(struct server *srv, int socket,
			struct PacketHead* packet, const void *msg, int32 len) {
	bool r = true;
	struct RouterScheduleFailStatus *status =
			(struct RouterScheduleFailStatus*)packet;
	LOG_MSG2("router[%d] schedule fail, error code: %d",
			status->router_id, status->status);
}

}  // namespace crawler_task_logic


/*
 * robots_logic.cc
 *
 *  Created on: 2016年4月6日
 *      Author: Harvey
 */

#include "robot/robot_logic.h"
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include "logic/logic_unit.h"
#include "net/packet_processing.h"
#include "net/packet_define.h"
#include "net/comm_head.h"
#include "logic/logic_comm.h"
#include "basic/template.h"
#include "basic/scoped_ptr.h"
#include "robot/robot_engine.h"

#define HEART_STATE_DEBUG

#define DEFAULT_CONFIG_PATH     "./plugins/robot/robot_config.xml"
#define ONE_DAY_SEC             (24 * 60 * 60)

namespace robot_logic {

RobotLogic *RobotLogic::instance_ = NULL;

RobotLogic::RobotLogic()
        : is_prase_finsh_(false)
        , next_back_time_(0) {

    //初始化读写锁
    InitThreadrw(&lock_);

    if (!InitConfig()) {
        assert(0);
    }

    //初始化下次备份的时间
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    int curr_hour_sec = tm_now->tm_min * 60 + tm_now->tm_sec;
    if(tm_now->tm_hour > 23){
        next_back_time_ = now + ONE_DAY_SEC - curr_hour_sec;
    }else{
        next_back_time_ = now + (23 - tm_now->tm_hour) * 60 * 60 - curr_hour_sec;
    }

}

RobotLogic::~RobotLogic() {
    DeinitThreadrw(lock_);
}

RobotLogic *RobotLogic::GetInstance() {
    if (NULL == instance_) {
        instance_ = new RobotLogic();
    }
    return instance_;
}

void RobotLogic::FreeInstance() {
    delete instance_;
    instance_ = NULL;
}

void RobotLogic::SaveRobotPlugin(struct plugin *pl) {
    robot_pl_ = pl;
}

struct plugin *RobotLogic::GetRobotPlugin() {
    return robot_pl_;
}

int32 RobotLogic::GetRobotId() {
    return slb_agent_.client_id_;
}

bool RobotLogic::InitConfig() {

    FILE *fp = NULL;
    try {

        //读取连接服务器列表
        fp = fopen(DEFAULT_CONFIG_PATH, "r");
        if (NULL == fp) {
            LOG_MSG("robot_config.xml open failed");
            return false;
        }

        Json::Reader reader;
        Json::Value root;

        char* parg = new char[2048];
        while (true) {
            memset(parg, 0, 2048);
            if (feof(fp) != 0) {
                break;
            }

            if (fgets(parg, 2047, fp) == NULL) {
                continue;
            }

            if (!reader.parse(parg, root)) {
                LOG_MSG2("config reader.parse error data=", parg);
                continue;
            }

            LOG_MSG2("fgets parg = %s", parg);

            //初始化slb的连接方式
            slb_agent_.host_ = root["host"].asString();
            slb_agent_.port_ = root["port"].asInt();
            slb_agent_.conn_socket_ = -1;
            slb_agent_.is_register_ = false;
            slb_agent_.token_ = "";

            //初始化robot的mac， passwd
            slb_agent_.client_type_ = root["client_type"].asInt();
            slb_agent_.mac_ = root["mac"].asString();
            slb_agent_.passwd_ = root["passwd"].asString();
        }

        is_prase_finsh_ = true;
        delete[] parg;
    } catch (std::string &ex) {
        fclose(fp);
        LOG_MSG2("catch= %s", ex.c_str());
    }

    fclose(fp);
    fp = NULL;

    return true;
}

bool RobotLogic::OnRobotMessage(struct server *srv, const int socket,
                                const void *msg, const int len) {

    //消息合法性检查
    if (NULL == srv || socket < 0 || NULL == msg || len < PACKET_HEAD_LENGTH) {
        return false;
    }

    //解操作码
    int32 operate_code = -1;
    net::PacketProsess::UnpackOpcode(operate_code, msg, len);

    //根据操作码
    RobotEngine *robot_mgr = RobotEngine::GetInstance();
    switch (operate_code) {
        case S2C_ROBOT_REGISTER_SUCCESS: {
            RobotRegisterResult(socket, msg, len);
            break;
        }
        case R2S_LOGIN_ROUTER_RESULT: {
            LoginRouterResult(socket, msg, len);
            break;
        }
        case S2C_ALLOCATING_MULTI_ROBOT_TASK: {
            robot_mgr->MultiRobotTask(msg, len);
            break;
        }
        default:
            break;
    }

    return true;
}

void RobotLogic::RequestSLBRegister() {

    //如果router登录成功，则返回
    if (SUCCESS == router_agent_.is_login_ || !is_prase_finsh_) {
        return;
    }

    //如果与slb没有连接， 则重新连接slb
    if (slb_agent_.conn_socket_ < 0) {

        int conn_socket = ConnectServer(slb_agent_.host_, slb_agent_.port_);
        if (conn_socket < 0) {
            LOG_MSG("Connect SLB Server Failed, 5s second Retry");
            return;
        }

        //连接成功
        slb_agent_.conn_socket_ = conn_socket;
    }

    //注册
    RobotRequestRegister(slb_agent_.conn_socket_);

    return;
}

bool RobotLogic::RobotRequestRegister(const int socket) {

    struct RobotRegisterInfo robot_register_msg;
    MAKE_HEAD(robot_register_msg, C2S_ROBOT_REQUEST_REGISTER, 0, 0, 0, 0);
    robot_register_msg.crawler_type_ = 10;
    robot_register_msg.level = 1000;
    memset(robot_register_msg.passwd, 0, PASSWORD_SIZE);
    snprintf(robot_register_msg.passwd, PASSWORD_SIZE, "%s",
             slb_agent_.passwd_.c_str());
    memset(robot_register_msg.mac, 0, MAC_SIZE);
    snprintf(robot_register_msg.mac, MAC_SIZE, "%s", slb_agent_.mac_.c_str());

    bool r = false;
    r = send_message(socket, &robot_register_msg);
    if (!r) {
        return false;
    }

    LOG_MSG2("Robot Request Register SendMsg Suecess, ServerSocket = %d", socket);

    return true;
}

bool RobotLogic::RobotRegisterResult(const int socket, const void *msg,
                                     const int len) {

    bool r = false;

    struct RobotRegisterSuccess robot_reg_result;
    r = robot_reg_result.UnpackStream(msg, len);
    if (!r) {
        LOG_MSG("Robot Register Unpack Msg Failed");
        return false;
    }

    //判断是否注册成功
    if (NULL == robot_reg_result.token || strlen(robot_reg_result.token) <= 0) {
        LOG_MSG2("Robot Request Register Failed, token = %s", robot_reg_result.token);
        return false;
    }

    //判断分配的router是否合法
    if (NULL == robot_reg_result.router_ip
            || strlen(robot_reg_result.router_ip) <= 0) {
        LOG_MSG("SLB Not Allocate Router");
        return false;
    }

    //获得router的信息
    router_agent_.host_ = string(robot_reg_result.router_ip);
    router_agent_.port_ = robot_reg_result.router_port;
    router_agent_.conn_socket_ = -1;
    router_agent_.is_login_ = FAILED;

    //修改注册状态
    slb_agent_.client_id_ = robot_reg_result.crawler_id_;
    slb_agent_.is_register_ = true;
    slb_agent_.token_ = string(robot_reg_result.token);

    LOG_MSG2("Robot Register SLB Success, SLBHost = %s, Port = %d",
            slb_agent_.host_.c_str(), slb_agent_.port_);

    LOG_MSG2("Allocate Connect RouterIp = %s, RouterPort = %d",
            robot_reg_result.router_ip,
            robot_reg_result.router_port);

    //请求登录Router
    RequestLoginRouter();

    return true;
}

bool RobotLogic::RequestLoginRouter() {

    //如果已经登录则返回
    if (SUCCESS == router_agent_.is_login_) {
        return true;
    }

    //没有登录，先判断连接
    if (router_agent_.conn_socket_ <= 0) {
        router_agent_.conn_socket_ = ConnectServer(router_agent_.host_,
                                                   router_agent_.port_);
        if (router_agent_.conn_socket_ < 0) {
            LOG_MSG("Connect Router Server Failed, 5s second Retry");
            return false;
        }

        LOG_MSG2("Connect Router Success, RouterSocket = %d",
                router_agent_.conn_socket_);
    }

    //登录Router
    struct RobotRequestLoginRouter req_login_msg;
    MAKE_HEAD(req_login_msg, C2R_REQUEST_LOGIN_ROUTER, 0, 0, 0, 0);

    req_login_msg.crawler_id_ = slb_agent_.client_id_;
    req_login_msg.crawler_type_ = robot_type;
    memset(req_login_msg.token, 0, TOKEN_SIZE);
    snprintf(req_login_msg.token, TOKEN_SIZE, "%s", slb_agent_.token_.c_str());

    bool r = false;
    r = send_message(router_agent_.conn_socket_, &req_login_msg);
    if (!r) {
        LOG_MSG2("Send Router Login Msg Failed, RouterSocket = %d",
                router_agent_.conn_socket_);
        return false;
    }

    LOG_MSG2("Request Login Router, RouterSocket = %d", router_agent_.conn_socket_);

    return true;
}

bool RobotLogic::LoginRouterResult(const int socket, const void *msg,
                                   const int32 len) {

    bool r = false;
    struct RobotLoginRouterResult login_result_msg;
    r = login_result_msg.UnpackStream(msg, len);
    if (!r) {
        LOG_MSG("RobotLoginRouterResult Unpack Failed");
        return false;
    }

    //登录 router 失败
    if (SUCCESS != login_result_msg.is_success) {
        router_agent_.is_login_ = FAILED;
        LOG_MSG("Login Router Failed, 5s Retry");
        return false;
    }

    //断开与slb的连接
    closelockconnect(slb_agent_.conn_socket_);
    slb_agent_.conn_socket_ = -1;

    //登录Router成功
    router_agent_.is_login_ = SUCCESS;

    //向服务端发送未发送的任务
    RobotEngine::GetInstance()->CheckIsHaveFeedBackTask();

    LOG_MSG2("Login Router Success, RouterSocket = %d, Allocate RouterIp = %s, RouterPort = %d \n",
            socket, router_agent_.host_.c_str(), router_agent_.port_);

    return true;
}

int RobotLogic::ConnectServer(const std::string host, const int16 port) {

    struct server *srv = logic::CoreSoUtils::GetSRV();
    if (NULL == srv) {
        LOG_MSG("GetSRV failed");
        return NULL;
    }

    if (NULL == srv->create_connect_socket) {
        LOG_MSG("srv->create_connect_socket NULL");
        return NULL;
    }

    char str_port[sizeof(int16)];
    memset(str_port, 0, sizeof(int16));
    sprintf(str_port, "%d", port);

    //连接服务器
    struct sock_adapter *sock_adp = srv->create_connect_socket(srv,
                                                               host.c_str(),
                                                               str_port);
    if (NULL == sock_adp) {
        return -1;
    }

    return sock_adp->sock;
}

bool RobotLogic::OnIniTimer(struct server *srv) {
    if (NULL != srv->add_time_task) {
        LOG_MSG("Init Connect Server Timer");
        srv->add_time_task(srv, "robot", SLB_REGISTER_INTERVAL_TIMER, 5, -1);
        srv->add_time_task(srv, "robot", HEART_BEAT_CHECK_INTERVAL, 10, -1);
        srv->add_time_task(srv, "robot", STATE_REPORT_INTERVAL, 8, -1);
        srv->add_time_task(srv, "robot", BACKUP_LOG_FILE_INTERVAL, 5, -1);
        srv->add_time_task(srv, "robot", TEST_INTERVAL, 5, 1);
    }

    return true;
}

bool RobotLogic::OnTimeout(struct server *srv, char* id, int opcode, int time) {

    switch (opcode) {
        case SLB_REGISTER_INTERVAL_TIMER: {
            RequestSLBRegister();
            break;
        }
        case HEART_BEAT_CHECK_INTERVAL: {
            HeartBeatCheck();
            break;
        }
        case STATE_REPORT_INTERVAL: {
            ReportRobotState();
            break;
        }
        case BACKUP_LOG_FILE_INTERVAL: {
            BackUpLogFileTimer();
            break;
        }
        case TEST_INTERVAL:{
            //RobotEngine::GetInstance()->Test();
            break;
        }
        default:
            break;
    }

    return true;
}

bool RobotLogic::HeartBeatCheck() {

    if (router_agent_.conn_socket_ <= 0) {
        return false;
    }

    struct PacketHead heart_beat_packet;
    MAKE_HEAD(heart_beat_packet, C2S_HEART_BEAT_CHECK_MSG, 0, 0, 0, 0);

    heart_beat_packet.crawler_id_ = slb_agent_.client_id_;
    heart_beat_packet.crawler_type_ = robot_type;

    bool r = false;
    r = send_message(router_agent_.conn_socket_, &heart_beat_packet);
    if (!r) {
        RouterDisconnect();
        LOG_MSG("Send Router HeartBeat Msg Failed");
        return false;
    }

    string current_time = logic::SomeUtils::GetLocalTime(time(NULL));
    LOG_MSG2("HeartBeatCheck, connSocket = %d, RouterIp = %s, RouterPort = %d, currtime = %s",
            router_agent_.conn_socket_,
            router_agent_.host_.c_str(),
            router_agent_.port_,
            current_time.c_str());

    return true;
}

bool RobotLogic::OnBroadcastClose(struct server *srv, const int socket) {

    if (socket == slb_agent_.conn_socket_) {
        LOG_MSG("SLB Disconnect");
        slb_agent_.conn_socket_ = -1;
    }

    if (socket == router_agent_.conn_socket_) {
        RouterDisconnect();
    }

    return true;
}

void RobotLogic::RouterDisconnect() {

    closelockconnect(router_agent_.conn_socket_);

    std::string current_time = logic::SomeUtils::GetLocalTime(time(NULL));
    LOG_MSG2("Router Disconnect, CurrentTime = %s", current_time.c_str());
    router_agent_.conn_socket_ = -1;
    router_agent_.is_login_ = FAILED;
}

bool RobotLogic::ReportRobotState() {

    if (router_agent_.conn_socket_ < 0) {
        return false;
    }

    bool r = false;

    struct RobotStatePacket report_state_msg;
    MAKE_HEAD(report_state_msg, C2S_REPORT_STATE_MSG, 0, 0, 0, 0);

    report_state_msg.crawler_id_ = GetRobotId();
    report_state_msg.crawler_type_ = robot_type;

    report_state_msg.max_task_num = 10000;
    report_state_msg.curr_task_num = RobotEngine::GetInstance()
            ->GetCurrTaskQueueNum();

    r = send_message(router_agent_.conn_socket_, &report_state_msg);
    if (!r) {
        LOG_MSG("Send Router Curr Task State Failed");
        return false;
    }

    string current_time = logic::SomeUtils::GetLocalTime(time(NULL));
    LOG_MSG2("Send Router State Msg max_task_num = %d, curr_task_num = %d, currtime = %s",
            report_state_msg.max_task_num, report_state_msg.curr_task_num, current_time.c_str());

    return true;
}

bool RobotLogic::SendMsgToRouter(struct PacketHead *packet) {

    if (router_agent_.conn_socket_ <= 0 ||
    NULL == packet) {
        LOG_MSG2("Router ConnSocket = %d", router_agent_.conn_socket_);
        return false;
    }

    bool r = false;
    r = send_message(router_agent_.conn_socket_, packet);
    if (!r) {
        LOG_MSG2("Send Msg To Router Failed, Operacode = %d", packet->operate_code_);
        return false;
    }

    return true;
}

void RobotLogic::BackUpLogFileTimer() {

    time_t now = time(NULL);
    if (now >= next_back_time_) {

        FILE *fp = NULL;
        char buf[4096];
        std::string commond;

        //执行备份
        commond = "cp -f nohup.out " + logic::SomeUtils::GetBackUpFileName();
        fp = popen(commond.c_str(), "r");
        pclose(fp);

        //清空nohup
        commond = "cp /dev/null nohup.out";
        fp = popen(commond.c_str(), "r");
        pclose(fp);

        next_back_time_ += ONE_DAY_SEC;
        LOG_DEBUG2("Next BackUp LogFile Time = %d", next_back_time_);
    }
}

} /* namespace robot_logic */

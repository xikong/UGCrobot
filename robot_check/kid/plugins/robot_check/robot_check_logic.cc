/*
 * robot_check_logic.cc
 *
 *  Created on: 2016年5月3日
 *      Author: Harvey
 */

#include "robot_check_logic.h"
#include <fcntl.h>
#include <string>
#include <time.h>
#include "logic/logic_comm.h"
#include "net/comm_head.h"
#include "net/packet_processing.h"
#include "net/packet_define.h"
#include "core/common.h"
#include "json/reader.h"
#include "json/value.h"
#include "robot_check/robot_check_engine.h"

#define DEFAULT_CONFIG_PATH     "./plugins/robot_check/robot_check_config.json"

namespace robot_check_logic {

RobotCheckLogic *RobotCheckLogic::instance_ = NULL;

RobotCheckLogic::RobotCheckLogic()
 : next_check_time_(0),
   is_parse_finsh_(false) {
    if(!Init()){
        assert(0);
    }
}

RobotCheckLogic::~RobotCheckLogic() {

}

RobotCheckLogic *RobotCheckLogic::GetInstance(){
    if(NULL == instance_){
        instance_ = new RobotCheckLogic();
    }
    return instance_;
}

void RobotCheckLogic::FreeInstance(){
    delete instance_;
    instance_ = NULL;
}

bool RobotCheckLogic::Init(){

    bool r = false;
    FILE *fp = NULL;

    try{
        fp = fopen(DEFAULT_CONFIG_PATH, "r");
        if(NULL == fp){
            LOG_MSG("robot_config.xml open failed");
            return false;
        }

        Json::Reader reader;
        Json::Value root;

        char* parg = new char[2048];
        while(true) {

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

            //获取日志文件
            std::string filepath = root["task_path"].asString();
            int check_interval = root["check_interval"].asInt();

            //打开日志文件
            r = RobotCheckEngine::GetInstance()->OpenTaskLogFile(filepath, check_interval);
            if(!r){
                LOG_MSG2("Init TaskLogFile Failed, task_log_file_path = %s", filepath.c_str());
                continue;
            }
        }

        is_parse_finsh_ = true;
        delete[] parg;
    }
    catch(std::string &ex){
        LOG_MSG2("catch ex = %s", ex.c_str());
        fclose(fp);
        return false;
    }

    fclose(fp);
    return true;
}

bool RobotCheckLogic::OnIniTimer(struct server *srv){

    if(NULL != srv->add_time_task){
        srv->add_time_task(srv, "robot_check", TASK_LOG_CHECK_INTERVAL, 1, -1);
    }

    return true;
}

bool RobotCheckLogic::OnTimeout(struct server *srv, char* id, int opcode, int srv_time){

    time_t current_time = time(NULL);
    switch(opcode){
    case TASK_LOG_CHECK_INTERVAL:{
        TaskLogCheckTimer(current_time);
        break;
    }
    default:
        break;
    }
    return true;
}

void RobotCheckLogic::TaskLogCheckTimer(int current_time){

    //当前时间大于下次检查的时间，检查
    if(current_time > next_check_time_ && is_parse_finsh_){

        //开始检查
        RobotCheckEngine::GetInstance()->CheckRobotTaskValid(current_time);

        //下次检查时间
        current_time = time(NULL);
        next_check_time_ = current_time + 5;
    }
}

void RobotCheckLogic::SaveRobotCheckPlugin(struct plugin *pl){
    robot_check_pl_ = pl;
}

struct plugin *RobotCheckLogic::GetRobotCheckPlugin(){
    return robot_check_pl_;
}

} /* namespace robot_check_logic */

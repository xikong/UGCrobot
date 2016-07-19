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
#include <sstream>
#include <iostream>
#include <fstream>
#include <regex>

#include "logic/logic_comm.h"
#include "net/comm_head.h"
#include "net/packet_processing.h"
#include "net/packet_define.h"
#include "core/common.h"
#include "json/reader.h"
#include "json/value.h"
#include "robot_check/robot_check_engine.h"

#define DEFAULT_CONFIG_PATH     "./plugins/robot_check/robot_check_config.xml"

namespace robot_check_logic {

RobotCheckLogic *RobotCheckLogic::instance_ = NULL;

RobotCheckLogic::RobotCheckLogic()
        : load_num_(0),
          comlete_num_(0),
          check_date_("") {
    if (!Init()) {
        assert(0);
    }
}

RobotCheckLogic *RobotCheckLogic::GetInstance() {
    if (NULL == instance_) {
        instance_ = new RobotCheckLogic();
    }
    return instance_;
}

void RobotCheckLogic::FreeInstance() {
    delete instance_;
    instance_ = NULL;
}

bool RobotCheckLogic::Init() {
    robot_check_db_.reset(new RobotCheckDB());
    bool bParseResult = false;
    Json::Reader reader;
    Json::Value root;
    std::ifstream is;
    is.open(DEFAULT_CONFIG_PATH, std::ios::binary);
    if (reader.parse(is, root)) {
        int mysql_size = root["mysql"].size();
        for (int i = 0; i < mysql_size; ++i) {
            Json::Value mysql_val = root["mysql"][i];
            string host = mysql_val["host"].asString();
            int port = mysql_val["port"].asInt();
            string user = mysql_val["user"].asString();
            string pass = mysql_val["pass"].asString();
            string name = mysql_val["name"].asString();
            base::ConnAddr mysql_addr(host, port, user, pass, name);

            //Init Mysql
            config::FileConfig config;
            config.mysql_db_list_.push_back(mysql_addr);
            base_logic::DataControllerEngine::Init(&config);
            bParseResult = true;
            break;
        }

        check_date_ = root["check_date"].asString();
        if (check_date_.empty()) {
            LOG_ERROR("Check date empty");
            bParseResult = false;
        }
    } else {
        LOG_ERROR("Json::Reader Parse Config Failed");
    }

    is.close();
    return bParseResult;
}

bool RobotCheckLogic::OnIniTimer(struct server *srv ) {
    if (NULL != srv->add_time_task) {
        srv->add_time_task(srv, "robot_check", TASK_LOG_CHECK_INTERVAL, 2, 1);
    }
    return true;
}

bool RobotCheckLogic::OnTimeout(struct server *srv, char* id, int opcode,
                                int srv_time ) {

    time_t current_time = time(NULL);
    switch (opcode) {
        case TASK_LOG_CHECK_INTERVAL: {
            if (!LoadTaskFromDb()) {
                LOG_ERROR2("Check date valid must like 2016-07-19, check_date = %s",
                        check_date_.c_str());
                exit(0);
            }
            break;
        }
        default:
            break;
    }
    return true;
}

void RobotCheckLogic::SaveRobotCheckPlugin(struct plugin *pl ) {
    robot_check_pl_ = pl;
}

struct plugin *RobotCheckLogic::GetRobotCheckPlugin() {
    return robot_check_pl_;
}

bool RobotCheckLogic::LoadTaskFromDb() {
    // 2016-07-19
    if (check_date_.size() != 10) {
        return false;
    }
    if (check_date_[4] != '-' || check_date_[7] != '-') {
        return false;
    }
    string year = check_date_.substr(0, 3);
    if (0 == atoi(year.c_str())) {
        return false;
    }
    string month = check_date_.substr(5, 6);
    if (0 == atoi(month.c_str())) {
        return false;
    }
    string day = check_date_.substr(8, 9);
    if (0 == atoi(day.c_str())) {
        return false;
    }

    std::list<RobotTask *> list;
    robot_check_db_->FetchTodayTask(list, check_date_);
    if (list.size() <= 0) {
        LOG_MSG("Today RobotTask is Empty");
        exit(1);
    }

    RobotCheckEngine *engine = RobotCheckEngine::GetInstance();
    while (list.size() > 0) {
        RobotTask *task = list.front();
        list.pop_front();
        engine->PushNewCheckTaskInQueue(task);
        LOG_DEBUG2("Load RobotTask task_id = %lld, url = %s", task->task_id_, task->url_.c_str());
        ++load_num_;
    } LOG_DEBUG2("Load RobotCheckTask From DB TaskNum = %d", load_num_);
    return true;
}

void RobotCheckLogic::UpdateRobotTaskStatus(RobotTask &task ) {
    robot_check_db_->RecordRobotTaskStatus(task);
    ++comlete_num_;
    if (comlete_num_ == load_num_) {
        LOG_DEBUG2("All RobotCheckTask Complete, TaskNum = %d", load_num_);
        exit(0);
    }
}

} /* namespace robot_check_logic */

/*
 * task_factory.cc
 *
 *  Created on: 2016年4月26日
 *      Author: Harvey
 */

#include "task_engine.h"
#include "net/comm_head.h"

using namespace base_logic;

extern TaskEngine *GetTaskEngineByType(const int task_type){

    TaskEngine *engine = NULL;

    switch(task_type){
    case TASK_WEIBO:{
        engine = TaskWeiBoEngine::GetInstance();
        break;
    }
    case TASK_TIANYA:{
        engine = TaskTianYaEngine::GetInstance();
        break;
    }
    case TASK_TIEBA:{
        engine = TaskTieBaEngine::GetInstance();
        break;
    }
    case TASK_QQ:{
        engine = TaskQQEngine::GetInstance();
        break;
    }
    case TASK_MOP:{
        engine = TaskMopEngine::GetInstance();
        break;
    }
    case TASK_DOUBAN:{
        engine = TaskDouBanEngine::GetIntance();
        break;
    }
    default:
        LOG_MSG2("Not Suppot This TaskType = %d task", task_type);
        break;
    }

    return engine;
}




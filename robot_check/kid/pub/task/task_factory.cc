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
    case TASK_TIEBA:{
        engine = TaskTieBaEngine::GetInstance();
        break;
    }
    default:
        LOG_MSG2("Not Suppot This TaskType = %d task", task_type);
        break;
    }

    return engine;
}





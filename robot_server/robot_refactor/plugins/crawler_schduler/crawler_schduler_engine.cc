//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月14日 Author: kerry

#include "crawler_schduler/crawler_schduler_engine.h"
#include <errno.h>
#include <list>
#include <string>
#include <ctime>
#include "net/comm_head.h"
#include "net/packet_processing.h"
#include "logic/logic_comm.h"
#include "logic/logic_unit.h"
#include "basic/template.h"

#define DEFAULT_CONFIG_PATH  "./plugins/schduler_config.xml"

router_schduler::SchdulerEngine   *GetRouterSchdulerEngine(void) {
    return new router_schduler::SchdulerEngineImpl();
}

namespace router_schduler {

bool SchdulerEngineImpl::SetSchduler(const int32 id, void* schduler) {
    return SetRouterSchduler(id, (base_logic::RouterScheduler*)(schduler));
}


bool SchdulerEngineImpl::SetRouterSchduler(const int32 id,
        base_logic::RouterScheduler* schduler) {
    RouterSchdulerManager* schduler_mgr =
            RouterSchdulerEngine::GetRouterSchdulerManager();
    return schduler_mgr->SetRouterSchduler(id, schduler);
}

bool SchdulerEngineImpl::GetRouterSchduler(const int32 id,
        base_logic::RouterScheduler* schduler) {
    RouterSchdulerManager* schduler_mgr =
            RouterSchdulerEngine::GetRouterSchdulerManager();
    return schduler_mgr->GetRouterSchduler(id, schduler);
}

bool SchdulerEngineImpl::DelRouterSchduler(const int32 id) {
    RouterSchdulerManager* schduler_mgr =
            RouterSchdulerEngine::GetRouterSchdulerManager();
    return schduler_mgr->DelRouterSchduler(id);
}


bool SchdulerEngineImpl::FindRouterSchduler(const int socket,
    base_logic::RouterScheduler* schduler) {
    RouterSchdulerManager* schduler_mgr =
            RouterSchdulerEngine::GetRouterSchdulerManager();
    return schduler_mgr->FindRouterSchduler(socket, schduler);
}

bool SchdulerEngineImpl::CloseRouterSchduler(const int socket) {
/*
    RouterSchdulerManager* schduler_mgr =
            RouterSchdulerEngine::GetRouterSchdulerManager();
    return schduler_mgr->CloseRouterSchduler(socket);
*/
}

bool SchdulerEngineImpl::SetRecvTime(const int socket) {
/*
    RouterSchdulerManager* schduler_mgr =
            RouterSchdulerEngine::GetRouterSchdulerManager();
    return schduler_mgr->SetRecvTime(socket);
*/
}

bool SchdulerEngineImpl::SetSendTime(const int socket) {
/*
    RouterSchdulerManager* schduler_mgr =
            RouterSchdulerEngine::GetRouterSchdulerManager();
    return schduler_mgr->SetRecvTime(socket);
*/
}

bool SchdulerEngineImpl::CheckHeartPacket(const int socket) {
/*
    RouterSchdulerManager* schduler_mgr =
            RouterSchdulerEngine::GetRouterSchdulerManager();
    return schduler_mgr->CheckHeartPacket(socket);
*/
}

bool SchdulerEngineImpl::SendOptimalRouter(const void* data,
        const int32 len, int16 crawler_type) {
    RouterSchdulerManager* schduler_mgr =
                        RouterSchdulerEngine::GetRouterSchdulerManager();
    return schduler_mgr->SendOptimalRouter(data, len, crawler_type);
}

bool SchdulerEngineImpl::CheckOptimalRouter(int crawler_type) {
    RouterSchdulerManager* schduler_mgr =
            RouterSchdulerEngine::GetRouterSchdulerManager();
    return schduler_mgr->CheckOptimalRouter(crawler_type);
}

bool SchdulerEngineImpl::SetSendErrorCount(int socket) {
/*
    RouterSchdulerManager* schduler_mgr =
            RouterSchdulerEngine::GetRouterSchdulerManager();
    return schduler_mgr->SetSendErrorCount(socket);
*/
}

bool SchdulerEngineImpl::SetRecvErrorCount(int socket) {
/*
    RouterSchdulerManager* schduler_mgr =
            RouterSchdulerEngine::GetRouterSchdulerManager();
    return schduler_mgr->SetRecvErrorCount(socket);
*/
}

bool SchdulerEngineImpl::SetRouterIdToRouter(uint32 router_id, int socket) {
    RouterSchdulerManager* schduler_mgr =
            RouterSchdulerEngine::GetRouterSchdulerManager();
    return schduler_mgr->SetRouterIdToRouter(router_id, socket);
}

void SchdulerEngineImpl::CheckIsEffective() {
    RouterSchdulerManager* schduler_mgr =
            RouterSchdulerEngine::GetRouterSchdulerManager();
    schduler_mgr->CheckIsEffective();
}

RouterSchdulerManager* RouterSchdulerEngine::schduler_mgr_ = NULL;
RouterSchdulerEngine* RouterSchdulerEngine::schduler_engine_ = NULL;


RouterSchdulerManager::RouterSchdulerManager() {
    schduler_cache_ = new RouterSchedulerCache();
    Init();
}

RouterSchdulerManager::~RouterSchdulerManager() {
    DeinitThreadrw(lock_);
}


void RouterSchdulerManager::Init() {
    InitThreadrw(&lock_);

    bool r = false;
    std::string path = DEFAULT_CONFIG_PATH;
    config::FileConfig* config = config::FileConfig::GetFileConfig();
    if (config == NULL) {
        return;
    }
    r = config->LoadConfig(path);
}

bool RouterSchdulerManager::SetRouterSchduler(const int32 id,
         base_logic::RouterScheduler* schduler) {
    base_logic::WLockGd lk(lock_);

    int socket = schduler->socket();
    RouterSchedulerSocketMap::iterator it = schduler_cache_->router_scheduler_socket_map_.find(socket);
    if (schduler_cache_->router_scheduler_socket_map_.end() != it) {
        LOG_DEBUG2("find old socket reconnected, socket=%d", socket);
        return false;
    }

    schduler_cache_->router_scheduler_list_.push_back((*schduler));

    base::MapAdd<RouterSchedulerSocketMap, int, base_logic::RouterScheduler>(
                schduler_cache_->router_scheduler_socket_map_, schduler->socket(),
                (*schduler));

    return base::MapAdd<RouterSchedulerIdMap, int32, base_logic::RouterScheduler>(
            schduler_cache_->router_scheduler_id_map_, id, (*schduler));
}

bool RouterSchdulerManager::GetRouterSchduler(const int32 id,
         base_logic::RouterScheduler* schduler) {
    base_logic::RLockGd lk(lock_);
    return base::MapGet<RouterSchedulerIdMap,RouterSchedulerIdMap::iterator,
            int32, base_logic::RouterScheduler>(
            schduler_cache_->router_scheduler_id_map_, id, (*schduler));
}

bool RouterSchdulerManager::DelRouterSchduler(const int32 id) {
    base_logic::WLockGd lk(lock_);
    base_logic::RouterScheduler schduler;
    base::MapGet<RouterSchedulerIdMap, RouterSchedulerIdMap::iterator,
                int32, base_logic::RouterScheduler>(
                schduler_cache_->router_scheduler_id_map_, id, schduler);
    schduler.set_is_effective(false);
    base::MapDel<RouterSchedulerSocketMap, RouterSchedulerSocketMap::iterator, int>(
                schduler_cache_->router_scheduler_socket_map_, schduler.socket());
    return base::MapDel<RouterSchedulerIdMap, RouterSchedulerIdMap::iterator, int32>(
            schduler_cache_->router_scheduler_id_map_, id);
}

bool RouterSchdulerManager::FindRouterSchduler(const int socket,
        base_logic::RouterScheduler* schduler) {
    base_logic::RLockGd lk(lock_);
    return base::MapGet<RouterSchedulerSocketMap, RouterSchedulerSocketMap::iterator,
            int32, base_logic::RouterScheduler>(
            schduler_cache_->router_scheduler_socket_map_, socket, (*schduler));
}

bool RouterSchdulerManager::CloseRouterSchduler(int socket) {
    base_logic::WLockGd lk(lock_);
    base_logic::RouterScheduler schduler;
    base::MapGet<RouterSchedulerSocketMap, RouterSchedulerSocketMap::iterator,
            int32, base_logic::RouterScheduler>(
            schduler_cache_->router_scheduler_socket_map_, socket, schduler);
    schduler.set_is_effective(false);
    base::MapDel<RouterSchedulerSocketMap, RouterSchedulerSocketMap::iterator, int>(
                  schduler_cache_->router_scheduler_socket_map_, socket);
    return base::MapDel<RouterSchedulerIdMap, RouterSchedulerIdMap::iterator, int32>(
            schduler_cache_->router_scheduler_id_map_, schduler.id());
}

bool RouterSchdulerManager::SetSendTime(int socket) {
    base_logic::WLockGd lk(lock_);
    base_logic::RouterScheduler& schduler = schduler_cache_->router_scheduler_socket_map_[socket];
    schduler.set_send_last_time(time(NULL));
    return true;

}


bool RouterSchdulerManager::SetRecvTime(int socket) {
    base_logic::WLockGd lk(lock_);
    base_logic::RouterScheduler& schduler = schduler_cache_->router_scheduler_socket_map_[socket];
    schduler.set_recv_last_time(time(NULL));
    return true;
}

bool RouterSchdulerManager::SetSendErrorCount(int socket) {
    base_logic::WLockGd lk(lock_);
    base_logic::RouterScheduler& schduler = schduler_cache_->router_scheduler_socket_map_[socket];
    schduler.add_send_error_count();
    return true;
}

bool RouterSchdulerManager::SetRecvErrorCount(int socket) {
    base_logic::WLockGd lk(lock_);
    base_logic::RouterScheduler& schduler = schduler_cache_->router_scheduler_socket_map_[socket];
    schduler.add_recv_error_count();
    return true;
}

bool RouterSchdulerManager::SendOptimalRouter(const void* data,
        const int32 len, int16 crawler_type) {
    base_logic::WLockGd lk(lock_);
    LOG_MSG2("router count: %d", schduler_cache_->router_scheduler_list_.size());
    if (schduler_cache_->router_scheduler_list_.size() <= 0) {
        return false;
    }

    base_logic::RouterScheduler schduler;
    schduler.set_idle_tasks(crawler_type, 0);
    RouterSchedulerList::iterator it = schduler_cache_->router_scheduler_list_.begin();
    LOG_DEBUG("-------------------------router task count begin-------------------------");
    for (; it != schduler_cache_->router_scheduler_list_.end(); it++) {
	LOG_DEBUG2("router_id = %d, crawler_type = %d, idle tasks = %d", it->id(), crawler_type,  it->idle_tasks(crawler_type));
        if (!(*it).is_effective() || !(*it).has_crawler_type(crawler_type)) {
	    LOG_DEBUG2("router[%d] is not effective or has no crawler type: %d", it->id(), crawler_type);
	    continue;
        }
	if (!base_logic::superior_to(schduler, *it, crawler_type)) {
		schduler = *it;
		LOG_DEBUG("schduler superior to *it");
	}
    }
    LOG_DEBUG("------------------------- router task count end -------------------------");

    LOG_DEBUG2("schduler->idle_tasks=%d", schduler.idle_tasks(crawler_type));
    if (schduler.idle_tasks(crawler_type) == 0)
        return false;
    struct PacketHead* packet = (struct PacketHead*)data;
    struct AssignmentMultiTask* multi_task =
            (struct AssignmentMultiTask*)packet;
    multi_task->crawler_type = crawler_type;

    if (!send_message(schduler.socket(), packet)) {
        schduler.add_send_error_count();
        schduler.set_is_effective(false);
        LOG_MSG2("schduler.socket()=%d,error msg=%s", (int)schduler.socket(), strerror(errno));
	} else {
		time_t t = time(NULL);
		LOG_MSG2("[%s] Assign tasks to router[%d], socket = %d", 
				ctime(&t), schduler.id(), schduler.socket());
	}
    return true;
}

bool RouterSchdulerManager::SetRouterIdToRouter(uint32 router_id, int socket) {
/*
    base_logic::WLockGd lk(lock_);
    RouterSchedulerIdMap::iterator it = schduler_cache_->router_scheduler_id_map_.find(router_id);
    if (schduler_cache_->router_scheduler_id_map_.end() != it) {
        LOG_DEBUG2("find repeat router id: %d, router socket: %d", router_id, socket);
        return false;
    }
    return base::MapAdd<RouterSchedulerIdMap, uint32, int>(
            schduler_cache_->router_scheduler_id_map_, router_id, socket);
*/
}


bool RouterSchdulerManager::CheckOptimalRouter(int crawler_type) {
    base_logic::RLockGd lk(lock_);
//    base_logic::RouterScheduler
    RouterSchedulerList::iterator it = schduler_cache_->router_scheduler_list_.begin();
    for (; it != schduler_cache_->router_scheduler_list_.end(); ++it) {
    	if (it->has_crawler_type(crawler_type) && it->idle_tasks(crawler_type) > 0)
    		return true;
    }
    return false;
}


bool RouterSchdulerManager::CheckHeartPacket(const int socket) {
    time_t current_time = time(NULL);
    bool r = false;
    base_logic::WLockGd lk(lock_);

    if(0 != socket) {
    	//update recv_last_time
    	base_logic::RouterScheduler& router_schduler = schduler_cache_->router_scheduler_socket_map_[socket];
    	base_logic::RouterScheduler& router_schduler_from_schduler_map = schduler_cache_->router_scheduler_id_map_[router_schduler.id()];
    	router_schduler_from_schduler_map.set_recv_last_time(current_time);
		LOG_DEBUG2("location of router_schduler = %p set crawler schduler recv_last_time socket=%d current_time=%d recv_last_time=%d",
				&router_schduler_from_schduler_map, socket, (int)current_time, router_schduler_from_schduler_map.recv_last_time());
    	return true;
    }
    RouterSchedulerIdMap::iterator it =
            schduler_cache_->router_scheduler_id_map_.begin();
    for (; it != schduler_cache_->router_scheduler_id_map_.end(); it++) {
        base_logic::RouterScheduler& schduler = it->second;
        if((current_time - schduler.recv_last_time() > 300)) {
        	schduler.add_send_error_count();
             LOG_DEBUG2("location of schduler=%p current_time=%d router_schduler out of time %d socket=%d send_error_count=%d",
             		&schduler, (int)current_time, (int)schduler.recv_last_time(), schduler.socket(), schduler.send_error_count());
                }

        if (schduler.send_error_count() > 3) {
        LOG_DEBUG("close connection");
            schduler.set_is_effective(false);
            base::MapDel<RouterSchedulerSocketMap, RouterSchedulerSocketMap::iterator, int>(
                          schduler_cache_->router_scheduler_socket_map_, schduler.socket());
            base::MapDel<RouterSchedulerIdMap, RouterSchedulerSocketMap::iterator, int32>(
                    schduler_cache_->router_scheduler_id_map_, schduler.id());
            closelockconnect(schduler.socket());
            continue;
        }
    }
    return true;
}

void RouterSchdulerManager::CheckIsEffective() {
    base_logic::WLockGd lk(lock_);
    time_t current_time = time(NULL);
    RouterSchedulerList::iterator it =
            schduler_cache_->router_scheduler_list_.begin();
    for (; it != schduler_cache_->router_scheduler_list_.end();) {
        base_logic::RouterScheduler schduler = (*it);
        if (schduler.recv_last_time()+TIMEOUT_TICK < current_time) {
        	schduler.set_is_effective(false);
        	LOG_MSG2("don't receive router[%d] status more than %d s, erase it",
        			schduler.id(), TIMEOUT_TICK);
        }
        if (!schduler.is_effective()) {
            base::MapDel<RouterSchedulerSocketMap, RouterSchedulerSocketMap::iterator, int>(
                          schduler_cache_->router_scheduler_socket_map_, schduler.socket());
            base::MapDel<RouterSchedulerIdMap, RouterSchedulerSocketMap::iterator, int32>(
                    schduler_cache_->router_scheduler_id_map_, schduler.id());
            schduler_cache_->router_scheduler_list_.erase(it++);
        }
        else
            it++;
    }
}

}  //  namespace router_schduler

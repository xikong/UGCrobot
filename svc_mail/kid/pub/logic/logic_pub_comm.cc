/*
 * logic_comm.cc
 *
 *  Created on: 2016年5月3日
 *      Author: Harvey
 */

#include "logic/logic_pub_comm.h"

namespace base_logic {

void RFileLockGd::RLockInit(FILE *fp){
    m_fd = fileno(fp);
    m_lock = (struct flock *) malloc(sizeof(struct flock));

    m_lock->l_type = F_RDLCK;
    m_lock->l_start = 0;
    m_lock->l_len = 0;
    m_lock->l_whence = SEEK_SET;
    m_lock->l_pid = getpid();
}

void WFileLockGd::WLockInit(FILE *fp){
    m_fd = fileno(fp);
    m_lock = (struct flock *) malloc(sizeof(struct flock));

    m_lock->l_type = F_WRLCK;
    m_lock->l_start = 0;
    m_lock->l_len = 0;
    m_lock->l_whence = SEEK_SET;
    m_lock->l_pid = getpid();
}


} /* namespace base_logic */

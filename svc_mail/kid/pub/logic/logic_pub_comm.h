/*
 * logic_comm.h
 *
 *  Created on: 2016年5月3日
 *      Author: Harvey
 */

#ifndef KID_PUB_LOGIC_LOGIC_PUB_COMM_H_
#define KID_PUB_LOGIC_LOGIC_PUB_COMM_H_

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

namespace base_logic {

class RFileLockGd {
 public:
    RFileLockGd(FILE *fp) {
        RLockInit(fp);
        fcntl(m_fd, F_SETLKW, &m_lock);
    }

    ~RFileLockGd() {
        m_lock->l_type = F_UNLCK;
        fcntl(m_fd, F_SETLK, &m_lock);
        free(m_lock);
    }

 private:
    void RLockInit(FILE *fp);

 private:
    int m_fd;
    struct flock *m_lock;
};

class WFileLockGd {
 public:
    WFileLockGd(FILE *fp) {
        WLockInit(fp);
        fcntl(m_fd, F_SETLKW, &m_lock);
    }

    ~WFileLockGd() {
        m_lock->l_type = F_UNLCK;
        fcntl(m_fd, F_SETLK, &m_lock);
        free(m_lock);
    }

 private:
    void WLockInit(FILE *fp);
 private:
    int m_fd;
    struct flock *m_lock;
};

} /* namespace base_logic */

#endif /* KID_PUB_LOGIC_LOGIC_PUB_COMM_H_ */

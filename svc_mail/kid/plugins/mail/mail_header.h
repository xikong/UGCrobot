/*
 * mail_header.h
 *
 *  Created on: 2016年6月14日
 *      Author: Harvey
 */

#ifndef KID_PLUGINS_MAIL_MAIL_HEADER_H_
#define KID_PLUGINS_MAIL_MAIL_HEADER_H_

#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <string>
#include <iostream>
#include <map>
#include <queue>
#include <vector>
#include <list>
#include <set>
#include <sstream>

#include "core/common.h"
#include "basic/basictypes.h"
#include "logic/logic_unit.h"
#include "logic/logic_comm.h"
#include "basic/scoped_ptr.h"
#include "json/json.h"
#include "logic/auto_crawler_infos.h"

using std::string;
#define MAXLINE  1024
namespace mail_logic{


const string        MAIL_OK = "250";
const int           INVALID_SOCK = -1;

enum eMailType{
    MT_UNKNOWON,
    MT_163,
    MT_QQ,
    MT_KUNYAN,
};

typedef struct MailAccountInfo{
    string      username_;
    string      password_;
    string      hostname_;
} MailAccountInfo;

typedef struct MailTask{

    MailTask()
            : from_(""),
              to_(""),
              mail_content_id_(-1) {

    }

    ~MailTask(){
        LOG_DEBUG("MailTask Destroy");
    }

    string from_;
    string to_;
    uint64 mail_content_id_;
} MailTask;

} /* namespace mail_logic */


#endif /* KID_PLUGINS_MAIL_MAIL_HEADER_H_ */

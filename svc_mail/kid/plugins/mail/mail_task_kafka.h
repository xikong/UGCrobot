/*
 * mail_task_kafka.h
 *
 *  Created on: 2016年6月13日
 *      Author: Harvey
 */

#ifndef KID_PLUGINS_MAIL_MAIL_TASK_KAFKA_H_
#define KID_PLUGINS_MAIL_MAIL_TASK_KAFKA_H_

#include "mail/mail_header.h"

#include "logic/base_values.h"
#include "queue/kafka_consumer.h"

namespace mail_logic {

class MailTaskConsumer{
 public:
    MailTaskConsumer();
    ~MailTaskConsumer();

 public:
    void FectchTasks(std::list<string> *list);

 private:
    bool Initialize();

 private:
    kafka_consumer      kafka_consumer_;
};

} /* namespace mail_logic */

#endif /* KID_PLUGINS_MAIL_MAIL_TASK_KAFKA_H_ */

#define CONSUMER_INIT_FAILED -1
#define CONSUMER_INIT_SUCCESS 0
#define CONSUMER_CONFIG_ERROR 3
#define PULL_DATA_SUCCESS 0
#define PULL_DATA_FAILED 1
#define PULL_DATA_TIMEOUT 2

kafka_consumer_::kafka_consumer_() {

}

kafka_consumer_::~kafka_consumer_() {
	rd_kafka_consume_stop(rkt_, partition_);
	rd_kafka_topic_destroy(rkt_);
	rd_kafka_destroy(rk_);
}

int kafka_consumer_::Init(const int partition, const char* topic, const char* brokers, MsgConsume msg_consume)
{
	char err_str[512];
	partition_ = partition;
	msg_consume_ = msg_consume;

	printf("partition=%d, topic=%s, brokers=%s\n", partition, topic, brokers);
	rd_kafka_conf_t *conf = rd_kafka_conf_new();

	if (NULL == conf)
		return CONSUMER_INIT_FAILED;
	if (RD_KAFKA_CONF_OK != rd_kafka_conf_set(conf, "group.id", "one", err_str, sizeof(err_str)))
		return CONSUMER_INIT_FAILED;

//	rd_kafka_conf_set(conf, "queued.min.messages", "1000000", NULL, 0);

	if (!(rk_ = rd_kafka_new(RD_KAFKA_CONSUMER, conf,
		err_str, sizeof(err_str)))) {
		printf("%% Failed to create new consumer: %s\n",err_str);
		return CONSUMER_INIT_FAILED;
	}
	//rd_kafka_set_log_level(rk_, LOG_DEBUG);
	if (rd_kafka_brokers_add(rk_, brokers) == 0) {
		printf("%% No valid brokers specified\n");
		return CONSUMER_INIT_FAILED;
	}
	rd_kafka_topic_conf_t *topic_conf = rd_kafka_topic_conf_new();

	rkt_ = rd_kafka_topic_new(rk_, topic, topic_conf);
	if (NULL == rkt_)
	{
		printf("topic creat failed\n");
		return CONSUMER_INIT_FAILED;
	}
  printf("rkt_=%p,partition=%d\n", rkt_, partition);

	if (rd_kafka_consume_start(this->rkt_, partition, RD_KAFKA_OFFSET_END) == -1){
		printf("Failed to start consuming:");
		return CONSUMER_INIT_FAILED;
	}
	return CONSUMER_INIT_SUCCESS;
}

int kafka_consumer_::PullData(std::string& data)
{

	rd_kafka_message_t *rkmessage;
	int consume_result = PULL_DATA_SUCCESS;

	printf("217consumer_info->rk:%p, rkt:%p, partition=%d\n", rk_, rkt_, partition_);
	printf("217 begin poll\n");
	rd_kafka_poll(rk_, 0);
	printf("219begin consume\n");
	rkmessage = rd_kafka_consume(rkt_, partition_, 1000);
	if (!rkmessage) 
		return PULL_DATA_TIMEOUT;
	printf("rkmessage->err:%d\n", (int)rkmessage->err);
	if (rkmessage->err) {
		if (rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION ||
				rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC)
			return CONSUMER_CONFIG_ERROR;
		return PULL_DATA_FAILED;
	}
	data = std::string((const char*)rkmessage->payload, rkmessage->len);
	rd_kafka_message_destroy(rkmessage);

	return consume_result;
}

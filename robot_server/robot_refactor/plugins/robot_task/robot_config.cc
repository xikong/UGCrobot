/*
 * config.cc
 *
 *  Created on: 2016年5月26日
 *      Author: zjc
 */

#include "robot_config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sstream>

#include "logic/logic_comm.h"
#include "logic/base_values.h"

namespace robot_task_logic {

Config* Config::config_ = NULL;

const int MAX_BUFFER_SIZE = 64 * 1024;

Config::Config(const char *filename)
    : assign_task_tick(60),
      fetch_task_tick(10),
      recycle_task_tick(20),
      clean_no_effective_client_tick(20),
      fetch_ip_tick(60),
      flush_cookie_use_time_tick(60),
      fetch_content_tick(60),
      fetch_cookies_tick(60),
      reply_self_state_tick(20),
      tieba_task_tick(10 * 60),
      weibo_task_tick(10 * 60),
      tianya_task_tick(10 * 60),
      qzone_task_tick(10 * 60),
      maopu_task_tick(10 * 60),
      douban_task_tick(10 * 60),
      taoguba_task_tick(10 * 60),
      snowball_task_tick(10 * 60),
      tieba_cookie_tick(24 * 60 * 60),
      weibo_cookie_tick(24 * 60 * 60),
      tianya_cookie_tick(24 * 60 * 60),
      qzone_cookie_tick(24 * 60 * 60),
      maopu_cookie_tick(24 * 60 * 60),
      douban_cookie_tick(24 * 60 * 60),
      taoguba_cookie_tick(24 * 60 * 60),
      snowball_cookie_tick(24 * 60 * 60),
      factor(10 * 60),
      begin_hour(7),
      begin_min(30),
      end_hour(23),
      end_min(30) {
  // TODO Auto-generated constructor stub
  ReadConfig(filename);
}

Config::~Config() {
  // TODO Auto-generated destructor stub
  if (NULL != config_)
    delete config_;
}

Config* Config::GetConfig() {
  if (NULL == config_) {
    config_ = new Config();
  }
  return config_;
}

bool Config::ReadConfig(const char *filename) {
  FILE *hFile = fopen(filename, "r");
  if (NULL == hFile) {
    LOG_MSG2("open %s file error", filename);
    return false;
  }
  struct stat file_stat;
  if (0 != stat(filename, &file_stat)) {
    LOG_MSG2("get %s stat error", filename);
    return false;
  }
  off_t filesize = file_stat.st_size;
  char buf[MAX_BUFFER_SIZE];
  fread(buf, 1, filesize, hFile);
  std::string data(buf, filesize);
  LOG_DEBUG2("config content:\n %s", data.c_str());

  int error_code = 0;
  std::string error_str;
  base_logic::ValueSerializer* engine = base_logic::ValueSerializer::Create(
      0, &data);
  base_logic::Value* value = engine->Deserialize(&error_code, &error_str);
  if (0 != error_code || NULL == value) {
    LOG_MSG2("deserialize error, error_code = %d, error_str = %s",
        error_code, error_str);
    return false;
  }

  base_logic::DictionaryValue* config = (base_logic::DictionaryValue*) value;
  Deserialize(config);
  delete config;
  base_logic::ValueSerializer::DeleteSerializer(0, engine);
  return true;
}

void Config::Deserialize(base_logic::DictionaryValue *dict) {
  if (NULL == dict) {
    return;
  }
  int64 tmp;
#define WSTRING(x)    L ## x
#define SET_VALUE(x)                \
    dict->GetBigInteger(WSTRING(#x), &tmp);   \
    dict->HasKey(WSTRING(#x)) && (x = tmp)

  SET_VALUE(assign_task_tick);
  SET_VALUE(fetch_task_tick);
  SET_VALUE(recycle_task_tick);
  SET_VALUE(clean_no_effective_client_tick);
  SET_VALUE(fetch_ip_tick);
  SET_VALUE(flush_cookie_use_time_tick);
  SET_VALUE(fetch_content_tick);
  SET_VALUE(fetch_cookies_tick);
  SET_VALUE(reply_self_state_tick);

  SET_VALUE(tieba_task_tick);
  SET_VALUE(weibo_task_tick);
  SET_VALUE(tianya_task_tick);
  SET_VALUE(qzone_task_tick);
  SET_VALUE(maopu_task_tick);
  SET_VALUE(douban_task_tick);
  SET_VALUE(taoguba_task_tick);
  SET_VALUE(snowball_task_tick);

  SET_VALUE(tieba_cookie_tick);
  SET_VALUE(weibo_cookie_tick);
  SET_VALUE(tianya_cookie_tick);
  SET_VALUE(qzone_cookie_tick);
  SET_VALUE(maopu_cookie_tick);
  SET_VALUE(douban_cookie_tick);
  SET_VALUE(taoguba_cookie_tick);
  SET_VALUE(snowball_cookie_tick);

  SET_VALUE(factor);
  SET_VALUE(begin_hour);
  SET_VALUE(begin_min);
  SET_VALUE(end_hour);
  SET_VALUE(end_min);

  cookie_tick_map[0] = tieba_cookie_tick;
  cookie_tick_map[1] = weibo_cookie_tick;
  cookie_tick_map[2] = tianya_cookie_tick;
  cookie_tick_map[3] = qzone_cookie_tick;
  cookie_tick_map[4] = maopu_cookie_tick;
  cookie_tick_map[5] = douban_cookie_tick;
  cookie_tick_map[6] = taoguba_cookie_tick;
  cookie_tick_map[7] = snowball_cookie_tick;
}

void Config::Print() const {
  std::stringstream os;
  os << "\n--------------------- Config Begin ---------------------"
     << std::endl;

#define PRINT(v) \
  os << '\t' << #v << " = " << v << std::endl

  PRINT(assign_task_tick);
  PRINT(fetch_task_tick);
  PRINT(recycle_task_tick);
  PRINT(clean_no_effective_client_tick);
  PRINT(fetch_ip_tick);
  PRINT(flush_cookie_use_time_tick);
  PRINT(fetch_content_tick);
  PRINT(fetch_cookies_tick);

  PRINT(tieba_task_tick);
  PRINT(weibo_task_tick);
  PRINT(tianya_task_tick);
  PRINT(qzone_task_tick);
  PRINT(maopu_task_tick);
  PRINT(douban_task_tick);
  PRINT(taoguba_task_tick);
  PRINT(snowball_task_tick);

  PRINT(tieba_cookie_tick);
  PRINT(weibo_cookie_tick);
  PRINT(tianya_cookie_tick);
  PRINT(qzone_cookie_tick);
  PRINT(maopu_cookie_tick);
  PRINT(douban_cookie_tick);
  PRINT(taoguba_cookie_tick);
  PRINT(snowball_cookie_tick);

  PRINT(factor);
  PRINT(begin_hour);
  PRINT(begin_min);
  PRINT(end_hour);
  PRINT(end_min);
  os << "--------------------- Config End  ---------------------" << std::endl;
  LOG_MSG2("%s", os.str().c_str());
#undef PRINT
}

int Config::GetCookieTickByAttrId(int64 attr_id) const {
  int index = attr_id % PLATFORM_BEGIN_ID;
  if (index >= PLATFORM_COUNT) {
    return -1;
  }
  return cookie_tick_map[index];
}
}

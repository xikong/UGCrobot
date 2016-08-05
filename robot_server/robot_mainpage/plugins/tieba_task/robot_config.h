/*
 * config.h
 *
 *  Created on: 2016年5月26日
 *      Author: zjc
 */

#ifndef KID_PLUGINS_TIEBA_TASK_ROBOT_CONFIG_H_
#define KID_PLUGINS_TIEBA_TASK_ROBOT_CONFIG_H_

#define ROBOT_CONFIG_FILE   "./config"

namespace base_logic {
class DictionaryValue;
}

namespace tieba_task_logic {

class Config {
 protected:
  Config(const char *filename = ROBOT_CONFIG_FILE);
  virtual ~Config();
 public:
  bool ReadConfig(const char *filename);
  void Deserialize(base_logic::DictionaryValue *dict);
  void Print() const;
  static Config* GetConfig();
 private:
  static Config *config_;
 public:
  // 定时任务配置
  int assign_task_tick;
  int fetch_task_tick;
  int fetch_main_task_tick;
  int recycle_task_tick;
  int clean_no_effective_client_tick;
  int fetch_ip_tick;
  int flush_cookie_use_time_tick;
  int fetch_content_tick;
  int fetch_cookies_tick;
  int reply_self_state_tick;

  // 每种平台任务执行间隔
  int tieba_task_tick;
  int weibo_task_tick;
  int tianya_task_tick;
  int qzone_task_tick;
  int maopu_task_tick;
  int douban_task_tick;
  int taoguba_task_tick;
  int snowball_task_tick;

  // cookie 使用间隔
  int cookie_use_tick;

  // 贴吧黑名单更新间隔
  int fetch_black_kw_tick;
 private:
  Config(const Config &config);
  Config& operator=(const Config &config);
};
}

class Config {
 public:
  Config();
  virtual ~Config();
};
#endif /* KID_PLUGINS_TIEBA_TASK_ROBOT_CONFIG_H_ */


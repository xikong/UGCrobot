/*
 * kw_blacklist.h
 *
 *  Created on: 2016年6月15日
 *      Author: zjc
 */

#ifndef KID_PLUGINS_TIEBA_TASK_KW_BLACKLIST_H_
#define KID_PLUGINS_TIEBA_TASK_KW_BLACKLIST_H_

#include <set>
#include <string>

namespace tieba_task_logic {

class CrawlerTaskDB;

class KwBlacklist {
 public:
  static KwBlacklist* GetKwBlacklist();

  bool FetchBlackKws(CrawlerTaskDB *task_db);
  bool IsBlackKw(std::string kw_url) const {
    return black_kw_set_.end() != black_kw_set_.find(kw_url);
  }
 private:
  KwBlacklist();
  KwBlacklist(const KwBlacklist &other);
  KwBlacklist& operator=(const KwBlacklist &other);
 private:
  static KwBlacklist *singleton_;

  typedef std::set<std::string> BlackKwSet;
  BlackKwSet black_kw_set_;
};

} /* namespace robot_task_logic */

#endif /* KID_PLUGINS_TIEBA_TASK_KW_BLACKLIST_H_ */

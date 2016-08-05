/*
 * kw_blacklist.cc
 *
 *  Created on: 2016年6月15日
 *      Author: zjc
 */

#include "kw_blacklist.h"
#include "logic/auto_crawler_infos.h"
#include "crawler_task_db.h"

#include <list>

namespace tieba_task_logic {

using base_logic::BlackKw;

KwBlacklist* KwBlacklist::singleton_ = NULL;

KwBlacklist* KwBlacklist::GetKwBlacklist() {
  if (NULL == singleton_) {
    singleton_ = new KwBlacklist();
  }
  return singleton_;
}

KwBlacklist::KwBlacklist() {
  // TODO Auto-generated constructor stub

}

bool KwBlacklist::FetchBlackKws(CrawlerTaskDB *task_db) {
  if (NULL == task_db) {
    return false;
  }
  std::list<BlackKw> kw_list;
  if (!task_db->FetchKwBlacklist(kw_list)) {
    return false;
  }
  black_kw_set_.clear();
  std::list<BlackKw>::iterator it = kw_list.begin();
  for (; it != kw_list.end(); ++it) {
    black_kw_set_.insert(it->url);
  }
  LOG_MSG2("fetch %d black kw", black_kw_set_.size());
  return true;
}
} /* namespace robot_task_logic */

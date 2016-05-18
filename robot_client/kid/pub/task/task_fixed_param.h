/*
 * comm_arg.h
 *
 *  Created on: 2016年5月10日
 *      Author: Harvey
 */

#ifndef KID_PUB_TASK_TASK_FIXED_PARAM_H_
#define KID_PUB_TASK_TASK_FIXED_PARAM_H_

#include <string>
using std::string;

//贴吧固定提交url
static const string TIEBA_FIXED_URL_COMMIT_POST_ADD =		"http://tieba.baidu.com/f/commit/post/add";
static const string TIEBA_FIXED_URL_LIKE_KW =				"http://tieba.baidu.com/f/like/commit/add";
static const string TIEBA_FIXED_URL_SIGN_KW = 				"http://tieba.baidu.com/sign/add";

//贴吧回复固定参数
static const string TIEBA_FIXED_PARAM_REPLY	=
		"ie=utf-8&vcode_md5=&rich_text=1&files=%5B%5D&mouse_pwd=&mouse_pwd_isclick=0&__type__=reply";

static const string TIEBA_FIXED_PARAM_REPLY_FLOOR =
		"ie=utf-8&rich_text=1&lp_type=0&lp_sub_type=0&new_vcode=1&tag=11&anonymous=0";

//淘股吧

static const string TAOGUBA_FIXED_POST_REPLY_URL = "http://www.taoguba.com.cn/addReply";
static const string TAOGUBA_FIXED_PARAM = "firstFlag=Y&quoteContent=&quoteUserID=&quoteUserName=&stockName1=%E7%89%9B%E8%82%A1%E4%BB%A3%E7%A0%81&recommondStokLsit=%E7%89%9B%E8%82%A1%E4%BB%A3%E7%A0%81&stockName2=%E7%89%9B%E8%82%A1%E4%BB%A3%E7%A0%81&recommondStokLsit=%E7%89%9B%E8%82%A1%E4%BB%A3%E7%A0%81&stockName3=%E7%89%9B%E8%82%A1%E4%BB%A3%E7%A0%81&recommondStokLsit=%E7%89%9B%E8%82%A1%E4%BB%A3%E7%A0%81";


//雪球
static const string XUEQIU_FIXED_GET_SESSION_TOKEN_URL = 	"https://xueqiu.com/service/csrf?api=/service/comment/add?type=status";
static const string XUEQIU_FIXED_POST_REPLY_URL = 			"https://xueqiu.com/service/comment/add?type=status";
static const string XUEQIU_FIXED_POST_REPLY_URSER_URL = 	"https://xueqiu.com/service/poster";


#endif /* KID_PUB_TASK_TASK_FIXED_PARAM_H_ */

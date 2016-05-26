//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月9日 Author: kerry

#include "net/packet_processing.h"
#include <list>
#include <string>
#include "basic/unzip.h"
#include "basic/zip.h"
#include "protocol/data_packet.h"
#include "net/comm_head.h"
#include "logic/logic_comm.h"

#define DUMPPACKBUF     4096 * 10

namespace net {

bool PacketProsess::PacketStream(const PacketHead* packet_head,
                                 void ** packet_stream,
                                 int32* packet_stream_length) {
  bool r = true;
  int16 packet_length = packet_head->packet_length;
  int8 is_zip_encrypt = packet_head->is_zip_encrypt;
  int8 type = packet_head->type;
  int16 signature = packet_head->signature;
  int16 operate_code = packet_head->operate_code;
  int16 data_length = packet_head->data_length;
  int32 timestamp = packet_head->timestamp;
  int64 session_id = packet_head->session_id;
  int16 crawler_type = packet_head->crawler_type;
  int32 crawler_id = packet_head->crawler_id;
  int16 server_type = packet_head->server_type;
  int32 server_id = packet_head->server_id;
  int32 reserved = packet_head->reserved;

  char* body_stream = NULL;
  char* data = NULL;

  LOG_DEBUG2("operate_code = %d", operate_code);
  switch (operate_code) {
    case CRAWLER_MGR_REG:
    case ANALYTICAL_REG: {
      struct CrawlerMgrReg* vCrawlerMgrReg = (struct CrawlerMgrReg*) packet_head;
      BUILDHEAD(CRAWLER_MGR_REG_SIZE);

      out.Write16(vCrawlerMgrReg->level);
      out.WriteData(vCrawlerMgrReg->password, PASSWORD_SIZE - 1);
      out.WriteData(vCrawlerMgrReg->mac, MAC_SIZE - 1);

      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case CRAWLER_MGR_REGSTATE: {
      struct CrawlerMgrRegState* vCrawlerMgrRegState =
          (struct CrawlerMgrRegState*) packet_head;
      BUILDHEAD(CRAWLER_MGR_REG_STATE_SIZE);

      out.Write32(vCrawlerMgrRegState->id);
      out.WriteData(vCrawlerMgrRegState->token, TOKEN_SIZE - 1);

      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case CRAWLER_REG_FAILED: {
      struct CrawlerFailed* vCrawlerFailed = (struct CrawlerFailed*) packet_head;
      BUILDHEAD(CRAWLER_FAILED_SIZE);
      out.Write32(vCrawlerFailed->erron_code);
      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case REPLY_HARDINFO: {
      struct ReplyHardInfo* vReplyHardInfo = (struct ReplyHardInfo*) packet_head;
      BUILDHEAD(REPLY_HARDINFO_SIZE);

      out.Write32(vReplyHardInfo->id);
      out.WriteData(vReplyHardInfo->token, TOKEN_SIZE - 1);
      out.WriteData(vReplyHardInfo->cpu, HADRINFO_SIZE - 1);
      out.WriteData(vReplyHardInfo->mem, HADRINFO_SIZE - 1);
      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case ASSIGNMENT_SINGLE_TASK: {
      struct AssignmentSingleTask* vAssignmentSingleTask =
          (struct AssignmentSingleTask*) packet_head;
      BUILDHEAD(ASSIGNMENT_SINGLE_TASK_SIZE);

      out.Write32(vAssignmentSingleTask->id);
      out.Write64(vAssignmentSingleTask->task_id);
      out.Write8(vAssignmentSingleTask->depth);
      out.Write8(vAssignmentSingleTask->machine);
      out.Write8(vAssignmentSingleTask->storage);
      out.WriteData(vAssignmentSingleTask->url, URL_SIZE - 1);

      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case ASSIGNMENT_MULTI_TASK: {
      struct AssignmentMultiTask* vAssignmentMultiTask =
          (struct AssignmentMultiTask*) packet_head;
      int32 len = 0;
      data_length = 0;

      std::list<struct TaskUnit*>::iterator it = vAssignmentMultiTask->task_set
          .begin();
      for (; it != vAssignmentMultiTask->task_set.end(); ++it) {
        data_length += (*it)->len;
      }

      BUILDHEAD(data_length);
      it = vAssignmentMultiTask->task_set.begin();
      for (; it != vAssignmentMultiTask->task_set.end(); it++) {
        out.Write16((*it)->len);
        out.Write64((*it)->task_id);
        out.Write64((*it)->attr_id);
        out.Write64((*it)->unix_time);
        out.Write8((*it)->max_depth);
        out.Write8((*it)->current_depth);
        out.Write8((*it)->machine);
        out.Write8((*it)->storage);
        out.Write8((*it)->is_login);
        out.Write8((*it)->is_over);
        out.Write8((*it)->is_forge);
        out.Write8((*it)->method);
        out.WriteData((*it)->url, strlen((*it)->url));
      }

      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case ASSIGNMENT_DETAIL_TASK: {
      struct AssignmentDetailTask* vAssignmentDetailTask =
          (struct AssignmentDetailTask*) packet_head;
      int32 len = 0;
      data_length = vAssignmentDetailTask->task_set.size() * TASK_UNIT_SIZE
          + sizeof(int32);

      BUILDHEAD(data_length);
      out.Write32(vAssignmentDetailTask->id);
      std::list<struct TaskUnit*>::iterator it = vAssignmentDetailTask->task_set
          .begin();

      for (; it != vAssignmentDetailTask->task_set.end(); it++) {
        out.Write64((*it)->task_id);
        out.Write64((*it)->attr_id);
        out.Write64((*it)->unix_time);
        out.Write8((*it)->max_depth);
        out.Write8((*it)->current_depth);
        out.Write8((*it)->machine);
        out.Write8((*it)->storage);
        out.Write8((*it)->is_login);
        out.Write8((*it)->is_over);
        out.Write8((*it)->is_forge);
        out.Write8((*it)->method);
        out.WriteData((*it)->url, URL_SIZE - 1);
      }

      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case REPLY_TASK_STATE: {
      struct ReplyTaskState* vReplyTaskState =
          (struct ReplyTaskState*) packet_head;
      BUILDHEAD(REPLYTASKTATE_SIZE);
      out.Write64(vReplyTaskState->jobid);
      out.Write8(vReplyTaskState->state);
      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case REPLY_DETAIL_STATE: {
      struct ReplyDetailState* vReplyDetailState =
          (struct ReplyDetailState*) packet_head;
      BUILDHEAD(REPLYDETAILTATE_SIZE);
      out.Write32(vReplyDetailState->id);
      out.WriteData(vReplyDetailState->token, TOKEN_SIZE - 1);
      out.Write64(vReplyDetailState->jobid);
      out.Write8(vReplyDetailState->state);
      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case GET_CRAWL_CONTENT_NUM: {
      struct CrawlContentNum* vCrawlContentNum =
          (struct CrawlContentNum*) packet_head;

      BUILDHEAD(CRAWLCONTENTNUM_SIZE);

      out.Write32(vCrawlContentNum->id);
      out.Write64(vCrawlContentNum->jobid);
      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case REPLY_CRAWL_CONTENT_NUM: {
      struct ReplyCrawlContentNum* vReplyCrawlContentNum =
          (struct ReplyCrawlContentNum*) packet_head;

      BUILDHEAD(REPLYCRAWLCONTENTNUM_SIZE);
      out.Write32(vReplyCrawlContentNum->id);
      out.WriteData(vReplyCrawlContentNum->token, TOKEN_SIZE - 1);
      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case GET_FORGEINFO: {
      struct GetForgeInfo* vGetForgeInfo = (struct GetForgeInfo*) (packet_head);
      BUILDHEAD(GETFORGEINFO_SIZE);
      out.Write32(vGetForgeInfo->task_id);
      //out.WriteData(vGetForgeInfo->token, TOKEN_SIZE - 1);
      out.Write64(vGetForgeInfo->task_id);
      out.Write8(vGetForgeInfo->forge_type);
      out.Write8(vGetForgeInfo->num);
      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case REPLY_FOGEINFO_UA: {
      struct ReplyUAForgeInfo* vReplyUAForgeInfo =
          (struct ReplyUAForgeInfo*) packet_head;

      int32 len = 0;
      data_length = sizeof(int64);
      std::list<struct UAForgeInfo*>::iterator it = vReplyUAForgeInfo
          ->forgen_set.begin();
      for (; it != vReplyUAForgeInfo->forgen_set.end(); ++it) {
        data_length += (*it)->len;
      }

      BUILDHEAD(data_length);
      out.Write64(vReplyUAForgeInfo->task_id);

      it = vReplyUAForgeInfo->forgen_set.begin();
      for (; it != vReplyUAForgeInfo->forgen_set.end(); it++) {
        out.Write16((*it)->len);
        out.Write32((*it)->id);
        out.Write8((*it)->type);
        out.WriteData((*it)->forgen_info, strlen((*it)->forgen_info));
      }

      body_stream = const_cast<char*>(out.GetData());
      break;
    }
    case REPLY_FOGEINFO_IP: {
      struct ReplyIPForgeInfo* vReplyIPForgeInfo =
          (struct ReplyIPForgeInfo*) packet_head;
      int32 len = 0;
      data_length = sizeof(int64);

      std::list<struct IPForgeInfo*>::iterator it = vReplyIPForgeInfo
          ->forgen_set.begin();
      for (; it != vReplyIPForgeInfo->forgen_set.end(); ++it) {
        data_length += ((*it)->len);
      }

      BUILDHEAD(data_length);
      out.Write64(vReplyIPForgeInfo->task_id);

      it = vReplyIPForgeInfo->forgen_set.begin();
      for (; it != vReplyIPForgeInfo->forgen_set.end(); it++) {
        data_length += (*it)->len;

        out.Write16((*it)->len);
        out.Write32((*it)->id);
        out.Write8((*it)->type);
        out.WriteData((*it)->forgen_info, strlen((*it)->forgen_info));
      }

      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case ANALYTICAL_INFO: {
      struct AnalysiInfo* vAnalysiInfo = (struct AnalysiInfo*) packet_head;
      int32 len = 0;

      int32 n = vAnalysiInfo->analysi_set.size();
      int32 n_len = ANALYIUNIT_SIZE;
      data_length = vAnalysiInfo->analysi_set.size() * (ANALYIUNIT_SIZE);

      BUILDHEAD(data_length);
      std::list<struct AnalysiUnit*>::iterator it = vAnalysiInfo->analysi_set
          .begin();

      for (; it != vAnalysiInfo->analysi_set.end(); it++) {
        out.Write64((*it)->ayalysi_id);
        out.Write64((*it)->task_id);
        out.Write32((*it)->attr_id);
        out.Write8((*it)->type);
        out.Write8((*it)->max_depth);
        out.Write8((*it)->cur_depth);
        out.WriteData((*it)->name, NAME_SIZE - 1);
        out.WriteData((*it)->key, KEY_SIZE - 1);
      }
      body_stream = const_cast<char*>(out.GetData());
      break;
    }
    case ANALYTICAL_STATE: {
      struct AnalysiState* vAnalysiState = (struct AnalysiState*) (packet_head);
      BUILDHEAD(ANALYSISTATE_SIZE);
      out.Write64(vAnalysiState->id);
      out.Write8(vAnalysiState->state);
      body_stream = const_cast<char*>(out.GetData());
      break;
    }
    case ANALYTICAL_URL_SET: {
      struct AnalyticalURLSet* vAnalyticalURLSet =
          (struct AnalyticalURLSet*) (packet_head);

      int32 len = 0;
      data_length = vAnalyticalURLSet->analytical_url_set.size()
          * (sizeof(int8) * 3 + sizeof(int64) + URL_SIZE - 1);
      BUILDHEAD(data_length);

      std::list<struct AnalyticalURLUnit*>::iterator it = vAnalyticalURLSet
          ->analytical_url_set.begin();

      for (; it != vAnalyticalURLSet->analytical_url_set.end(); it++) {
        out.Write64((*it)->task_id);
        out.Write64((*it)->attr_id);
        out.Write8((*it)->max_depth);
        out.Write8((*it)->current_depth);
        out.Write8((*it)->method);
        out.WriteData((*it)->url, URL_SIZE - 1);
      }
      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case DELIVERED_COOKIE_SET: {
      struct LoginCookieSet* vLoginCookieSet =
          (struct LoginCookieSet*) (packet_head);

      int32 len = 0;
      data_length = sizeof(int32) + sizeof(int64);
      std::list<struct LoginCookieUnit*>::iterator itr = vLoginCookieSet
          ->login_cookie_set.begin();

      for (; itr != vLoginCookieSet->login_cookie_set.end(); ++itr) {
        data_length = data_length + (*itr)->login_cookie_body.size() + 2;
      }

      BUILDHEAD(data_length);
      out.Write32(vLoginCookieSet->manage_id);
      out.Write64(vLoginCookieSet->attr_id);

      for (itr = vLoginCookieSet->login_cookie_set.begin();
          itr != vLoginCookieSet->login_cookie_set.end(); ++itr) {
        out.Write16((*itr)->len);
        int32 body_length = (*itr)->login_cookie_body.size();
        const char* tmp = (*itr)->login_cookie_body.c_str();
        out.WriteData(tmp, body_length);
      }
      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case HEART_PACKET:
    case GET_HARDINFO:
    case HEART_TO_SLB:
    case TEMP_CRAWLER_OP: {
      BUILDHEAD(0);
      body_stream = const_cast<char*>(out.GetData());
      break;
    }

    case REGISTER_SERVER: {
      struct RegisterServer *plg_svc_reg =
          (struct RegisterServer *) packet_head;
      BUILDHEAD(PLUGIN_SVC_REG_SIZE);

      out.Write16(plg_svc_reg->level);
      out.WriteData(plg_svc_reg->password, PASSWORD_SIZE - 1);
      out.WriteData(plg_svc_reg->mac, MAC_SIZE - 1);
      out.Write16(plg_svc_reg->port);

      body_stream = const_cast<char *>(out.GetData());
      break;
    }
    case PLUGIN_SVC_REG_STATE: {
      struct PluginSvcMgrRouterReg *plg_svc_reg =
          (struct PluginSvcMgrRouterReg *) packet_head;
      BUILDHEAD(PLUGIN_SVC_REG_STATE_SIZE);

      out.WriteData(plg_svc_reg->token, TOKEN_SIZE - 1);

      body_stream = const_cast<char *>(out.GetData());

      break;
    }
    case PLUGIN_SVC_MGR_ROUTER_REG: {
      struct PluginSvcMgrRouterReg *router_reg =
          (struct PluginSvcMgrRouterReg *) packet_head;
      BUILDHEAD(PLUGIN_SVC_ROUTER_REG_SIZE);
      out.WriteData(router_reg->token, TOKEN_SIZE - 1);
      out.Write32(router_reg->router_id);

      body_stream = const_cast<char *>(out.GetData());
      break;
    }
    case PLUGIN_SVC_MGR_ROUTER_REG_STATE: {
      struct PluginSvcMgrRouterReg *router_reg =
          (struct PluginSvcMgrRouterReg*) packet_head;
      BUILDHEAD(PLUGIN_SVC_ROUTER_REG_SIZE);

      out.WriteData(router_reg->token, TOKEN_SIZE - 1);
      out.Write32(router_reg->router_id);

      body_stream = const_cast<char *>(out.GetData());
      break;
    }
    case REPLY_ROUTER_REG: {
      struct ReplyRouterReg *reply_router_reg =
          (struct ReplyRouterReg *) packet_head;
      BUILDHEAD(REPLY_ROUTER_REG_SIZE);

      out.Write8(reply_router_reg->state);

      body_stream = const_cast<char *>(out.GetData());
      break;
    }
    case PLUGIN_SVC_AVAILABLE_RESOURCE_NUM: {
      struct ReplyPlgSvcTaskState *available_tasks =
          (struct ReplyPlgSvcTaskState *) packet_head;
      BUILDHEAD(REPLY_PLG_SVC_AVAILABLE_TASKS);
      out.Write32(available_tasks->max_tasks);
      out.Write32(available_tasks->cur_tasks);

      body_stream = const_cast<char *>(out.GetData());
      break;
    }
    case ASSIGN_WEIBO_TASK: {
      struct AssingWeiboTask *weibo_task = (struct AssingWeiboTask*) packet_head;
      uint16 data_len = WEIBO_TASK_SIZE;
      std::list<struct WeiboTaskUnit*>::iterator it =
          weibo_task->task_set.begin();
      for (; weibo_task->task_set.end() != it; ++it) {
        data_len += WEIBO_TASK_UNIT_SIZE;
        data_len += (*it)->cookie.size();
        data_len += (*it)->content.size();
        data_len += (*it)->addr.size();
        data_len += (*it)->topic_id.size();
        data_len += (*it)->host_uin.size();
      }
      BUILDHEAD(data_len);
      out.Write16(weibo_task->tasks_num);
      it = weibo_task->task_set.begin();
      for (; weibo_task->task_set.end() != it; ++it) {
        out.Write64((*it)->task_id);
        out.Write64((*it)->cookie_id);

        out.Write16((*it)->cookie.size());
        out.WriteData((*it)->cookie.c_str(), (*it)->cookie.size());

        out.Write16((*it)->content.size());
        out.WriteData((*it)->content.c_str(), (*it)->content.size());

        out.Write16((*it)->addr.size());
        out.WriteData((*it)->addr.c_str(), (*it)->addr.size());

        out.Write16((*it)->topic_id.size());
        out.WriteData((*it)->topic_id.c_str(), (*it)->topic_id.size());

        out.Write16((*it)->host_uin.size());
        out.WriteData((*it)->host_uin.c_str(), (*it)->host_uin.size());
      }
      body_stream = const_cast<char*>(out.GetData());
      break;
    }
    case ASSIGN_TIANYA_TASK: {
      struct AssignTianyaTask *tianya_task =
          (struct AssignTianyaTask*) packet_head;
      uint16 data_len = TIANYA_TASK_SIZE;
      std::list<struct TianyaTaskUnit*>::iterator it = tianya_task->task_set
          .begin();
      for (; tianya_task->task_set.end() != it; ++it) {
        data_len += TIANYA_TASK_UNIT_SIZE;
        data_len += (*it)->cookie.size();
        data_len += (*it)->content.size();
        data_len += (*it)->addr.size();
        data_len += (*it)->pre_url.size();
        data_len += (*it)->pre_title.size();
        data_len += (*it)->pre_user_id.size();
        data_len += (*it)->pre_user_name.size();
      }
      BUILDHEAD(data_len);
      out.Write16(tianya_task->tasks_num);
      it = tianya_task->task_set.begin();
      for (; tianya_task->task_set.end() != it; ++it) {
        out.Write64((*it)->task_id);
        out.Write64((*it)->cookie_id);

        out.Write16((*it)->cookie.size());
        out.WriteData((*it)->cookie.c_str(), (*it)->cookie.size());

        out.Write16((*it)->content.size());
        out.WriteData((*it)->content.c_str(), (*it)->content.size());

        out.Write16((*it)->addr.size());
        out.WriteData((*it)->addr.c_str(), (*it)->addr.size());

        out.Write64((*it)->pre_post_time);

        out.Write16((*it)->pre_url.size());
        out.WriteData((*it)->pre_url.c_str(), (*it)->pre_url.size());

        out.Write16((*it)->pre_title.size());
        out.WriteData((*it)->pre_title.c_str(), (*it)->pre_title.size());

        out.Write16((*it)->pre_user_id.size());
        out.WriteData((*it)->pre_user_id.c_str(), (*it)->pre_user_id.size());

        out.Write16((*it)->pre_user_name.size());
        out.WriteData((*it)->pre_user_name.c_str(),
                      (*it)->pre_user_name.size());
      }
      body_stream = const_cast<char*>(out.GetData());
      break;
    }
    case ASSIGN_TIEBA_TASK: {
      LOG_DEBUG2("packet stream ASSIGN_TIEBA_TASK: %d", ASSIGN_TIEBA_TASK);
      struct AssignTiebaTask *tieba_task = (struct AssignTiebaTask*) packet_head;
      uint16 data_len = TIEBA_TASK_SIZE;
      std::list<struct TiebaTaskUnit*>::iterator it =
          tieba_task->task_set.begin();
      for (; tieba_task->task_set.end() != it; ++it) {
        data_len += TIEBA_TASK_UNIT_SIZE;
        data_len += (*it)->cookie.size();
        data_len += (*it)->addr.size();
        data_len += (*it)->url.size();
        data_len += (*it)->row_key.size();
        data_len += (*it)->ua.size();
      }
      BUILDHEAD(data_len);
      out.Write16(tieba_task->tasks_num);
      it = tieba_task->task_set.begin();
      for (; tieba_task->task_set.end() != it; ++it) {
        out.Write16((*it)->task_len);
        out.Write16((*it)->task_type);
        out.Write64((*it)->task_id);
        out.Write16((*it)->row_key.size());
        out.WriteData((*it)->row_key.c_str(), (*it)->row_key.size());
        out.Write64((*it)->cookie_id);

        out.Write16((*it)->cookie.size());
        out.WriteData((*it)->cookie.c_str(), (*it)->cookie.size());

        out.Write16((*it)->addr.size());
        out.WriteData((*it)->addr.c_str(), (*it)->addr.size());

        out.Write16((*it)->ua.size());
        out.WriteData((*it)->ua.c_str(), (*it)->ua.size());

        out.Write16((*it)->url.size());
        out.WriteData((*it)->url.c_str(), (*it)->url.size());
      }
      body_stream = const_cast<char*>(out.GetData());
      break;
    }
    case ASSIGN_QZONE_TASK: {
      struct AssignQzoneTask *qzone_task = (struct AssignQzoneTask*) packet_head;
      uint16 data_len = QZONE_TASK_SIZE;
      std::list<struct QzoneTaskUnit*>::iterator it =
          qzone_task->task_set.begin();
      for (; qzone_task->task_set.end() != it; ++it) {
        data_len += QZONE_TASK_UNIT_SIZE;
        data_len += (*it)->cookie.size();
        data_len += (*it)->content.size();
        data_len += (*it)->addr.size();
        data_len += (*it)->topic_id.size();
      }
      BUILDHEAD(data_len);
      out.Write16(qzone_task->tasks_num);
      it = qzone_task->task_set.begin();
      for (; qzone_task->task_set.end() != it; ++it) {
        out.Write64((*it)->task_id);
        out.Write64((*it)->cookie_id);

        out.Write16((*it)->cookie.size());
        out.WriteData((*it)->cookie.c_str(), (*it)->cookie.size());

        out.Write16((*it)->content.size());
        out.WriteData((*it)->content.c_str(), (*it)->content.size());

        out.Write16((*it)->addr.size());
        out.WriteData((*it)->addr.c_str(), (*it)->addr.size());

        out.Write64((*it)->host_uin);

        out.Write16((*it)->topic_id.size());
        out.WriteData((*it)->topic_id.c_str(), (*it)->topic_id.size());
      }
      body_stream = const_cast<char*>(out.GetData());
      break;
    }
    default:
      r = false;
      break;
  }

  if (r) {
    char* packet = NULL;
    int32 packet_body_len = 0;
    if (is_zip_encrypt == ZIP_AND_NOENCRYPT) {
      uint8* zip_data = NULL;
      uint64 zip_len = 0;
      const uint8* unzip =
          const_cast<const uint8*>(reinterpret_cast<uint8*>(body_stream));
      zip_len = CompressionStream(unzip,
                                  packet_length - sizeof(int16) - sizeof(int8),
                                  &zip_data);
      free(body_stream);
      body_stream = NULL;
      packet = reinterpret_cast<char*>(zip_data);
      packet_body_len = zip_len;
    } else {
      packet = body_stream;
      packet_body_len = packet_length - sizeof(int16) - sizeof(int8);
    }

    packet::DataOutPacket out_packet(
        false, packet_body_len + sizeof(int16) + sizeof(int8));
    out_packet.Write16(packet_body_len + sizeof(int16) + sizeof(int8));
    out_packet.Write8(is_zip_encrypt);
    out_packet.WriteData(packet, packet_body_len);
    free(packet);
    packet = NULL;
    *packet_stream = reinterpret_cast<void*>(const_cast<char*>(out_packet
        .GetData()));
    *packet_stream_length = packet_body_len + sizeof(int16) + sizeof(int8);
  }

  /*
   packet::DataOutPacket out_packet(false,
   body_len + sizeof(int16) + sizeof(int8));
   out_packet.Write16(body_len + sizeof(int16) + sizeof(int8));
   out_packet.Write8(is_zip_encrypt);
   out_packet.WriteData(body_stream, body_len);
   if (body_stream) {delete [] body_stream; body_stream = NULL;}

   *packet_stream =
   reinterpret_cast<void*>(const_cast<char*>(out_packet.GetData()));

   if (packet_head->data_length == 0)
   *packet_stream_length = body_len + sizeof(int16) + sizeof(int8);
   else
   *packet_stream_length = packet_length;
   */

  return r;
}

bool PacketProsess::UnpackStream(const void* packet_stream, int32 len,
                                 struct PacketHead** packet_head) {
  bool r = true;
  packet::DataInPacket in_packet(reinterpret_cast<const char*>(packet_stream),
                                 len);
  int16 packet_length = in_packet.Read16();
  int8 is_zip_encrypt = in_packet.Read8();
  char* body_stream = NULL;
  int32 body_length = 0;
  int32 temp = 0;

  // 是否解压解码
  if (is_zip_encrypt == ZIP_AND_NOENCRYPT) {
    char* temp_body_stream =
        reinterpret_cast<char*>(const_cast<void*>(packet_stream))
            + sizeof(int16) + sizeof(int8);
    const uint8* zipdata = reinterpret_cast<uint8*>(temp_body_stream);
    uint8* unzipdata = NULL;
    int32 temp_body_len = len - sizeof(int16) - sizeof(int8);
    body_length = DecompressionStream(zipdata, temp_body_len, &unzipdata);
    body_stream = reinterpret_cast<char*>(unzipdata);
  } else {
    body_stream = reinterpret_cast<char*>(const_cast<void*>(packet_stream))
        + sizeof(int16) + sizeof(int8);
    body_length = len - sizeof(int16) - sizeof(int8);
  }

  BUILDPACKHEAD();

  switch (operate_code) {
    case CRAWLER_MGR_REG:
    case ANALYTICAL_REG: {
      struct CrawlerMgrReg* vCrawlerMgrReg = new struct CrawlerMgrReg;
      *packet_head = (struct PacketHead*) vCrawlerMgrReg;

      FILLHEAD()
      ;

      vCrawlerMgrReg->level = in.Read16();
      int temp = 0;
      memcpy(vCrawlerMgrReg->password, in.ReadData(PASSWORD_SIZE - 1, temp),
      PASSWORD_SIZE - 1);
      int32 password_len =
          (temp - 1) < (PASSWORD_SIZE - 1) ? (temp - 1) : (PASSWORD_SIZE - 1);

      vCrawlerMgrReg->password[password_len] = '\0';

      memcpy(vCrawlerMgrReg->mac, in.ReadData(MAC_SIZE - 1, temp),
      MAC_SIZE - 1);
      int32 mac_len = (temp - 1) < (MAC_SIZE - 1) ? (temp - 1) : (MAC_SIZE - 1);
      vCrawlerMgrReg->mac[mac_len] = '\0';

      break;
    }

    case CRAWLER_MGR_REGSTATE: {
      struct CrawlerMgrRegState* vCrawlerMgrRegState =
          new struct CrawlerMgrRegState;
      *packet_head = (struct PacketHead*) vCrawlerMgrRegState;

      FILLHEAD()
      ;
      vCrawlerMgrRegState->id = in.Read32();
      int temp = 0;
      memcpy(vCrawlerMgrRegState->token, in.ReadData(TOKEN_SIZE - 1, temp),
      TOKEN_SIZE - 1);
      int32 token_len =
          (temp - 1) < (TOKEN_SIZE - 1) ? (temp - 1) : (TOKEN_SIZE - 1);
      vCrawlerMgrRegState->token[token_len] = '\0';
      break;
    }

    case CRAWLER_REG_FAILED: {
      struct CrawlerFailed* vCrawlerFailed = new struct CrawlerFailed;
      *packet_head = (struct PacketHead*) vCrawlerFailed;

      FILLHEAD()
      ;
      vCrawlerFailed->erron_code = in.Read32();
      break;
    }

    case REPLY_HARDINFO: {
      struct ReplyHardInfo* vReplyHardInfo = new struct ReplyHardInfo;
      *packet_head = (struct PacketHead*) vReplyHardInfo;

      FILLHEAD()
      ;

      vReplyHardInfo->id = in.Read32();
      int temp = 0;
      memcpy(vReplyHardInfo->token, in.ReadData(TOKEN_SIZE - 1, temp),
      TOKEN_SIZE - 1);
      int32 token_len =
          (temp - 1) < (TOKEN_SIZE - 1) ? (temp - 1) : (TOKEN_SIZE - 1);
      vReplyHardInfo->token[token_len] = '\0';

      memcpy(vReplyHardInfo->cpu, in.ReadData(HADRINFO_SIZE - 1, temp),
      HADRINFO_SIZE - 1);
      int32 cpu_len =
          (temp - 1) < (HADRINFO_SIZE - 1) ? (temp - 1) : (HADRINFO_SIZE - 1);
      vReplyHardInfo->cpu[cpu_len] = '\0';

      memcpy(vReplyHardInfo->mem, in.ReadData(HADRINFO_SIZE - 1, temp),
      HADRINFO_SIZE - 1);
      int32 mem_len =
          (temp - 1) < (HADRINFO_SIZE - 1) ? (temp - 1) : (HADRINFO_SIZE - 1);
      vReplyHardInfo->mem[mem_len] = '\0';
      break;
    }

    case ASSIGNMENT_SINGLE_TASK: {
      struct AssignmentSingleTask* vAssignmentSingleTask =
          new AssignmentSingleTask;
      *packet_head = (struct PacketHead*) vAssignmentSingleTask;

      FILLHEAD()
      ;

      vAssignmentSingleTask->id = in.Read32();
      vAssignmentSingleTask->task_id = in.Read64();
      vAssignmentSingleTask->depth = in.Read8();
      vAssignmentSingleTask->machine = in.Read8();
      vAssignmentSingleTask->storage = in.Read8();
      int temp = 0;
      memcpy(vAssignmentSingleTask->url, in.ReadData(URL_SIZE - 1, temp),
      URL_SIZE - 1);
      int32 url_len = (temp - 1) < (URL_SIZE - 1) ? (temp - 1) : (URL_SIZE - 1);
      vAssignmentSingleTask->url[url_len] = '\0';

      break;
    }
      /*
       case ASSIGNMENT_MULTI_TASK : {
       struct AssignmentMultiTask* vAssignmentMultiTask =
       new AssignmentMultiTask;
       *packet_head = (struct PacketHead*)vAssignmentMultiTask;

       FILLHEAD();

       int32 set_size = data_length - sizeof(int32);
       int32 i = 0;
       int32 num = set_size / (TASK_UNIT_SIZE - 1);
       int32 len = 0;
       for (; i< num; i++) {
       struct TaskUnit* unit = new struct TaskUnit;
       int32 temp = 0;
       unit->task_id = in.Read64();
       unit->attr_id = in.Read64();
       unit->unix_time = in.Read64();
       unit->max_depth = in.Read8();
       unit->current_depth = in.Read8();
       unit->machine = in.Read8();
       unit->storage = in.Read8();
       unit->is_login = in.Read8();
       unit->is_over = in.Read8();
       unit->is_forge = in.Read8();
       unit->method = in.Read8();
       memcpy(unit->url, in.ReadData(URL_SIZE - 1, temp), URL_SIZE - 1);
       int32 url_len = (temp - 1) < (URL_SIZE - 1) ?
       (temp - 1) : (URL_SIZE - 1);
       unit->url[url_len] = '\0';

       vAssignmentMultiTask->task_set.push_back(unit);
       }
       break;
       }
       */

    case ASSIGNMENT_DETAIL_TASK: {
      struct AssignmentDetailTask* vAssignmentDetailTask =
          new AssignmentDetailTask;
      *packet_head = (struct PacketHead*) vAssignmentDetailTask;

      FILLHEAD()
      ;

      vAssignmentDetailTask->id = in.Read32();

      int32 set_size = data_length - sizeof(int32);
      int32 i = 0;
      int32 num = set_size / (TASK_UNIT_SIZE - 1);
      int32 len = 0;
      for (; i < num; i++) {
        struct TaskUnit* unit = new struct TaskUnit;
        int32 temp = 0;
        unit->task_id = in.Read64();
        unit->attr_id = in.Read64();
        unit->unix_time = in.Read64();
        unit->max_depth = in.Read8();
        unit->current_depth = in.Read8();
        unit->machine = in.Read8();
        unit->storage = in.Read8();
        unit->is_login = in.Read8();
        unit->is_over = in.Read8();
        unit->is_forge = in.Read8();
        unit->method = in.Read8();
        memcpy(unit->url, in.ReadData(URL_SIZE - 1, temp),
        URL_SIZE - 1);
        int32 url_len =
            (temp - 1) < (URL_SIZE - 1) ? (temp - 1) : (URL_SIZE - 1);
        unit->url[url_len] = '\0';

        vAssignmentDetailTask->task_set.push_back(unit);
      }
      break;
    }

    case REPLY_TASK_STATE: {
      struct ReplyTaskState* vReplyTaskState = new struct ReplyTaskState;

      *packet_head = (struct PacketHead*) vReplyTaskState;

      FILLHEAD()
      ;

      vReplyTaskState->jobid = in.Read64();
      vReplyTaskState->state = in.Read8();
      break;
    }

    case REPLY_DETAIL_STATE: {
      struct ReplyDetailState* vReplyDetailState = new struct ReplyDetailState;

      *packet_head = (struct PacketHead*) vReplyDetailState;

      FILLHEAD()
      ;

      vReplyDetailState->id = in.Read32();
      int32 temp = 0;
      memcpy(vReplyDetailState->token, in.ReadData(TOKEN_SIZE - 1, temp),
      TOKEN_SIZE - 1);
      int32 token_len =
          (temp - 1) < (TOKEN_SIZE - 1) ? (temp - 1) : (TOKEN_SIZE - 1);
      vReplyDetailState->token[token_len] = '\0';

      vReplyDetailState->jobid = in.Read64();
      vReplyDetailState->state = in.Read8();
      break;
    }

    case GET_CRAWL_CONTENT_NUM: {
      struct CrawlContentNum* vCrawlContentNum = new struct CrawlContentNum;
      *packet_head = (struct CrawlContentNum*) vCrawlContentNum;

      FILLHEAD()
      ;

      vCrawlContentNum = new struct CrawlContentNum;
      vCrawlContentNum->id = in.Read32();
      vCrawlContentNum->jobid = in.Read64();
      break;
    }

    case REPLY_CRAWL_CONTENT_NUM: {
      struct ReplyCrawlContentNum* vReplyCrawlContentNum =
          new ReplyCrawlContentNum;
      *packet_head = (struct ReplyCrawlContentNum*) vReplyCrawlContentNum;

      FILLHEAD()
      ;

      vReplyCrawlContentNum->id = in.Read32();
      int32 temp = 0;
      memcpy(vReplyCrawlContentNum->token, in.ReadData(TOKEN_SIZE - 1, temp),
      TOKEN_SIZE - 1);
      int32 token_len =
          (temp - 1) < (TOKEN_SIZE - 1) ? (temp - 1) : (TOKEN_SIZE - 1);
      vReplyCrawlContentNum->token[token_len] = '\0';

      vReplyCrawlContentNum->task_id = in.Read64();
      vReplyCrawlContentNum->num = in.Read32();

      break;
    }
    case CRAWL_HBASE_STORAGE_INFO:
    case CRAWL_FTP_STORAGE_INFO: {
      struct CrawlStorageInfo* vCrawlStorageInfo = new struct CrawlStorageInfo;
      *packet_head = (struct PacketHead*) vCrawlStorageInfo;

      FILLHEAD()
      ;

      int32 t;
      for (int32 read_bytes = 0;
          data_length - read_bytes > CRAWLSTORAGEINFO_SIZE;) {
        struct StorageUnit *unit = new StorageUnit();
        unit->len = in.Read16();
        read_bytes += unit->len;
        unit->task_id = in.Read64();
        unit->attr_id = in.Read64();
        unit->max_depth = in.Read8();
        unit->cur_depth = in.Read8();
        memset(unit->table_name, '\0', sizeof(unit->table_name));
        memcpy(unit->table_name, in.ReadData(TABLE_NAME_SIZE - 1, t),
        TABLE_NAME_SIZE - 1);
        LOG_DEBUG2("read table_name: try to read %d bytes, real read %d bytes",
            TABLE_NAME_SIZE-1, t);
        unit->table_name[TABLE_NAME_SIZE - 1] = '\0';
        int rowkey_size = ROWKEY_SIZE(unit->len);
        unit->rowkey.assign(in.ReadData(rowkey_size, t), rowkey_size);
        LOG_DEBUG2("read rowkey: try to read %d bytes, real read %d bytes",
            rowkey_size, t);
        vCrawlStorageInfo->storage_set.push_back(unit);
      }
      /*
       int32 set_size = data_length - sizeof(int32) - (TOKEN_SIZE - 1);
       LOG_DEBUG2("opcode = %d, data_length = %d",
       CRAWL_HBASE_STORAGE_INFO, data_length);
       int32 i = 0;
       int32 num = set_size / CRAWLSTORAGEINFO_SIZE;
       int32 len = 0;
       for (; i < num; i++) {
       struct StorageUnit* unit = new struct StorageUnit;
       temp = 0;
       unit->len = in.Read16();
       unit->task_id = in.Read64();
       unit->attr_id = in.Read64();
       unit->max_depth = in.Read8();
       unit->cur_depth = in.Read8();

       memcpy(unit->key_name, in.ReadData(STORAGE_INFO_SIZE - 1, temp),
       STORAGE_INFO_SIZE - 1);

       int32 name_len =
       (temp) < (STORAGE_INFO_SIZE - 1) ?
       (temp) : (STORAGE_INFO_SIZE - 1);
       unit->key_name[name_len] = '\0';

       temp = 0;
       memcpy(unit->pos_name, in.ReadData(URL_SIZE - 1, temp),
       URL_SIZE - 1);
       int32 pos_len = (temp) < (URL_SIZE - 1) ? (temp) : (URL_SIZE - 1);
       unit->pos_name[pos_len] = '\0';

       vCrawlStorageInfo->storage_set.push_back(unit);
       }
       */
      break;
    }

    case GET_FORGEINFO: {
      struct GetForgeInfo* vGetForgeInfo = new struct GetForgeInfo;
      *packet_head = (struct GetForgeInfo*) vGetForgeInfo;

      FILLHEAD()
      ;

      vGetForgeInfo->task_id = in.Read64();
      vGetForgeInfo->forge_type = in.Read8();
      vGetForgeInfo->num = in.Read8();

      break;
    }

      /*
       case REPLY_FOGEINFO_UA: {
       struct ReplyUAForgeInfo* vReplyUAForgeInfo =
       new struct ReplyUAForgeInfo;
       *packet_head = (struct PacketHead*)vReplyUAForgeInfo;

       FILLHEAD();

       vReplyUAForgeInfo->id = in.Read32();
       vReplyUAForgeInfo->task_id = in.Read64();

       int32 set_size = data_length - sizeof(int32) - sizeof(int64);
       int32 i = 0;
       int32 num = set_size / (UA_FORGEN_SIZE - 1 + sizeof(int32) + sizeof(8));
       int32 len = 0;
       for (; i< num; i++) {
       struct UAForgeInfo* unit = new struct UAForgeInfo;
       int32 temp = 0;
       unit->id = in.Read32();
       unit->type = in.Read8();
       memcpy(unit->forgen_info, in.ReadData(UA_FORGEN_SIZE -1, temp),
       UA_FORGEN_SIZE - 1);
       int32 ua_forgen_len = (temp - 1) < (UA_FORGEN_SIZE - 1) ?
       (temp - 1) : (UA_FORGEN_SIZE - 1);
       unit->forgen_info[ua_forgen_len] = '\0';


       vReplyUAForgeInfo->forgen_set.push_back(unit);
       }
       break;
       }
       */
      /*
       case REPLY_FOGEINFO_IP: {
       struct ReplyIPForgeInfo* vReplyIPForgeInfo =
       new struct ReplyIPForgeInfo;
       *packet_head = (struct PacketHead*)vReplyIPForgeInfo;

       FILLHEAD();

       vReplyIPForgeInfo->id = in.Read32();
       vReplyIPForgeInfo->task_id = in.Read64();

       int32 set_size = data_length - sizeof(int32) - sizeof(int64);
       int32 i = 0;
       int32 num = set_size / (IP_FORGEN_SIZE - 1 + sizeof(int32) + sizeof(8));
       int32 len = 0;
       for (; i< num; i++) {
       struct IPForgeInfo* unit = new struct IPForgeInfo;
       int32 temp = 0;
       unit->id = in.Read32();
       unit->type = in.Read8();
       memcpy(unit->forgen_info, in.ReadData(IP_FORGEN_SIZE -1, temp),
       IP_FORGEN_SIZE - 1);
       int32 ip_forgen_len = (temp - 1) < (IP_FORGEN_SIZE - 1) ?
       (temp - 1) : (IP_FORGEN_SIZE - 1);
       unit->forgen_info[ip_forgen_len] = '\0';

       vReplyIPForgeInfo->forgen_set.push_back(unit);
       }
       break;
       }
       */

    case ANALYTICAL_INFO: {
      struct AnalysiInfo* vAnalysiInfo = new struct AnalysiInfo;
      *packet_head = (struct PacketHead*) vAnalysiInfo;
      FILLHEAD()
      ;

      int32 set_size = data_length;
      int32 i = 0;
      int32 num = set_size / ANALYIUNIT_SIZE;
      int32 len = 0;
      for (; i < num; i++) {
        struct AnalysiUnit* unit = new struct AnalysiUnit;
        int32 temp = 0;
        unit->ayalysi_id = in.Read64();
        unit->task_id = in.Read64();
        unit->attr_id = in.Read32();
        unit->type = in.Read8();
        unit->max_depth = in.Read8();
        unit->cur_depth = in.Read8();
        memcpy(unit->name, in.ReadData(NAME_SIZE - 1, temp),
        NAME_SIZE - 1);
        int32 name_len =
            (temp - 1) < (NAME_SIZE - 1) ? (temp - 1) : (NAME_SIZE - 1);
        unit->name[name_len] = '\0';

        memcpy(unit->key, in.ReadData(KEY_SIZE - 1, temp),
        KEY_SIZE - 1);
        int32 key_len =
            (temp - 1) < (KEY_SIZE - 1) ? (temp - 1) : (KEY_SIZE - 1);
        unit->key[key_len] = '\0';

        vAnalysiInfo->analysi_set.push_back(unit);
      }
      break;
    }

    case ANALYTICAL_URL_SET: {
      struct AnalyticalURLSet* vAnalyticalURLSet = new struct AnalyticalURLSet;
      *packet_head = (struct PacketHead*) vAnalyticalURLSet;
      FILLHEAD()
      ;

      int32 set_size = data_length - sizeof(int32) - (TOKEN_SIZE - 1);
      int32 i = 0;
      int32 num = set_size / ANALYTICAL_URL_UNIT_SIZE;
      int32 len = 0;

      vAnalyticalURLSet->id = in.Read32();
      int32 temp = 0;
      memcpy(vAnalyticalURLSet->token, in.ReadData(TOKEN_SIZE - 1, temp),
      TOKEN_SIZE - 1);
      int32 token_len = (temp) < (TOKEN_SIZE - 1) ? (temp) : (TOKEN_SIZE - 1);
      vAnalyticalURLSet->token[token_len] = '\0';

      for (; i < num; i++) {
        struct AnalyticalURLUnit* unit = new struct AnalyticalURLUnit;
        int32 temp = 0;
        unit->task_id = in.Read64();
        unit->attr_id = in.Read64();
        unit->max_depth = short(in.Read8());
        unit->current_depth = short(in.Read8());
        unit->method = short(in.Read8());

        memcpy(unit->url, in.ReadData(URL_SIZE - 1, temp),
        URL_SIZE - 1);
        int32 url_len =
            (temp - 1) < (URL_SIZE - 1) ? (temp - 1) : (URL_SIZE - 1);
        unit->url[url_len] = '\0';

        vAnalyticalURLSet->analytical_url_set.push_back(unit);
      }
      break;
    }
    case ANALYTICAL_STATE: {
      struct AnalysiState* vAnalysiState = new struct AnalysiState;
      *packet_head = (struct PacketHead*) vAnalysiState;

      FILLHEAD()
      ;
      vAnalysiState->id = in.Read64();
      vAnalysiState->state = in.Read8();
      break;
    }

    case LOGIN_REQUIRE_COOKIES: {
      struct RequireLoginCookie* vRequireLoginCookie =
          new struct RequireLoginCookie;
      *packet_head = (struct PacketHead*) vRequireLoginCookie;

      FILLHEAD()
      ;

      vRequireLoginCookie->manage_id = in.Read32();
      int temp = 0;
      //memcpy(vRequireLoginCookie->token, in.ReadData(TOKEN_SIZE -1, temp), TOKEN_SIZE -1);
      int token_len =
          (temp - 1) < (TOKEN_SIZE - 1) ? (temp - 1) : (TOKEN_SIZE - 1);
      //vRequireLoginCookie->token[token_len] = '\0';
      vRequireLoginCookie->attr_id = in.Read64();
      vRequireLoginCookie->amount = in.Read8();
      break;
    }

    case CRAWLER_AVAILABLE_RESOURCE_NUM: {

      struct CrawlerAvailableResourceNum* vCrawlerAvailableResource =
          new struct CrawlerAvailableResourceNum;
      *packet_head = (struct PacketHead*) vCrawlerAvailableResource;
      FILLHEAD()
      ;
      vCrawlerAvailableResource->task_num = in.Read16();
      LOG_DEBUG2("vCrawlerAvailableResource->task_num = %d", (int)vCrawlerAvailableResource->task_num);
      break;
    }

    case HEART_PACKET:
    case GET_HARDINFO:
    case TEMP_CRAWLER_OP: {
      struct PacketHead* vHead = new struct PacketHead;
      *packet_head = (struct PacketHead*) vHead;
      FILLHEAD()
      ;
      break;
    }
    case PLUGIN_SVC_REG_STATE: {
      struct PluginSvcRegState *plg_svc_reg_state = new PluginSvcRegState;
      *packet_head = plg_svc_reg_state;

      FILLHEAD()
      ;

      int temp = 0;
      memcpy(plg_svc_reg_state->token, in.ReadData(TOKEN_SIZE - 1, temp),
      TOKEN_SIZE - 1);
      int token_len =
          (temp - 1) < (TOKEN_SIZE - 1) ? (temp - 1) : (TOKEN_SIZE - 1);
      plg_svc_reg_state->token[token_len] = '\0';
      break;
    }
    case PLUGIN_SVC_MGR_ROUTER_REG: {
      struct PluginSvcMgrRouterReg *router_reg = new PluginSvcMgrRouterReg;
      *packet_head = router_reg;

      FILLHEAD()
      ;

      int temp = 0;
      memcpy(router_reg->token, in.ReadData(TOKEN_SIZE - 1, temp),
      TOKEN_SIZE - 1);
      int32 token_len =
          (temp - 1) < (TOKEN_SIZE - 1) ? (temp - 1) : (TOKEN_SIZE - 1);
      router_reg->token[token_len] = '\0';

      router_reg->router_id = in.Read32();

      break;
    }
    case PLUGIN_SVC_MGR_ROUTER_REG_STATE: {
      struct PluginSvcMgrRouterRegState *router_reg_state =
          new PluginSvcMgrRouterRegState;
      *packet_head = router_reg_state;

      FILLHEAD()
      ;

      router_reg_state->router_id = in.Read32();
      router_reg_state->state = in.Read8();

      break;
    }
    case ROUTER_STATUS: {
      struct RouterStatus *status = new RouterStatus();
//		LOG_DEBUG2("new RouterStatus: %X", status);
      *packet_head = status;
      FILLHEAD()
      ;
      uint32 read_bytes = 0;
      status->router_id = in.Read32();
      read_bytes += sizeof(int32);
      while (status->data_length - read_bytes >= ROUTER_STATUS_UNIT_SIZE) {
        struct RouterStatusUnit *unit = new RouterStatusUnit();
//			LOG_DEBUG2("new RouterStatusUnit: %X", unit);
        unit->crawler_type = in.Read16();
        unit->max_tasks = in.Read32();
        unit->cur_tasks = in.Read32();
        read_bytes += ROUTER_STATUS_UNIT_SIZE;
        status->status_list.push_back(unit);
      }
      break;
    }
    case ROBOT_TASK_STATE: {
      struct RobotTaskStatus *status = new RobotTaskStatus();
      *packet_head = status;
      FILLHEAD()
      ;
      status->task_type = in.Read16();
      status->is_success = in.Read8();
      status->task_id = in.Read64();
      status->cookie_id = in.Read64();
      break;
    }
    default:
      r = false;
      break;
  }
  return r;
}

void PacketProsess::DeletePacket(const void* packet_stream, int32 len,
                                 struct PacketHead* packet_head) {
  packet::DataInPacket in_packet(reinterpret_cast<const char*>(packet_stream),
                                 len);
  int16 packet_length = in_packet.Read16();
  int8 is_zip_encrypt = in_packet.Read8();
  char* body_stream = NULL;
  int32 body_length = 0;
  int32 temp = 0;

  // 是否解压解码
  if (is_zip_encrypt == ZIP_AND_NOENCRYPT) {
    char* temp_body_stream =
        reinterpret_cast<char*>(const_cast<void*>(packet_stream))
            + sizeof(int16) + sizeof(int8);
    const uint8* zipdata = reinterpret_cast<uint8*>(temp_body_stream);
    uint8* unzipdata = NULL;
    int32 temp_body_len = len - sizeof(int16) - sizeof(int8);
    body_length = DecompressionStream(zipdata, temp_body_len, &unzipdata);
    body_stream = reinterpret_cast<char*>(unzipdata);
  } else {
    body_stream = reinterpret_cast<char*>(const_cast<void*>(packet_stream))
        + sizeof(int16) + sizeof(int8);
    body_length = len - sizeof(int16) - sizeof(int8);
  }

  BUILDPACKHEAD();

  switch (operate_code) {
    case CRAWLER_MGR_REG:
    case ANALYTICAL_REG: {
      struct CrawlerMgrReg* vCrawlerMgrReg = (struct CrawlerMgrReg*) packet_head;
      delete vCrawlerMgrReg;
      break;
    }

    case CRAWLER_MGR_REGSTATE: {
      struct CrawlerMgrRegState* vCrawlerMgrRegState =
          (struct CrawlerMgrRegState*) packet_head;
      delete vCrawlerMgrRegState;
      break;
    }

    case CRAWLER_REG_FAILED: {
      struct CrawlerFailed* vCrawlerFailed = (struct CrawlerFailed*) packet_head;
      delete vCrawlerFailed;
      break;
    }

    case REPLY_HARDINFO: {
      struct ReplyHardInfo* vReplyHardInfo = (struct ReplyHardInfo*) packet_head;
      delete vReplyHardInfo;
      break;
    }

    case ASSIGNMENT_SINGLE_TASK: {
      struct AssignmentSingleTask* vAssignmentSingleTask =
          (struct AssignmentSingleTask*) packet_head;
      delete vAssignmentSingleTask;
    }

    case ASSIGNMENT_MULTI_TASK: {
      struct AssignmentMultiTask* vAssignmentMultiTask =
          (struct AssignmentMultiTask*) packet_head;
      delete vAssignmentMultiTask;
      break;
    }

    case ASSIGNMENT_DETAIL_TASK: {
      struct AssignmentDetailTask* vAssignmentDetailTask =
          (struct AssignmentDetailTask*) packet_head;
      delete vAssignmentDetailTask;
      break;
    }

    case REPLY_TASK_STATE: {
      struct ReplyTaskState* vReplyTaskState =
          (struct ReplyTaskState*) packet_head;
      delete vReplyTaskState;
      break;
    }

    case REPLY_DETAIL_STATE: {
      struct ReplyDetailState* vReplyDetailState =
          (struct ReplyDetailState*) packet_head;
      delete vReplyDetailState;
      break;
    }

    case GET_CRAWL_CONTENT_NUM: {
      struct CrawlContentNum* vCrawlContentNum =
          (struct CrawlContentNum*) packet_head;
      delete vCrawlContentNum;
      break;
    }

    case REPLY_CRAWL_CONTENT_NUM: {
      struct ReplyCrawlContentNum* vReplyCrawlContentNum =
          (struct ReplyCrawlContentNum*) packet_head;
      delete vReplyCrawlContentNum;
      break;
    }
    case CRAWL_HBASE_STORAGE_INFO:
    case CRAWL_FTP_STORAGE_INFO: {
      struct CrawlStorageInfo* vCrawlStorageInfo =
          (struct CrawlStorageInfo*) packet_head;
      delete vCrawlStorageInfo;
      break;
    }

    case GET_FORGEINFO: {
      struct GetForgeInfo* vGetForgeInfo = (struct GetForgeInfo*) packet_head;
      delete vGetForgeInfo;
      break;
    }

    case REPLY_FOGEINFO_UA: {
      struct ReplyUAForgeInfo* vReplyUAForgeInfo =
          (struct ReplyUAForgeInfo*) packet_head;
      delete vReplyUAForgeInfo;
      break;
    }
    case REPLY_FOGEINFO_IP: {
      struct ReplyIPForgeInfo* vReplyIPForgeInfo =
          (struct ReplyIPForgeInfo*) packet_head;
      delete vReplyIPForgeInfo;
      break;
    }

    case ANALYTICAL_INFO: {
      struct AnalysiInfo* vAnalysiInfo = (struct AnalysiInfo*) packet_head;
      delete vAnalysiInfo;
      break;
    }

    case ANALYTICAL_URL_SET: {
      struct AnalyticalURLSet* vAnalyticalURLSet =
          (struct AnalyticalURLSet*) packet_head;
      delete vAnalyticalURLSet;
      break;
    }
    case ANALYTICAL_STATE: {
      struct AnalysiState* vAnalysiState = (struct AnalysiState*) packet_head;
      delete vAnalysiState;
      break;
    }
    case LOGIN_REQUIRE_COOKIES: {

      //LOG_DEBUG2("%s","begin to parse request!");
      struct RequireLoginCookie* vRequireLoginCookie =
          (struct RequireLoginCookie*) packet_head;
      delete vRequireLoginCookie;
      break;
    }

    case CRAWLER_AVAILABLE_RESOURCE_NUM: {

      struct CrawlerAvailableResourceNum* vCrawlerAvailableResource =
          (struct CrawlerAvailableResourceNum*) packet_head;
      delete vCrawlerAvailableResource;
      break;
    }

    case HEART_PACKET:
    case GET_HARDINFO:
    case TEMP_CRAWLER_OP: {
      struct PacketHead* vHead = (struct PacketHead*) packet_head;
      delete vHead;
      break;
    }
    case PLUGIN_SVC_REG_STATE: {
      struct PluginSvcRegState *reg_state =
          (struct PluginSvcRegState *) packet_head;
      delete reg_state;
      break;
    }
    case PLUGIN_SVC_MGR_ROUTER_REG: {
      struct PluginSvcMgrRouterReg *router_reg =
          (struct PluginSvcMgrRouterReg *) packet_head;
      delete router_reg;
      break;
    }
    case PLUGIN_SVC_MGR_ROUTER_REG_STATE: {
      struct PluginSvcMgrRouterRegState *reg_state =
          (struct PluginSvcMgrRouterRegState *) packet_head;
      delete reg_state;
      break;
    }
    case DELIVERED_COOKIE_SET: {
      struct LoginCookieSet *login_cookie_set =
          (struct LoginCookieSet *) packet_head;
      delete login_cookie_set;
      break;
    }
    case ROUTER_STATUS: {
      struct RouterStatus *router_status = (struct RouterStatus *) packet_head;
      std::list<struct RouterStatusUnit *>::iterator it = router_status
          ->status_list.begin();
      for (; it != router_status->status_list.end();) {
//			LOG_DEBUG2("delete RouterStatusUnit: %X", *it);
        delete *it++;
      }
//		LOG_DEBUG2("delete RouterStatus: %X", router_status);
      delete router_status;
      router_status = NULL;
      break;
    }
    case ASSIGN_WEIBO_TASK: {
      struct AssingWeiboTask *weibo_task = (struct AssingWeiboTask*) packet_head;
      delete weibo_task;
      break;
    }
    case ROBOT_TASK_STATE: {
      struct RobotTaskStatus *status = (struct RobotTaskStatus*) packet_head;
      delete status;
      break;
    }
    case ASSIGN_TIANYA_TASK: {
      struct AssignTianyaTask *tianya_task =
          (struct AssignTianyaTask*) packet_head;
      delete tianya_task;
      break;
    }
    default:
      break;
  }
}

void PacketProsess::DumpPacket(const struct PacketHead* packet_head) {
  int16 packet_length = packet_head->packet_length;
  int8 is_zip_encrypt = packet_head->is_zip_encrypt;
  int8 type = packet_head->type;
  int16 operate_code = packet_head->operate_code;
  int16 data_length = packet_head->data_length;
  int32 timestamp = packet_head->timestamp;
  int64 session_id = packet_head->session_id;
  int16 crawler_type = packet_head->crawler_type;
  int32 crawler_id = packet_head->crawler_id;
  int16 server_type = packet_head->server_type;
  int32 server_id = packet_head->server_id;
  int32 reserved = packet_head->reserved;
  int16 signature = packet_head->signature;

  char buf[DUMPPACKBUF];
  memset(buf, '\0', sizeof(buf));
  bool r = false;
  int32 j = 0;

  switch (operate_code) {
    case CRAWLER_MGR_REG:
    case ANALYTICAL_REG: {
      struct CrawlerMgrReg* vCrawlerMgrReg = (struct CrawlerMgrReg*) packet_head;
      PRINT_TITLE("struct CrawlerMgrReg Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT(vCrawlerMgrReg->level);
      PRINT_STRING(vCrawlerMgrReg->password);
      PRINT_STRING(vCrawlerMgrReg->mac);
      PRINT_END("struct CrawlerMgrReg Dump End");
      break;
    }
    case REGISTER_SERVER: {
      struct RegisterServer *svc_reg = (struct RegisterServer *) packet_head;
      PRINT_TITLE("struct PluginSvcMgrReg Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT(svc_reg->level);
      PRINT_STRING(svc_reg->password);
      PRINT_STRING(svc_reg->mac);
      PRINT_INT(svc_reg->port);
      PRINT_END("struct PluginSvcMgrReg Dump End");
      break;
    }
    case CRAWLER_MGR_REGSTATE: {
      struct CrawlerMgrRegState* vCrawlerMgrRegState =
          (struct CrawlerMgrRegState*) packet_head;
      PRINT_TITLE("struct CrawlerMgrRegState Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT(vCrawlerMgrRegState->id);
      PRINT_STRING(vCrawlerMgrRegState->token);
      PRINT_END("struct CrawlerMgrRegState Dump End");
      break;
    }

    case CRAWLER_REG_FAILED: {
      struct CrawlerFailed* vCrawlerFailed = (struct CrawlerFailed*) packet_head;
      PRINT_TITLE("struct CrawlerFailed Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT(vCrawlerFailed->erron_code);
      PRINT_END("struct CrawlerFailed Dump End");
      break;
    }

    case REPLY_HARDINFO: {
      struct ReplyHardInfo* vReplyHardInfo = (struct ReplyHardInfo*) packet_head;
      PRINT_TITLE("struct ReplyHardInfo Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT(vReplyHardInfo->id);
      PRINT_STRING(vReplyHardInfo->token);
      PRINT_STRING(vReplyHardInfo->cpu);
      PRINT_STRING(vReplyHardInfo->mem);
      PRINT_END("struct ReplyHardInfo Dump End");
      break;
    }

    case ASSIGNMENT_SINGLE_TASK: {
      struct AssignmentSingleTask* vAssignmentSingleTask =
          (struct AssignmentSingleTask*) packet_head;
      PRINT_TITLE("struct AssignmentSingleTask Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT(vAssignmentSingleTask->id);
      PRINT_INT64(vAssignmentSingleTask->task_id);
      PRINT_INT(vAssignmentSingleTask->depth);
      PRINT_INT(vAssignmentSingleTask->machine);
      PRINT_INT(vAssignmentSingleTask->storage);
      PRINT_END("struct AssignmentSingleTask Dump End");
      break;
    }
    case ASSIGNMENT_MULTI_TASK: {
      struct AssignmentMultiTask* vAssignmentMultiTask =
          (struct AssignmentMultiTask*) packet_head;
      std::list<struct TaskUnit*>::iterator it = vAssignmentMultiTask->task_set
          .begin();
      PRINT_TITLE("struct AssignmentMultiTask Dump Begin");
      DUMPHEAD()
      ;
      for (; it != vAssignmentMultiTask->task_set.end(); it++) {
        PRINT_INT((*it)->len);
        PRINT_INT64((*it)->task_id);
        PRINT_INT64((*it)->attr_id);
        PRINT_INT64((*it)->unix_time);
        PRINT_INT64((*it)->max_depth);
        PRINT_INT64((*it)->current_depth);
        PRINT_INT64((*it)->machine);
        PRINT_INT64((*it)->storage);
        PRINT_INT64((*it)->is_login);
        PRINT_INT64((*it)->is_over);
        PRINT_INT64((*it)->is_forge);
        PRINT_INT64((*it)->method);
        PRINT_STRING((*it)->url);
      }
      PRINT_END("struct AssignmentDetailTask Dump End");
      break;
    }
    case ASSIGNMENT_DETAIL_TASK: {
      struct AssignmentDetailTask* vAssignmentDetailTask =
          (struct AssignmentDetailTask*) packet_head;
      std::list<struct TaskUnit*>::iterator it = vAssignmentDetailTask->task_set
          .begin();
      PRINT_TITLE("struct AssignmentDetailTask Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT(vAssignmentDetailTask->id);
      for (; it != vAssignmentDetailTask->task_set.end(); it++) {
        PRINT_INT64((*it)->task_id);
        PRINT_INT64((*it)->attr_id);
        PRINT_INT64((*it)->unix_time);
        PRINT_INT64((*it)->max_depth);
        PRINT_INT64((*it)->current_depth);
        PRINT_INT64((*it)->machine);
        PRINT_INT64((*it)->storage);
        PRINT_INT64((*it)->is_login);
        PRINT_INT64((*it)->is_over);
        PRINT_INT64((*it)->is_forge);
        PRINT_INT64((*it)->method);
        PRINT_STRING((*it)->url);
      }
      PRINT_END("struct AssignmentDetailTask Dump End");
      break;
    }
    case REPLY_TASK_STATE: {
      struct ReplyTaskState* vReplyTaskState =
          (struct ReplyTaskState*) packet_head;
      PRINT_TITLE("struct ReplyTaskState Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT(vReplyTaskState->jobid);
      //PRINT_STRING(vReplyTaskState->token);
      PRINT_INT(vReplyTaskState->jobid);
      PRINT_INT(vReplyTaskState->state);
      PRINT_END("struct ReplyTaskState Dump End");
      break;
    }
    case REPLY_DETAIL_STATE: {
      struct ReplyDetailState* vReplyDetailState =
          (struct ReplyDetailState*) packet_head;
      PRINT_TITLE("struct ReplyDetailState Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT(vReplyDetailState->id);
      PRINT_STRING(vReplyDetailState->token);
      PRINT_INT(vReplyDetailState->jobid);
      PRINT_INT(vReplyDetailState->state);
      PRINT_END("struct ReplyDetailState Dump End");
      break;
    }
    case GET_CRAWL_CONTENT_NUM: {
      struct CrawlContentNum* vCrawlContentNum =
          (struct CrawlContentNum*) packet_head;
      PRINT_TITLE("struct CrawlContentNum Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT(vCrawlContentNum->id);
      PRINT_INT64(vCrawlContentNum->jobid);
      PRINT_END("struct CrawlContentNum Dump End");
      break;
    }
    case REPLY_CRAWL_CONTENT_NUM: {
      struct ReplyCrawlContentNum* vReplyCrawlContentNum =
          (struct ReplyCrawlContentNum*) packet_head;
      PRINT_TITLE("struct ReplyCrawlContentNum Dump Begin");
      PRINT_INT(vReplyCrawlContentNum->id);
      PRINT_STRING(vReplyCrawlContentNum->token);
      PRINT_INT64(vReplyCrawlContentNum->task_id);
      PRINT_INT(vReplyCrawlContentNum->num);
      PRINT_END("struct ReplyCrawlContentNum Dump End");
      break;
    }
    case CRAWL_HBASE_STORAGE_INFO:
    case CRAWL_FTP_STORAGE_INFO: {
      struct CrawlStorageInfo* vCrawlStorageInfo =
          (struct CrawlStorageInfo*) packet_head;
      std::list<struct StorageUnit*>::iterator it = vCrawlStorageInfo
          ->storage_set.begin();
      PRINT_TITLE("struct CrawlStorageInfo Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT(vCrawlStorageInfo->crawler_id);
      //PRINT_STRING(vCrawlStorageInfo->token);
      for (; it != vCrawlStorageInfo->storage_set.end(); it++) {
        PRINT_INT((*it)->len);
        PRINT_INT64((*it)->task_id);
        PRINT_INT64((*it)->attr_id);
        PRINT_INT((*it)->max_depth);
        PRINT_INT((*it)->cur_depth);
        PRINT_STRING((*it)->table_name);
        PRINT_STRING((*it)->rowkey.c_str());
      }
      PRINT_END("struct CrawlStorageInfo Dump End");
      break;
    }
    case GET_FORGEINFO: {
      struct GetForgeInfo* vGetForgeInfo = (struct GetForgeInfo*) packet_head;
      PRINT_TITLE("struct GetForgeInfo Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT(vGetForgeInfo->task_id);
      //PRINT_STRING(vGetForgeInfo->token);
      PRINT_INT64(vGetForgeInfo->task_id);
      PRINT_INT(vGetForgeInfo->forge_type);
      PRINT_INT(vGetForgeInfo->num);
      PRINT_END("struct GetForgeInfo Dump End");
      break;
    }

    case REPLY_FOGEINFO_IP: {
      struct ReplyIPForgeInfo* vReplyForgeInfo =
          (struct ReplyIPForgeInfo*) packet_head;
      std::list<struct IPForgeInfo*>::iterator it = vReplyForgeInfo->forgen_set
          .begin();
      PRINT_TITLE("struct ReplyIPForgeInfo Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT64(vReplyForgeInfo->task_id);
      for (; it != vReplyForgeInfo->forgen_set.end(); it++) {
        PRINT_INT((*it)->len);
        PRINT_INT((*it)->id);
        PRINT_INT((*it)->type);
        PRINT_STRING((*it)->forgen_info);
      }
      PRINT_END("struct ReplyIPForgeInfo Dump End");
      break;
    }
    case REPLY_FOGEINFO_UA: {
      struct ReplyUAForgeInfo* vReplyForgeInfo =
          (struct ReplyUAForgeInfo*) packet_head;
      std::list<struct UAForgeInfo*>::iterator it = vReplyForgeInfo->forgen_set
          .begin();
      PRINT_TITLE("struct ReplyUAForgeInfo Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT64(vReplyForgeInfo->task_id);
      for (; it != vReplyForgeInfo->forgen_set.end(); it++) {
        PRINT_INT((*it)->len);
        PRINT_INT((*it)->id);
        PRINT_INT((*it)->type);
        PRINT_STRING((*it)->forgen_info);
      }
      PRINT_END("struct ReplyUAForgeInfo Dump End");
      break;
    }
    case ANALYTICAL_INFO: {
      struct AnalysiInfo* vAnalysiInfo = (struct AnalysiInfo*) packet_head;
      std::list<struct AnalysiUnit*>::iterator it = vAnalysiInfo->analysi_set
          .begin();
      PRINT_TITLE("struct AnalysiInfo Dump Begin");
      DUMPHEAD()
      ;
      for (; it != vAnalysiInfo->analysi_set.end(); it++) {
        PRINT_INT64((*it)->ayalysi_id);
        PRINT_INT64((*it)->task_id);
        PRINT_INT((*it)->attr_id);
        PRINT_INT((*it)->type);
        PRINT_INT((*it)->cur_depth);
        PRINT_INT((*it)->max_depth);
        PRINT_STRING((*it)->name);
        PRINT_STRING((*it)->key);
      }
      PRINT_END("struct AnalysiInfo Dump End");
      break;
    }
    case ANALYTICAL_STATE: {
      struct AnalysiState* vAnalysiState = (struct AnalysiState*) packet_head;
      PRINT_TITLE("struct AnalysiState Dump Begin");
      DUMPHEAD()
      ;
      PRINT_INT64(vAnalysiState->id);
      PRINT_INT(vAnalysiState->state);
      PRINT_END("struct AnalysiState Dump End");
      break;
    }
      /*      case ANALYTICAL_URL_SET : {
       struct AnalyticalURLSet* vAnalyticalURLSet =
       (struct AnalyticalURLSet*)packet_head;
       std::list<struct AnalyticalURLUnit*>::iterator it =
       vAnalyticalURLSet->analytical_url_set.begin();
       PRINT_TITLE("struct AnalyticalURLSet Dump Begin");
       DUMPHEAD();
       PRINT_INT(vAnalyticalURLSet->id);
       PRINT_STRING(vAnalyticalURLSet->token);
       for (; it != vAnalyticalURLSet->analytical_url_set.end(); it++) {
       PRINT_INT64((*it)->task_id);
       PRINT_INT64((*it)->attr_id);
       PRINT_INT64((*it)->max_depth);
       PRINT_INT64((*it)->current_depth);
       PRINT_INT64((*it)->method);
       PRINT_STRING((*it)->url);
       }
       PRINT_END("struct AnalyticalURLSet Dump End");
       break;
       }*/

    case HEART_PACKET:
    case GET_HARDINFO:
    case TEMP_CRAWLER_OP: {
      PRINT_TITLE("struct PacketHead Dump Begin");
      DUMPHEAD()
      ;
      PRINT_END("struct PacketHead Dump End");
    }
    case ROUTER_STATUS: {
      PRINT_TITLE("struct RouterStatus Dump Begin");
      DUMPHEAD()
      ;
      struct RouterStatus *router_status = (struct RouterStatus *) packet_head;
      std::list<struct RouterStatusUnit *>::iterator it = router_status
          ->status_list.begin();
      for (; it != router_status->status_list.end(); ++it) {
        PRINT_INT((*it)->crawler_type);
        PRINT_INT((*it)->max_tasks);
        PRINT_INT((*it)->cur_tasks);
      }
      PRINT_END("struct RouterStatus Dump End");
      break;
    }
    case ASSIGN_WEIBO_TASK: {
      PRINT_TITLE("struct AssingWeiboTask Dump Begin");
      DUMPHEAD()
      ;
      struct AssingWeiboTask *weibo_task = (struct AssingWeiboTask*) packet_head;
      PRINT_INT(weibo_task->tasks_num);
      std::list<struct WeiboTaskUnit*>::iterator it =
          weibo_task->task_set.begin();
      for (; weibo_task->task_set.end() != it; ++it) {
        PRINT_INT64((*it)->task_id);
        PRINT_INT64((*it)->cookie_id);

        PRINT_INT((*it)->cookie.size());
        PRINT_STRING((*it)->cookie.c_str());

        PRINT_INT((*it)->content.size());
        PRINT_STRING((*it)->content.c_str());

        PRINT_INT((*it)->addr.size());
        PRINT_STRING((*it)->addr.c_str());

        PRINT_INT((*it)->topic_id.size());
        PRINT_STRING((*it)->topic_id.c_str());

        PRINT_INT((*it)->host_uin.size());
        PRINT_STRING((*it)->host_uin.c_str());
      }
      PRINT_END("struct AssingWeiboTask Dump End");
      break;
    }
    case ASSIGN_TIANYA_TASK: {
      PRINT_TITLE("struct AssingTianyaTask Dump Begin");
      DUMPHEAD()
      ;
      struct AssignTianyaTask *tianya_task =
          (struct AssignTianyaTask*) packet_head;
      PRINT_INT(tianya_task->tasks_num);
      std::list<struct TianyaTaskUnit*>::iterator it = tianya_task->task_set
          .begin();
      for (; tianya_task->task_set.end() != it; ++it) {
        PRINT_INT64((*it)->task_id);
        PRINT_INT64((*it)->cookie_id);

        PRINT_INT((*it)->cookie.size());
        PRINT_STRING((*it)->cookie.c_str());

        PRINT_INT((*it)->content.size());
        PRINT_STRING((*it)->content.c_str());

        PRINT_INT((*it)->addr.size());
        PRINT_STRING((*it)->addr.c_str());

        PRINT_INT64((*it)->pre_post_time);

        PRINT_INT((*it)->pre_url.size());
        PRINT_STRING((*it)->pre_url.c_str());

        PRINT_INT((*it)->pre_title.size());
        PRINT_STRING((*it)->pre_title.c_str());

        PRINT_INT((*it)->pre_user_id.size());
        PRINT_STRING((*it)->pre_user_id.c_str());

        PRINT_INT((*it)->pre_user_name.size());
        PRINT_STRING((*it)->pre_user_name.c_str());
      }
      PRINT_END("struct AssingTianyaTask Dump End");
      break;

    }
    case ASSIGN_TIEBA_TASK: {
      PRINT_TITLE("struct AssingTiebaTask Dump Begin");
      DUMPHEAD()
      ;
      struct AssignTiebaTask *tieba_task = (struct AssignTiebaTask*) packet_head;
      PRINT_INT(tieba_task->tasks_num);
      std::list<struct TiebaTaskUnit*>::iterator it =
          tieba_task->task_set.begin();
      for (; tieba_task->task_set.end() != it; ++it) {
        PRINT_INT((*it)->task_len);
        PRINT_INT((*it)->task_type);
        PRINT_INT64((*it)->task_id);
        PRINT_INT((*it)->row_key.size());
        PRINT_STRING((*it)->row_key.c_str());
        PRINT_INT64((*it)->cookie_id);

        PRINT_INT((*it)->cookie.size());
        PRINT_STRING((*it)->cookie.c_str());

        PRINT_INT((*it)->addr.size());
        PRINT_STRING((*it)->addr.c_str());

        PRINT_INT((*it)->url.size());
        PRINT_STRING((*it)->url.c_str());

        PRINT_INT((*it)->ua.size());
        PRINT_STRING((*it)->ua.c_str());
      }
      PRINT_END("struct AssingTiebaTask Dump End");
      break;
    }
    case ASSIGN_QZONE_TASK: {
      PRINT_TITLE("struct AssingQzoneTask Dump Begin");
      DUMPHEAD()
      ;
      struct AssignQzoneTask *qzone_task = (struct AssignQzoneTask*) packet_head;
      PRINT_INT(qzone_task->tasks_num);
      std::list<struct QzoneTaskUnit*>::iterator it =
          qzone_task->task_set.begin();
      for (; qzone_task->task_set.end() != it; ++it) {
        PRINT_INT64((*it)->task_id);
        PRINT_INT64((*it)->cookie_id);

        PRINT_INT((*it)->cookie.size());
        PRINT_STRING((*it)->cookie.c_str());

        PRINT_INT((*it)->content.size());
        PRINT_STRING((*it)->content.c_str());

        PRINT_INT((*it)->addr.size());
        PRINT_STRING((*it)->addr.c_str());

        PRINT_INT64((*it)->host_uin);

        PRINT_INT((*it)->topic_id.size());
        PRINT_STRING((*it)->topic_id.c_str());
      }
      PRINT_END("struct AssingQzoneTask Dump End");
      break;
    }
    default:
      break;
  }
  if (buf[0] != '\0')
    LOG_DEBUG2("%s\n", buf);
}

void PacketProsess::HexEncode(const void *bytes, size_t size) {
  struct PacketHead* head = (struct PacketHead*) bytes;
  static const char kHexChars[] = "0123456789ABCDEF";
  std::string sret(size * 3, '\0');
  for (size_t i = 0; i < size; ++i) {
    char b = reinterpret_cast<const char*>(bytes)[i];
    sret[(i * 3)] = kHexChars[(b >> 4) & 0xf];
    sret[(i * 3) + 1] = kHexChars[b & 0xf];
    if ((((i * 3) + 2 + 1) % 24) != 0)
      sret[(i * 3) + 2] = '\40';
    else
      sret[(i * 3) + 2] = '\n';
  }LOG_DEBUG2("===start====\nopcode[%d]:\n%s\n====end====\n",
      head->operate_code, sret.c_str());
}

void PacketProsess::ClearCrawlerTaskList(struct PacketHead* packet_head) {
  struct AssignmentMultiTask* task = (struct AssignmentMultiTask*) packet_head;
  while (task->task_set.size() > 0) {
    struct TaskUnit* unit = task->task_set.front();
    task->task_set.pop_front();
    if (unit) {
      LOG_DEBUG2("delete task unit: %X", unit);
      delete unit;
      unit = NULL;
    }
  }
}

void PacketProsess::ClearDetailTaskList(struct PacketHead* packet_head) {
  struct AssignmentDetailTask* task = (struct AssignmentDetailTask*) packet_head;
  while (task->task_set.size() > 0) {
    struct TaskUnit* unit = task->task_set.front();
    task->task_set.pop_front();
    if (unit) {
      delete unit;
      unit = NULL;
    }
  }
}

void PacketProsess::ClearHBaseAnalyticalTaskList(
    struct PacketHead* packet_head) {
  struct AnalysiInfo* task = (struct AnalysiInfo*) packet_head;
  while (task->analysi_set.size() > 0) {
    struct AnalysiUnit* unit = task->analysi_set.front();
    task->analysi_set.pop_front();
    if (unit) {
      delete unit;
      unit = NULL;
    }
  }
}
void PacketProsess::ClearLoginCookieList(struct PacketHead* packet_head) {
  struct LoginCookieSet* set = (struct LoginCookieSet*) packet_head;
  while (set->login_cookie_set.size() > 0) {
    struct LoginCookieUnit* unit = set->login_cookie_set.front();
    set->login_cookie_set.pop_front();
    if (unit) {
      delete unit;
      unit = NULL;
    }
  }
}

void PacketProsess::ClearWeiboTaskList(struct PacketHead* packet_head) {
  struct AssingWeiboTask *set = (struct AssingWeiboTask*) packet_head;
  while (set->task_set.size() > 0) {
    struct WeiboTaskUnit* unit = set->task_set.front();
    set->task_set.pop_front();
    if (unit) {
      delete unit;
      unit = NULL;
    }
  }
}

void PacketProsess::ClearTianyaTaskList(struct PacketHead* packet_head) {
  struct AssignTianyaTask *set = (struct AssignTianyaTask*) packet_head;
  while (set->task_set.size() > 0) {
    struct TianyaTaskUnit* unit = set->task_set.front();
    set->task_set.pop_front();
    if (unit) {
      delete unit;
      unit = NULL;
    }
  }
}

void PacketProsess::ClearTiebaTaskList(struct PacketHead* packet_head) {
  struct AssignTiebaTask *set = (struct AssignTiebaTask*) packet_head;
  while (set->task_set.size() > 0) {
    struct TiebaTaskUnit* unit = set->task_set.front();
    set->task_set.pop_front();
    if (unit) {
      delete unit;
      unit = NULL;
    }
  }
}

void PacketProsess::ClearQzoneTaskList(struct PacketHead* packet_head) {
  struct AssignQzoneTask *set = (struct AssignQzoneTask*) packet_head;
  while (set->task_set.size() > 0) {
    struct QzoneTaskUnit* unit = set->task_set.front();
    set->task_set.pop_front();
    if (unit) {
      delete unit;
      unit = NULL;
    }
  }
}

uint64 PacketProsess::CompressionStream(const uint8* unzip_data,
                                        uint64 unzip_len, uint8** zip_data) {
  MZip zip_engine;
  unzip_len = zip_engine.ZipData(unzip_data, unzip_len, zip_data);
  return unzip_len;
}

uint64 PacketProsess::DecompressionStream(const uint8* zip_data, uint64 zip_len,
                                          uint8** unzip_data) {
  MUnZip unzip_engine;
  zip_len = unzip_engine.UnZipData(zip_data, zip_len, unzip_data);
  return zip_len;
}

}  //  namespace net

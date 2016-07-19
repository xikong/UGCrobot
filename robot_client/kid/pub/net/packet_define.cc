/*
 * packet_define.cc
 *
 *  Created on: 2016年4月7日
 *      Author: Harvey
 */

#include "net/packet_define.h"
#include "net/comm_head.h"

#define DUMP_LOG

PacketHead::PacketHead() {
    in_ = NULL;
    out_ = NULL;
}

PacketHead::~PacketHead() {
    delete in_;
    delete out_;
}

bool PacketHead::PackHead(const int packet_length) {

    out_ = new packet::DataOutPacket(
            false, packet_length - sizeof(int16) - sizeof(int8));

    //压入除包长度和是否压缩标志的包头信息
    out_->Write8(this->type_);
    out_->Write16(this->signature_);
    out_->Write16(this->operate_code_);
    out_->Write16(this->data_length_);
    out_->Write32(this->timestamp_);
    out_->Write64(this->session_id_);
    out_->Write16(this->crawler_type_);
    out_->Write32(this->crawler_id_);
    out_->Write16(this->server_type_);
    out_->Write32(this->server_id_);
    out_->Write32(this->reserved_);

    return true;
}

bool PacketHead::UnpackHead(const void* packet_stream, int32 len) {

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
        body_length = net::PacketProsess::DecompressionStream(zipdata,
                                                              temp_body_len,
                                                              &unzipdata);
        body_stream = reinterpret_cast<char*>(unzipdata);
    } else {
        body_stream = reinterpret_cast<char*>(const_cast<void*>(packet_stream))
                + sizeof(int16) + sizeof(int8);
        body_length = len - sizeof(int16) - sizeof(int8);
    }

    in_ = new packet::DataInPacket(reinterpret_cast<char*>(body_stream),
                                   body_length);

    this->packet_length_ = packet_length;
    this->is_zip_encrypt_ = is_zip_encrypt;

    this->type_ = in_->Read8();
    this->signature_ = in_->Read16();
    this->operate_code_ = in_->Read16();
    this->data_length_ = in_->Read16();
    this->timestamp_ = in_->Read32();
    this->session_id_ = in_->Read64();
    this->crawler_type_ = in_->Read16();
    this->crawler_id_ = in_->Read32();
    this->server_type_ = in_->Read16();
    this->server_id_ = in_->Read32();
    this->reserved_ = in_->Read32();

#if 0
    LOG_MSG2("Receive Server Msg, PacketLength = %d, ServerType = %d, Opcode = %d",
            len, this->server_type_, this->operate_code_);
#endif

    return true;
}

bool PacketHead::PackStream(void **packet_stream, int32 &packet_stream_length) {

    this->PackHead(packet_stream_length);

    //是否压缩加密
    net::PacketProsess::IsZipPacket(this->is_zip_encrypt_, PACKET_HEAD_LENGTH,
                                    this->out_, packet_stream,
                                    packet_stream_length);

    return true;
}

bool RobotRegisterInfo::PackStream(void **packet_stream,
                                   int32 &packet_stream_length) {

    //压包头
    this->PackHead(ROBOT_REGISTER_INFO_SIZE);

    //压包体
    this->out_->Write16(this->level);
    this->out_->WriteData(this->passwd, PASSWORD_SIZE - 1);
    this->out_->WriteData(this->mac, MAC_SIZE - 1);

    //是否压缩加密
    net::PacketProsess::IsZipPacket(this->is_zip_encrypt_,
                                    ROBOT_REGISTER_INFO_SIZE, this->out_,
                                    packet_stream, packet_stream_length);

    return true;
}

bool RobotRegisterSuccess::UnpackStream(const void* packet_stream, int32 len) {

    this->UnpackHead(packet_stream, len);

    //解包体
    int temp = 0;

    //分配的router_ip
    memcpy(this->router_ip, this->in_->ReadData(IP_FORGEN_SIZE - 1, temp),
           IP_FORGEN_SIZE - 1);
    int32 ip_len =
            (temp - 1) < (IP_FORGEN_SIZE - 1) ?
                    (temp - 1) : (IP_FORGEN_SIZE - 1);
    this->router_ip[ip_len] = '\0';

    //分配的router_port
    this->router_port = this->in_->Read16();

    //分配的token
    memcpy(this->token, this->in_->ReadData(TOKEN_SIZE - 1, temp),
           TOKEN_SIZE - 1);
    int32 token_len =
            (temp - 1) < (TOKEN_SIZE - 1) ? (temp - 1) : (TOKEN_SIZE - 1);
    this->token[token_len] = '\0';

    return true;
}

bool RobotRequestLoginRouter::PackStream(void **packet_stream,
                                         int32 &packet_stream_length) {

    this->PackHead(REG_LOGIN_ROUTER_SIZE);

    this->out_->WriteData(this->token, TOKEN_SIZE);

    //是否压缩加密
    net::PacketProsess::IsZipPacket(this->is_zip_encrypt_,
                                    REG_LOGIN_ROUTER_SIZE, this->out_,
                                    packet_stream, packet_stream_length);

    return true;
}

bool RobotLoginRouterResult::UnpackStream(const void *packet_stream,
                                          int32 len) {

    this->UnpackHead(packet_stream, len);

    this->is_success = this->in_->Read8();

    return true;
}

bool RobotStatePacket::PackStream(void **packet_stream,
                                  int32 &packet_stream_length) {

    this->PackHead(ROBOT_STATE_SIZE);

    this->out_->Write32(max_task_num);
    this->out_->Write32(curr_task_num);

    //是否压缩加密
    net::PacketProsess::IsZipPacket(this->is_zip_encrypt_, ROBOT_STATE_SIZE,
                                    this->out_, packet_stream,
                                    packet_stream_length);

    return true;
}

bool TaskHead::UnpackTaskHead(packet::DataInPacket *in, int &temp) {

    this->task_id_ = in->Read64();
    this->cookie_id_ = in->Read64();

    ReadDataByLen(this->cookie_, temp, in);
    ReadDataByLen(this->content_, temp, in);
    ReadDataByLen(this->forge_ip_, temp, in);
    ReadDataByLen(this->forge_ua_, temp, in);

    LOG_DEBUG2("task_id = %d", this->task_id_);
    LOG_DEBUG2("cookie_id = %d", this->cookie_id_);
    LOG_DEBUG2("cookie = %s", this->cookie_.c_str());
    LOG_DEBUG2("content = %s", this->content_.c_str());
    LOG_DEBUG2("forge_ip = %s", this->forge_ip_.c_str());
    LOG_DEBUG2("forge_ua = %s", this->forge_ua_.c_str());

    return true;
}

bool TaskWeiBoPacket::UnpackTaskBody(packet::DataInPacket *in, int &temp) {

    this->UnpackTaskHead(in, temp);

    ReadDataByLen(this->topic_id_, temp, in);
    ReadDataByLen(this->host_uin_, temp, in);

    return true;
}

bool TaskTianYaPacket::UnpackTaskBody(packet::DataInPacket *in, int &temp) {

    this->UnpackTaskHead(in, temp);

    this->pre_post_time_ = in->Read64();
    ReadDataByLen(this->pre_url_, temp, in);
    ReadDataByLen(this->pre_title_, temp, in);
    ReadDataByLen(this->pre_user_id_, temp, in);
    ReadDataByLen(this->pre_user_name_, temp, in);

    return true;
}

bool TaskTieBaPacket::UnpackTaskBody(packet::DataInPacket *in, int &temp) {

    this->UnpackTaskHead(in, temp);

    ReadDataByLen(this->pre_url_, temp, in);
    ReadDataByLen(this->kw_, temp, in);
    ReadDataByLen(this->fid_, temp, in);
    ReadDataByLen(this->tbs_, temp, in);
    this->floor_num_ = in->Read32();
    ReadDataByLen(this->repost_id_, temp, in);

    LOG_DEBUG2("pre_url = %s", this->pre_url_.c_str()); LOG_DEBUG2("kw = %s", this->kw_.c_str()); LOG_DEBUG2("fid = %s", this->fid_.c_str()); LOG_DEBUG2("tbs = %s", this->tbs_.c_str()); LOG_DEBUG2("floor_num = %d", this->floor_num_); LOG_DEBUG2("repost_id = %s", this->repost_id_.c_str());

    return true;
}

bool TaskQQPacket::UnpackTaskBody(packet::DataInPacket *in, int &temp) {

    this->UnpackTaskHead(in, temp);

    this->host_uin_ = in->Read64();
    ReadDataByLen(this->topic_id_, temp, in);

    return true;
}

bool TaskMopPacket::UnpackTaskBody(packet::DataInPacket *in, int &temp) {

    //解任务头
    this->UnpackTaskHead(in, temp);

    ReadDataByLen(this->pre_url_, temp, in);
    ReadDataByLen(this->pCatId_, temp, in);
    ReadDataByLen(this->catalogId_, temp, in);
    ReadDataByLen(this->fmtoken_, temp, in);
    ReadDataByLen(this->currformid_, temp, in);

    return true;
}

bool TaskDouBanPacket::UnpackTaskBody(packet::DataInPacket *in, int &temp) {

    this->UnpackTaskHead(in, temp);

    ReadDataByLen(this->pre_url_, temp, in);

    return true;
}

bool TaskTaoGuBaPacket::UnpackTaskBody(packet::DataInPacket *in, int &temp) {

    this->UnpackTaskHead(in, temp);

    ReadDataByLen(this->topicID_, temp, in);
    ReadDataByLen(this->subject_, temp, in);

    LOG_DEBUG2("topic_id = %s", this->topicID_.c_str()); LOG_DEBUG2("subject = %s", this->subject_.c_str());

    return true;
}

bool TaskXueQiuPacket::UnpackTaskBody(packet::DataInPacket *in, int &temp) {

    this->UnpackTaskHead(in, temp);

    ReadDataByLen(this->pre_url_, temp, in);
    ReadDataByLen(this->topic_id_, temp, in);

    LOG_DEBUG2("pre_url = %s", this->pre_url_.c_str()); LOG_DEBUG2("topic_id = %s", this->topic_id_.c_str());

    return true;
}

bool TaskIGuBaPacket::UnpackTaskBody(packet::DataInPacket *in, int &temp) {

    this->UnpackTaskHead(in, temp);

    ReadDataByLen(this->pre_url_, temp, in);
    LOG_DEBUG2("pre_url = %s", this->pre_url_.c_str());

    return true;
}

bool TaskTongHuaShunPacket::UnpackTaskBody(packet::DataInPacket *in, int &temp){

    this->UnpackTaskHead(in, temp);

    ReadDataByLen(this->pre_url_, temp, in);

    LOG_DEBUG2("pre_url = %s", this->pre_url_.c_str());

    return true;
}


bool MultiTaskList::UnpackStream(const void *packet_stream, int32 len) {

    this->UnpackHead(packet_stream, len);

    int temp = 0;
    int8 task_num = this->in_->Read16();

    for (int8 i = 0; i < task_num; ++i) {

        int task_type = this->in_->Read16();
        LOG_DEBUG2("task_type = %d", task_type);

        struct TaskHead *task = NULL;
        switch (task_type) {
            case TASK_WEIBO: {
                struct TaskWeiBoPacket *task_temp = new struct TaskWeiBoPacket;
                task_temp->UnpackTaskBody(this->in_, temp);
                task = (struct TaskHead *) task_temp;
                break;
            }
            case TASK_TIANYA: {
                struct TaskTianYaPacket *task_temp = new struct TaskTianYaPacket;
                task_temp->UnpackTaskBody(this->in_, temp);
                task = (struct TaskHead *) task_temp;
                break;
            }
            case TASK_TIEBA: {
                struct TaskTieBaPacket *task_temp = new struct TaskTieBaPacket;
                task_temp->UnpackTaskBody(this->in_, temp);
                task = (struct TaskHead *) task_temp;
                break;
            }
            case TASK_QQ: {
                struct TaskQQPacket *task_temp = new struct TaskQQPacket;
                task_temp->UnpackTaskBody(this->in_, temp);
                task = (struct TaskHead *) task_temp;
                break;
            }
            case TASK_MOP: {
                struct TaskMopPacket *task_temp = new struct TaskMopPacket;
                task_temp->UnpackTaskBody(this->in_, temp);
                task = (struct TaskHead *) task_temp;
                break;
            }
            case TASK_DOUBAN: {
                struct TaskDouBanPacket *task_temp = new struct TaskDouBanPacket;
                task_temp->UnpackTaskBody(this->in_, temp);
                task = (struct TaskHead *) task_temp;
                break;
            }
            case TASK_TAOGUBA: {
                struct TaskTaoGuBaPacket *task_temp =
                        new struct TaskTaoGuBaPacket;
                task_temp->UnpackTaskBody(this->in_, temp);
                task = (struct TaskHead *) task_temp;
                break;
            }
            case TASK_XUEQIU: {
                struct TaskXueQiuPacket *task_temp = new struct TaskXueQiuPacket;
                task_temp->UnpackTaskBody(this->in_, temp);
                task = (struct TaskHead *) task_temp;
                break;
            }
            case TASK_TONGHUASHUN:{
                struct TaskTongHuaShunPacket *task_temp = new struct TaskTongHuaShunPacket;
                task_temp->UnpackTaskBody(this->in_, temp);
                task = (struct TaskHead *)task_temp;
                break;
            }
                //TUDO 其他任务
            default:
                break;
        }

        if (NULL == task) {
            LOG_MSG2("Not Support This Task, task_type = %d", task_type);
            continue;
        }

        //设置任务类型等
        task->task_type_ = task_type;
        task->feed_server_id_ = this->server_id_;

        this->multi_task_list_.push_back(task);
    }

    return true;
}

bool FeedBackTaskStatus::PackStream(void **packet_stream,
                                    int32 &packet_stream_length) {

    //包体长度
    int feed_back_msg_size = FEEDBACK_TASK_STATUS_SIZE + this->error_code.size()
            + sizeof(int16);

    this->PackHead(feed_back_msg_size);

    this->out_->Write16(this->task_type);
    this->out_->Write8(this->is_success);
    this->out_->Write16(this->error_code.size());
    this->out_->WriteData(this->error_code.c_str(), this->error_code.size());
    this->out_->Write64(this->task_id);
    this->out_->Write64(this->cookie_id);

    //是否压缩加密
    net::PacketProsess::IsZipPacket(this->is_zip_encrypt_, feed_back_msg_size,
                                    this->out_, packet_stream,
                                    packet_stream_length);

    return true;
}

bool ReadDataByLen(string &data, int &temp, packet::DataInPacket *in) {

    if (NULL == in) {
        return false;
    }

    int16 str_len = 0;
    char str_temp[4096];
    memset(str_temp, 0, 4096);

    str_len = in->Read16();
    memcpy(str_temp, in->ReadData(str_len, temp), str_len);
    data = string(str_temp);

    return true;
}

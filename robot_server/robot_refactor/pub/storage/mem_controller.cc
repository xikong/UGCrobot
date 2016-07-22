//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015年9月17日 Author: kerry

#include "storage/mem_controller.h"
#include <string>
#include "libmemcached/memcached.h"
#include "libmemcachedutil-1.0/pool.h"
#include "dic/base_dic_redis_auto.h"

namespace base_logic {

MemController::MemController() {
  engine_ = base_storage::DictionaryStorageEngine::Create(
      base_storage::IMPL_MEM);
}

void MemController::Release() {
}

void MemController::InitParam(std::list<base::ConnAddr>* addrlist) {
  engine_->Connections(*addrlist);
}

bool MemController::ReadData(const int32 type, base_logic::Value* value,
                             void (*storage_get)(void*, base_logic::Value*)) {
  bool r = false;
  switch (type) {
    case MEM_KEY_VALUE:
      r = ReadKeyValueData(value);
      break;
    case BATCH_KEY_VALUE:
      r = ReadBatchKeyValue(value);
      break;
    default:
      break;
  }
  return r;
}

bool MemController::WriteData(const int32 type, base_logic::Value* value) {
  bool r = false;
  switch (type) {
    case MEM_KEY_VALUE:
      WriteKeyValueData(value);
      break;
    default:
      break;
  }
  return r;
}

bool MemController::WriteKeyValueData(base_logic::Value* value) {
  bool r = false;
  base_logic::DictionaryValue* dict = (base_logic::DictionaryValue*) (value);
  base_logic::BinaryValue* binary = NULL;
  std::string key;
  char* str_value = NULL;
  size_t value_len = 0;
  r = dict->GetString(L"memkey", &key);
  if (!r)
    return r;
  r = dict->GetBinary(L"memvalue", &binary);
  if (!r)
    return r;
  r = engine_->SetValue(key.c_str(), key.length(), binary->GetBuffer(),
                        binary->GetSize());
  return r;
}

bool MemController::ReadKeyValueData(base_logic::Value* value) {
  base_logic::DictionaryValue* dict = (base_logic::DictionaryValue*) (value);
  base_logic::BinaryValue* binary = NULL;
  std::string key;
  char* str_value = NULL;
  size_t value_len = 0;
  bool r = dict->GetString(L"memkey", &key);
  if (!r)
    return r;
  r = engine_->GetValue(key.c_str(), key.length(), &str_value, &value_len);
  if (!r || value == NULL || value_len <= 0)
    return false;
  binary = base_logic::BinaryValue::Create(str_value, value_len);
  dict->Set(L"memvalue", (base_logic::Value*) (binary));
  return true;
}

bool MemController::ReadBatchKeyValue(base_logic::Value* value) {
  bool r = false;
  base_logic::DictionaryValue* dict = (base_logic::DictionaryValue*) (value);
  base_logic::BinaryValue* keys_binary = NULL;
  base_logic::BinaryValue* keys_len_binary = NULL;
  base_logic::ListValue* list = new base_logic::ListValue();
  int32 count = 0;
  r = dict->GetBinary(L"batchmemkeys", &keys_binary);
  if (!r)
    return false;
  r = dict->GetBinary(L"batchmemkeyslen", &keys_len_binary);
  if (!r)
    return false;
  r = dict->GetInteger(L"batchmemkeyscount", &count);
  if (!r)
    return false;

  const char** keys = (const char**) (keys_binary->GetBuffer());
  const size_t * keys_len = (const size_t *) (keys_len_binary->GetBuffer());
  r = engine_->MGetValue(keys, keys_len, count);
  if (!r)
    return false;
  char return_key[MEMCACHED_MAX_KEY] = { 0 };
  size_t return_key_length = MEMCACHED_MAX_KEY;
  char *return_value = NULL;
  size_t return_value_length = 0;
  while (engine_->FetchValue(return_key, &return_key_length, &return_value,
                             &return_value_length)) {
    if (return_key && return_value) {
      list->Append(
          base_logic::Value::CreateBinaryValue(return_value,
                                               return_value_length));
    }
  }
  if (return_value)
    free(return_value);
  dict->Set(L"resultvalue", (base_logic::Value*) (list));
  return true;
}
}  // namespace base_logic


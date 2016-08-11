/*
 * sys_param.cc
 *
 *  Created on: 2016年8月10日
 *      Author: zjc
 */

#include "sys_param.h"

namespace step {

SysParam::SysParam(FieldMap& field_map)
    : Header(field_map){
}

SysParam::~SysParam() {
}

} /* namespace step */

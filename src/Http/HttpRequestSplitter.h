﻿/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xia-chu/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_HTTPREQUESTSPLITTER_H
#define ZLMEDIAKIT_HTTPREQUESTSPLITTER_H

#include <string>
#include "Network/Buffer.h"
using namespace std;
using namespace toolkit;

namespace mediakit {

class HttpRequestSplitter {
public:
    HttpRequestSplitter(){};
    virtual ~HttpRequestSplitter(){};

    /**
     * 添加数据
     * @param data 需要添加的数据
     * @param len 数据长度
     */
    virtual void input(const char *data,size_t len);

protected:
    /**
     * 收到请求头
     * @param data 请求头数据
     * @param len 请求头长度
     *
     * @return 请求头后的content长度,
     *  <0 : 代表后面所有数据都是content，此时后面的content将分段通过onRecvContent函数回调出去
     *  0 : 代表为后面数据还是请求头,
     *  >0 : 代表后面数据为固定长度content,此时将缓存content并等到所有content接收完毕一次性通过onRecvContent函数回调出去
     */
    virtual ssize_t onRecvHeader(const char *data,size_t len) = 0;

    /**
     * 收到content分片或全部数据
     * onRecvHeader函数返回>0,则为全部数据
     * @param data content分片或全部数据
     * @param len 数据长度
     */
    virtual void onRecvContent(const char *data,size_t len) {};

    /**
     * 判断数据中是否有包尾,不同协议覆盖onSearchPacketTail来解析自己的流格式
     * @param data 数据指针
     * @param len 数据长度
     * @return nullptr代表未找到包位，否则返回包尾指针
     */
    virtual const char *onSearchPacketTail(const char *data, size_t len);

    /**
     * 设置content len
     */
    void setContentLen(ssize_t content_len);

    /**
     * 恢复初始设置
     */
     void reset();

     /**
      * 剩余数据大小
      */
     size_t remainDataSize();

private:
    ssize_t _content_len = 0;
    size_t _remain_data_size = 0;
    BufferLikeString _remain_data;
};

} /* namespace mediakit */

#endif //ZLMEDIAKIT_HTTPREQUESTSPLITTER_H

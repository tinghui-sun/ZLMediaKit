﻿/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xia-chu/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#if defined(ENABLE_UDPTS)
#include <stddef.h>
#include "UdpTsSelector.h"

namespace mediakit{

INSTANCE_IMP(UdpTsSelector);

void UdpTsSelector::clear(){
    lock_guard<decltype(_mtx_map)> lck(_mtx_map);
	_map_process.clear();
}


UdpTsProcess::Ptr UdpTsSelector::getProcess(const string &stream_id,bool makeNew) {
    lock_guard<decltype(_mtx_map)> lck(_mtx_map);
    auto it = _map_process.find(stream_id);
    if (it == _map_process.end() && !makeNew) {
        return nullptr;
    }

    UdpTsProcessHelper::Ptr &ref = _map_process[stream_id];
    if (!ref) {
        ref = std::make_shared<UdpTsProcessHelper>(stream_id, shared_from_this());
        ref->attachEvent();
        createTimer();
    }
    return ref->getProcess();
}

void UdpTsSelector::createTimer() {
    if (!_timer) {
        //创建超时管理定时器
        weak_ptr<UdpTsSelector> weakSelf = shared_from_this();
        _timer = std::make_shared<Timer>(3.0f, [weakSelf] {
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return false;
            }
            strongSelf->onManager();
            return true;
        }, EventPollerPool::Instance().getPoller());
    }
}

void UdpTsSelector::delProcess(const string &stream_id,const UdpTsProcess *ptr) {
    UdpTsProcess::Ptr process;
    {
        lock_guard<decltype(_mtx_map)> lck(_mtx_map);
        auto it = _map_process.find(stream_id);
        if (it == _map_process.end()) {
            return;
        }
        if (it->second->getProcess().get() != ptr) {
            return;
        }
        process = it->second->getProcess();
		_map_process.erase(it);
    }
    process->onDetach();
}

void UdpTsSelector::onManager() {
    List<UdpTsProcess::Ptr> clear_list;
    {
        lock_guard<decltype(_mtx_map)> lck(_mtx_map);
        for (auto it = _map_process.begin(); it != _map_process.end();) {
            if (it->second->getProcess()->alive()) {
                ++it;
                continue;
            }
            WarnL << "UdpTsProcess timeout:" << it->first;
            clear_list.emplace_back(it->second->getProcess());
            it = _map_process.erase(it);
        }
    }

    clear_list.for_each([](const UdpTsProcess::Ptr &process) {
        process->onDetach();
    });
}

UdpTsSelector::UdpTsSelector() {
}

UdpTsSelector::~UdpTsSelector() {
}

UdpTsProcessHelper::UdpTsProcessHelper(const string &stream_id, const weak_ptr<UdpTsSelector> &parent) {
    _stream_id = stream_id;
    _parent = parent;
    _process = std::make_shared<UdpTsProcess>(stream_id);
}

UdpTsProcessHelper::~UdpTsProcessHelper() {
}

void UdpTsProcessHelper::attachEvent() {
    _process->setListener(shared_from_this());
}

bool UdpTsProcessHelper::close(MediaSource &sender, bool force) {
    //此回调在其他线程触发
    if (!_process || (!force && _process->getTotalReaderCount())) {
        return false;
    }
    auto parent = _parent.lock();
    if (!parent) {
        return false;
    }
    parent->delProcess(_stream_id, _process.get());
    WarnL << "close media:" << sender.getSchema() << "/" << sender.getVhost() << "/" << sender.getApp() << "/" << sender.getId() << " " << force;
    return true;
}

int UdpTsProcessHelper::totalReaderCount(MediaSource &sender) {
    return _process ? _process->getTotalReaderCount() : sender.totalReaderCount();
}

UdpTsProcess::Ptr &UdpTsProcessHelper::getProcess() {
    return _process;
}

}//namespace mediakit
#endif//defined(ENABLE_UDPTS)
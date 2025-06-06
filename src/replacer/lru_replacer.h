#pragma once

#include <list>
#include <mutex>
#include <vector>

#include "common/config.h"
#include "replacer/replacer.h"
#include "unordered_map"

/*
LRUReplacer实现了LRU替换策略
*/
class LRUReplacer : public Replacer
{
public:
    /**
     * @description: 创建一个新的LRUReplacer
     * @param {size_t} num_pages LRUReplacer最多需要存储的page数量
     */
    explicit LRUReplacer(size_t num_pages);

    ~LRUReplacer();

    bool victim(frame_id_t *frame_id);

    void pin(frame_id_t frame_id);

    void unpin(frame_id_t frame_id);

    size_t Size();

private:
    std::mutex latch_;                                                        // 互斥锁
    std::list<frame_id_t> LRUlist_;                                           // 按加入的时间顺序存放unpinned pages的frame id，首部表示最近被访问
    std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> LRUhash_; // frame_id_t -> unpinned pages的frame id
    size_t max_size_ = BUFFER_POOL_SIZE;                                      // 最大容量（与缓冲池的容量相同）
};

#pragma once

#include <cassert>
#include <memory>
#include <queue>
#include <utility>

#include "common/context_finals.h"
#include "rm_defs_finals.h"

class RmFileHandle
{
public:
    int record_size;
    bool ban = false;
    std::vector<char *> records;

    explicit RmFileHandle(int record_size) : record_size(record_size) {}

    std::unique_ptr<RmRecord> get_record(char *rid)
    {
        return std::make_unique<RmRecord>(rid, record_size);
    }

    void insert_record(char *rid)
    {
        if (ban)
        {
            return;
        }
        records.emplace_back(rid);
    }

    void delete_record(char *rid)
    {
        if (ban)
        {
            return;
        }
        records.erase(std::remove(records.begin(), records.end(), rid), records.end());
    }

    void update_record(const char *old_rid_, char *new_rid_)
    {
        if (ban)
        {
            return;
        }
        for (auto &rid_ : records)
        {
            if (rid_ == old_rid_)
            {
                rid_ = new_rid_;
                return;
            }
        }
    }
};

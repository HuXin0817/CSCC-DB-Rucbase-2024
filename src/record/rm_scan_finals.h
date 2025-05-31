#pragma once

#include <queue>

#include "rm_defs_finals.h"

class RmScan : public RecScan
{
    std::vector<char *>::const_iterator it;
    std::vector<char *>::const_iterator end;

public:
    RmScan(RmFileHandle *file_handle)
    {
        it = file_handle->records.begin();
        end = file_handle->records.end();
    }

    void next() override
    {
        it++;
    }

    bool is_end() const override
    {
        return it == end;
    }

    char *rid() const override
    {
        return *it;
    }
};

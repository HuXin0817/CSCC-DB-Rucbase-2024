#pragma once

#include <mutex>

#include "ix_index_handle_finals.h"

class IxScan : public RecScan
{
private:
    rmdb_btree::iterator it, end;

public:
    IxScan(const rmdb_btree::iterator &lower_key, const rmdb_btree::iterator &upper_key)
    {
        it = lower_key;
        end = upper_key;
    }

    void next() override { ++it; }

    bool is_end() const override { return it == end; }

    char *rid() const override { return *it; }
};

#pragma once

#include <queue>
#include <shared_mutex>
#include <utility>

#include "btree.h"
#include "common/context_finals.h"
#include "common/value_finals.h"
#include "transaction/transaction_finals.h"

class IxScan;

class IndexScanExecutor;

#define rmdb_btree btree::btree_set<char *, IxCompare>

class IxCompare
{
public:
    IxCompare() = default;

    explicit IxCompare(const IndexMeta &index_meta) : cols(index_meta.cols_) {}

    bool operator()(const char *a, const char *b) const
    {
        for (const auto &col : cols)
        {
            auto value1 = a + col.offset;
            auto value2 = b + col.offset;
            switch (col.type)
            {
            case TYPE_INT:
            {
                int ia = *((int *)value1);
                int ib = *(int *)value2;
                if (ia != ib)
                {
                    return ia < ib;
                }
                break;
            }
            case TYPE_FLOAT:
            {
                float fa = *(float *)value1;
                float fb = *(float *)value2;
                if (fa != fb)
                {
                    return fa < fb;
                }
                break;
            }
            case TYPE_STRING:
            {
                auto res = std::memcmp(value1, value2, col.len);
                if (res != 0)
                {
                    return res < 0;
                }
                break;
            }
            }
        }
        return false;
    }

private:
    std::vector<ColMeta> cols;
};

class IxIndexHandle
{
public:
    static bool unique_check;

    rmdb_btree bp_tree_;

public:
    explicit IxIndexHandle(const IndexMeta &index_meta) : bp_tree_(IxCompare(index_meta)) {}

    bool exists_entry(char *key) const { return bp_tree_.contains(key); }

    void insert_entry(char *key) { bp_tree_.insert(key); }

    void delete_entry(char *key) { bp_tree_.erase(key); }

    auto upper_bound(char *key) const { return bp_tree_.upper_bound(key); }

    auto lower_bound(char *key) const { return bp_tree_.lower_bound(key); }

    auto begin() const { return bp_tree_.begin(); }

    auto end() const { return bp_tree_.end(); }
};

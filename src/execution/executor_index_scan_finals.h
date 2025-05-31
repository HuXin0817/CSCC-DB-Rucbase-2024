#pragma once

#include <climits>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#include "execution_manager_finals.h"
#include "executor_abstract_finals.h"
#include "executor_gap_lock_finals.h"
#include "index/ix_memory_scan_finals.h"
#include "record/rm_scan_finals.h"

class IndexScanExecutor : public AbstractExecutor
{
private:
    TabMeta *tab_;
    RmFileHandle *fh_;
    std::vector<ColMeta> *cols_;
    IxIndexHandle *ih_;
    std::unique_ptr<IxScan> scan_;
    char *rid_ = nullptr;
    std::unique_ptr<GapLockExecutor> gap_lock;
    Context *context_;

public:
    IndexScanExecutor(SmManager *sm_manager, const std::string &tab_name, const std::vector<Condition> &conds, const IndexMeta &index_meta_, Context *context)
    {
        context_ = context;
        tab_ = sm_manager->db_.get_table(tab_name);
        fh_ = sm_manager->fhs_[tab_->fd_].get();
        cols_ = &tab_->cols;
        ih_ = sm_manager->ihs_[index_meta_.fd_].get();
        gap_lock = std::make_unique<GapLockExecutor>(sm_manager, tab_, conds, context_);

        auto lower_position_ = ih_->lower_bound(gap_lock->lower_key_);
        auto upper_position_ = ih_->upper_bound(gap_lock->upper_key_);
        scan_ = std::make_unique<IxScan>(lower_position_, upper_position_);
    }

    void beginTuple() override
    {
        find_next_valid_tuple();
    }

    void nextTuple() override
    {
        scan_->next();
        find_next_valid_tuple();
    }

    std::unique_ptr<RmRecord> Next() override { return fh_->get_record(rid_); }

    bool is_end() const override { return scan_->is_end(); }

    char *rid() const override { return rid_; }

    size_t tupleLen() const override { return fh_->record_size; }

    const std::vector<ColMeta> &cols() const override { return *cols_; }

private:
    void find_next_valid_tuple()
    {
        while (!scan_->is_end())
        {
            rid_ = scan_->rid();
            if (gap_lock->gap->overlap(rid_))
            {
                return;
            }
            scan_->next();
        }
    }
};

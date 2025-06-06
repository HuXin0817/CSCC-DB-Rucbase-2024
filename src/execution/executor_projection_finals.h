#pragma once

#include <typeinfo>

#include "execution_manager_finals.h"
#include "execution_merge_join_finals.h"
#include "execution_sort_finals.h"
#include "executor_abstract_finals.h"
#include "executor_index_scan_finals.h"
#include "executor_seq_scan_finals.h"

class ProjectionExecutor : public AbstractExecutor
{
private:
    std::unique_ptr<AbstractExecutor> prev_; // 投影节点的儿子节点
    std::vector<ColMeta> cols_;              // 需要投影的字段

public:
    ProjectionExecutor(std::unique_ptr<AbstractExecutor> prev, const std::vector<TabCol> &sel_cols)
    {
        prev_ = std::move(prev);

        auto &prev_cols = prev_->cols();
        for (auto &sel_col : sel_cols)
        {
            auto pos = get_col(prev_cols, sel_col);
            auto& col = *pos;
            cols_.push_back(col);
        }
    }

    const std::vector<ColMeta> &cols() const override
    {
        if (dynamic_cast<IndexScanExecutor *>(prev_.get()) != nullptr)
            return cols_;
        else if (dynamic_cast<NestedLoopJoinExecutor *>(prev_.get()) != nullptr)
            return cols_;
        else if (dynamic_cast<MergeJoinExecutor *>(prev_.get()) != nullptr)
            return cols_;
        else if (dynamic_cast<SeqScanExecutor *>(prev_.get()) != nullptr)
            return cols_;
        else if (dynamic_cast<SortExecutor *>(prev_.get()) != nullptr)
            return cols_;
        else
            return prev_->cols();
    };

    void beginTuple() override { prev_->beginTuple(); }

    void nextTuple() override { prev_->nextTuple(); }

    std::unique_ptr<RmRecord> Next() override { return prev_->Next(); }

    bool is_end() const override { return prev_->is_end(); };
};

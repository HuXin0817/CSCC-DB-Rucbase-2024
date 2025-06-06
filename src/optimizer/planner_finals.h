#pragma once

#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "analyze/analyze_finals.h"
#include "common/common_finals.h"
#include "common/context_finals.h"
#include "execution/execution_manager_finals.h"
#include "parser/parser.h"
#include "plan_finals.h"

class Planner
{
private:
    SmManager *sm_manager_;

    bool enable_nestedloop_join = true;
    bool enable_sortmerge_join = false;

public:
    Planner(SmManager *sm_manager) : sm_manager_(sm_manager) {}

    std::shared_ptr<Plan> do_planner(std::shared_ptr<Query> query, Context *context);

    void set_enable_nestedloop_join(bool set_val)
    {
        enable_nestedloop_join = set_val;
    }

    void set_enable_sortmerge_join(bool set_val)
    {
        enable_sortmerge_join = set_val;
    }

private:
    std::shared_ptr<Query> logical_optimization(std::shared_ptr<Query> query, Context *context);
    std::shared_ptr<Plan> physical_optimization(const std::shared_ptr<Query> &query, Context *context);

    std::shared_ptr<Plan> make_one_rel(const std::shared_ptr<Query> &query, Context *context = nullptr);

    std::shared_ptr<Plan> generate_sort_plan(const std::shared_ptr<Query> &query, std::shared_ptr<Plan> plan);

    std::shared_ptr<Plan> generate_select_plan(std::shared_ptr<Query> query, Context *context);

    static std::shared_ptr<Plan> generate_agg_plan(const std::shared_ptr<Query> &query, std::shared_ptr<Plan> plan);

    IndexMeta get_index_cols(const std::string &tab_name, const std::vector<Condition> &curr_conds);

    ColType interp_sv_type(ast::SvType sv_type)
    {
        std::map<ast::SvType, ColType> m = {{ast::SV_TYPE_INT, TYPE_INT}, {ast::SV_TYPE_FLOAT, TYPE_FLOAT}, {ast::SV_TYPE_STRING, TYPE_STRING}, {ast::SV_TYPE_DATETIME, TYPE_STRING}};
        return m.at(sv_type);
    }
    std::shared_ptr<Plan> generate_join_sort_plan(const std::string &table, std::vector<Condition> &conds, TabCol &col, std::shared_ptr<Plan> plan);

    bool get_merge_join_index(const std::string &tab_name, const TabCol &col);
};

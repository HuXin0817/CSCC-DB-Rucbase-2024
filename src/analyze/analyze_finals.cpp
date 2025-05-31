#include "analyze_finals.h"

std::shared_ptr<Query> Analyze::do_analyze(std::shared_ptr<ast::TreeNode> parse)
{
    const auto &db_ = sm_manager_->db_;

    std::shared_ptr<Query> query = std::make_shared<Query>();
    if (auto x = std::dynamic_pointer_cast<ast::SelectStmt>(parse))
    {
        // 处理表名
        query->tables = std::move(x->tabs);

        for (const auto &table : query->tables)
        {
            if (!db_.is_table(table))
            {
                throw RMDBError();
            }
        }
        bool only_one_table = query->tables.size() == 1;

        // 处理target list，再target list中添加上表名，例如 a.id
        for (auto &sv_sel_col : x->cols)
        {
            TabCol sel_col = {.tab_name = sv_sel_col->tab_name, .col_name = sv_sel_col->col_name, .alias = sv_sel_col->alias};
            if (only_one_table)
            {
                sel_col.tab_name = query->tables[0];
            }
            if (auto y = std::dynamic_pointer_cast<ast::AggFunc>(sv_sel_col))
            {
                sel_col.aggFuncType = y->type;
                x->has_agg = true;
            }
            query->cols.push_back(sel_col);
        }

        std::vector<ColMeta> all_cols;
        get_all_cols(query->tables, all_cols);
        if (query->cols.empty())
        {
            // select all columns
            for (const auto &col : all_cols)
            {
                TabCol sel_col = {.tab_name = col.tab_name, .col_name = col.name};
                query->cols.push_back(sel_col);
            }
        }
        else
        {
            // infer table name from column name
            for (auto &sel_col : query->cols)
            {
                if (sel_col.col_name != "*")
                {
                    sel_col = check_column(all_cols, sel_col); // 列元数据校验
                }
            }
        }
        // 处理group_by
        get_having(x->group_by, query->having_conds, query->tables[0], query->cols);

        // 处理where条件
        get_clause(x->conds, query->conds);
        check_clause(query->tables, query->conds);
    }
    else if (auto x = std::dynamic_pointer_cast<ast::UpdateStmt>(parse))
    {
        // 处理表名
        query->tables.push_back(x->tab_name);

        // 检查表是否存在
        if (!sm_manager_->db_.is_table(x->tab_name))
        {
            throw RMDBError();
        }

        // 处理需要更新的列和值
        for (const auto &set_clause : x->set_clauses)
        {
            // 使用局部变量来避免多次创建对象
            SetClause update_clause(convert_sv_value(set_clause->val));

            if (set_clause->self_update)
            {
                update_clause.set_op(set_clause->op);
            }
            else
            {
                update_clause.op = UpdateOp::ASSINGMENT;
            }

            // 类型转换
            auto tab = sm_manager_->db_.get_table(x->tab_name);
            auto &col = tab->get_col(set_clause->col_name);

            // 如果类型不匹配，进行类型转换
            if (col.type != update_clause.rhs.type)
            {
                if (!can_cast_type(update_clause.rhs.type, col.type))
                {
                    throw RMDBError();
                }
                else
                {
                    cast_value(update_clause.rhs, col.type);
                }
            }

            // 将更新的 SetClause 添加到 query 中
            update_clause.lhs = std::make_unique<ColMeta>(col);
            query->set_clauses.push_back(update_clause);
        }

        // 处理where条件
        get_clause(x->conds, query->conds);
        check_clause(query->tables, query->conds);
    }
    else if (auto x = std::dynamic_pointer_cast<ast::DeleteStmt>(parse))
    {
        // 处理表名
        query->tables.push_back(x->tab_name);

        // 检查表是否存在
        if (!sm_manager_->db_.is_table(x->tab_name))
        {
            throw RMDBError();
        }

        // 处理where条件
        get_clause(x->conds, query->conds);
        check_clause(query->tables, query->conds);
    }
    else if (auto x = std::dynamic_pointer_cast<ast::InsertStmt>(parse))
    {
        // 处理表名
        query->tables.push_back(x->tab_name);

        // 检查表是否存在
        if (!sm_manager_->db_.is_table(x->tab_name))
        {
            throw RMDBError();
        }

        // 处理insert的values值
        for (auto &sv_val : x->vals)
        {
            query->values.push_back(convert_sv_value(sv_val));
        }
    }
    else
    {
        // do nothing
    }
    query->parse = std::move(parse);
    return query;
}

TabCol Analyze::check_column(const std::vector<ColMeta> &all_cols, TabCol &target)
{
    if (target.tab_name.empty())
    {
        // Table name not specified, infer table name from column name
        std::string inferred_tab_name;
        bool found = false;
        for (const auto &col : all_cols)
        {
            if (col.name == target.col_name)
            {
                if (found)
                {
                    throw RMDBError();
                }
                inferred_tab_name = col.tab_name;
                found = true;
            }
        }
        if (!found)
        {
            throw RMDBError();
        }
        target.tab_name = inferred_tab_name;
    }
    else
    {
        // Make sure target column exists
        auto it = std::find_if(all_cols.begin(), all_cols.end(), [&target](const ColMeta &col)
                               { return col.tab_name == target.tab_name && col.name == target.col_name; });
        if (it == all_cols.end())
        {
            throw RMDBError();
        }
    }
    return target;
}

void Analyze::get_all_cols(const std::vector<std::string> &tab_names, std::vector<ColMeta> &all_cols)
{
    for (auto &sel_tab_name : tab_names)
    {
        // 这里db_不能写成get_db(), 注意要传指针
        const auto &sel_tab_cols = sm_manager_->db_.get_table(sel_tab_name)->cols;
        all_cols.insert(all_cols.end(), sel_tab_cols.begin(), sel_tab_cols.end());
    }
}

void Analyze::get_having(std::shared_ptr<ast::GroupBy> &group_by, std::vector<HavingCond> &having_conds, const std::string &table_name, const std::vector<TabCol> &select_cols)
{
    bool has_non_agg_col = false;
    bool has_agg_func = false;

    // 检查 select_cols 中是否同时包含聚合函数和非聚合列
    for (const auto &sel_col : select_cols)
    {
        if (sel_col.aggFuncType != ast::default_type)
        {
            has_agg_func = true;
        }
        else
        {
            has_non_agg_col = true;
        }

        // 如果同时存在聚合函数和非聚合列，但没有 GROUP BY 子句，抛出异常
        if (has_non_agg_col && has_agg_func && !group_by)
        {
            throw RMDBError();
        }
    }

    if (!group_by)
    {
        return;
    }

    const auto &sel_tab_cols = sm_manager_->db_.get_table(table_name)->cols;

    // 标记 group_by 中的列
    for (auto &col : group_by->cols)
    {
        col->tab_name = table_name;
    }
    // 检查 select_cols 的合法性
    for (const auto &select_col : select_cols)
    {
        bool is_valid = false;
        // 检查是否在 GROUP BY 列中
        for (const auto &group_col : group_by->cols)
        {
            if (select_col.col_name == group_col->col_name && select_col.tab_name == group_col->tab_name)
            {
                is_valid = true;
                break;
            }
        }
        // 检查是否是聚合函数
        if (!is_valid && select_col.aggFuncType != ast::default_type)
        {
            is_valid = true;
        }
        // 如果既不在 GROUP BY 列中，也不是聚合函数，则抛出异常
        if (!is_valid)
        {
            throw RMDBError();
        }
    }

    // 处理 having 条件
    auto sv_having_conds = group_by->having_conds;
    for (auto &expr : sv_having_conds)
    {
        HavingCond cond;
        cond.lhs_col = {.tab_name = table_name, .col_name = expr->lhs->col_name, .alias = expr->lhs->alias, .aggFuncType = expr->lhs->type};
        if (cond.lhs_col.col_name != "*")
            check_column(sel_tab_cols, cond.lhs_col);
        cond.op = convert_sv_comp_op(expr->op);
        if (auto rhs_val = std::dynamic_pointer_cast<ast::Value>(expr->rhs))
        {
            cond.rhs_val = convert_sv_value(rhs_val);
        }
        else
        {
            throw RMDBError();
        }
        having_conds.push_back(cond);
    }
}

void Analyze::get_clause(const std::vector<std::shared_ptr<ast::BinaryExpr>> &sv_conds, std::vector<Condition> &conds)
{
    conds.clear();
    for (auto &expr : sv_conds)
    {
        Condition cond;
        if (std::dynamic_pointer_cast<ast::AggFunc>(expr->lhs) != nullptr)
        {
            throw RMDBError();
        }
        cond.lhs_col = {.tab_name = expr->lhs->tab_name, .col_name = expr->lhs->col_name};
        cond.op = convert_sv_comp_op(expr->op);
        if (auto rhs_val = std::dynamic_pointer_cast<ast::Value>(expr->rhs))
        {
            cond.is_rhs_val = true;
            cond.rhs_val = convert_sv_value(rhs_val);
        }
        else if (auto rhs_col = std::dynamic_pointer_cast<ast::Col>(expr->rhs))
        {
            cond.is_rhs_val = false;
            cond.rhs_col = {.tab_name = rhs_col->tab_name, .col_name = rhs_col->col_name};
        }
        else
        {
            throw RMDBError();
        }
        conds.push_back(cond);
    }
}

void Analyze::check_clause(const std::vector<std::string> &tab_names, std::vector<Condition> &conds)
{
    // auto all_cols = get_all_cols(tab_names);
    std::vector<ColMeta> all_cols;
    get_all_cols(tab_names, all_cols);

    for (auto &cond : conds)
    {
        // Infer table name from column name
        cond.lhs_col = check_column(all_cols, cond.lhs_col);
        if (!cond.is_rhs_val && !cond.is_subquery)
        {
            cond.rhs_col = check_column(all_cols, cond.rhs_col);
        }

        // Get lhs column metadata
        auto lhs_tab = sm_manager_->db_.get_table(cond.lhs_col.tab_name);
        auto lhs_col = lhs_tab->get_col(cond.lhs_col.col_name);
        cond.lhs = lhs_col;

        ColType lhs_type = lhs_col.type;
        ColType rhs_type;

        if (cond.is_rhs_val && !cond.is_subquery)
        {
            cond.rhs_val.init_raw(lhs_col.len);
            rhs_type = cond.rhs_val.type;

            // Check if rhs_val can be cast to lhs_type
            if (!can_cast_type(rhs_type, lhs_type))
            {
                throw RMDBError();
            }

            // Perform the cast if necessary
            if (rhs_type != lhs_type)
            {
                cast_value(cond.rhs_val, lhs_type);
            }
        }
        else if (!cond.is_subquery)
        {
            // Get rhs column metadata
            auto rhs_tab = sm_manager_->db_.get_table(cond.rhs_col.tab_name);
            auto rhs_col = rhs_tab->get_col(cond.rhs_col.col_name);
            cond.rhs = rhs_col;
            rhs_type = rhs_col.type;

            if (lhs_type != rhs_type)
            {
                throw RMDBError();
            }
        }
        else if (cond.subQuery->stmt != nullptr)
        {
            // 1、子查询的列数只能为1
            if (cond.subQuery->stmt->cols.size() != 1)
            {
                throw RMDBError();
            }
            // 2、获取子查询的列类型
            auto sub_tab = sm_manager_->db_.get_table(cond.subQuery->stmt->tabs[0]);
            auto sub_col = sub_tab->get_col(cond.subQuery->stmt->cols[0]->col_name);
            // 3、检查子查询的列类型是否与左边的列类型相同
            cond.subQuery->subquery_type = sub_col.type;
            if (!can_cast_type(sub_col.type, cond.lhs.type))
            {
                throw RMDBError();
            }

            // 4、分析子查询计划
            cond.subQuery->query = do_analyze(cond.subQuery->stmt);
        }
        else
        {
            // 子查询是数组（）
            // 创建一个临时集合来存储转换后的值
            std::unordered_set<Value> tempResult;

            // 遍历子查询结果
            for (const Value &val : cond.subQuery->result)
            {
                Value newVal = val;
                // 检查类型是否一致或可转换
                if (cond.lhs.type != val.type)
                {
                    if (!can_cast_type(cond.lhs.type, val.type))
                    {
                        throw RMDBError();
                    }
                    else
                    {
                        cast_value(newVal, cond.lhs.type);
                    }
                }
                tempResult.insert(newVal); // 将转换后的值插入临时集合
            }

            // 用临时集合替换原始集合
            cond.subQuery->result = std::move(tempResult);
        }

        // 遍历每一个cond
    }
    return;
}

bool Analyze::can_cast_type(ColType from, ColType to)
{
    // Add logic to determine if a type can be cast to another type
    if (from == to)
        return true;
    if (from == TYPE_INT && to == TYPE_FLOAT)
        return true;
    if (from == TYPE_FLOAT && to == TYPE_INT)
        return true;
    return false;
}

void Analyze::cast_value(Value &val, ColType to)
{
    // Add logic to cast val to the target type
    if (val.type == TYPE_INT && to == TYPE_FLOAT)
    {
        int int_val = val.int_val;
        val.type = TYPE_FLOAT;
        val.float_val = static_cast<float>(int_val);
    }
    else if (val.type == TYPE_FLOAT && to == TYPE_INT)
    {
        // do not things
    }
    else
    {
        throw RMDBError();
    }
}

Value Analyze::convert_sv_value(const std::shared_ptr<ast::Value> &sv_val)
{
    Value val;
    if (auto int_lit = std::dynamic_pointer_cast<ast::IntLit>(sv_val))
    {
        val.set_int(int_lit->val);
    }
    else if (auto float_lit = std::dynamic_pointer_cast<ast::FloatLit>(sv_val))
    {
        val.set_float(float_lit->val);
    }
    else if (auto str_lit = std::dynamic_pointer_cast<ast::StringLit>(sv_val))
    {
        val.set_str(str_lit->val);
    }
    else
    {
        throw RMDBError();
    }
    return val;
}

CompOp Analyze::convert_sv_comp_op(ast::SvCompOp op)
{
    std::map<ast::SvCompOp, CompOp> m = {
        {ast::SV_OP_EQ, OP_EQ},
        {ast::SV_OP_LT, OP_LT},
        {ast::SV_OP_GT, OP_GT},
        {ast::SV_OP_LE, OP_LE},
        {ast::SV_OP_GE, OP_GE},
    };
    return m.at(op);
}

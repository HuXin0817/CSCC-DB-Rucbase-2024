#pragma once

#include <map>

#include "common/context_finals.h"
#include "errors_finals.h"
#include "parser/parser.h"
#include "plan_finals.h"
#include "planner_finals.h"
#include "transaction/transaction_manager_finals.h"

class Optimizer
{
private:
    Planner *planner_;

public:
    explicit Optimizer(Planner *planner) : planner_(planner) {}

    std::shared_ptr<Plan> plan_query(const std::shared_ptr<Query>& query, Context *context)
    {
        if (auto x = std::dynamic_pointer_cast<ast::Help>(query->parse))
        {
            // help;
            return std::make_shared<OtherPlan>(T_Help, std::string());
        }
        else if (auto x = std::dynamic_pointer_cast<ast::ShowTables>(query->parse))
        {
            // show tables;
            return std::make_shared<OtherPlan>(T_ShowTable, std::string());
        }
        else if (auto x = std::dynamic_pointer_cast<ast::DescTable>(query->parse))
        {
            // desc table;
            return std::make_shared<OtherPlan>(T_DescTable, x->tab_name);
        }
        else if (auto x = std::dynamic_pointer_cast<ast::DescIndex>(query->parse))
        {
            // show index;
            return std::make_shared<OtherPlan>(T_DescIndex, x->tab_name);
        }
        else if (auto x = std::dynamic_pointer_cast<ast::TxnBegin>(query->parse))
        {
            // begin;
            return std::make_shared<OtherPlan>(T_Transaction_begin, std::string());
        }
        else if (auto x = std::dynamic_pointer_cast<ast::TxnAbort>(query->parse))
        {
            // abort;
            return std::make_shared<OtherPlan>(T_Transaction_abort, std::string());
        }
        else if (auto x = std::dynamic_pointer_cast<ast::TxnCommit>(query->parse))
        {
            // commit;
            return std::make_shared<OtherPlan>(T_Transaction_commit, std::string());
        }
        else if (auto x = std::dynamic_pointer_cast<ast::TxnRollback>(query->parse))
        {
            // rollback;
            return std::make_shared<OtherPlan>(T_Transaction_rollback, std::string());
        }
        else if (auto x = std::dynamic_pointer_cast<ast::SetStmt>(query->parse))
        {
            // Set Knob Plan
            return std::make_shared<SetKnobPlan>(x->set_knob_type_, x->bool_val_);
        }
        else if (auto x = std::dynamic_pointer_cast<ast::CreateStaticCheckpoint>(query->parse))
        {
            return std::make_shared<OtherPlan>(T_Create_StaticCheckPoint, std::string());
        }
        else if (auto x = std::dynamic_pointer_cast<ast::CrashStmt>(query->parse))
        {
            return std::make_shared<OtherPlan>(T_Crash, std::string());
        }
        else if (auto x = std::dynamic_pointer_cast<ast::LoadStmt>(query->parse))
        {
            return std::make_shared<OtherPlan>(T_LoadData, x->tab_name, x->file_name);
        }
        else if (auto x = std::dynamic_pointer_cast<ast::IoEnable>(query->parse))
        {
            return std::make_shared<OtherPlan>(T_IoEnable, x->set_io_enable);
        }
        else
        {
            return planner_->do_planner(query, context);
        }
    }
};

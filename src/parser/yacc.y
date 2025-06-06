%{
#include "ast.h"
#include "yacc.tab.h"
#include <iostream>
#include <memory>

int yylex(YYSTYPE *yylval, YYLTYPE *yylloc);

void yyerror(YYLTYPE *locp, const char* s) {
    std::cerr << "Parser Error at line " << locp->first_line << " column " << locp->first_column << ": " << s << std::endl;
}

using namespace ast;
%}

// request a pure (reentrant) parser
%define api.pure full
// enable location in error handler
%locations
// enable verbose syntax error message
%define parse.error verbose

// keywords
%token SHOW TABLES CREATE TABLE DROP DESC INSERT INTO VALUES DELETE FROM ASC ORDER BY
WHERE UPDATE SET SELECT INT CHAR FLOAT DATETIME INDEX AND JOIN EXIT HELP TXN_BEGIN TXN_COMMIT TXN_ABORT TXN_ROLLBACK ORDER_BY ENABLE_NESTLOOP ENABLE_SORTMERGE STATIC_CHECKPOINT CRASH
MAX MIN AVG COUNT SUM GROUP HAVING AS IN NOT LOAD SIGN_ADD SIGN_SUB
// non-keywords
%token LEQ NEQ GEQ T_EOF
%token OUTPUT_FILE ON OFF

// type-specific tokens
%token <sv_str> IDENTIFIER VALUE_STRING VALUE_PATH
%token <sv_int> VALUE_INT
%token <sv_float> VALUE_FLOAT
%token <sv_bool> VALUE_BOOL

// specify types for non-terminal symbol
%type <sv_node> stmt dbStmt ddl dml txnStmt setStmt crashStmt io_stmt
%type <sv_field> field
%type <sv_fields> fieldList
%type <sv_type_len> type
%type <sv_comp_op> op
%type <sv_expr> expr
%type <sv_val> value
%type <sv_vals> valueList
%type <sv_str> tbName colName ALIAS fileName
%type <sv_strs> tableList colNameList
%type <sv_col> col
%type <agg_col> aggFunc
%type <sv_cols> colList selector
%type <sv_set_clause> setClause
%type <sv_set_clauses> setClauses
%type <sv_cond> condition
%type <sv_conds> whereClause optWhereClause
%type <sv_having_conds> havingClause optHavingClause
%type <sv_having_cond> havingCondition
%type <sv_group_by> optGroupByClause groupByClause
%type <sv_orderby>  order_clause opt_order_clause
%type <sv_orderby_dir> opt_asc_desc
%type <sv_setKnobType> set_knob_type

%%
start:
        stmt ';'
    {
        parse_tree = $1;
        YYACCEPT;
    }
    |   HELP
    {
        parse_tree = std::make_shared<Help>();
        YYACCEPT;
    }
    |   EXIT
    {
        parse_tree = nullptr;
        YYACCEPT;
    }
    |   T_EOF
    {
        parse_tree = nullptr;
        YYACCEPT;
    }
    |  io_stmt
    {
        parse_tree = $1;
        YYACCEPT;
    }
    ;

stmt:
        dbStmt
    |   ddl
    |   dml
    |   txnStmt
    |   setStmt
    |   crashStmt
    ;

crashStmt:
        CRASH
    {
        $$ = std::make_shared<CrashStmt>();
    }
    ;

txnStmt:
        TXN_BEGIN
    {
        $$ = std::make_shared<TxnBegin>();
    }
    |   TXN_COMMIT
    {
        $$ = std::make_shared<TxnCommit>();
    }
    |   TXN_ABORT
    {
        $$ = std::make_shared<TxnAbort>();
    }
    | TXN_ROLLBACK
    {
        $$ = std::make_shared<TxnRollback>();
    }
    ;

dbStmt:
        SHOW TABLES
    {
        $$ = std::make_shared<ShowTables>();
    }
    |   LOAD fileName INTO tbName
    {
         $$ = std::make_shared<LoadStmt>($2, $4);
    }
    ;

setStmt:
        SET set_knob_type '=' VALUE_BOOL
    {
        $$ = std::make_shared<SetStmt>($2, $4);
    }
    ;
io_stmt:
        SET OUTPUT_FILE ON
    {
        $$ = std::make_shared<IoEnable>(true);
    }
    |   SET OUTPUT_FILE OFF
    {
        $$ = std::make_shared<IoEnable>(false);
    }
    ;
ddl:
        CREATE TABLE tbName '(' fieldList ')'
    {
        $$ = std::make_shared<CreateTable>($3, $5);
    }
    |   DROP TABLE tbName
    {
        $$ = std::make_shared<DropTable>($3);
    }
    |   DESC tbName
    {
        $$ = std::make_shared<DescTable>($2);
    }
    |   CREATE INDEX tbName '(' colNameList ')'
    {
        $$ = std::make_shared<CreateIndex>($3, $5);
    }
    |   DROP INDEX tbName '(' colNameList ')'
    {
        $$ = std::make_shared<DropIndex>($3, $5);
    }
    |  SHOW INDEX FROM tbName
    {
    	$$ = std::make_shared<DescIndex>($4);
    }
    ;
    |   CREATE STATIC_CHECKPOINT
    {
        $$ = std::make_shared<CreateStaticCheckpoint>();
    }
    ;

dml:
        INSERT INTO tbName VALUES '(' valueList ')'
    {
        $$ = std::make_shared<InsertStmt>($3, $6);
    }
    |   DELETE FROM tbName optWhereClause
    {
        $$ = std::make_shared<DeleteStmt>($3, $4);
    }
    |   UPDATE tbName SET setClauses optWhereClause
    {
        $$ = std::make_shared<UpdateStmt>($2, $4, $5);
    }
    |   SELECT selector FROM tableList optWhereClause optGroupByClause opt_order_clause
    {
	$$ = std::make_shared<SelectStmt>($2, $4, $5, $6, $7);
    }
    ;

fieldList:
        field
    {
        $$ = std::vector<std::shared_ptr<Field>>{$1};
    }
    |   fieldList ',' field
    {
        $$.push_back($3);
    }
    ;

colNameList:
        colName
    {
        $$ = std::vector<std::string>{$1};
    }
    | colNameList ',' colName
    {
        $$.push_back($3);
    }
    ;

field:
        colName type
    {
        $$ = std::make_shared<ColDef>($1, $2);
    }
    ;

type:
        INT
    {
        $$ = std::make_shared<TypeLen>(SV_TYPE_INT, sizeof(int));
    }
    |   CHAR '(' VALUE_INT ')'
    {
        $$ = std::make_shared<TypeLen>(SV_TYPE_STRING, $3);
    }
    |   FLOAT
    {
        $$ = std::make_shared<TypeLen>(SV_TYPE_FLOAT, sizeof(float));
    }
    |   DATETIME
    {
        $$ = std::make_shared<TypeLen>(SV_TYPE_DATETIME, 19);
    }
    ;

valueList:
        value
    {
        $$ = std::vector<std::shared_ptr<Value>>{$1};
    }
    |   valueList ',' value
    {
        $$.push_back($3);
    }
    ;

value:
        VALUE_INT
    {
        $$ = std::make_shared<IntLit>($1);
    }
    |   VALUE_FLOAT
    {
        $$ = std::make_shared<FloatLit>($1);
    }
    |   VALUE_STRING
    {
        $$ = std::make_shared<StringLit>($1);
    }
    |   VALUE_BOOL
    {
        $$ = std::make_shared<BoolLit>($1);
    }
    ;

condition:
        col op expr
    {
        $$ = std::make_shared<BinaryExpr>($1, $2, $3);
    }
    |   col op '(' dml ')'
    {
	$$ = std::make_shared<SubQueryExpr>($1, $2, $4);
    }
    |   col op '(' valueList ')'
    {
	$$ = std::make_shared<SubQueryExpr>($1, $2, $4);
    }
    ;

havingCondition:
    aggFunc op expr
    {
	$$ = std::make_shared<HavingCause>($1 ,$2, $3);
    }
    ;

optWhereClause:
        /* epsilon */ { /* ignore*/ }
    |   WHERE whereClause
    {
        $$ = $2;
    }
    ;

whereClause:
        condition 
    {
        $$ = std::vector<std::shared_ptr<BinaryExpr>>{$1};
    }
    |   whereClause AND condition
    {
        $$.push_back($3);
    }
    ;

col:
        tbName '.' colName
    {
        $$ = std::make_shared<Col>($1, $3);
    }
    |   colName
    {
        $$ = std::make_shared<Col>("", $1);
    }
    |   aggFunc
    {
        $$ = $1;
    }
    |   colName AS ALIAS
    {
	$$ = std::make_shared<Col>("", $1, $3);
	$$->alias = $3;
    }
    |   aggFunc AS ALIAS
    {
	$$ = $1;
	$$->alias = $3;
    }
    ;
aggFunc:
        SUM '(' col ')'
    {
        $$ = std::make_shared<AggFunc>($3->tab_name, $3->col_name, AggFuncType::SUM);
    }
    |   MIN '(' col ')'
    {
        $$ = std::make_shared<AggFunc>($3->tab_name, $3->col_name, AggFuncType::MIN);
    }
    |   MAX '(' col ')'
    {
        $$ = std::make_shared<AggFunc>($3->tab_name, $3->col_name, AggFuncType::MAX);
    }
    |   COUNT '(' col ')'
    {
        $$ = std::make_shared<AggFunc>($3->tab_name, $3->col_name, AggFuncType::COUNT);
    }
    |   COUNT '(' '*' ')'
    {
        $$ = std::make_shared<AggFunc>("", "*", AggFuncType::COUNT);
    }
    ;


colList:
        col
    {
        $$ = std::vector<std::shared_ptr<Col>>{$1};
    }
    |   colList ',' col
    {
        $$.push_back($3);
    }
    ;

optGroupByClause:
    /* empty */
    {
        $$ = nullptr;
    }
    | groupByClause optHavingClause
    {
        $$ = $1;
        $$->having_conds=std::move($2);
    }
    ;
groupByClause:
    GROUP BY colList
    {
        $$ = std::make_shared<GroupBy>($3);
    }
    ;
optHavingClause:
    /* empty */
    {
        $$ = std::vector<std::shared_ptr<HavingCause>> {};
    }
    | HAVING  havingClause
    {
        $$ = $2;
    }
    ;

havingClause:
      havingCondition
    {
        $$ = std::vector<std::shared_ptr<HavingCause>> {$1};
    }
    | havingClause AND havingCondition
    {
    	$$.push_back($3);
    }
    ;

op:
        '='
    {
        $$ = SV_OP_EQ;
    }
    |   '<'
    {
        $$ = SV_OP_LT;
    }
    |   '>'
    {
        $$ = SV_OP_GT;
    }
    |   NEQ
    {
        $$ = SV_OP_NE;
    }
    |   LEQ
    {
        $$ = SV_OP_LE;
    }
    |   GEQ
    {
        $$ = SV_OP_GE;
    }
    |   IN
    {
	$$ = SV_OP_IN;
    }
    |   NOT IN
    {
    	$$ = SV_OP_NOT_IN;
    }
    ;

expr:
        value
    {
        $$ = std::static_pointer_cast<Expr>($1);
    }
    |   col
    {
        $$ = std::static_pointer_cast<Expr>($1);
    }
    ;

setClauses:
        setClause
    {
        $$ = std::vector<std::shared_ptr<SetClause>>{$1};
    }
    |   setClauses ',' setClause
    {
        $$.push_back($3);
    }
    ;

setClause:
        colName '=' value
    {
        $$ = std::make_shared<SetClause>($1, $3);
    }
    |   colName '=' colName  value
    {
        $$ = std::make_shared<SetClause>($1, $4, 4);
    }
    |   colName '=' colName SIGN_ADD value
    {
        $$ = std::make_shared<SetClause>($1, $5, 0);
    }
    |   colName '=' colName SIGN_SUB value
    {
        $$ = std::make_shared<SetClause>($1, $5, 1);
    }
    |   colName '=' colName '*' value
    {
        $$ = std::make_shared<SetClause>($1, $5, 2);
    }
    |   colName '=' colName '/' value
    {
        $$ = std::make_shared<SetClause>($1, $5, 3);
    }
    ;

selector:
        '*'
    {
        $$ = {};
    }
    |   colList
    ;

tableList:
        tbName
    {
        $$ = std::vector<std::string>{$1};
    }
    |   tableList ',' tbName
    {
        $$.push_back($3);
    }
    |   tableList JOIN tbName
    {
        $$.push_back($3);
    }
    ;

opt_order_clause:
    ORDER BY order_clause      
    { 
        $$ = $3; 
    }
    |   /* epsilon */ { /* ignore*/ }
    ;

order_clause:
      col  opt_asc_desc 
    { 
        $$ = std::make_shared<OrderBy>($1, $2);
    }
    ;   

opt_asc_desc:
    ASC          { $$ = OrderBy_ASC;     }
    |  DESC      { $$ = OrderBy_DESC;    }
    |       { $$ = OrderBy_DEFAULT; }
    ;    

set_knob_type:
    ENABLE_NESTLOOP { $$ = EnableNestLoop; }
    |   ENABLE_SORTMERGE { $$ = EnableSortMerge; }
    ;

tbName: IDENTIFIER;

ALIAS: IDENTIFIER;

colName: IDENTIFIER;

fileName: VALUE_PATH;
%%

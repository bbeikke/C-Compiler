#include "globals.h"
#include "AST.h"
#include "symtab.h"
#include "string.h"
#include "parse.h"
static int stack[MAXSTACK];
static int sp = 0;

/* Add declaration as sibling*/
TreeNode* newDecList(TreeNode* decList, TreeNode* declaration)
{

     TreeNode* node = decList;

     while(node->sibling != NULL)
          node = node->sibling;
     node->sibling = declaration;


     return decList;
}


TreeNode* newVarDec(TreeNode* typeSpecifier, char* ID, int lineno)
{
     TreeNode* root = newASTNode(VARDEC_AST, lineno);
     root->child[0] = typeSpecifier;
     root->attr.name = strdup(ID);
     root->type = TYPE_INTEGER;
     if(!GLOBAL)
          pushTable(CompoundST);
     VarSymbol* vs = lookup_var(root->attr.name);
     if(vs != NULL)
          ErrorMsg(root, "variable has been declared before: ", ID);
     else
          insert_var(root->attr.name, GLOBAL, CompoundST->startOffset++, TYPE_INTEGER);
     if(!GLOBAL)
          popTable();
     return root;
}
// var_declaration: type_specifier ID LSB NUMBER RSB SEMI
TreeNode* newArrayDec(TreeNode* typeSpecifier, char* ID, int size, int lineno)
{
     TreeNode* root = newASTNode(ARRAYDEC_AST, lineno);
     root->child[0] = typeSpecifier;
     root->attr.name = strdup(ID);
     root->type = TYPE_ARRAY;
     root->attr.value = size;
     pushTable(CompoundST);
     VarSymbol* vs = lookup_var(root->attr.name);
     if(vs != NULL)
          ErrorMsg(root, "array has been declared before: ", ID);
     else
     {
          insert_var(root->attr.name, GLOBAL, CompoundST->startOffset, TYPE_ARRAY);
          CompoundST->startOffset += size;
     }
     popTable(CompoundST);
     return root;
}
TreeNode* newTypeSpe(ExpType type, int lineno)
{
     TreeNode* root = newASTNode(TYPE_AST, lineno);
     root->type = type;
     return root;
}
// fun_declaration: type_specifier ID LBracket params RBracket compound_stmt
TreeNode* newFunDec(TreeNode* typeSpecifier, char* ID, TreeNode* params, TreeNode* compound, int lineno)
{
     TreeNode* root = newASTNode(FUNDEC_AST, lineno);
     root->child[0] = typeSpecifier;
     root->child[1] = params;
     root->child[2] = compound;
     root->attr.name = strdup(ID);
     FunSymbol* fs = lookup_fun(ID);
     /* print symbol table to test */
#ifdef DEBUG_SYM
     fprintf(listing, "symbol table of function: %s\n", root->attr.name);
     printSymTab(ParamST);
#endif
     /* check if the function has been declared before */
     if(fs != NULL)
          ErrorMsg(root, "function has been declared before: ", ID);
     else
     {
          root->symbolTable = tables;
          if(params == NULL)
               insert_fun(root->attr.name, tables, 0);               
          else
               insert_fun(root->attr.name, tables, params->attr.value);
     }
     /* type check of return and declaration */
     if(root->child[0]->type != root->child[2]->type)
     {
          fprintf(stderr, "%d: return type is conflict with function declaration\n", root->lineno);
     }
     /* change GLOBAL flag to true as function declaration come to end */
     GLABAL = TRUE;
     return root;
}
// paramList==NULL => params: VOID
TreeNode* newParams(TreeNode* paramList)
{
     // params: param_list
     int num = 0;
     TreeNode* temp;
     pushTable(ParamST);
     ParamST = newSymbolTable(PARAM);
     /* change GLOBAL flag to false as function declaration begins */
     GLOBAL = FALSE;
     if(paramList != NULL)
     {
          for(temp=paramList;temp!=NULL;temp++)
               num++;
          paramList->attr.value = num;
          return paramList;
          /* save Parameter Symbol Table */
     }
     else
     {
          return NULL;
     }
}
// paramList==NULL => param_list:param
TreeNode* newParamList(TreeNode* paramList, TreeNode* param)
{
     // param_list: param_list SEMI param
     TreeNode* node = paramList;
     if(paramList != NULL)
     {
          while(node->sibling != NULL)
               node = node->sibling;
          node->sibling = param;
          return paramList;
     }
     else // param_list: param
     {
          return param;
     }
}
// type: 1=id is array 0=otherwise
TreeNode* newParam(TreeNode* typeSpecifier, char* ID, int type, int lineno)
{
     // param: type_specifier ID
     TreeNode* root = NULL;
     if(type == 0)
     {
          root = newASTNode(PARAMID_AST, lineno);
          root->child[0] = typeSpecifier;
          root->attr.name = strdup(ID);
          pushTable(ParamST);
          if(lookup_var(root->attr.name) != NULL)
               ErrorMsg(root, "parameter has been declared before:", ID);
          else
               insert_var(root->attr.name, PARAM, ParamST->startOffset++ , TYPE_INTEGER);
          popTable();
          root->type = TYPE_INTEGER;
     }
     else // param: type_specifier ID LSB RSB
     {
          root = newASTNode(PARAMARRAY_AST, lineno);
          root->child[0] = typeSpecifier;
          root->attr.name = strdup(ID);
          pushTable(ParamST);
          if(lookup_var(root->attr.name) != NULL)
               ErrorMsg(root, "parameter has been declared before: ", ID);
          else
               insert_var(root->attr.name, PARAM, ParamST->startOffset++ , TYPE_ARRAY);
          popTable();
          root->type = TYPE_ARRAY;
     }
     return root;
}
TreeNode* newCompound(TreeNode* localDecs, TreeNode* stmtList, int lineno)
{
     TreeNode* node = localDecs;
     TreeNode* root = newASTNode(COMPOUND_AST, lineno);
     VarSymbol* vs = NULL;
     root->child[0] = localDecs;
     root->child[1] = stmtList;
     /* assign type of Return to Compound */
     if(stmtList != NULL && stmtList->type != TYPE_UNDEFINED) /* has return stmt */
          root->type = stmtList->type;
     else
          root->type = TYPE_VOID;
     root->symbolTable = CompoundST;
     /* DEBUG: print symbol table of this compount statement */
#ifdef DEBUG_SYM
     fprintf(listing, "symbo table of Compound\n");
     printSymTab(CompoundST);
#endif
     
     /* change all local declaration of Compound Stmt to LOCAL from default GLOBAL */
     pushTable(CompoundST);
     for(node = localDecs; node!=NULL; node=node->sibling)
     {
          vs = lookup_var(node->attr.name);
          assert(vs != NULL);
          vs->scope = LOCAL;
     }
     CompoundST = newSymbolTable(LOCAL);
     return root;
}
TreeNode* newLocalDecs(TreeNode* localDecs, TreeNode* varDec)
{
     TreeNode* node = localDecs;
     if(localDecs == NULL)
          return varDec;
     while(node->sibling != NULL)
          node = node->sibling;
     node->sibling = varDec;
     return localDecs;
}
TreeNode* newStmtList(TreeNode* stmtList, TreeNode* stmt)
{
     TreeNode* node = stmtList;
     if(stmtList == NULL)
          return stmt;
     while(node->sibling != NULL)
          node = node->sibling;
     node->sibling = stmt;
     if(stmt->type != TYPE_UNDEFINED)       /* return stmt */
     {
          if(stmtList->type == TYPE_UNDEFINED) /* there is no return stmt before */
               stmtList->type = stmt->type; /* assign return stmt type to stmtList */
          else
               if(stmtList->type != stmt->type)
                    ErrorMsg(stmt, "Return type mismatch", "");
     }
     return stmtList;
}
// stmt includes 5 type
TreeNode* newStmt(TreeNode* stmt)
{
     return stmt;
}
// expression_stmt: SEMI
TreeNode* newExpStmt(TreeNode* expression, int lineno)
{
     TreeNode* root = newASTNode(EXPSTMT_AST, lineno);
     root->child[0] =expression;
     root->type = expression->type;
     return root;
}    
TreeNode* newSelectStmt(TreeNode* expression, TreeNode* stmt, TreeNode* elseStmt, int lineno)
{
     TreeNode* root = newASTNode(SELESTMT_AST, lineno);
     root->child[0] = expression;
     root->child[1] = stmt;
     root->child[2] = elseStmt;
     return root;
}
TreeNode* newIterStmt(TreeNode* expression,  TreeNode* stmt, int lineno)
{
     assert(expression != NULL);
     TreeNode* root = newASTNode(ITERSTMT_AST, lineno);
     root->child[0] = expression;
     root->child[1] = stmt;
     return root;
}
// expression=NULL => return_stmt: RETURN SEMI;
TreeNode* newRetStmt(TreeNode* expression, int lineno)
{
     TreeNode* root = newASTNode(RETSTMT_AST, lineno);
     root->child[0] = expression;
     if(expression != NULL)
          root->type = TYPE_INTEGER;
     else
          root->type = TYPE_VOID;
     return root;
}
// assign statement
TreeNode* newAssignExp(TreeNode* var, TreeNode* expression, int lineno)
{
     TreeNode* root = newASTNode(ASSIGN_AST, lineno);
     root->child[0] = var;
     root->child[1] = expression;
     if(!(var->type==TYPE_INTEGER && expression->type==TYPE_INTEGER))
          ErrorMsg(root, "type mismatch", "");
     return root;
}
TreeNode* newExpression(TreeNode* simpExp)
{
     return simpExp;
}
TreeNode* newVar(char* ID, int lineno)
{
     TreeNode* root = newASTNode(VAR_AST, lineno);
     VarSymbol* vs = NULL;
     pushTable(CompoundST);
     vs = lookup_var(ID);
     if( vs == NULL)
          ErrorMsg(root, "variable not defined: ", ID);
     else
          root->attr.name = vs->name;
     popTable();
     return root;
}
TreeNode* newArrayVar(char* ID, TreeNode* expression, int lineno)
{
     TreeNode* root = newASTNode(ARRAYVAR_AST, lineno);
     VarSymbol* vs = NULL;
     pushTable(CompoundST);
     vs = lookup_var(ID);
     if( vs == NULL)
          ErrorMsg(root, "variable not defined: ", ID);
     else
          root->attr.name = vs->name;
     if( vs->type != TYPE_ARRAY)
          ErrorMsg(root, "variable is not array: ", ID);
     else
     {
          if(expression->type != TYPE_INTEGER)
               ErrorMsg(root, "type mismatch ", "");
          else
               root->child[0] = expression;
     }
     popTable();
     return root;
}
TreeNode* newSimpExp(TreeNode* addExp1, int relop, TreeNode* addExp2, int lineno)
{
     TreeNode* root = NULL;
     /* simple_expression: additive_expression relop additive_expression */
     if(relop != -1)            
     {
          root = newASTNode(EXP_AST, lineno);
          root->child[0] = addExp1;
          root->child[1] = addExp2;
          root->attr.op = relop;
          if(root->attr.op == PLUS || root->attr.op == MINUS ||
             root->attr.op == MULTI || root->attr.op == DIV)
          {
               if(!(addExp1->type == TYPE_INTEGER && addExp2->type == TYPE_INTEGER))
                    ErrorMsg(root, "type mismatch","");
               else
                    root->type = TYPE_INTEGER;
          }
          else
          {
               if(!(addExp1->type == TYPE_INTEGER && addExp2->type ==TYPE_INTEGER))
                    ErrorMsg(root, "type mismatch", "");
               else
                    root->type = TYPE_INTEGER;
          }
             
     }
     else                       /* simple_expression: additive_expression  */
     {
          root = addExp1;
     }
     return root;
}
TreeNode* newRelop(int opType)
{
     return NULL;
}
TreeNode* newAddExp(TreeNode* addExp, int addop, TreeNode* term, int lineno)
{
     TreeNode* root = newASTNode(EXP_AST, lineno);
     root->child[0] = addExp;
     root->child[1] = term;
     root->attr.op = addop;
     root->type = TYPE_INTEGER;
     if(!(addExp->type == TYPE_INTEGER && term->type == TYPE_INTEGER))
          ErrorMsg(root, "type mismatch", "");
     return root;
}
TreeNode* newAddOp(int opType)
{
     return NULL;
}
TreeNode* newTerm(TreeNode* term, int mulop, TreeNode* factor, int lineno)
{
     TreeNode* root = newASTNode(EXP_AST, lineno);
     root->child[0] = term;
     root->child[1] = factor;
     root->attr.op = mulop;
     root->type = TYPE_INTEGER;
     if(!(term->type == TYPE_INTEGER && factor->type == TYPE_INTEGER))
          ErrorMsg(root, "type mismatch", "");
     return root;
}
TreeNode* newTermFactor(TreeNode* factor)
{
     return factor;
}
TreeNode* newCall(char* ID, TreeNode* args, int lineno)
{
     FunSymbol* fun;
     VarSymbol* var;
     TreeNode* tmp = args;
     /* Check parameters */
     fun = lookup_fun(ID);
     ASSERT(fun != NULL){
          fprintf(stderr, "call before declaration @line%d\n", lineno);
     }
     var = fun->paramList;
     /*compare the parameters with args one-by-one*/
     while(var && tmp){
          ASSERT(var->type == tmp->type){
               fprintf(stderr, "call function with type-mismatch @line%d\n",lineno);
          }
          var = var->next;
          tmp = tmp->sibling;
     }

     ASSERT(!var && !tmp){
          fprintf(stderr, "call function with number-mismatch @line%d\n", lineno);
     }

     /*Check finished*/
     TreeNode* root = newASTNode(CALLSTMT_AST, lineno);
     root->child[0] = args;
     root->attr.name = strdup(ID);

     return root;
}
TreeNode* newArgs(TreeNode* argList)
{
     return argList;
}

/* Add the exp as sibling*/
TreeNode* newArgList(TreeNode* argList, TreeNode* expression)
{
     TreeNode* node = argList;
     while(node->sibling != NULL)
          node = node->sibling;
     node->sibling = expression;
     return argList;
}
TreeNode* newASTNode(ASTType type, int lineno)
{
     int i;

     TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
     ASSERT(node != NULL){
          fprintf(stderr, "Failed to malloc for TreeNode @line%d.\n", lineno);
     }

     for(i=0; i<MAXCHILDREN; ++i){
          node->child[i] = NULL;
     }
     node->sibling = NULL;
     node->astType = type;
     node->type = TYPE_UNDEFINED;
     node->lineno = lineno;
     return node;
}

TreeNode* newNumNode(int num, int lineno)
{
     TreeNode* root = newASTNode(NUM_AST, lineno);
     root->attr.value = num;
     return root;
}
void printNodeKind(TreeNode* node)
{
     if(node == NULL)
           return;
     switch(node->astType)
     {
     case VARDEC_AST:
          printf("Var declaration \n");
          break;
     case ARRAYDEC_AST:
          printf("Array declaration \n");
          break;
     case FUNDEC_AST:
          printf("Function declaration \n");
          break;
     case TYPE_AST:
          printf("Type specifier \n");
          break;
     case PARAMID_AST:
          printf("Param of ID \n");
          break;
     case PARAMARRAY_AST:
          printf("Param of Array \n");
          break;
     case COMPOUND_AST:
          printf("Counpound statements \n");
          break;
     case EXPSTMT_AST:
          printf("Expression statement\n");
          break;
     case SELESTMT_AST:
          printf("Select statement\n");
          break;
     case ITERSTMT_AST:
          printf("Iteration statement\n");
          break;
     case RETSTMT_AST:
          printf("Return statement\n");
          break;
     case ASSIGN_AST:
          printf("Assign statement\n");
          break;
     case EXP_AST:
          printf("Expression \n");
          break;
     case VAR_AST:
          printf("Var \n");
          break;
     case ARRAYVAR_AST:
          printf("Array var ASt\n");
          break;
     case FACTOR_AST:
          printf("Factor \n");
          break;
     case CALLSTMT_AST:
          printf("Call stement \n");
          break;
     default:
          printf("No such AST type\n");
          break;
     }
}
void printAST(TreeNode* root, int indent)
{
     TreeNode* node = root;
     int i;
     while(node != NULL)
     {
          for (i = 0; i<indent; ++i)
          {
               printf("  ");
          }
          printNodeKind(node);
          printAST(node->child[0], indent+4);
          printAST(node->child[1], indent+4);
          printAST(node->child[2], indent+4); 
          printAST(node->child[3], indent+4);         
          node = node->sibling;
     }
}

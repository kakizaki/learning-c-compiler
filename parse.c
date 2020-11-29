#include "9cc.h"


static bool confirm_type_token() {
  if (confirm_token(TK_INT)) {
    return true;
  }

  return false;
}


static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}


static Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}


static Node *new_node_num(int val) {
  Node *node = new_node(ND_NUM);
  node->value = val;
  return node;
}


static Node* new_add(Node *lhs, Node *rhs) {
  updateType(lhs);
  updateType(rhs);
  
  if (lhs->evalType->kind == TY_INT && rhs->evalType->kind == TY_INT) {
    return new_node_binary(ND_ADD, lhs, rhs);
  }
  if (lhs->evalType->ptr_to != NULL && rhs->evalType->kind == TY_INT) {
    return new_node_binary(ND_PTR_ADD, lhs, rhs);
  }
  if (rhs->evalType->ptr_to != NULL && lhs->evalType->kind == TY_INT) {
    // HACK 左辺と右辺を入れ替え
    return new_node_binary(ND_PTR_ADD, rhs, lhs);
  }
  error_current_token("invalid operands");
}


static Node* new_sub(Node *lhs, Node *rhs) {
  updateType(lhs);
  updateType(rhs);

  if (lhs->evalType->kind == TY_INT && rhs->evalType->kind == TY_INT) {
    return new_node_binary(ND_SUB, lhs, rhs);
  }
  if (lhs->evalType->ptr_to != NULL && rhs->evalType->kind == TY_INT) {
    return new_node_binary(ND_PTR_SUB, lhs, rhs);
  }
  if (lhs->evalType->ptr_to != NULL && rhs->evalType->ptr_to != NULL) {
    return new_node_binary(ND_PTR_DIFF, lhs, rhs);
  }
  error_current_token("invalid operands");
}


static Node *new_node_var(LVar *v) {
  Node *node = new_node(ND_LVAR);
  node->var = v;
  return node;
}


static Node *new_node_array_indexer(LVar *v, Node *indexer) {
  Node *deref = new_node(ND_DEREF);
  deref->lhs = new_add(new_node_var(v), indexer);
  return deref;
}


// program = function*
Function *program() {
  Function head = {};
  Function *cur = &head;

  while (!at_eof()) {
    Function *f = function();
    cur->next = f;
    cur = f;
  }

  return head.next;
}


// function = 'int' ident function_param_list "{" statement* "}"
Function* function() {
  clear_local_varlist();

  expect_token(TK_INT);

  Token *ident = expect_ident();
  Function *func = calloc(1, sizeof(Function));
  func->name = copy_string(ident->str, ident->len);
  func->params = function_param_list();

  expect("{");
  NodeList head = { 0, 0 };
  NodeList* cur = &head;

  while (consume_reserved("}") == false) {
    cur->next = calloc(1, sizeof(NodeList));
    cur = cur->next;
    cur->node = statement();
  }

  func->block = head.next;
  func->locals = get_local_varlist();
  func->stackSize = update_offset_local_varlist();
  return func;
}


// HACK 一次元配列のみ
// declaration = 'int' ('*')* ident ('[' num ']')? ("=" expression)
Node *declaration() {
  Type *t = NULL;

  // 
  if (consume_token(TK_INT)) {
    t = type_int();
  }

  if (t == NULL) {
    error_current_token("サポートされていない型です");
  }

  // ポインタ
  while (consume_reserved("*")) {
    Type *ptr = type_pointer_to(t);
    t = ptr;
  }

  Token *ident = expect_ident();
  if (find_lvar(get_local_varlist(), ident) != NULL) {
    error_at(ident->str, "すでに定義されています");
  }

  if (consume_reserved("[")) {
    Type *arr = type_array_to(t, expect_number());
    t = arr;
    expect("]");
  }

  Node *assign = new_node(ND_ASSIGN);
  assign->lhs = new_node_var(add_local_var(ident, t));

  if (consume_reserved("=")) {
    assign->rhs = expression();
  }
  else {
    assign->rhs = new_node_num(0);
  }
  return assign;
}


// function_param = 'int' ('*')* ident
LVar *function_param() {
  Type *t = NULL;

  if (consume_token(TK_INT)) {
    t = type_int();
  }
  if (t == NULL) {
    error_current_token("サポートされていない変数の型です");
  }

  // ポインタ
  while (consume_reserved("*")) {
    Type *ptr = type_pointer_to(t);
    t = ptr;
  }

  Token *ident = expect_ident();
  if (find_lvar(get_local_varlist(), ident) != NULL) {
    error_at(ident->str, "すでに定義されています");
  }

  LVar *v = add_local_var(ident, t);
  return v;
}


// function_param_list = '(' (function_param (',' function_param)* )?  ')'
VarList *function_param_list() {
  expect("(");
  if (consume_reserved(")")) {
    return NULL;
  }

  LVar *v = function_param();
  VarList *head = calloc(1, sizeof(VarList));
  head->var = v;

  VarList *cur = head;
  while (consume_reserved(",")) {
    v = function_param();
    cur->next = calloc(1, sizeof(VarList));
    cur->next->var = v;
    cur = cur->next;
  }
  expect(")");
  return head;
}



// statement = expression ";" 
//          | declaration ";"
//          | "{" statement* "}"
//          | "if" "(" expression ")" statement ("else" statement)?
//          | "while" "(" expression ")" statement
//          | "for" "(" expression? ";" expression? ";" expression? ")" statement
//          | "return" expression ";"
Node *statement() {
  Node *node;

  if (consume_reserved("{")) {
    NodeList head = { 0, 0 };
    NodeList* cur = &head;

    while (consume_reserved("}") == false) {
      cur->next = calloc(1, sizeof(NodeList));
      cur = cur->next;
      cur->node = statement();
    }

    node = new_node(ND_BLOCK);
    node->block = head.next;
    updateType(node);
    return node;
  }

  if (consume_token(TK_IF)) {
    node = new_node(ND_IF);
    
    expect("(");
    node->cond = expression();
    expect(")");

    node->trueStatement = statement();

    if (consume_token(TK_ELSE)) {
      node->kind = ND_IF_ELSE;
      node->falseStatement = statement();
    }
    updateType(node);
    return node;
  }

  if (consume_token(TK_WHILE)) {
    node = new_node(ND_WHILE);

    expect("(");
    node->cond = expression();
    expect(")");

    node->trueStatement = statement();
    updateType(node);
    return node;
  }

  if (consume_token(TK_FOR)) {
    node = new_node(ND_FOR);

    expect("(");
    if (consume_reserved(";")) {
      node->init = NULL;
    } else {
      node->init = expression();
      expect(";");
    }

    if (consume_reserved(";")) {
      node->cond = NULL;
    } else {
      node->cond = expression();
      expect(";");
    }

    if (consume_reserved(")")) {
      node->loop = NULL;
    } else {
      node->loop = expression();
      expect(")");
    }

    node->trueStatement = statement();
    updateType(node);
    return node;
  }


  if (consume_token(TK_RETURN)) {
    node = new_node(ND_RETURN);
    node->lhs = expression();
    expect(";");
    updateType(node);
    return node;
  }

  if (confirm_type_token()) {
    node = declaration();
    expect(";");
    updateType(node);
    return node;
  }

  node = expression();
  expect(";");
  updateType(node);
  return node;
}


// expression = assign
Node *expression() {
  return assign();
}


// assign = equality ("=" assign)?
Node *assign() {
  Node *node = equality();

  if (consume_reserved("=")) {
    node = new_node_binary(ND_ASSIGN, node, assign());
    return node;
  }

  return node;
}


// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume_reserved("==")) {
      node = new_node_binary(ND_EQ, node, relational());
    }
    else if (consume_reserved("!=")) {
      node = new_node_binary(ND_NE, node, relational());
    }
    else {
      return node;
    }
  }
}


// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume_reserved("<")) {
      node = new_node_binary(ND_LT, node, add());
    }
    else if (consume_reserved("<=")) {
      node = new_node_binary(ND_LE, node, add());
    }
    else if (consume_reserved(">")) {
      node = new_node_binary(ND_LT, add(), node);
    }
    else if (consume_reserved(">=")) {
      node = new_node_binary(ND_LE, add(), node);
    }
    else {
      return node;
    }
  }
}




// add = mul ('+' mul | '-' mul)*
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume_reserved("+")) {
      node = new_add(node, mul());
    }
    else if (consume_reserved("-")) {
      node = new_sub(node, mul());
    }
    else
      return node; 
  }
}


// mul = unary ('*' unary | '/' unary)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume_reserved("*"))
      node = new_node_binary(ND_MUL, node, unary());
    else if (consume_reserved("/"))
      node = new_node_binary(ND_DIV, node, unary());
    else
      return node;
  }
}


// function_arg_list = '(' ( assign (',' assign)* )?  ')'
NodeList *function_arg_list() {
  expect("(");
  if (consume_reserved(")")) {
    return NULL;
  }

  NodeList* head = calloc(1, sizeof(NodeList));
  head->node = assign();
  
  NodeList* cur = head;
  while (consume_reserved(",")) {
    cur->next = calloc(1, sizeof(NodeList));
    cur = cur->next;
    cur->node = assign();
  }

  expect(")");
  return head;
}


// HACK 配列は一次元のみ
// primary = num 
//        | indent ( '[' expression ']' | function_arg_list )?
//        | '(' expression ')'
Node *primary() {
  if (consume_reserved("(")) {
    Node *node = expression();
    expect(")");
    return node;
  }

  Token *ident = consume_ident();
  if (ident) {
    if (confirm_reserved("(")) {
      Node *node = new_node(ND_FUNC_CALL);
      node->funcName = copy_string(ident->str, ident->len);
      node->args = function_arg_list();
      return node;
    }
    
    LVar *v = find_lvar(get_local_varlist(), ident);
    if (v == NULL) {
        error_at(ident->str, "定義されていない変数を使用しました");
    }

    if (v->type->kind == TY_ARRAY) {
      if (consume_reserved("[")) {
        Node *exp = expression();
        expect("]");
        return new_node_array_indexer(v, exp) ;
      }
    }

    return new_node_var(v);
  }

  return new_node_num(expect_number());
}


// unary = 'sizeof' unary
//        | '+'? primary
//        | '-'? primary
//        | '&' unary
//        | '*' unary
Node *unary() {
  if (consume_token(TK_SIZEOF)) {
    Node *node = unary();
    updateType(node);

    node = new_node_num(node->evalType->size);
    return node;
  }

  if (consume_reserved("&")) {
    Node *node = new_node(ND_ADDR);
    node->lhs = unary();
    return node;
  }

  if (consume_reserved("*")) {
    Node *node = new_node(ND_DEREF);
    node->lhs = unary();
    return node;
  }

  if (consume_reserved("+")) {
    return primary();
  }

  if (consume_reserved("-")) {
    return new_node_binary(ND_SUB, new_node_num(0), primary());
  }

  return primary();
}

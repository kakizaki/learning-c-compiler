#include "9cc.h"

static VarList *localVarList = NULL;
static int stackSize = 0;


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
  node->val = val;
  return node;
}


static char* copyString(const char* s, int len) {
  char* d = malloc(len + 1);
  strncpy(d, s, len);
  d[len] = '\0';
  return d;
}


static LVar *existsOrNewVariable(Token *t) {
  LVar *l = find_lvar(localVarList, t);
  if (l != NULL) {
    return l;
  }

  stackSize += 8;
  l = calloc(1, sizeof(LVar));
  l->name = copyString(t->str, t->len);
  l->len = t->len;
  l->offset = stackSize;

  VarList *v = calloc(1, sizeof(VarList));
  v->var = l;
  v->next = localVarList;
  localVarList = v;
  return l;
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


// function = ident func-params? "{" statement* "}"
Function* function() {
  localVarList = NULL;
  stackSize = 0;

  Token *ident = expect_ident();
  Function *func = calloc(1, sizeof(Function));
  func->name = copyString(ident->str, ident->len);
  func->params = funcParams();

  expect("{");
  NodeList head = { 0, 0 };
  NodeList* cur = &head;

  while (consume_reserved("}") == false) {
    cur->next = calloc(1, sizeof(NodeList));
    cur = cur->next;
    cur->node = statement();
  }

  func->block = head.next;
  func->locals = localVarList;
  func->stackSize = stackSize;
  return func;
}


// func-params = '(' ( ident (',' ident)* )?  ')'
VarList *funcParams() {
  expect("(");
  if (consume_reserved(")")) {
    return NULL;
  }

  Token *ident = expect_ident();
  VarList *cur = calloc(1, sizeof(VarList));
  cur->var = existsOrNewVariable(ident);

  VarList *head = cur;
  while (consume_reserved(",")) {
    ident = expect_ident();
    VarList *l = calloc(1, sizeof(VarList));
    l->var = existsOrNewVariable(ident);
    cur->next = l;
    cur = l;
  } 
  expect(")");
  return head;
}



// statement = expression ";" 
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
    return node;
  }

  if (consume_token(TK_WHILE)) {
    node = new_node(ND_WHILE);

    expect("(");
    node->cond = expression();
    expect(")");

    node->trueStatement = statement();
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
    return node;
  }


  if (consume_token(TK_RETURN)) {
    node = new_node(ND_RETURN);
    node->lhs = expression();
  } else {
    node = expression();
  }

  expect(";");
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
    if (consume_reserved("+")) 
      node = new_node_binary(ND_ADD, node, mul());
    else if (consume_reserved("-"))
      node = new_node_binary(ND_SUB, node, mul());
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


// func-args = '(' ( assign (',' assign)* )?  ')'
NodeList *funcArgs() {
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


// primary = num 
//        | indent func-args?
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
      node->funcName = copyString(ident->str, ident->len);
      node->args = funcArgs();
      return node;
    } else {
      Node *node = new_node(ND_LVAR);
      LVar *v = existsOrNewVariable(ident);
      node->offset = v->offset;
      return node;
    }
  }

  return new_node_num(expect_number());
}


// unary = ('+' | '-')? primary
Node *unary() {
  if (consume_reserved("+")) {
    return primary();
  }
  if (consume_reserved("-")) {
    return new_node_binary(ND_SUB, new_node_num(0), primary());
  }
  return primary();
}

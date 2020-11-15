#include "9cc.h"

Node *code[100];


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


// program = statement*
void program() {
  int i = 0;
  while (!at_eof()) {
    code[i] = statement();
    ++i;
  }
  code[i] = NULL;
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
      LVar *lvar = find_lvar(ident);
      if (lvar) {
        node->offset = lvar->offset;
      }
      else {
        lvar = calloc(1, sizeof(LVar));
        lvar->name = ident->str;
        lvar->len = ident->len;

        // 新しい変数をリストの先頭にする
        if (locals) {
          node->offset = locals->offset + 8;
        } else {
          node->offset = 8;
        }
        lvar->offset = node->offset;
        lvar->next = locals;
        locals = lvar;
      }
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

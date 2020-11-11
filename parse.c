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
//          | "if" "(" expression ")" statement ("else" statement)?
//          | "while" "(" expression ")" statement
//          | "for" "(" expression? ";" expression? ";" expression? ")" statement
//          | "return" expression ";"
Node *statement() {
  Node *node;

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
    if (consume(";")) {
      node->init = NULL;
    } else {
      node->init = expression();
      expect(";");
    }

    if (consume(";")) {
      node->cond = NULL;
    } else {
      node->cond = expression();
      expect(";");
    }

    if (consume(")")) {
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

  if (consume("=")) {
    node = new_node_binary(ND_ASSIGN, node, assign());
    return node;
  }

  return node;
}


// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("==")) {
      node = new_node_binary(ND_EQ, node, relational());
    }
    else if (consume("!=")) {
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
    if (consume("<")) {
      node = new_node_binary(ND_LT, node, add());
    }
    else if (consume("<=")) {
      node = new_node_binary(ND_LE, node, add());
    }
    else if (consume(">")) {
      node = new_node_binary(ND_LT, add(), node);
    }
    else if (consume(">=")) {
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
    if (consume("+")) 
      node = new_node_binary(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node_binary(ND_SUB, node, mul());
    else
      return node; 
  }
}


// mul = unary ('*' unary | '/' unary)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node_binary(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node_binary(ND_DIV, node, unary());
    else
      return node;
  }
}


// primary = num | indent | '(' expression ')'
Node *primary() {
  if (consume("(")) {
    Node *node = expression();
    expect(")");
    return node;
  }

  Token *ident = consume_ident();
  if (ident) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;

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

  return new_node_num(expect_number());
}


// unary = ('+' | '-')? primary
Node *unary() {
  if (consume("+")) {
    return primary();
  }
  if (consume("-")) {
    return new_node_binary(ND_SUB, new_node_num(0), primary());
  }
  return primary();
}

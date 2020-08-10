#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// 入力文字列
char* user_input;

// --- tokenizer ---
// TOKEN
typedef enum {
  TK_RESERVED,  // 記号
  TK_NUM,       // 整数トークン
  TK_EOF,       // 入力の終わりを表すトークン
} TokenKind;

// リスト構造のため、名前を先に宣言している
typedef struct Token Token;

struct Token {
  TokenKind kind;  // トークンの型
  Token *next;     // 次の入力トークン
  int val;         // tyがTK_NUMの場合、その数値
  char *str;       // トークン文字列
  int len;         // トークンの長さ
};

// 現在着目しているトークン
Token *token;

// エラーメッセージを出力し、プログラムを終了する
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}


bool startWith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}


// 現在のトークンが期待する記号だった場合に、次のトークンへ移動する
// 結果を返す
bool consume(char *op) {
  if (token->kind != TK_RESERVED 
  || strlen(op) != token->len 
  || memcmp(token->str, op, token->len) != 0) {
    return false;
  }

  token = token->next;
  return true;
}

// 現在のトークンが期待する記号だった場合に、次のトークンへ移動する 
void expect(char *op) {
  if (token->kind != TK_RESERVED 
  || strlen(op) != token->len
  || memcmp(token->str, op, token->len) != 0) {
    error_at(token->str, "'%c'ではありません", op);
  }

  token = token->next;
}

// 現在のトークンが数字だった場合に、次のトークンへ移動する
int expect_number() {
  if (token->kind != TK_NUM) 
    error_at(token->str, "'数ではありません");

  int val = token->val;
  token = token->next;
  return val;
}

// 現在のトークンがEOFかどうか
bool at_eof() {
  return token->kind == TK_EOF;
}

// 新しいトークンを確保し、cur->nextにセットする
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));

  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}



// トークンに分割する
Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (startWith(p, "<=") 
    || startWith(p, ">=") 
    || startWith(p, "==")
    || startWith(p, "!=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    if (*p == '<' || *p == '>') {
      cur = new_token(TK_RESERVED, cur, p, 1);
      p++;
      continue;
    }

    if (*p == '+' 
    || *p == '-' 
    || *p == '*' 
    || *p == '/'
    || *p == '('
    || *p == ')'
    ) {
      cur = new_token(TK_RESERVED, cur, p, 1);
      p++;
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}


// --- 抽象構文木 ---
// 抽象構文木のノードの種類
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // 整数

  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
  NodeKind kind; // ノードの型
  Node *lhs; // 左辺
  Node *rhs; // 右辺
  int val; // kind が ND_NUM の場合のみ使う
};


Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();


Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}
 
Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

// expr = equality
Node *expr() {
  return equality();
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

// primary = num | '(' expr ')'
Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
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


// --- generator ---
void gen(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);
  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  }

  printf("  push rax\n");
}


// --- main ---
//
int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数がただしくありません\n");
    return 1;
  }

  //
  user_input = argv[1];
  token = tokenize(argv[1]);
  Node *node = expr();

  //
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  //
  gen(node);

  // 
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}









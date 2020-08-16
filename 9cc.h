#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

// 
// tokenize.c : TOKEN
// 

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

void error_at(char *loc, char *fmt, ...);
void expect(char *op);
int expect_number();
bool consume(char *op);

Token *tokenize(char *p);



// 
// parse.c : 抽象構文木
// 

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


// 
// codege.c
//

void codegen(Node *node);



//
// global variables
//

// 入力文字列
extern char* user_input;

// 現在着目しているトークン
extern Token *token;


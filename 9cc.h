#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

// utility
char* copy_string(const char* s, int len);

// 
// tokenize.c : TOKEN
// 

typedef enum {
  TK_RESERVED,  // 記号
  TK_IDENT,     // 識別子
  TK_NUM,       // 整数トークン
  TK_RETURN,    // return
  TK_IF,        // if
  TK_ELSE,      // else
  TK_WHILE,     // while
  TK_FOR,       // for
  TK_INT,       // int
  TK_SIZEOF,    // sizeof
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


//
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_current_token(char *fmt, ...);

void expect(char *op);
void expect_token(TokenKind t);
int expect_number();
Token *expect_ident();

bool confirm_reserved(char *op);
bool confirm_token(TokenKind t);

bool consume_reserved(char *op);
bool consume_token(TokenKind t);
Token *consume_ident();

bool at_eof();

Token *tokenize(char *p);



// 
// var.c : 変数
// 


// 変数の型
typedef enum { TY_INT, TY_PTR, TY_ARRAY } TypeKind;
typedef struct Type Type;

struct Type {
  TypeKind kind;
  int size;
  Type *ptr_to;
  size_t array_size;
};


// ローカル変数
typedef struct LVar LVar;

struct LVar {
  char *name;     // 変数の名前
  int len;        // 名前の長さ
  int offset;     // RBPからのオフセット
  Type *type;
};


typedef struct VarList VarList;

struct VarList {
  LVar *var;
  VarList *next;
};

LVar *find_lvar(VarList *l, Token *tok);

void clear_local_varlist();
VarList *get_local_varlist();
LVar *add_local_var(Token *t, Type *type);
int update_offset_local_varlist();



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

  ND_PTR_ADD,
  ND_PTR_SUB,
  ND_PTR_DIFF,

  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=

  ND_ASSIGN,  // =
  ND_LVAR,    // ローカル変数 

  ND_RETURN,
  ND_IF,
  ND_IF_ELSE,
  ND_WHILE,
  ND_FOR,
  ND_BLOCK,

  ND_FUNC_CALL,

  ND_ADDR,
  ND_DEREF,
} NodeKind;

typedef struct Node Node;
typedef struct NodeList NodeList;

// 抽象構文木のノードの型
struct Node {
  NodeKind kind;  // ノードの型
  Type *evalType;       // 演算結果の値の型
  Node *lhs;      // 左辺
  Node *rhs;      // 右辺
  int value;        // kind が ND_NUM の場合のみ使う
  LVar *var;      // kind が ND_LVAR の場合のみ使う



  // if (cond) trueStatement; else falseStatement;
  // while (cond) trueStatement;
  // for (init; cond; loop) trueStatement;
  Node *cond;
  Node *trueStatement;
  Node *falseStatement;
  Node *init;
  Node *loop;

  // { }
  NodeList *block;

  // function call / function define
  char* funcName;
  NodeList *args;
};

// 
struct NodeList {
  Node* node;
  NodeList *next;
};



// 関数
typedef struct Function Function;

struct Function {
  Function *next;
  char *name;
  VarList *params;
  VarList *locals;
  NodeList *block;
  int stackSize;
};


//
struct Program {
  Function *function;
};


Function* program();
Function* function();

VarList *function_param_list();
LVar *function_param();
NodeList *function_arg_list();

Node *statement();
Node *declaration();
Node *expression();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();


//
// type.c
// 
Type *type_int();
Type *type_pointer_to(Type *base);
Type *type_array_to(Type *base, int array_size);
bool isInteger(Type* t);
void updateType(Node *node);



// 
// codegen.c
//

void codegen();



//
// global variables
//

// 入力文字列
extern char* user_input;

// 現在着目しているトークン
extern Token *token;

//
extern Node *code[100];

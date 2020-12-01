
#include "9cc.h"


void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}


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


void error_current_token(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = token->str - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 現在のトークンが期待する記号だった場合に、次のトークンへ移動する 
void expect(char *op) {
  if (token->kind != TK_RESERVED 
  || strlen(op) != token->len
  || memcmp(token->str, op, token->len) != 0) {
    error_at(token->str, "'%s'ではありません", op);
  }

  token = token->next;
}


// 現在のトークンが期待するトークンだった場合に、次のトークンへ移動する
void expect_token(TokenKind t) {
  if (token->kind != t) {
    error_at(token->str, "'トークンではありません");
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


// 現在のトークンが識別子だった場合に、次のトークンへ移動する
Token *expect_ident() {
  if (token->kind != TK_IDENT) 
    error_at(token->str, "識別子ではありません");

  Token *t = token;
  token = token->next;
  return t;
}



static bool startWith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}


static bool isVariableNameHead(char p) {
  return 'a' <= p && p <= 'z' || 'A' <= p && p <= 'Z' || p == '_';
}

static bool isVariableName(char p) {
  return isVariableNameHead(p) || '0' <= p && p <= '9';
}

static int getIdentWord(char *p) {
  int len = 0;

  if (isVariableNameHead(*p)) {
    do {
        len++;
        p++;
    } while (isVariableName(*p));
  }
  return len;
}

static bool is_alnum(char c) {
  return ('a' <= c && c <= 'z') 
  || ('A' <= c && c <= 'Z')
  || ('0' <= c && c <= '9')
  || (c == '_');
}

static bool is_reserved(const char* p, const char* word, int len) {
  if (strncmp(p, word, len) != 0) {
    return false;
  }

  if (is_alnum(p[len])) {
    return false;
  }

  return true;
}

static bool is_reserved_char(const char p) {
    if (
      p == '<' || p == '>'
    || p == '+' || p == '-' || p == '*' || p == '/'
    || p == '(' || p == ')'
    || p == '{' || p == '}'
    || p == '[' || p == ']'
    || p == ';' || p == '='
    || p == ',' || p == '&'
    ) {
      return true;
    }

    return false;
}


// 現在のトークンが期待する記号かどうか確認する
// 結果を返す
bool confirm_reserved(char *op) {
  if (token->kind != TK_RESERVED 
  || strlen(op) != token->len 
  || memcmp(token->str, op, token->len) != 0) {
    return false;
  }
  return true;
}


// 現在のトークンが期待する種類かどうか確認する
// 結果を返す
bool confirm_token(TokenKind t) {
  if (token->kind == t) {
    return true;
  }
  return false;
}



// 現在のトークンが期待する記号だった場合に、次のトークンへ移動する
// 結果を返す
bool consume_reserved(char *op) {
  if (token->kind != TK_RESERVED 
  || strlen(op) != token->len 
  || memcmp(token->str, op, token->len) != 0) {
    return false;
  }

  token = token->next;
  return true;
}


// 現在のトークンが期待するトークンだった場合に、次のトークンへ移動する
bool consume_token(TokenKind t) {
  if (token->kind != t) {
    return false;
  }

  token = token->next;
  return true;
}


// 現在のトークンが TK_IDENT だった場合に、次のトークンへ移動する
// TK_IDENT だった場合はトークンを返す
// TK_IDENT でない場合は NULL を返す
Token *consume_ident() {
  if (token->kind != TK_IDENT) {
    return NULL;
  }
  Token* ident = token;
  token = token->next;
  return ident;
}


// 現在のトークンがEOFかどうか
bool at_eof() {
  return token->kind == TK_EOF;
}


// 新しいトークンを確保し、cur->nextにセットする
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
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
  int length = 0;
  char* word = "";

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

    if (is_reserved_char(*p)) {
       cur = new_token(TK_RESERVED, cur, p, 1);
       p++;
       continue;
    }

    word = "return";
    length = strlen(word);
    if (is_reserved(p, word, length)) {
      cur = new_token(TK_RETURN, cur, p, length);
      p += length;
      continue;
    }

    word = "if";
    length = strlen(word);
    if (is_reserved(p, word, length)) {
      cur = new_token(TK_IF, cur, p, length);
      p += length;
      continue;
    }

    word = "else";
    length = strlen(word);
    if (is_reserved(p, word, length)) {
      cur = new_token(TK_ELSE, cur, p, length);
      p += length;
      continue;
    }

    word = "while";
    length = strlen(word);
    if (is_reserved(p, word, length)) {
      cur = new_token(TK_WHILE, cur, p, length);
      p += length;
      continue;
    }

    word = "for";
    length = strlen(word);
    if (is_reserved(p, word, length)) {
      cur = new_token(TK_FOR, cur, p, length);
      p += length;
      continue;
    }

    word = "int";
    length = strlen(word);
    if (is_reserved(p, word, length)) {
      cur = new_token(TK_INT, cur, p, length);
      p += length;
      continue;
    }

    word = "char";
    length = strlen(word);
    if (is_reserved(p, word, length)) {
      cur = new_token(TK_CHAR, cur, p, length);
      p += length;
      continue;
    }

    word = "sizeof";
    length = strlen(word);
    if (is_reserved(p, word, length)) {
      cur = new_token(TK_SIZEOF, cur, p, length);
      p += length;
      continue;
    }

    length = getIdentWord(p);
    if (0 < length) {
      cur = new_token(TK_IDENT, cur, p, length);
      p += length;
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

#include "9cc.h"


// 入力文字列
char* user_input;

// 現在着目しているトークン
Token *token;

//
LVar *find_lvar(VarList *list, Token *tok) {
  for (VarList *v = list; v && v->var; v = v->next) {
    if (v->var->len == tok->len && !memcmp(tok->str, v->var->name, v->var->len)) {
      return v->var;
    }
  }
  return NULL;
}


static void test_array_add() {
  int a[2];
  printf("# %x, %x, %x\n", a, a+1, &a+1); // 0, +4, +8
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
  
  printf("# will tokenize\n");
  token = tokenize(argv[1]);
  
  printf("# will parse\n");
  Function *mainFunction = program();

  printf("# will codegen\n");
  codegen(mainFunction);

  return 0;
}



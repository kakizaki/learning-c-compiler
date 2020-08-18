#include "9cc.h"


// 入力文字列
char* user_input;

// 現在着目しているトークン
Token *token;



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
  program();

  //
  codegen();

  return 0;
}



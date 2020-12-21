#include "9cc.h"
#include <stdio.h>
#include <errno.h>


// 入力ファイル
char *filename;

// 入力文字列
char *user_input;

// 現在着目しているトークン
Token *token;


char* copy_string(const char* s, int len) {
  char* d = malloc(len + 1);
  strncpy(d, s, len);
  d[len] = '\0';
  return d;
}



// static void test_array_add() {
//   int a[2];
//   printf("# %x, %x, %x\n", a, a+1, &a+1); // 0, +4, +8
// }

static char *read_file(char *path) {
  // ファイルを開く
  FILE *fp = fopen(path, "r");
  if (!fp) {
    error("cannot open %s: %s", path, strerror(errno));
  }

  // int filemax = 10 * 1024 * 1024;
  // char *buf = malloc(filemax);
  // int size = fread(buf, 1, filemax -2, fp);
  // if (!feof(fp)) {
  //   error("%s: file too large");
  // }

  // //
  // if (size == 0 || buf[size - 1] != '\n') {
  //   buf[size++] = '\n';
  // }
  // buf[size] = '\0';
  // fclose(fp);
  // return buf;


  // ファイルの長さを調べる
  if (fseek(fp, 0, SEEK_END) == -1) {
    error("%s: fseek: %s", path, strerror(errno));
  }
  size_t size = ftell(fp);
  if (fseek(fp, 0, SEEK_SET) == -1) {
    error("%s: fseek: %s", path, strerror(errno));
  }

  // ファイルの内容を読み込む
  char *buf = calloc(1, size + 2);
  fread(buf, size, 1, fp);

  // ファイルが必ず "\n\0" で終わっているようにする
  if (size == 0 || buf[size - 1] != '\n') {
    buf[size++] = '\n';
  }
  buf[size] = '\0';
  fclose(fp);
  return buf;
}


// --- main ---
//
int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数がただしくありません\n");
    return 1;
  }
  
  //
  filename = argv[1];
  user_input = read_file(filename);
  
  printf("# will tokenize\n");
  token = tokenize(user_input);
  
  printf("# will parse\n");
  Program *p = program();

  printf("# will codegen\n");
  codegen(p);

  return 0;
}



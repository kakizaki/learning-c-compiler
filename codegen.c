
#include "9cc.h"


static void gen_lval(Node *node) {
  if (node->kind != ND_LVAR) {
    error("代入の左辺値が変数ではありません");
  }

  // 変数のアドレスを push
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}


int publishedLabelCount = 0;

int publishLabelID() {
  int i = publishedLabelCount;
  publishedLabelCount++;
  return i;
}


static void gen(Node *node) {
  int labelID;

  switch (node->kind) {

  // 数値
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;

  // 変数の値を参照
  case ND_LVAR:
    // 変数のアドレスを push
    gen_lval(node);

    // 変数のアドレスを取り出し、そのアドレスにある値を取り出し、その値を push
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;

  // 変数へ代入
  case ND_ASSIGN:
    // 変数のアドレスを push
    gen_lval(node->lhs);

    // 右辺の値を push
    gen(node->rhs);

    // 右辺の値を取り出し、変数のアドレスを取り出し、変数のアドレスへ右辺の値をセット
    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;

  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;

  case ND_IF:
    labelID = publishLabelID();
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lend%d\n", labelID);
    gen(node->trueStatement);
    printf(".Lend%d:\n", labelID);
    return;

  case ND_IF_ELSE:
    labelID = publishLabelID();
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lelse%d\n", labelID);
    gen(node->trueStatement);
    printf("  jmp .Lend%d\n",labelID);
    printf(".Lelse%d:\n", labelID);
    gen(node->falseStatement);
    printf(".Lend%d:\n", labelID);
    return;

  case ND_WHILE:
    labelID = publishLabelID();
    printf(".Lbegin%d:\n", labelID);
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%d\n", labelID);
    gen(node->trueStatement);
    printf("  jmp .Lbegin%d\n", labelID);
    printf(".Lend%d:\n", labelID);
    return;

  case ND_FOR:
    labelID = publishLabelID();
    if (node->init != NULL) {
      gen(node->init);
    }
    printf(".Lbegin%d:\n", labelID);
    if (node->cond != NULL) {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .Lend%d\n", labelID);
    }
    gen(node->trueStatement);
    if (node->loop != NULL) {
      gen(node->loop);
    }
    printf("  jmp .Lbegin%d\n", labelID);
    printf(".Lend%d:\n", labelID);
    return;

  case ND_BLOCK:
    for (NodeList* n = (node->block); n && n->node; n = n->next) {
      gen(n->node);
    }
    return;
  }

  // 上記以外の 2項演算

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



void codegen() {
  // アセンブリの前半部分
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // 変数26個の領域を確保
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, 208\n");

  // 
  int i = 0;
  while (code[i] != NULL) {
    gen(code[i]);

    // 式の評価結果としてスタックに1つの値が残っているはずなので
    // スタックが溢れないように pop しておく
    printf("  pop rax\n");

    ++i;
  }

  // 最後の式の結果が rax に残っているので
  // それが返り値になる
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}


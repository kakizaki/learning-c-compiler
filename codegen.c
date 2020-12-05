
#include "9cc.h"


static char *funcname;

// 変数のアドレスを push
static void push_val_address(Node *node) {
  if (node->kind != ND_VAR) {
    error("代入の左辺値が変数ではありません");
  }

  // 変数のアドレスを push
  if (node->var->is_local) {
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->var->offset);
    printf("  push rax\n");
  }
  else {
    printf("  push offset %s\n", node->var->name);
  }
}

// 変数のアドレスを取り出し、そのアドレスにある値を取り出し、その値を push
static void push_load_value(Type *t) {
  printf("  pop rax\n");

  if (t->size == 1) {
    printf("  movsx rax, byte ptr [rax]\n");
    printf("  push rax\n");
  } else {
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
  }
}


int publishedLabelCount = 0;

int publishLabelID() {
  int i = publishedLabelCount;
  publishedLabelCount++;
  return i;
}

static char *argreg_1[] = { "dil", "sil", "dl", "cl", "r8b", "r9b" };
static char *argreg_8[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };


static void gen(Node *node) {
  int labelID;

  switch (node->kind) {

  // 数値
  case ND_NUM:
    printf("  push %d\n", node->value);
    return;

  // 文字列
  case ND_STR:
    printf("  push offset .LC%d\n", node->string->id);
    return;

  // 変数の値を参照
  case ND_VAR:
    // 変数のアドレスを push
    push_val_address(node);
    
    // 配列変数はポインタとして扱うため、値を読まず、アドレスのままでいい
    if (node->var->type->kind != TY_ARRAY) {
      push_load_value(node->var->type);
    }
    return;

  // 変数へ代入
  case ND_ASSIGN:
    // 変数のアドレスを push
    if (node->lhs->kind == ND_DEREF) {
      // ND_DEREF なものの指定されたアドレスに値をセットさせるので、アドレス値が期待される
      gen(node->lhs->lhs);

      // node->lhs->lhs が変数だけなら以下でいいが、*(a+8) もあるので gen に流す
      //    push_val_address(node->lhs->lhs);
      //    printf("  pop rax\n");
      //    printf("  mov rax, [rax]\n");
      //    printf("  push rax\n")
      // gen(node->lhs) (gen(ND_DEREF)) はアドレス先の値を push するので処理が異なる (ほしいのはアドレス)
      
    } else {
      push_val_address(node->lhs);
    }
    printf("#  aaaa\n");

    // 右辺の値を push
    gen(node->rhs);
    
    printf("#  bbbb\n");

    // 右辺の値を取り出し、変数のアドレスを取り出し、変数のアドレスへ右辺の値をセット
    printf("  pop rdi\n");
    printf("  pop rax\n");

    if (node->lhs->evalType->size == 1) {
      printf("  mov [rax], dil\n");
    } else {
      printf("  mov [rax], rdi\n");
    }
    printf("  push rdi\n");
    return;

  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  jmp .L.return.%s\n", funcname);
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

  case ND_FUNC_CALL:
    {
      int argsLength = 0;
      for (NodeList* n = (node->args); n && n->node; n = n->next) {
        gen(n->node);
        argsLength++;
      }
      for (int i = argsLength - 1; 0 <= i ; i--) {
        printf("  pop %s\n", argreg_8[i]);
      }
    }

    // 関数呼び出しの前に RSP を 16 の倍数とする
    labelID = publishLabelID();
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n");
    printf("  jnz .L.call.%d\n", labelID);
    // and の結果、0 なので調整しない
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->funcName);
    printf("  jmp .L.end.%d\n", labelID);
    // 調整する
    printf(".L.call.%d:\n", labelID);
    printf("  sub rsp, 8\n"); // 現状、push, pop など8バイト操作しかないため?
    printf("  mov rax, 0\n");
    printf("  call %s\n", node->funcName);
    printf("  add rsp, 8\n");
    //
    printf(".L.end.%d:\n", labelID);
    printf("  push rax\n");
    return;

  case ND_ADDR:
    push_val_address(node->lhs);
    return;
  
  case ND_DEREF:
    gen(node->lhs);
    push_load_value(node->lhs->evalType);
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
  case ND_PTR_ADD:
    printf("  imul rdi, %d\n", node->lhs->evalType->ptr_to->size);
    printf("  add rax, rdi\n");
    break;
  case ND_PTR_SUB:
    printf("  imul rdi, %d\n", node->lhs->evalType->ptr_to->size);
    printf("  sub rax, rdi\n");
    break;
  case ND_PTR_DIFF:
    printf("  sub rax, rdi\n");
    printf("  cqo\n");
    printf("  mov rdi, %d\n", node->lhs->evalType->ptr_to->size);
    printf("  idiv rdi\n");
    break;
  }

  printf("  push rax\n");
}



void codegen(Program *program) {
  printf(".intel_syntax noprefix\n");

  printf(".data\n");
  for (VarList *v = program->global_var; v; v = v->next) {
    printf("%s:\n", v->var->name);
    printf("  .zero %d\n", v->var->type->size);
  }

  for (StringList *s = program->string_list; s; s = s->next) {
    printf(".LC%d:\n", s->s->id);
    for (int i = 0; i < s->s->length; i++) {
      printf("  .byte %d\n", s->s->p[i]);
    }
  }

  for (Function *f = program->function; f; f = f->next) {
    funcname = f->name;

    printf(".global %s\n", f->name);
    printf("%s:\n", f->name);

    // 変数の領域を確保
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", f->stackSize);

    // 引数をスタックに追加
    int i = 0;
    for (VarList *v = f->params; v; v = v->next) {
      if (v->var->type->size == 1) {
        printf("  mov [rbp-%d], %s\n", v->var->offset, argreg_1[i]);
      } else {
        printf("  mov [rbp-%d], %s\n", v->var->offset, argreg_8[i]);
      }
      i++;
    }  

    for (NodeList *n = f->block; n && n->node; n = n->next) {
      gen(n->node);

      // 式の評価結果としてスタックに1つの値が残っているはずなので
      // スタックが溢れないように pop しておく
      // HACK コメントアウト
      //   "main() {a=1;if(0)1;return a;}"" がエラーとなる
      //   else を通る際に push がないが、ここで pop してしまっている
      //printf("  pop rax # aaa\n");
    }

    // 最後の式の結果が rax に残っているので
    // それが返り値になる
    printf(".L.return.%s:\n", f->name);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
  }
}


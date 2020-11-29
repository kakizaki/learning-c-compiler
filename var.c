#include "9cc.h"


LVar *find_lvar(VarList *list, Token *tok) {
  for (VarList *v = list; v && v->var; v = v->next) {
    if (v->var->len == tok->len && !memcmp(tok->str, v->var->name, v->var->len)) {
      return v->var;
    }
  }
  return NULL;
}


static VarList *localVarList;

void clear_local_varlist() {
    localVarList = NULL;
}

VarList *get_local_varlist() {
    return localVarList;
}

// 新しい変数を追加して返す
LVar *add_local_var(Token *t, Type *type) {
  LVar *l = calloc(1, sizeof(LVar));
  l->name = copy_string(t->str, t->len);
  l->type = type;
  l->len = t->len;

  // 新しい変数を先頭に追加
  VarList *v = calloc(1, sizeof(VarList));
  v->var = l;
  v->next = localVarList;
  localVarList = v;
  return l;
}

// ローカル変数のオフセットを再計算
int update_offset_local_varlist() {
  // 新しいものが先頭にあり、その順にオフセットをセットする
  //   オフセットが小さいほうがアドレスとしては大きい値になる (スタックを使用する方向)
  //   変数の定義順にアドレスが大きい値となる
  int offset = 0;
  for (VarList *v = localVarList; v; v = v->next) {
    offset += v->var->type->size;
    v->var->offset = offset;
  }
  return offset;
}





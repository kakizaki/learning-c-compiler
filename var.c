#include "9cc.h"



Var *find_var(VarList *list, Token *tok) {
  for (VarList *v = list; v; v = v->next) {
    if (v->var->len == tok->len && !memcmp(tok->str, v->var->name, v->var->len)) {
      return v->var;
    }
  }
  return NULL;
}


// 新しい変数を追加して返す
static Var *new_var(Token *t, Type *type, bool is_local) {
  Var *v = calloc(1, sizeof(Var));
  v->name = copy_string(t->str, t->len);
  v->type = type;
  v->len = t->len;
  v->is_local = is_local;
  return v;
}



static VarList local_vars;
static VarList *current_local_var;
static int sum_local_size;

void clear_local_varlist() { 
  local_vars.next = NULL;
  current_local_var = &local_vars;
  sum_local_size = 0;
}

VarList *get_local_varlist() {
  return local_vars.next;
}

Var *add_local_var(Token *t, Type *type) {
  sum_local_size = sum_local_size + type->size;
  
  VarList *v = calloc(1, sizeof(VarList));
  v->var = new_var(t, type, true);
  current_local_var->next = v;
  current_local_var = v;
  return v->var;
}


// ローカル変数のオフセットを再計算
int update_offset_local_varlist() {
  // オフセットが小さいほうがアドレスとしては大きい値になる (スタックを使用する方向)
  // 変数の定義順にアドレスが大きい値となる
  int offset = sum_local_size;
  for (VarList *v = local_vars.next; v; v = v->next) {
    v->var->offset = offset;
    offset -= v->var->type->size;
  }
  return sum_local_size;
}


//
static VarList global_vars;
static VarList *current_global_var;

void clear_global_varlist() {
  global_vars.next = NULL;
  current_global_var = &global_vars;
}

VarList *get_global_varlist() {
  return global_vars.next;
}

// 新しい変数を追加して返す
Var *add_global_var(Token *t, Type *type) {
  VarList *v = calloc(1, sizeof(VarList));
  v->var = new_var(t, type, false);
  current_global_var->next = v;
  current_global_var = v;
  return v->var;
}



#include "9cc.h"




static Var *find_var(VarList *list, Token *tok) {
  for (VarList *v = list; v; v = v->next) {
    if (v->var->len == tok->len && !memcmp(tok->begin, v->var->name, v->var->len)) {
      return v->var;
    }
  }
  return NULL;
}


// 新しい変数を追加して返す
static Var *new_var(Token *t, Type *type, bool is_local) {
  Var *v = calloc(1, sizeof(Var));
  v->name = copy_string(t->begin, t->len);
  v->type = type;
  v->len = t->len;
  v->is_local = is_local;
  return v;
}


// スコープ
typedef struct Scope Scope;

struct Scope {
  Scope *parent;

  VarList *var;
};

static Scope global_scope;
static Scope *local_scope;

static void add_to_scope(Var *v) {
  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = v;

  Scope *s = NULL;
  if (v->is_local) {
    s = local_scope;
  } else {
    s = &global_scope;
  }

  if (s->var == NULL) {
    s->var = vl;
  } else {
    vl->next = s->var;
    s->var = vl;
  }
}

void begin_scope() {
  Scope *p = local_scope;
  local_scope = calloc(1, sizeof(Scope));
  local_scope->parent = p;
}

void end_scope() {
  if (local_scope != NULL) {
    local_scope = local_scope->parent;
  }
}


Var *find_var_in_global(Token *tok) {
  return find_var(global_scope.var, tok);
}


Var *find_var_in_scopes(Token *tok) {
  for (Scope *n = local_scope; n != NULL; n = n->parent) {
    Var *v = find_var(n->var, tok);
    if (v != NULL) {
      return v;
    }
  }
  return find_var_in_global(tok);
}


Var *find_var_in_current_scope(Token *tok) {
  if (local_scope != NULL) {
    return find_var(local_scope->var, tok);
  }
  return NULL;
}


// local variable
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

  add_to_scope(v->var);
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

static int string_literal_id;
static StringList string_list;
static StringList *string_list_current;


void clear_global_varlist() {
  global_vars.next = NULL;
  current_global_var = &global_vars;
  
  string_literal_id = 1;
  string_list.next = NULL;
  string_list_current = &string_list;
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

  add_to_scope(v->var);
  return v->var;
}

StringLiteral *add_string_literal(Token *t) {
  StringLiteral* s = calloc(1, sizeof(StringLiteral));
  s->p = copy_string(t->strLiteral, t->strLiteralLen);
  s->length = t->strLiteralLen;
  s->id = string_literal_id;
  string_literal_id++;

  StringList* l = calloc(1, sizeof(StringList));
  l->s = s;
  string_list_current->next = l;
  string_list_current = l;
  return s;
}

StringList *get_string_list() {
  return string_list.next;
}

#include "9cc.h"

static Type *int_type;

static Type *char_type;

Type *type_int() {
    if (int_type == NULL) {
        int_type = calloc(1, sizeof(Type));
        int_type->kind = TY_INT;
        int_type->size_of_kind = 4;
        int_type->size = 8;
    }
    return int_type;
}

Type *type_char() {
    if (char_type == NULL) {
        char_type = calloc(1, sizeof(Type));
        char_type->kind = TY_CHAR;
        char_type->size_of_kind = 1;
        char_type->size = 1;
    }
    return char_type;
}

Type *type_pointer_to(Type *t) {
    Type *p = calloc(1, sizeof(Type));
    p->kind = TY_PTR;
    p->size_of_kind = 8;
    p->size = 8;
    p->ptr_to = t;
    return p;
}

Type *type_array_to(Type *t, int length) {
    Type *p = calloc(1, sizeof(Type));
    p->kind = TY_ARRAY;
    p->size_of_kind = t->size_of_kind * length;
    p->size = t->size * length;
    p->array_length = length;
    p->ptr_to = t;
    return p;
}


bool is_integer(Type *t) {
    return t->kind == TY_INT || t->kind == TY_CHAR;
}

void updateType(Node *node) {
    if (node == NULL || node->evalType != NULL) {
        return;
    }

    updateType(node->lhs);
    updateType(node->rhs);
    updateType(node->cond);
    updateType(node->trueStatement);
    updateType(node->falseStatement);
    updateType(node->init);
    updateType(node->loop);

    for (NodeList *n = node->block; n; n = n->next) {
        updateType(n->node);
    }
    for (NodeList *n = node->args; n; n = n->next) {
        updateType(n->node);
    }

    switch (node->kind) {
    case ND_ADD:
    case ND_SUB:
    case ND_PTR_DIFF:
    case ND_MUL:
    case ND_DIV:
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_FUNC_CALL:
    case ND_NUM:
        node->evalType = type_int();
        return;
    case ND_VAR:
        node->evalType = node->var->type;
        return;
    case ND_PTR_ADD:
    case ND_PTR_SUB:
    case ND_ASSIGN:
        node->evalType = node->lhs->evalType;
        return;
    case ND_ADDR:
        node->evalType = type_pointer_to(node->lhs->evalType);
        return;
    case ND_DEREF:
        if (node->lhs->evalType->ptr_to != NULL) {
            node->evalType = node->lhs->evalType->ptr_to;
        } else {
            node->evalType = node->lhs->evalType;
        }
        return;
    }
}


#include "9cc.h"

Type *int_type = &(Type) { TY_INT };


static Type *pointerTo(Type *t) {
    Type *p = calloc(1, sizeof(Type));
    p->kind = TY_PTR;
    p->ptr_to = t;
    return p;
}

bool isInteger(Type *t) {
    return t->kind == TY_INT;
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
        node->evalType = int_type;
        return;
    case ND_LVAR:
        node->evalType = node->var->type;
        return;
    case ND_PTR_ADD:
    case ND_PTR_SUB:
    case ND_ASSIGN:
        node->evalType = node->lhs->evalType;
        return;
    case ND_ADDR:
        node->evalType = pointerTo(node->lhs->evalType);
        return;
    case ND_DEREF:
        if (node->lhs->evalType->kind == TY_PTR) {
            node->evalType = node->lhs->evalType->ptr_to;
        } else {
            node->evalType = node->lhs->evalType;
        }
        return;
    }
}


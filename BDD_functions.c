#include <stdio.h>
#include <string.h>
#include "HashTable.c"

#define true 1
#define false 0

typedef struct Node {
    int level;
    Node *low;
    Node *high;
    int value;
} Node;

typedef struct BDD {
    int var_count;
    int node_count;
    Node *root;
    Node *terminal_node_zero;
    Node *terminal_node_one;
} BDD;

typedef struct Term {
    char vars[64];
    int neg[64];
    int var_count;
    struct Term *next;
} Term;

typedef struct Expression {
    Term *terms;
} Expression;

typedef enum {
    VAR,
    NEG,
    AND,
    OR,
    END,
    INVALID
} SymbolType;

typedef struct {
    SymbolType type;
    char var;
} Symbol;

typedef struct {
    const char *input;
    int pos;
} InputReader;

void free_expression(Expression *expression) {
    while(expression->terms) {
        Term *tmp = expression->terms;
        expression->terms = expression->terms->next;
        free(tmp);
    }
    free(expression);
}

Symbol get_next_symbol(InputReader *inputReader) {
    while(inputReader->input[inputReader->pos] == ' ' ||
        inputReader->input[inputReader->pos] == '\t' ||
        inputReader->input[inputReader->pos] == '\n') {
        inputReader->pos++;
    }
    const char *input = inputReader->input;
    char c = input[inputReader->pos];

    Symbol symbol;
    symbol.type = INVALID;
    symbol.var = '\0';

    if (c >= 'A' && c <= 'Z') {
        symbol.type = VAR;
        symbol.var = c;
        inputReader->pos++;
        return symbol;
    }
    if (c == '*') {
        symbol.type = AND;
        inputReader->pos++;
        return symbol;
    }
    if (c == '+') {
        symbol.type = OR;
        inputReader->pos++;
        return symbol;
    }
    if (c == '(') {
        inputReader->pos++;
        c = input[inputReader->pos];
        if (c == '!') {
            inputReader->pos++;

            c = input[inputReader->pos];
            symbol.type = NEG;
            symbol.var = c;

            inputReader->pos++;
            if (input[inputReader->pos] == ')')
                inputReader->pos++;

            return symbol;
        }
        symbol.type = INVALID;
        return symbol;
    }
    if (c == '\0') {
        symbol.type = END;
        return symbol;
    }

    inputReader->pos++;
    return symbol;
}

Term* parse_term(InputReader *inputReader) {
    Term* term = calloc(1, sizeof(Term));
    while (1) {
        Symbol symbol = get_next_symbol(inputReader);

        if (symbol.type == VAR) {
            term->vars[term->var_count] = symbol.var;
            term->neg[term->var_count] = false;
            term->var_count++;
        }
        else if (symbol.type == NEG) {
            term->vars[term->var_count] = symbol.var;
            term->neg[term->var_count] = true;
            term->var_count++;
        }
        else if (symbol.type != AND){      //end of the term
            inputReader->pos--;
            break;
        }
    }

    if (term->var_count == 0) {
        free(term);
        return NULL;
    }

    return term;
}

Expression* parse_expression(const char *input) {
    Expression *expression = malloc(sizeof(Expression));
    expression->terms = NULL;

    InputReader inputReader;
    inputReader.input = input;
    inputReader.pos = 0;

    Term* term = parse_term(&inputReader);   //get first term
    if (!term) {
        expression->terms = NULL;
        return expression;
    }

    Term* head = term;
    Term* tail = term;

    while(1) {
        Symbol symbol = get_next_symbol(&inputReader);
        if (symbol.type == OR) {
            Term* newTerm = parse_term(&inputReader);
            if (newTerm) {
                tail->next = newTerm;    //link with other terms
                tail = tail->next;
            }
        } else break;
    }
    expression->terms = head;
    return expression;
}

Node* find_or_create_node(HashTable* table, int level, Node* low, Node* high, BDD *bdd) {
    if (low == high) return high;

    unsigned int idx = hash_func(level, low, high, table->size);
    while (1) {
        if (table->elems[idx] == NULL) {
            HashTableElem *new_elem = malloc(sizeof(HashTableElem));
            new_elem->level = level;
            new_elem->low = low;
            new_elem->high = high;
            new_elem->node = (Node*)malloc(sizeof(Node));
            new_elem->node->level = level;
            new_elem->node->low = low;
            new_elem->node->high = high;
            new_elem->node->value = -1;
            table->elems[idx] = new_elem;
            bdd->node_count++;
            return new_elem->node;
        }
        if (table->elems[idx]->level == level && table->elems[idx]->low == low &&
                   table->elems[idx]->high == high)
            return table->elems[idx]->node;

        idx = (idx + 1) % table->size;
    }
}

Node* BDD_build(HashTable* table, const Expression* expression, int level, const char* poradie, BDD* bdd) {
    if (level == bdd->var_count) {
        if (expression->terms == NULL) {
            if (!bdd->terminal_node_zero) {
                bdd->terminal_node_zero = malloc(sizeof(Node));
                bdd->terminal_node_zero->value = 0;
                bdd->terminal_node_zero->low = NULL;
                bdd->terminal_node_zero->high = NULL;
                bdd->terminal_node_zero->level = -1;
                bdd->node_count++;
            }
            return bdd->terminal_node_zero;
        }
        if (!bdd->terminal_node_one) {
            bdd->terminal_node_one = malloc(sizeof(Node));
            bdd->terminal_node_one->value = 1;
            bdd->terminal_node_one->low = NULL;
            bdd->terminal_node_one->high = NULL;
            bdd->terminal_node_one->level = -1;
            bdd->node_count++;
        }
        return bdd->terminal_node_one;
    }

    char currentVar = poradie[level];
    Expression *lowExpr = malloc(sizeof(Expression));
    Expression *highExpr = malloc(sizeof(Expression));
    lowExpr->terms = NULL;
    highExpr->terms = NULL;

    Term* lowTail = NULL;
    Term* highTail = NULL;

    for (Term* term = expression->terms; term != NULL; term = term->next) {
        int positiveVar = false;
        int negativeVar = false;
        int varIndex = -1;

        for (int i = 0; i < term->var_count; i++)
            if (term->vars[i] == currentVar) {
                varIndex = i;
                if (term->neg[i]) negativeVar = true;
                else positiveVar = true;
                break;
            }

        if (positiveVar) {        //var = 1
            Term* highTerm = malloc(sizeof(Term));
            highTerm->var_count = term->var_count - 1;
            highTerm->next = NULL;

            int idx=0;        //excluding currentVar and shifting other variables
            for(int i=0; i<term->var_count; i++)
                if(i != varIndex) {
                    highTerm->vars[idx] = term->vars[i];
                    highTerm->neg[idx] = term->neg[i];
                    idx++;
                }

            if(!highExpr->terms) {           //if list is empty highTerm is first
                highExpr->terms = highTerm;
                highTail = highTerm;
            } else {
                highTail->next = highTerm;    //else highTerm is last
                highTail = highTerm;
            }
        } else if (negativeVar) {          //var = 0
            Term* lowTerm = malloc(sizeof(Term));
            lowTerm->var_count = term->var_count - 1;
            lowTerm->next = NULL;

            int idx=0;           //excluding currentVar and shifting other variables
            for(int i=0; i<term->var_count; i++)
                if(i != varIndex) {
                    lowTerm->vars[idx] = term->vars[i];
                    lowTerm->neg[idx] = term->neg[i];
                    idx++;
                }

            if (!lowExpr->terms) {            //if list is empty lowTerm is first
                lowExpr->terms = lowTerm;
                lowTail = lowTerm;
            } else {
                lowTail->next = lowTerm;      //else lowTerm is last
                lowTail = lowTerm;
            }
        } else {
                   //if var wasn't found in the term
            Term* lowTerm = malloc(sizeof(Term));
            for (int i = 0; i < term->var_count; i++) {
                lowTerm->vars[i] = term->vars[i];
                lowTerm->neg[i] = term->neg[i];
            }
            lowTerm->var_count = term->var_count;
            lowTerm->next = NULL;
            if (!lowExpr->terms) {        //if list is empty lowTerm is first
                lowExpr->terms = lowTerm;
                lowTail = lowTerm;
            } else {                       //else lowTerm is last
                lowTail->next = lowTerm;
                lowTail = lowTerm;
            }

            Term* highTerm = malloc(sizeof(Term));
            highTerm->var_count = term->var_count;
            for (int i = 0; i < term->var_count; i++) {
                highTerm->vars[i] = term->vars[i];
                highTerm->neg[i] = term->neg[i];
            }
            highTerm->next = NULL;
            if (!highExpr->terms) {       //if is empty highTerm is first
                highExpr->terms = highTerm;
                highTail = highTerm;
            } else {                         //else highTerm is last
                highTail->next = highTerm;
                highTail = highTerm;
            }
        }
    }

    Node* lowChild = BDD_build(table, lowExpr, level+1, poradie, bdd);
    Node* highChild = BDD_build(table, highExpr, level+1, poradie, bdd);

    while(lowExpr->terms != NULL) {
        Term* tmp = lowExpr->terms;
        lowExpr->terms = lowExpr->terms->next;
        free(tmp);
    }
    while(highExpr->terms != NULL) {
        Term* tmp = highExpr->terms;
        highExpr->terms = highExpr->terms->next;
        free(tmp);
    }
    free(lowExpr);
    free(highExpr);

    return find_or_create_node(table, level, lowChild, highChild, bdd);
}

BDD* BDD_create(const char *bfunkcia, const char *poradie) {
    BDD *bdd = malloc(sizeof(BDD));
    bdd->var_count = (int)strlen(poradie);
    bdd->root = NULL;
    bdd->node_count = 0;
    bdd->terminal_node_zero = NULL;
    bdd->terminal_node_one = NULL;

    Expression* expression = parse_expression(bfunkcia);

    HashTable* table = create_table(512);

    bdd->root = BDD_build(table, expression, 0, poradie, bdd);

    free_expression(expression);
    return bdd;
}

char BDD_use(BDD* bdd, const char* vstupy) {
    if (!bdd) return -1;
    if (bdd->var_count != strlen(vstupy)) return -1;

    Node *node = bdd->root;
    while (node->value != 0 && node->value != 1) {
        int level = node->level;

        if (vstupy[level] == '0') node = node->low;      //if var is 0 going left
        else if (vstupy[level] == '1') node = node->high;  //if var is 1 going right
        else return -1;

        if (!node) return -1;
    }

    return node->value ? '1' : '0';
}

void free_node(Node *node) {
    if (!node) return;

    if (node->low && node->low->value != 0 && node->low->value != 1) free_node(node->low);
    if (node->high && node->high->value != 0 && node->high->value != 1) free_node(node->high);
}

void BDD_free(BDD *bdd) {
    if (!bdd) return;
    free_node(bdd->root);

    if (bdd->terminal_node_zero) {
        free(bdd->terminal_node_zero);
        bdd->terminal_node_zero = NULL;
    }
    if (bdd->terminal_node_one) {
        free(bdd->terminal_node_one);
        bdd->terminal_node_one = NULL;
    }
    free(bdd);
}

void shift_order(char *str) {
    int len = strlen(str);
    char first = str[0];
    for (int i = 0; i < len - 1; i++)
        str[i] = str[i + 1];
    str[len - 1] = first;
}

char* get_order(Term *term, int *idx) {
    char *order = malloc(64 * sizeof(char));
    while (term) {
        for (int i = 0; i < term->var_count; i++) {
            char var = term->vars[i];
            int found = false;
            for (int j = 0; j < *idx; j++) {
                if (order[j] == var) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                order[*idx] = var;
                (*idx)++;
            }
        }
        term = term->next;
    }
    order[*idx] = '\0';
    return order;
}

BDD* BDD_create_with_best_order(const char *bfunkcia) {
    Expression* expression = parse_expression(bfunkcia);
    Term* term = expression->terms;
    if (!term)
        return BDD_create(bfunkcia, "");

    int idx = 0;
    char* order = get_order(term, &idx);

    BDD *best_BDD = NULL;
    int best_count = -1;

    for (int i = 0; i < idx; i++) {
        BDD *new_bdd = BDD_create(bfunkcia, order);
        if (best_BDD == NULL || new_bdd->node_count < best_count) {
            if (best_BDD != NULL) BDD_free(best_BDD);

            best_BDD = new_bdd;
            best_count = new_bdd->node_count;
        }
        else BDD_free(new_bdd);

        shift_order(order);
    }
    free_expression(expression);
    free(order);
    return best_BDD;
}

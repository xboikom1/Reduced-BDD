#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "BDD_functions.c"

char* generate_expression(int var_count, int terms_count) {
    if (var_count <= 0 || terms_count <= 0) return NULL;

    char* expr = malloc(2048 * sizeof(char));
    expr[0] = '\0';

    char* vars = malloc(var_count * sizeof(char));
    for (int i = 0; i < var_count; i++)
        vars[i] = 'A' + i % 26;

    for (int i = 0; i < terms_count; i++) {
        int varsInTerm = 1 + rand() % var_count;
        int* used = calloc(var_count, sizeof(int));
        char term[1024];
        term[0] = '\0';

        for (int j = 0; j < varsInTerm; j++) {
            int index;
            do {
                index = rand() % var_count;
            } while (used[index]);   //selecting until finds an index that was not already used
            used[index] = 1;

            int neg = rand() % 2 == 0;
            char varExpr[8];
            if (neg)
                sprintf(varExpr, "(!%c)", vars[index]);    //negative var
            else
                sprintf(varExpr, "%c", vars[index]);     //positive var

            strcat(term, varExpr);
            if (j < varsInTerm - 1)
                strcat(term, "*");
        }
        free(used);

        strcat(expr, term);
        if (i < terms_count - 1)
            strcat(expr, " + ");
    }

    free(vars);
    return expr;
}

char find_correct_result(const char* bfunkcia, const char *order, const char* input) {
    if (strlen(order) != strlen(input)) return -1;
    Expression* expr = parse_expression(bfunkcia);

    if (!expr || !expr->terms) return -1;

    for (Term *term = expr->terms; term != NULL; term = term->next) {   //check all terms in the expression
        int term_value = 1;

        for (int i = 0; i < term->var_count; i++) {  //check all vars in the term
            char var = term->vars[i];
            int index = -1;

            for (int j = 0; order[j] != '\0'; j++)    //find var index in the order
                if (order[j] == var) {
                    index = j;
                    break;
                }

            int value = input[index] == '1';

            if (term->neg[i])   //if var is negative
                value = !value;

            if (value == 0) {   //term is false, going to the next term
                term_value = 0;
                break;
            }
        }
         //if one term is true, expression is true
        if (term_value)
            return '1';
    }
    return '0';
}

void verify_values(BDD *bdd, const char* bfunkcia, const char* order) {
    int varCount = strlen(order);
    int total = 1 << varCount;    //2^varCount

    char *input = malloc((varCount + 1) * sizeof(char));
    input[varCount] = '\0';
    int errors = 0;
    for (int i = 0; i < total; i++) {     //generating every i in binary system
        for (int j = 1; j <= varCount; j++) {
            int bit = (i >> varCount - j) & 1;   //always gets the actual bit and finds its value
            input[j] = bit ? '1' : '0';
        }

        char bdd_result = BDD_use(bdd, input);
        char correct_result = find_correct_result(bfunkcia, order, input);

        if (bdd_result != correct_result) {
            printf("Error, for input %s result should be %c.\n", input, correct_result);
            errors++;
        }
    }
    if (errors == 0) printf("All values results matches\n");
    free(input);
}

long get_current_time_in_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000L + tv.tv_usec / 1000L;
}

int main() {
    srand(time(NULL));
    long start = get_current_time_in_ms();
    int var_count = 23;
    char* bfunkcia = generate_expression(var_count, 20);
    printf("%s\n", bfunkcia);

    Expression* expression = parse_expression(bfunkcia);
    Term* term = expression->terms;
    int idx = 0;
    char* order = get_order(term, &idx);
    printf("Order: %s\n", order);

    BDD *bdd = BDD_create(bfunkcia, order);
    printf("BDD_create: Node count = %d\n", bdd->node_count);

    BDD *bdd_with_best_order = BDD_create_with_best_order(bfunkcia);
    printf("BDD_create_with_best_order: Node count = %d\n", bdd_with_best_order->node_count);

    verify_values(bdd, bfunkcia, order);

    int all_bdd_nodes = (1 << strlen(order) + 1) - 1;   //2^(n+1) - 1   (sum of geometric progression)
    printf("Nodes in full diagram: %d\n", all_bdd_nodes);

    double miera_zredukovania = (all_bdd_nodes - bdd->node_count) / (double) all_bdd_nodes * 100;
    printf("BDD reduction from full diagram: %.2f%% \n", miera_zredukovania);

    double ordered_bdd_miera_zredukovania = (bdd->node_count - bdd_with_best_order->node_count) / (double) bdd->node_count * 100;
    printf("Reduction with best order from default reduced bdd: %.2f%%\n", ordered_bdd_miera_zredukovania);

    long end = get_current_time_in_ms();
    printf("All tests completed in %.6lf seconds (for %d variables)\n",  (double)(end - start) / 1000.0, var_count);

    free(order);
    free(bfunkcia);
    return 0;
}

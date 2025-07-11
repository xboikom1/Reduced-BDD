#include "BDD_functions.c"

int main() {
    char* bfunkcia = "AB + AC + BC";
    char* poradie = "ABC";

    BDD *bdd = BDD_create(bfunkcia, poradie);
    printf("BDD_create: Node count = %d\n", bdd->node_count);
    BDD *bdd_with_best_order = BDD_create_with_best_order(bfunkcia);
    printf("BDD_create_with_best_order: Node count = %d\n", bdd_with_best_order->node_count);

    BDD_free(bdd);
    BDD_free(bdd_with_best_order);
    return 0;
}
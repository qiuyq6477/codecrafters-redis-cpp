#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    Dictionary *dict = create_dictionary();

    insert(dict, "name", "Alice");
    insert(dict, "age", "30");
    insert(dict, "city", "New York");

    printf("Name: %s\n", search(dict, "name"));
    printf("Age: %s\n", search(dict, "age"));
    printf("City: %s\n", search(dict, "city"));

    delete_key(dict, "age");
    printf("Age after delete: %s\n", search(dict, "age"));

    destroy_dictionary(dict);
    return 0;
}

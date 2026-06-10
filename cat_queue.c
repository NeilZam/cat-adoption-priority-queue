#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define INPUT_FILE "adopt.txt"
/* ---------- Global Modes ---------- */
typedef enum {
    MODE_ADOPTION = 0,      /* Max-heap by adoption_key (higher is better)   */
    MODE_TRIAGE   = 1       /* Min-heap by triage_key   (lower is more urgent)*/
} Mode;

/* ---------- Core Cat Record ---------- */
typedef struct Cat {
    char *name; 		    /* dynamically allocated, unique string (<= 25 chars + '\0') */
    char *breed; 		    /* dynamically allocated string */
    int age; 		        /* years */
    int friendliness; 	    /* 0..100 */
    int health;       	    /* 0..100      (higher means healthier) */
    int arrival_id;   	    /* strictly increasing when ADDed */
    int quarantine;   	    /* 0 or 1 (1 => cannot be adopted; S1) */
    double key;         	/* cached priority value for the **active** mode */
} Cat;

/* ---------- Array-Based Binary Heap of Cat* ---------- */
typedef struct {
    Cat **arr;              /* array of Cat* implementing the heap */
    int size;         	    /* current number of elements */
    int capacity;     	    /* allocated capacity */
    Mode mode;         	    /* controls comparator semantics */
} CatHeap;

/* ---------- Global Shelter State ---------- */
typedef struct {
    Mode mode;		        /* MODE_ADOPTION or MODE_TRIAGE */
    char *featured_breed;	/* NULL => none; else dynamically allocated breed string */
    double alpha;		    /* multiplier for featured breed (>= 1.0; default 1.0)*/
    int next_arrival_id;    /* increments on each ADD */
    CatHeap heap;		    /* priority structure */
} Shelter;



// Function Prototypes
void myMain(FILE *ifile);

int find_cat_index(const CatHeap *heap, const char *name);
double compute_adoption_key(const Cat *c, const Shelter *S);
double compute_triage_key(const Cat *c);
void recompute_all_keys_and_build(Shelter *S);
void cmd_add(Shelter *S, const char *name, const char *breed, int age, int friendl, int health);
void cmd_update(Shelter *S, const char *name, const char *field, int new_value);
void cmd_remove(Shelter *S, const char *name);
void cmd_peek(const Shelter *S);
void cmd_serve(Shelter *S);
void cmd_mode(Shelter *S, const char *mode_str);
void cmd_featured(Shelter *S, const char *breed, double alpha);
void cmd_print(const Shelter *S, int k);

// You may add more functions if necessary
char *copy_string(const char *s);
const char *mode_name(Mode mode);
void heap_init(CatHeap *heap, Mode mode);
void heap_destroy(CatHeap *heap);
int heap_left_index(CatHeap *heap, int idx);
int heap_right_index(CatHeap *heap, int idx);
int heap_parent_index(CatHeap *heap, int idx);
void heap_swap(Cat **a, Cat **b);
int higher_priority(CatHeap *heap, Cat *a, Cat *b);
void heap_heapify(CatHeap *heap, int idx);
void heap_heapify_up(CatHeap *heap, int idx);
void heap_build_pa6(CatHeap *heap);
void heap_insert(CatHeap *heap, Cat *cat);
Cat *heap_remove_root(CatHeap *heap);
void restore_position(CatHeap *heap, int idx);
void free_cat(Cat *cat);
void free_shelter(Shelter *S);

// BEGIN: DO NOT MODIFY THE MAIN FUNCTION
#ifndef MAIN_FUNCTION
int main(void) {
    // Open the input file for reading.
    // This is the only part of the entire code where the file
    // is going to be opened. You should not have any
    // fopen() function call in your functions. Simply
    // pass this existing FILE pointer when necessary.
    FILE *ifile = fopen(INPUT_FILE, "r");

    if( ifile == NULL ) {
        printf("File Does Not Exist!\n");
        return 1;
    }

    // Calls your own main function and passes the file stream
    myMain(ifile);

    // Close the file
    fclose(ifile);

    return 0;
}
#endif
// END: DO NOT MODIFY THE MAIN FUNCTION





// Function Definitions
void myMain(FILE *ifile) {
    // TODO: Complete this function
    // TODO BEGIN
    Shelter S;
    int q;
    char cmd[30];

    S.mode = MODE_ADOPTION;
    S.featured_breed = NULL;
    S.alpha = 1.0;
    S.next_arrival_id = 1;
    heap_init(&S.heap, MODE_ADOPTION);

    fscanf(ifile, "%d", &q);

    for (int i = 0; i < q; i++) {
        fscanf(ifile, "%s", cmd);

        if (strcmp(cmd, "ADD") == 0) {
            char name[30], breed[30];
            int age, friendl, health;

            fscanf(ifile, "%s %s %d %d %d", name, breed, &age, &friendl, &health);
            cmd_add(&S, name, breed, age, friendl, health);
        }
        else if (strcmp(cmd, "UPDATE") == 0) {
            char name[30], field[30];
            int value;

            fscanf(ifile, "%s %s %d", name, field, &value);
            cmd_update(&S, name, field, value);
        }
        else if (strcmp(cmd, "REMOVE") == 0) {
            char name[30];

            fscanf(ifile, "%s", name);
            cmd_remove(&S, name);
        }
        else if (strcmp(cmd, "PEEK") == 0) {
            cmd_peek(&S);
        }
        else if (strcmp(cmd, "SERVE") == 0) {
            cmd_serve(&S);
        }
        else if (strcmp(cmd, "MODE") == 0) {
            char mode_str[30];

            fscanf(ifile, "%s", mode_str);
            cmd_mode(&S, mode_str);
        }
        else if (strcmp(cmd, "FEATURED") == 0) {
            char breed[30];
            double alpha;

            fscanf(ifile, "%s %lf", breed, &alpha);
            cmd_featured(&S, breed, alpha);
        }
        else if (strcmp(cmd, "PRINT") == 0) {
            int k;

            fscanf(ifile, "%d", &k);
            cmd_print(&S, k);
        }
        else if (strcmp(cmd, "QUIT") == 0) {
            break;
        }
    }

    free_shelter(&S);
}
    // TODO END
char *copy_string(const char *s) {
    char *copy = malloc(strlen(s) + 1);

    if (copy != NULL)
        strcpy(copy, s);

    return copy;
}
const char *mode_name(Mode mode) {
    if (mode == MODE_ADOPTION)
        return "ADOPTION";

    return "TRIAGE";
}
void heap_init(CatHeap *heap, Mode mode) {
    heap->capacity = 10;
    heap->size = 0;
    heap->mode = mode;
    heap->arr = malloc(sizeof(Cat *) * heap->capacity);
}
void heap_destroy(CatHeap *heap) {
    free(heap->arr);
}
int heap_left_index(CatHeap *heap, int idx) {
    int val = idx * 2 + 1;

    if (val >= heap->size)
        return -1;

    return val;
}
int heap_right_index(CatHeap *heap, int idx) {
    int val = idx * 2 + 2;

    if (val >= heap->size)
        return -1;

    return val;
}
int heap_parent_index(CatHeap *heap, int idx) {
    if (idx == 0)
        return -1;

    return (idx - 1) / 2;
}
void heap_swap(Cat **a, Cat **b) {
    Cat *tmp = *a;
    *a = *b;
    *b = tmp;
}
int higher_priority(CatHeap *heap, Cat *a, Cat *b) {
    int cmp;

    if (heap->mode == MODE_ADOPTION) {
        if (a->key > b->key)
            return 1;
        if (a->key < b->key)
            return 0;
    }
    else {
        if (a->key < b->key)
            return 1;
        if (a->key > b->key)
            return 0;
    }

    cmp = strcmp(a->name, b->name);

    if (cmp < 0)
        return 1;

    if (cmp > 0)
        return 0;

    return a->arrival_id < b->arrival_id;
}
void heap_heapify(CatHeap *heap, int idx) {
    int left_idx = heap_left_index(heap, idx);
    int right_idx = heap_right_index(heap, idx);
    int best_idx = idx;

    if (left_idx != -1 && higher_priority(heap, heap->arr[left_idx], heap->arr[best_idx]))
        best_idx = left_idx;

    if (right_idx != -1 && higher_priority(heap, heap->arr[right_idx], heap->arr[best_idx]))
        best_idx = right_idx;

    if (best_idx != idx) {
        heap_swap(&(heap->arr[idx]), &(heap->arr[best_idx]));
        heap_heapify(heap, best_idx);
    }
}
void heap_heapify_up(CatHeap *heap, int idx) {
    int parent_idx = heap_parent_index(heap, idx);

    if (parent_idx != -1 && higher_priority(heap, heap->arr[idx], heap->arr[parent_idx])) {
        heap_swap(&(heap->arr[idx]), &(heap->arr[parent_idx]));
        heap_heapify_up(heap, parent_idx);
    }
}
void heap_build_pa6(CatHeap *heap) {
    int last_internal_idx;

    if (heap->size <= 1)
        return;

    last_internal_idx = heap_parent_index(heap, heap->size - 1);

    for (int i = last_internal_idx; i >= 0; i--)
        heap_heapify(heap, i);
}
void heap_insert(CatHeap *heap, Cat *cat) {
    if (heap->size == heap->capacity) {
        heap->capacity *= 2;
        heap->arr = realloc(heap->arr, sizeof(Cat *) * heap->capacity);
    }

    heap->arr[heap->size] = cat;
    heap_heapify_up(heap, heap->size);
    heap->size++;
}
Cat *heap_remove_root(CatHeap *heap) {
    Cat *root;

    if (heap->size == 0)
        return NULL;

    root = heap->arr[0];

    heap->size--;

    if (heap->size > 0) {
        heap->arr[0] = heap->arr[heap->size];
        heap_heapify(heap, 0);
    }

    return root;
}
void restore_position(CatHeap *heap, int idx) {
    if (idx < 0 || idx >= heap->size)
        return;

    heap_heapify_up(heap, idx);
    heap_heapify(heap, idx);
}
void free_cat(Cat *cat) {
    if (cat == NULL)
        return;

    free(cat->name);
    free(cat->breed);
    free(cat);
}
void free_shelter(Shelter *S) {
    for (int i = 0; i < S->heap.size; i++)
        free_cat(S->heap.arr[i]);

    heap_destroy(&S->heap);
    free(S->featured_breed);
}
int find_cat_index(const CatHeap *heap, const char *name) {
    for (int i = 0; i < heap->size; i++) {
        if (strcmp(heap->arr[i]->name, name) == 0)
            return i;
    }

    return -1;
}
double compute_adoption_key(const Cat *c, const Shelter *S) {
    double base = 1.6 * c->friendliness + 1.1 * c->health - 0.7 * c->age;
    double mult = 1.0;

    if (S->featured_breed != NULL && strcmp(c->breed, S->featured_breed) == 0)
        mult = S->alpha;

    return base * mult + (-0.000001 * c->arrival_id);
}
double compute_triage_key(const Cat *c) {
    int age_bonus = 0;

    if (c->age > 12)
        age_bonus = c->age - 12;

    return (100 - c->health) * 2.0 + age_bonus - 0.05 * c->friendliness;
}
void recompute_all_keys_and_build(Shelter *S) {
    for (int i = 0; i < S->heap.size; i++) {
        if (S->mode == MODE_ADOPTION)
            S->heap.arr[i]->key = compute_adoption_key(S->heap.arr[i], S);
        else
            S->heap.arr[i]->key = compute_triage_key(S->heap.arr[i]);
    }

    S->heap.mode = S->mode;
    heap_build_pa6(&S->heap);
}
void cmd_add(Shelter *S, const char *name, const char *breed, int age, int friendl, int health) {
    Cat *cat;

    if (find_cat_index(&S->heap, name) != -1) {
        printf("Name %s already exists.\n", name);
        return;
    }

    cat = malloc(sizeof(Cat));

    cat->name = copy_string(name);
    cat->breed = copy_string(breed);
    cat->age = age;
    cat->friendliness = friendl;
    cat->health = health;
    cat->arrival_id = S->next_arrival_id;
    cat->quarantine = 0;

    if (S->mode == MODE_ADOPTION)
        cat->key = compute_adoption_key(cat, S);
    else
        cat->key = compute_triage_key(cat);

    S->next_arrival_id++;

    heap_insert(&S->heap, cat);

    printf("Added %s.\n", name);
}
void cmd_update(Shelter *S, const char *name, const char *field, int new_value) {
    int idx = find_cat_index(&S->heap, name);
    Cat *cat;

    if (idx == -1) {
        printf("Cat %s not found.\n", name);
        return;
    }

    cat = S->heap.arr[idx];

    if (strcmp(field, "AGE") == 0) {
        cat->age = new_value;

        if (S->mode == MODE_ADOPTION)
            cat->key = compute_adoption_key(cat, S);
        else
            cat->key = compute_triage_key(cat);

        restore_position(&S->heap, idx);
        printf("Updated %s: AGE=%d. Priority adjusted.\n", name, new_value);
    }
    else if (strcmp(field, "FRIEND") == 0) {
        cat->friendliness = new_value;

        if (S->mode == MODE_ADOPTION)
            cat->key = compute_adoption_key(cat, S);
        else
            cat->key = compute_triage_key(cat);

        restore_position(&S->heap, idx);
        printf("Updated %s: FRIEND=%d. Priority adjusted.\n", name, new_value);
    }
    else if (strcmp(field, "HEALTH") == 0) {
        cat->health = new_value;

        if (S->mode == MODE_ADOPTION)
            cat->key = compute_adoption_key(cat, S);
        else
            cat->key = compute_triage_key(cat);

        restore_position(&S->heap, idx);
        printf("Updated %s: HEALTH=%d. Priority adjusted.\n", name, new_value);
    }
    else if (strcmp(field, "QUARANTINE") == 0) {
        cat->quarantine = new_value;
        restore_position(&S->heap, idx);
        printf("Updated %s: QUARANTINE=%d.\n", name, new_value);
    }
    else {
        printf("Unknown field %s.\n", field);
    }
}

void cmd_remove(Shelter *S, const char *name) {
    int idx = find_cat_index(&S->heap, name);
    Cat *removed;

    if (idx == -1) {
        printf("Cat %s not found.\n", name);
        return;
    }

    removed = S->heap.arr[idx];

    S->heap.size--;

    if (idx < S->heap.size) {
        S->heap.arr[idx] = S->heap.arr[S->heap.size];
        restore_position(&S->heap, idx);
    }

    free_cat(removed);

    printf("Removed %s.\n", name);
}
void cmd_peek(const Shelter *S) {
    Cat *c;

    if (S->heap.size == 0) {
        printf("No cats available.\n");
        return;
    }

    c = S->heap.arr[0];

    printf("Top[%s]: [%s] (key=%.2f, name=%s, breed=%s, age=%d, friend=%d, health=%d)\n",
           mode_name(S->mode),
           mode_name(S->mode),
           c->key,
           c->name,
           c->breed,
           c->age,
           c->friendliness,
           c->health);
}
void cmd_serve(Shelter *S) {
    Cat *served;

    if (S->heap.size == 0) {
        printf("No cats available.\n");
        return;
    }

    if (S->mode == MODE_TRIAGE) {
        served = heap_remove_root(&S->heap);

        printf("Serve now: %s (key=%.2f, name=%s, breed=%s, age=%d, friend=%d, health=%d)\n",
               served->name,
               served->key,
               served->name,
               served->breed,
               served->age,
               served->friendliness,
               served->health);

        free_cat(served);
    }
    else {
        Cat **skipped = malloc(sizeof(Cat *) * S->heap.size);
        int skipped_count = 0;

        served = heap_remove_root(&S->heap);

        while (served != NULL && served->quarantine == 1) {
            skipped[skipped_count] = served;
            skipped_count++;
            served = heap_remove_root(&S->heap);
        }

        for (int i = 0; i < skipped_count; i++)
            heap_insert(&S->heap, skipped[i]);

        free(skipped);

        if (served == NULL) {
            printf("No adoptable cats available.\n");
            return;
        }

        printf("Serve now: %s (key=%.2f, name=%s, breed=%s, age=%d, friend=%d, health=%d)\n",
               served->name,
               served->key,
               served->name,
               served->breed,
               served->age,
               served->friendliness,
               served->health);

        free_cat(served);
    }
}

void cmd_mode(Shelter *S, const char *mode_str) {
    if (strcmp(mode_str, "ADOPTION") == 0) {
        S->mode = MODE_ADOPTION;
        S->heap.mode = MODE_ADOPTION;
        recompute_all_keys_and_build(S);
        printf("Mode set to ADOPTION. Rebuilding priorities...\n");
    }
    else if (strcmp(mode_str, "TRIAGE") == 0) {
        S->mode = MODE_TRIAGE;
        S->heap.mode = MODE_TRIAGE;
        recompute_all_keys_and_build(S);
        printf("Mode set to TRIAGE. Rebuilding priorities...\n");
    }
    else {
        printf("Unknown mode %s.\n", mode_str);
    }
}
void cmd_featured(Shelter *S, const char *breed, double alpha) {
    free(S->featured_breed);
    S->featured_breed = NULL;

    if (strcmp(breed, "NONE") == 0) {
        S->alpha = 1.0;
        recompute_all_keys_and_build(S);
        printf("Featured breed cleared. Rebuilding priorities...\n");
    }
    else {
        S->featured_breed = copy_string(breed);
        S->alpha = alpha;
        recompute_all_keys_and_build(S);
        printf("Featured breed set to %s with alpha=%.2f. Rebuilding priorities...\n", breed, alpha);
    }
}
void cmd_print(const Shelter *S, int k) {
    CatHeap temp;
    int limit;

    if (S->heap.size == 0) {
        printf("No cats available.\n");
        return;
    }

    temp.size = S->heap.size;
    temp.capacity = S->heap.size;
    temp.mode = S->heap.mode;
    temp.arr = malloc(sizeof(Cat *) * temp.capacity);

    for (int i = 0; i < S->heap.size; i++)
        temp.arr[i] = S->heap.arr[i];

    heap_build_pa6(&temp);

    limit = k;

    if (limit > temp.size)
        limit = temp.size;

    for (int i = 1; i <= limit; i++) {
        Cat *c = heap_remove_root(&temp);
        printf("[%d] %s (key=%.2f, %s)\n", i, c->name, c->key, mode_name(S->mode));
    }

    free(temp.arr);
}
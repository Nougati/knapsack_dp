#ifndef BRANCH_AND_BOUND_H
#define BRANCH_AND_BOUND_H
#include <stdio.h>
/* Preprocessor definitions */
#define SIMPLE_SUM 2
#define BINARY_SOL 1
#define VARIABLE_ON 2
#define VARIABLE_UNCONSTRAINED 1
#define VARIABLE_OFF 0
#define LINEAR_ENUM_BRANCHING 0
#define RANDOM_BRANCHING 1
#define TRUNCATION_BRANCHING 2
#define TRUE 1
#define FALSE 0
#define NO_LOGGING 0
#define PARTIAL_LOGGING 1
#define FULL_LOGGING 2
#define FILE_LOGGING 3
#define MEMORY_EXCEEDED 1
#define TIMEOUT 2
#define NODE_OVERFLOW 1500000
#define LCHILD(x) 2 * x + 1
#define RCHILD(x) 2 * x + 2
#define PARENT(x) (x - 1) / 2

//#define GLOBAL_MEMORY_LIMIT 5368709120

/* Structure Declarations */
typedef struct p_instance
{
  int *variable_statuses;
  int parent_upper_bound;
  int parent_ID;
  int lower_bound;
  int upper_bound;
  int ID;
  struct p_instance *on_child;
  struct p_instance *off_child;
} Problem_Instance;

typedef struct queue 
{
  int front, rear, size, capacity;
  struct p_instance** array;
} Problem_Queue;

typedef struct min_heap {
    int size ;
    Problem_Instance *elem ;
} Min_Heap ;



/*LL shit*/
typedef struct linked_list_item
{
  struct linked_list_item *in_front;
  struct linked_list_item *behind;
  struct p_instance *problem;
} LL_Node_Queue_Item;

typedef struct linked_list_p_queue
{
  struct linked_list_item *head;
  struct linked_list_item *tail;
  int size;
} LL_Problem_Queue;

/* PQ shit */
void heapify(Min_Heap *hp, int i);
void heap_swap(Problem_Instance *n1, Problem_Instance *n2);
Min_Heap PQ_initialise_min_heap(int size);
void PQ_enqueue(Min_Heap *hp, Problem_Instance nd,  
              FILE *logging_stream, int logging_rule, int *node_limit_flag);
Problem_Instance *PQ_pop_node(Min_Heap *heap);
void heapify(Min_Heap *hp, int i);
void heap_swap(Problem_Instance *n1, Problem_Instance *n2);



/* Branch and Bound Declarations */
Problem_Instance *define_root_node(int n);

void branch_and_bound_bin_knapsack(int profits[], int weights[], int x[],
                                   int capacity, const long z, long *z_out, 
                                   int sol_out[], int n, char *problem_file, 
                                   int branching_strategy, time_t seed,
                                   int DP_method, int logging_rule, 
                                   FILE *logging_stream, double epsilon, 
                                   int *number_of_nodes, 
                                   long long int memory_allocation_limit,
                                   clock_t *start_time, int timeout,
                                   const int dualbound_type,
                                   int *root_dual_bound);

int find_heuristic_initial_GLB(int profits[], int weights[], int x[], const long z, 
                               int n, int capacity, char *problem_file,
                               int DP_method, const int dualbound_type,
                               const long long int memory_allocation_limit, const int timeout,
                               clock_t *start_time);

int find_branching_variable(int n, const long z, int *read_only_variables, 
                            int branching_strategy, int *profits);

/*
void generate_and_enqueue_nodes(Problem_Instance *parent, int n,
                          int branching_variable, 
                          LL_Problem_Queue *problems_list, int *count, 
                          FILE *logging_stream, int logging_rule,
                          int *node_limit_flag);
*/
void generate_and_enqueue_nodes(Problem_Instance *parent, int n,
                          int branching_variable, 
                          Min_Heap *problems_list, int *count, 
                          FILE *logging_stream, int logging_rule,
                          int *node_limit_flag);

Problem_Instance *select_and_dequeue_node(LL_Problem_Queue *node_queue);

void find_bounds(Problem_Instance *current_node, int profits[], int weights[],
                 int x[], int capacity, int n, const long z, int *lower_bound_ptr, 
                 int *upper_bound_ptr, char *problem_file, int DP_method,
                 int logging_rule, FILE *logging_stream, double eps,
                 const int dualbound_type, const long long int memory_allocation_limit, 
                 const int timeout, clock_t *start_time, int *LP_brancher);

void post_order_tree_clean(Problem_Instance *root_node);

int is_boundary_exceeded(long long int memory_limit, clock_t start_time, int timeout);


/* Queue Declarations */
Problem_Queue *create_queue(int capacity);

int is_full(Problem_Queue *queue);

int is_empty(Problem_Queue *queue);

void enqueue(Problem_Queue *queue, Problem_Instance *node);

Problem_Instance *dequeue(Problem_Queue *queue);

Problem_Instance *front(Problem_Queue *queue);

Problem_Instance *rear(Problem_Queue *queue);


/* LL Queue Declarations */
LL_Problem_Queue *LL_create_queue(void);

void LL_enqueue(LL_Problem_Queue *queue, Problem_Instance *problem, 
                FILE *logging_stream, int logging_rule, int *node_limit_flag);

Problem_Instance *LL_dequeue(LL_Problem_Queue *queue);

#endif

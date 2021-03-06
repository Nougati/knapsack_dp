/**fptas.c*********************************************************************
 *                                                                            *
 *  Really big epsilons:                                                      *
 *    Basically for epsilon >= 1, the lower bound guarantee relative to OPT   *
 *    goes to 0. P lower bounds OPT, but P decreases in the truncated profits,*
 *    so its guarantees decrease too.                                         *
 *                                                                            *
 ******************************************************************************/

#include <math.h>
#include <assert.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fptas.h"
#include "bench_extern.h"
#include "pisinger_reader.h"
#include "branch_and_bound.h"

/* lowerbound shiz */
void get_a_lower_bound(const int *profits, const int *weights, int n, int capacity,
                             int *lower_bound, double *upper_bound);
int partition_hey(double arr1[], double arr2[], int l, int h);
void swap_hey( double* a, double* b );
void quick_sort_parallel_lists_hey(double *list1, double *list2, int lo, int hi);

/* Iterative merge sort start */
void remove_linked_list(struct solution_pair** head_reference);
void copy_linked_list(struct solution_pair* old_head, 
                      struct solution_pair** new_head,
                      const int n, 
                      const long long int memory_allocation_limit);
void copy_solution_pair(struct solution_pair* old_pair,
                        struct solution_pair* new_pair, const int n);
int williamson_shmoys_DP_amended(struct problem_item items[], int capacity, int n,
                                int *solution_array, const long long int memory_allocation_limit,
                                const int timeout, clock_t *start_time);
void remove_from_linked_list(struct solution_pair* to_be_removed);
int dominates(struct solution_pair* A, struct solution_pair* B);
void insert_after(struct solution_pair* reference_pair, int new_weight, int new_profit,
                  int n, const long long int memory_allocation_limit);
int min(int x, int y);
void iterative_merge_sort(struct solution_pair** head_ref, int list_length, 
                          int n);
void merge(struct solution_pair** head_ref, int left_index, int mid_index, 
           int right_index, int n, int list_length);
int min(int x, int y) { return (x<y) ? x : y; }
void linked_list_insertion_sort(struct solution_pair **head_ref);

/* Preprocessor definitions */

/* That generic thingo */
#define typename(x) _Generic((x),        /* Get the name of a type */             \
                                                                                  \
        _Bool: "_Bool",                  unsigned char: "unsigned char",          \
         char: "char",                     signed char: "signed char",            \
    short int: "short int",         unsigned short int: "unsigned short int",     \
          int: "int",                     unsigned int: "unsigned int",           \
     long int: "long int",           unsigned long int: "unsigned long int",      \
long long int: "long long int", unsigned long long int: "unsigned long long int", \
        float: "float",                         double: "double",                 \
  long double: "long double",                   char *: "pointer to char",        \
       void *: "pointer to void",                int *: "pointer to int",         \
      default: "other")


Dynamic_Array *times_per_node;

/* FPTAS Functions */
/* FPTAS core function */
void FPTAS(double eps, int *profits, int *weights, int *x, int *sol_prime,
           const int n, int capacity, const long z, const int sol_flag,
           const int bounding_method, const char *problem_file, double *K,
           int *profits_prime, const int DP_method,
           const int *variable_statuses, const int dualbound_type, 
           const long long int memory_allocation_limit, const int timeout, 
           clock_t *start_time){
  /* Description
   *  Recreates the FPTAS for the 0,1 KP as described by V. Vasirani in his
   *   chapter on Knapsack in Approximation Algorithms.
   * Inputs
   *  eps - the epsilon value representing the level of approximation on the
   *        result. A lower epsilon value will indicate less error and thus 
   *        worse performance in terms of time. Epsilon is a positive real 
   *        number no greater than 1.
   *  profits - The input profit array
   *  weights - The input weight array
   *  x - The input problem optimal solution binary representation
   *  sol_prime - The array for the approximate solution S' to be stored. This 
   *   It is by definition the optimal solution to the KP problem with profits 
   *   described by profits_prime. For our purposes it'll stored as a bitstr.
   *  n - the number of items in the problem
   *  capacity - the capacity of the knapsack as specified by the problem
   *  sol_flag - a flag which should always be 1 (binary).
   *  bounding_method - a flag which should always be 2 (simple sum)
   *  problem_file - just the string of the problem file
   *  K - the husk where we store the K value determined by this algo.
   *
   *
   */
  int P = DP_max_profit(profits, n);

  /* Define K */
  *K = define_K(eps, P, n);

  /* Derive adjusted profits (and trivialise constrained nodes 0) */
  make_profit_primes(profits, profits_prime, *K, n, variable_statuses, dualbound_type);

  /* Symbolic profits (just for the computation) */
  int symbolic_profits_prime[n];
  for (int i = 0; i < n; i++) symbolic_profits_prime[i] = 0;
  make_symbolic_profit_primes(profits, symbolic_profits_prime, *K, n, 
                              variable_statuses, dualbound_type);

  /* Adjust capacity according to nodes constrained to be in */
  for (int i = 0; i < n; i++)
    if (variable_statuses[i] == VARIABLE_ON)
      capacity -= weights[i];
  for (int i = 0; i < n; i++) sol_prime[i] = 0;

  /* Feasibility check */
  if (capacity > 0)
  {
    /* Now with amended profits list "profit_primes," solve the DP */
    /*... With Vazirani's DP */
    if (DP_method == VASIRANI)
    {
      #ifdef BENCHMARKING
      clock_t DP_node_start_time = clock();
      #endif

      DP(symbolic_profits_prime, weights, x, sol_prime, n, capacity, z, 
         sol_flag, bounding_method, problem_file, memory_allocation_limit, 
         timeout, start_time);
      
      #ifdef BENCHMARKING
      clock_t DP_node_elapsed = clock() - DP_node_start_time;
      double DP_node_time_taken = ((double)DP_node_elapsed)/CLOCKS_PER_SEC;
      append_to_dynamic_array(times_per_node, DP_node_time_taken);
      /* If we timed out we'll look at it in B&B */
      if (bytes_allocated == -1 || *start_time == -1)
        return;
      #endif
    }

    /*... or Williamson and Shmoys. */
    else if (DP_method == WILLIAMSON_SHMOY)
    {
      /* Make problem item struct array */
      struct problem_item items_prime[n];
      for(int i = 0;  i < n; i++)
      {
        items_prime[i].weight = weights[i];
        items_prime[i].profit = symbolic_profits_prime[i];
      }
      #ifdef BENCHMARKING
      clock_t DP_node_start_time = clock();
      #endif

      //int result = williamson_shmoys_DP(items_prime, capacity, n, sol_prime, 
      //                                 memory_allocation_limit, timeout, 
      //                                  start_time);
        int result = williamson_shmoys_DP_amended(items_prime, capacity, n,
                                  sol_prime, memory_allocation_limit,
                                  timeout, start_time);

      #ifdef BENCHMARKING
      fflush(stdout);
      clock_t DP_node_elapsed = clock() - DP_node_start_time;
      double DP_node_time_taken = ((double)DP_node_elapsed)/CLOCKS_PER_SEC;
      append_to_dynamic_array(times_per_node, DP_node_time_taken);
      if (bytes_allocated == -1 || *start_time == -1)
        return;
      #endif
    }

    /* Force any variables on if necessary */
    for(int i = 0; i < n; i++) 
      if(variable_statuses[i] == VARIABLE_ON)
        sol_prime[i] = 1;
    /* Solution should be in sol_prime */
  }
  /* Make it infeasible */
  else if(capacity == 0)
  {
    for(int i = 0; i < n; i++) 
      if (variable_statuses[i] == VARIABLE_ON)
        sol_prime[i] = 1;
      else
        sol_prime[i] = 0;
  }
  else if(capacity < 0)
  {
    for(int i = 0; i < n; i++) 
      sol_prime[i] = 0;
  }
}

/* FPTAS: Define K */
float define_K(double eps, int P, int n){
  /* Description: 
   *   Defines real valued scaling factor K. K is epsP/n. As eps approaches 0,
   *   K does too. As eps approaches 1, K approaches P/n. As eps exceeds 1, K 
   *   scales progressively larger than the n-way split of P.
   * Notes:
   *  Epsilon can indeed exceed 1 as soon as epsilon hits 1 the lower bound
   *  guarantee on the profit with respect to OPT drops completely and is 
   *  bounded trivially by 0. As a result, a stronger lower bound would be P at
   *  this point, however there is no guarantee on how this adjusted P will look. 
   *  It may well be positive but with increasing epsilon values it would seem 
`  *  that P's would be crushed to a low point. 
   */
  assert(eps > 0);
  double K = (eps*P)/n;
  return K;
}

/* FPTAS: Make profit primes */
void make_profit_primes(int profits[], int profits_prime[], double K, int n,
                        const int *variable_statuses, const int dualbound_type)
 /* 
  * Description: 
  *  Derives the adjusted profits set profits', which is made by scaling every
  *   profit down by 1/K and flooring the result.
  * Inputs:
  *
  * Postconditions:
  *  
  * Notes:
  *  As eps increases, K increases, and so the profit' of each item will get scaled further
  *   and further downwards. Since P is the max, it will be scaled to 1 for eps = 1 
  *  We need dualbound_type because the dual bound we derive is in terms of S'',
  *   (i.e. the solution set to rounded up profits), rather than S'.
  */
{
  switch(dualbound_type)
  {
    /* Case: we need to round up */
    case APOSTERIORI_DUAL_ROUNDUP :
      if(K > 1)
        for(int i=0; i < n; i++)
        {
            double scaled_profit = profits[i]/K;
            profits_prime[i] = ceil(scaled_profit);
        }
      /* If K < 1, then profits would be inflated, so we simply solve exactly */
      else
        for(int i = 0; i < n; i++)
          profits_prime[i] = profits[i];
      break;
    /* For all other dual bounds, we round down */
    default :
      if(K > 1)
        for(int i=0; i < n; i++)
        {
            double scaled_profit = profits[i]/K;
            profits_prime[i] = floor(scaled_profit);
        }
      /* If K < 1, then profits would be inflated, so we simply solve exactly */
      else
        for(int i = 0; i < n; i++)
          profits_prime[i] = profits[i];
  }
}

/* FPTAS: Make symbolic profit primes function */ 
void make_symbolic_profit_primes(int profits[], int symbolic_profits_prime[], 
                                double K, int n, const int *variable_statuses, 
                                const int dualbound_type){ 
 /* Description: 
  *  Derives the adjusted profits set profits', which is made by scaling every
  *   profit down by 1/K and flooring the result, while zeroing constrained variables.
  *   The process of forcing domination is the symbolic component of this.
  * Inputs:
  *
  * Postconditions:
  *  
  * Notes:
  *  As eps increases, K increases, and so the profit' of each item will get scaled further
  *   and further downwards. Since P is the max, it will be scaled to 1 for eps = 1 
  *   
  */

  switch(dualbound_type)
  {
    /* Case: we need to round up */
    case APOSTERIORI_DUAL_ROUNDUP :
      if(K > 1)
      { 
        for(int i=0; i < n; i++)
        {
          if(variable_statuses[i] == VARIABLE_UNCONSTRAINED)
          {
            double scaled_profit = profits[i]/K;
            symbolic_profits_prime[i] = ceil(scaled_profit);
          }
          else
            symbolic_profits_prime[i] = VARIABLE_OFF; // We just make the item dominated
        }
      }
      else
      {
        for(int i = 0; i < n; i++)
        {
          if(variable_statuses[i] == VARIABLE_UNCONSTRAINED)
            symbolic_profits_prime[i] = profits[i];
          else
            symbolic_profits_prime[i] = VARIABLE_OFF;
        } 
      }
      break;
    /* All other cases: we round down */
    default :
      if(K > 1)
      { 
        for(int i=0; i < n; i++)
        {
          if(variable_statuses[i] == VARIABLE_UNCONSTRAINED)
          {
            double scaled_profit = profits[i]/K;
            symbolic_profits_prime[i] = floor(scaled_profit);
          }
          else
            symbolic_profits_prime[i] = VARIABLE_OFF; // We just make the item dominated
        }
      }
      else
      {
        for(int i = 0; i < n; i++)
        {
          if(variable_statuses[i] == VARIABLE_UNCONSTRAINED)
            symbolic_profits_prime[i] = profits[i];
          else
            symbolic_profits_prime[i] = VARIABLE_OFF;
        } 
      }
  }
}

/* General DP / FPTAS Aux: Find max profits */
int DP_max_profit(const int problem_profits[],
                  const int n){
  /*
    Description:
      Finds the highest profit in the array of item profits. In terms of Vasirani's notation
       this is P. 
    Inputs:
      problem_profits - the profit array
      n - the number of objects
    Outputs:
      max - the value of the highest profit item.
  */
  int max = 0;
  for(int i = 0; i < n; i++){
    if (max < problem_profits[i]){
      max = problem_profits[i];
    }
  }
  return max;
}


/* Vasirani DP functions */
/* Vasirani DP: General Dynamic programming algorithm */
void DP(const int problem_profits[], // profit primes?
        const int problem_weights[],
        const int x[],
        int sol[],
        const int n,
        const int capacity,
        const long z,
        const int sol_flag,
        const int bounding_method,
        const char *problem_file, //TODO check if I can remove this
        const long long int memory_allocation_limit,
        const int timeout, clock_t *start_time)
{
  /*
    Description:
      Carries out DP to compute the optimal solution for knapsack,
       given profits and weights.
    Inputs:
      problem_profits (int *) array start
      problem_weights (int *) array start
      sol (int *) array start - the output array
      n - the number of items in the array
    Postconditions:
      The array S will be totally filled out
    Notes:
      Indexing for this will be confuuusing
  */

  /* Find the max profit item and derive the profit upper bound based on it */
  //int max_profit = DP_max_profit(problem_profits, n);
  //int p_upper_bound = DP_p_upper_bound(problem_profits, n, max_profit,
  //                                     bounding_method);

  /* Hacked together lower bound getting machine */
  int lower_bound;
  double upper_bound; 
  get_a_lower_bound(problem_profits, problem_weights, n, capacity,
                    &lower_bound, &upper_bound);
  int p_upper_bound = (int) upper_bound;
  

  /* Add the amount allocated */
  bytes_allocated += (n+1)*sizeof(int *);
  if (memory_allocation_limit != -1 &&
      bytes_allocated > memory_allocation_limit)
  {
    printf("  Overallocation detected in Vazirani DP allocated! Bytes got to"
           " %lld.\n", bytes_allocated);
    bytes_allocated = -1;
    return;
  }

  /* Try to do first allocation */
  int **DP_table = (int **) malloc(sizeof(int *) * (n+1));

  if(DP_table == NULL)
  {
    /* malloc failed! */
    unsigned long long int bytes_requested = sizeof(int *) *(n+1);
    printf("  Failed malloc on DP_table! (%llu requested)\n", bytes_requested);
    bytes_allocated = -1;
    return;
  }

  /* Check that we're allowed to allocate the second one */
  bytes_allocated += sizeof(int)*p_upper_bound *(n+1);
  if (memory_allocation_limit != -1 &&
      bytes_allocated > memory_allocation_limit)
  {
    printf("  Overallocation detected in Vazirani DP allocated! Bytes got to"
           " %lld.\n", bytes_allocated);
    bytes_allocated = -1;
    free(DP_table);
    return;
  }

  /* Try to do second allocation */
  DP_table[0] = (int *) malloc(sizeof(int) * p_upper_bound * (n+1));
  if(DP_table[0] == NULL)
  {
    unsigned long long int bytes_requested = sizeof(int *) * p_upper_bound *(n+1);
    printf("  Failed malloc on DP_table[0]! (%llu requested)\n", bytes_requested);
    bytes_allocated = -1;
    return;
  }

  for(int i = 0; i < (n+1); i++)
    DP_table[i] = (*DP_table + p_upper_bound * i);

  /* Check if we've timed out, and if so, clean up and bail out*/
  if (did_timeout_occur(timeout, *start_time))
  {
    free(DP_table[0]);
    free(DP_table);
    *start_time = -1;
    return;
  }

  /* Compute base cases */
  DP_fill_in_base_cases(p_upper_bound, n+1, DP_table, problem_profits, 
                        problem_weights);

  /* Check if we've timed out, and if so, clean up and bail out*/
  if (did_timeout_occur(timeout, *start_time))
  {
    free(DP_table[0]);
    free(DP_table);
    *start_time = -1;
    return;
  }

  /* Compute general cases */
  DP_fill_in_general_cases(p_upper_bound, n+1, DP_table, problem_profits,
                           problem_weights);

  /* Check if we've timed out, and if so, clean up and bail out*/
  if (did_timeout_occur(timeout, *start_time))
  {
    free(DP_table[0]);
    free(DP_table);
    *start_time = -1;
    return;
  }

  /* Set my custom "positive infinity", find max profit, and derive the 
     solution set */
  int my_pinf = derive_pinf(problem_weights, n);
  int p = DP_find_best_solution(p_upper_bound, n+1, DP_table, capacity,
                                my_pinf);
  int n_solutions = DP_derive_solution_set(n+1, p_upper_bound, DP_table,
                                           problem_profits, sol, p,
                                           sol_flag);

  /* Clean up */
  bytes_allocated -= sizeof(DP_table[0]);
  bytes_allocated -= sizeof(DP_table);
  free(DP_table[0]);
  free(DP_table);
}

/* Vasirani DP: Derive upper bound P (table width) */
int DP_p_upper_bound(const int problem_profits[],
                     const int n,
                     const int P,
                     const int bounding_method){
  /*
    Description:
      Derives the upper bound for the DP table, based on the bounding
       method specified.
    Inputs:
      problem_profts[] - the profit array
      n - the number of objects
      P - the highest profit among every item in the set
  */
  int upper_bound;

  switch (bounding_method){
    case HYPER_TRIVIAL_BOUND: upper_bound = n*P; break;
    case TRIVIAL_BOUND: upper_bound = p_upper_bound_aux(problem_profits, n); break;
    default: printf("Invalid bounding method! Exiting...\n");
    exit(-1);
  }
  return upper_bound;
}

/* Vasirani DP: profit upper bound auxiliary function */
int p_upper_bound_aux(const int problem_profits[],
                      const int n){
  /*
    Description:
      An auxiliary function used to calculate the intelligent trivial
       upper bound on the profit.
    Inputs:
      problem_profits[] -  the profit array
      n - the number of objects
    Outputs:
      total - the sum of all the item profits
    Notes:
      There's a funny story behind this function.
  */

  int total=0;

  for(int i=0; i < n; i++){
    total += problem_profits[i];
  }
  return total;
}

/* Vasirani DP: Fill in base cases */
void DP_fill_in_base_cases(const int width,
                           const int n,
                           int ** DP_table, //int DP_table[][width],
                           const int problem_profits[],
                           const int problem_weights[]){
  /*
    Description:
      Fills in the base cases for the DP. In Vasirani's terminology this
       is every A(1,p) for every p between 0 and width.
    Inputs:
      width - the upper bound on the width of the table
      n - the number of rows. Remember this is passed in as n+1 to account
           for the 0th row of the table. As a result derive_pinf passes n-1
      DP_table - the two dimensional array to calculate the DP in
    Preconditions:
      The space for the table must be allocated
    Postconditions:
      The first row should indicate the minimum size solutions for the
      specified solution subset. Also, impossible solutions will be
       represented by infinity
    Notes:
      The rows of the table is expected to represent the number of items
       included. The columns of the table is expected to represent the
       maximum profit achievable.
      Also worth mentioning:
       A(i,p) = A[i][p]
      Another thing: I couldn't use the in built pinf because it was a double
       being stored in an int array. Instead I've defined my_pinf, which is a
       simple upper bound on the problem_weight of any solution represented by
       the sum of all the items in problem_weights + 1.
       This has led to the definition of the function derive_pinf.
  */

  int my_pinf = derive_pinf(problem_weights, n-1);
  // Go over the first column with 0's
  for(int i = 0; i < n; i++){
     DP_table[i][0] = 0;
  }

  // Go over the first row with infinities
  for(int i = 1; i < width; i++){
    DP_table[0][i] = my_pinf;
  }
}

/* Vasirani DP: General case computation */
void DP_fill_in_general_cases(const int width,
                              const int n,
                              int ** DP_table, //int DP_table[][width],
                              const int problem_profits[],
                              const int problem_weights[]){
  /*
    Description:
      Fills in the fields of the DP table which are defined by the
       general recurrence.
      Specifically fields A[1][nP] to A[n][nP] (using Vasirani's definition)
    Inputs:
      width - the upper bound on p, the max allowable profit for the given
               subproblem.
      n - the number of items and also the number of rows in the DP table
      DP_table - the n*width table which will store our solution.
      problem_profits - an array of all the problem profits
      problem_weights - an array of all the problem weights
    Preconditions:
      The base cases must be filled in for this general algorithm to return
       a reasonable result. For our purposes, the base cases include the cases
       where profit is 0 and where items = 0
    Postconditions:
      Hopefully a completed DP table which will, within it, contain the optimal
       solution to the 0,1 Knapsack.
    Notes:
      Serious consideration: the problem_profits are 0-indexed, as are the problem
       weights.
      As a result the field A[1][1] is actually the solution which considers the
       combination of problem_profits[0] and problem_weights[0].
      This means when designing this, it is *crucial* to account for this semantic
       indexing issue.
  */

  int a, b;
  for (int p = 1; p < width; p++){
    for (int i = 1; i < n; i++){
      if (problem_profits[i-1] <= p){
        a = DP_table[i-1][p];
        b = problem_weights[i-1] + DP_table[i-1][(p-problem_profits[i-1])];
        DP_table[i][p] = ((a < b) ? a : b);
      }else DP_table[i][p] = DP_table[i-1][p];
    }
  }
}

/* Vasirani DP: Find symbolic positive infinity */
int derive_pinf(const int problem_weights[],
                const int n){
  /*
    Description:
      Sums up all the item weights to obtain a trivial upper bound on the weight
      of any given solution.
    Input:
      problem_weights - a 1D array of the problem's weight values.
    Output:
      my_pinf - my version of positive infinity.
    Notes:
      This is a purely symbolic representation of infinity that adds a single linear
       pass of the weight set to the run time.
  */

  int my_pinf = 1;
  for(int i = 0; i < n; i++){
    my_pinf += problem_weights[i];
  }
  return my_pinf;
}

/* Vasirani DP: Find best solution from table */
int DP_find_best_solution(const int width,
                          const int n,
                          int ** DP_table, //const int DP_table[][width],
                          const int capacity,
                          const int my_pinf){
  /*
    Description:
      Given a completed DP_table, find max(p | A(n,p) <= Capacity)
    Inputs:
      width - the width of the tables
      n - the height of the table (also the number of items)
      DP_table - the dynamic programming 2d array
      problem_profits - the input problem's associated item profits
      problem_weights - the input problem's associated item weights
      capacity - the input problem's knapsack capacity
      my_pinf - the input problem's trivial upper bound on weight
    Preconditions:
      DP_table must be completed, both in basis and in general case, for us
       to derive an optimal solution.
    Outputs:
      p - the max(p | A(n,p) <= Capacity)
    Notes:
      This is simply the profit of the optimal solution, nothing else.
      In order to derive the item set, more analysis needs to be done.
  */

  int p= -1;
  for(int i = width-1; i >= 0; i--){
    if (DP_table[n-1][i] != my_pinf){
      if (DP_table[n-1][i] <= capacity){
        p = i;
        break;
      }
    }
  }
  return p;
}

/* Vasirani DP: Derive solution set */
int DP_derive_solution_set(int n,
                           const int width,
                           int ** DP_table,
                           const int problem_profits[],
                           int solution[],
                           int p,
                           const int sol_flag){

  /*
    Description:
      Given a completed DP_table, derive the indices of the items of the optimal set.
    Inputs:
      n - the number of rows in the Dynamic Programming table
      DP_table - the 2d array representing the constituent subproblems of the DP
      solution - the output array; it will simply hold the indices of the items that
       are included in the optimal solution.
      p - the profit of the optimal solution
      sol_flag - if 0, index notation, if 1, 0/1 notation
    Postconditions:
      solution will be filled out
    Notes:
      
  */

  int s_index = 0;
  while ((p > 0)&&(n > 0)){
    if (DP_table[n-2][p] > DP_table[n-1][p]){
      // then we had item n and the solution at A[n-1][p-profit(n)]
      if(sol_flag == INDEX_NOTATION) solution[s_index] = (n-1);
      else solution[n-2] = 1;
      s_index += 1;
      n -= 1;
      p -= problem_profits[n-1]; // remember table n corresponds to profit n-1
    }else{
      // A[n-1][p] must be the same as A[n][p]
      if (sol_flag != INDEX_NOTATION) solution[n-2] = 0;
      n -= 1;
    }
  }
 return s_index;
}


/* W&S DP Functions */
/* W&S DP Core Algorithm */
int williamson_shmoys_DP(struct problem_item items[], int capacity, int n,
                        int *solution_array, const long long int memory_allocation_limit,
                         const int timeout, clock_t *start_time)
{
 /***william_shmoys_DP********************************************************
  *  Description: Implements the dynamic programming algorithm for the       *
  *               knapsack problem as described by Williamson and Shmoys.    *
  *  Inputs:                                                                 *
  *    struct problem_item items[]                                           *
  *      An array of all the items in the problem instance, built from       *
  *      reading and encoding an input file. Each item struct has profit     * 
  *      and weight members.                                                 *
  *    int capacity                                                          *
  *      The capacity as described by the problem statement                  *
  *  Outputs:                                                                *
  *    int w                                                                 *
  *      the max weight of a tuple in the list at the end of the algorithm   *
  *  Notes:                                                                  *
  *                                                                          *
  *                                                                          *
  
  ****************************************************************************/

  /* Base case */
  struct solution_pair* head = NULL;
  struct solution_pair* current = NULL; 
  push(&head, 0, 0, n, memory_allocation_limit);

  /* Were we allowed to do that? */
  if(bytes_allocated == -1)
  {
    /* Clean up*/
    current = head;
    while (current != NULL)
    {
      current = current->next;
      free(head);
      head = current;
    }
    return -1;
  }

  /* Go to the first index we can fit */
  int first_index = 0;
  while ((first_index < n) && items[first_index++].weight > capacity)
    ;
  /* If none of the elements will fit, we need to leave */
  int first_iteration_flag = TRUE;
  if(first_index == n)
  {
    /* Clean up*/
    current = head;
    while (current != NULL)
    {
      current = current->next;
      bytes_allocated -= sizeof(head);
      free(head);
      head = current;
    }
    /* Assuming solution_array has been malloc'd already */
    for(int i=0; i < n; i++)
    {
      solution_array[i] = 0;
    }

    /* Max profit is 0 */
    return 0;
  }
  /* We can play */
  else
  {
    push(&head, items[first_index-1].weight, items[first_index-1].profit, n, 
         memory_allocation_limit);
    if(bytes_allocated == -1)
    {
      /* Clean up*/
      current = head;
      while (current != NULL)
      {
        current = current->next;
        free(head);
        head = current;
      }
      return -1;
    }
    head->solution_array[first_index-1] = 1;

    /* General case */
    for(int j=first_index; j < n; j++)
    {
      current = head;
      while(current != NULL)
      {
        /* Only add if we can feasibly add it */
        int possible_weight = current->weight + items[j].weight;
        if (possible_weight <= capacity)
        {
          /* Put new partial solution on the head */
          push(&head, possible_weight, current->profit + items[j].profit, n, 
               memory_allocation_limit);
          /* Just make sure we were allowed to do that */
          if(bytes_allocated == -1)
          {
            /* Clean up*/
            current = head;
            while (current != NULL)
            {
              current = current->next;
              free(head);
              head = current;
            }
            return -1;
          }
          /* Copy the partial solution array  */
          for (int i=0; i <= j; i++)
            head->solution_array[i] = current->solution_array[i];
          /* Distinguish it from the others */
          head->solution_array[j] = 1;
        }
        current = current->next;
      }
      /* Check for timeout */
      if (did_timeout_occur(timeout, *start_time))
      {
        *start_time = -1;
        break;
      }
      remove_dominated_pairs(&head, memory_allocation_limit, start_time, 
                             timeout, n, &first_iteration_flag);
  
      /* Contingency exit lane */
      if(bytes_allocated == -1 || *start_time == -1)
      {
        /* Clean up 
        current = head;
        while (current != NULL)
        {
          current = current->next;
          free(head); 
          head = current;
        }
        */
        //return -1;
        head = NULL;
        break;
      }
    }

    int max_profit = -1;
    /* Only do this block if there wasn't a timeout */
    if (*start_time != -1)
    {
      /* return max ((t,w) in A max) w*/
      current = head;
      struct solution_pair* best_pair;
      while (current != NULL)
      {
        if (current->profit > max_profit)
        {
          max_profit = current->profit;
          best_pair = current;
        }
        current = current->next;  
      }
    
      /* Assuming solution_array has been malloc'd already */
      for(int i=0; i < n; i++)
      {
        solution_array[i] = best_pair->solution_array[i];
      }
    }

    /* Clean up */
    current = head;
    while (current != NULL)
    {
      current = current->next;
      bytes_allocated -= sizeof(head);
      free(head);
      head = current;
    }

    return max_profit;
  }
}





int williamson_shmoys_DP_amended(struct problem_item items[], int capacity, int n,
                                int *solution_array, const long long int memory_allocation_limit,
                                const int timeout, clock_t *start_time)
{
  /*
   *  PROPOSED IMPROVEMENTS (POSTPONED FOR NOW)
   *   1. We make an array B of length n, where n is the number of items in the
   *       problem, with associated profits array p. Let A[j-1] be a 
   *       non-dominating array of solutions that have considered the first j-1
   *       items, in ascending order of profits. Also let LB be the lower bound
   *       on the KP instance.
   *   2, We let B[n] = p[n], B[n-1] = p[n]+p[n-1], ... , and so on until 
   *      B[0] = p[0]+p[1]+...+p[n].
   *   3. At the beginning of iteration j, we do: For every partial solution 
   *       with profit p_i and weight w_i stored as (p_i, w_i) at A[j-1][i], 
   *       compute the LP (upper) bound UB_ji on the remaining n-j items with 
   *       capacity W-w_i (you sort the items once at the beginning, and then 
   *       only check which are available by index)
   *   4. If p_i + UB_ij < LB, then the partial solution i cannot possibly lead
   *        to an optimal solution, so we remove solution i from A[j-1]. */
  /* Base case */
  struct solution_pair* head = NULL;
  struct solution_pair* current = NULL; 
  push(&head, 0, 0, n, memory_allocation_limit);

  /* Were we allowed to do that? */
  if(bytes_allocated == -1)
  {
    /* Clean up*/
    current = head;
    while (current != NULL)
    {
      current = current->next;
      free(head);
      head = current;
    }
    return -1;
  }

  /* Go to the index of the first item we can fit */
  int first_index = 0;
  while ((first_index < n) && items[first_index++].weight > capacity)
    ;

  /* If none of the elements will fit, we need to leave */
  int first_iteration_flag = TRUE;
  if(first_index == n)
  {
    /* Clean up*/
    current = head;
    while (current != NULL)
    {
      current = current->next;
      bytes_allocated -= sizeof(head);
      free(head);
      head = current;
    }
    /* Assuming solution_array has been malloc'd already */
    for(int i=0; i < n; i++)
    {
      solution_array[i] = 0;
    }

    /* Max profit is 0 */
    return 0;
  }

  /* We can play */
  else
  {
    /* We insert after head because we know that this new solution pair will be
       at least (0,0) */
    insert_after(head, items[first_index-1].weight, 
                 items[first_index-1].profit, n, memory_allocation_limit);

    if(bytes_allocated == -1)
    {
      /* Clean up*/
      current = head;
      while (current != NULL)
      {
        current = current->next;
        free(head);
        head = current;
      }
      return -1;
    }
    head->next->solution_array[first_index-1] = 1;
  
    /* Linked list has just (0,0) and first item that can fit 
       For each solution_pair in A(j): */
    
    struct solution_pair* copied_list_head;
    for(int j = first_index; j < n; j++)
    {
      /* Create a copy of the linked list, representing A(j-1) */
      copy_linked_list(head, &copied_list_head, n, memory_allocation_limit);
      current = copied_list_head;
      while(current != NULL)
      {
        int possible_weight = current->weight + items[j].weight;
        int possible_profit = current->profit + items[j].profit;

        /* If we could, hypothetically, have such a solution */
        if (possible_weight <= capacity)
        {
          /* We know the list is currently nondominated and sorted (base case 
              should have (0,0) and first item), so we navigate to the point 
              where this partial solution would be, according to its weight. 
              So, we first find the last node that is <= current */
          struct solution_pair *list_crawler = head;
          while(list_crawler->next != NULL 
                && list_crawler->next->weight <= possible_weight)
            list_crawler = list_crawler->next;
          
          /* Check if current is dominated by solution with equal weight */
          if (list_crawler->profit >= possible_profit)
          {
            /* Then the hypothetical solution is dominated by 
               equal weight item */
            current = current->next;
            continue; 
          }
            
          /* Hypothetical solution is not dominated so insert after 
             list_crawler, cementing it as a partial solution */
          insert_after(list_crawler, possible_weight, possible_profit, n, 
                       memory_allocation_limit);
          struct solution_pair* new_partial_solution = list_crawler->next;
    
          /* Copy the partial solution array */
          for (int i = 0; i <= j; i++)
            new_partial_solution->solution_array[i] = current->solution_array[i];
          new_partial_solution->solution_array[j] = 1;

          /* Remove local dominations before new partial solution */
          while(dominates(new_partial_solution, new_partial_solution->prev))
            remove_from_linked_list(new_partial_solution->prev);

          /* Remove local dominations after new partial solution */
          while(new_partial_solution->next != NULL 
                && dominates(new_partial_solution, new_partial_solution->next))
            remove_from_linked_list(new_partial_solution->next);
        }
      current = current->next;
      }
      remove_linked_list(&copied_list_head);
    }
    int max_profit = -1;
    /* Only do this block if there wasn't a timeout */
    if (*start_time != -1)
    {
      /* return max ((t,w) in A max) w*/
      current = head;
      struct solution_pair* best_pair;
      while (current != NULL)
      {
        if (current->profit > max_profit)
        {
          max_profit = current->profit;
          best_pair = current;
        }
        current = current->next;  
      }
    
      /* Assuming solution_array has been malloc'd already */
      for(int i=0; i < n; i++)
      {
        solution_array[i] = best_pair->solution_array[i];
      }
    }

    /* Clean up */
    current = head;
    while (current != NULL)
    {
      current = current->next;
      bytes_allocated -= sizeof(head);
      free(head);
      head = current;
    }

    return max_profit;

  }
}


/* W&S DP: Linked list push function */
void push(struct solution_pair** head_ref, int new_weight, int new_profit,
          int n, const long long int memory_allocation_limit)
{
 /***push*********************************************************************
  *  Description:                                                            *
  *    Given a pointer to a solution_pair structure pointer (the head of a   *
  *    linked list of such structures), link a new structure into the list   *
  *    with given weight new_weight and profit new_profit.                   *
  *  Postconditions:                                                         *
  *    This will put the new structure at the top of the list.               *
  *  Notes:                                                                  *
  *    This is taken from the geeksforgeeks linked list mergesort article    *
  *    This needs n in order to dynamically allocate enough space for the    *
  *    struct member solution_array                                          * 
  ****************************************************************************/
  struct solution_pair* new_solution_pair =
     (struct solution_pair*)calloc(sizeof(struct solution_pair) + n,
      sizeof(int));
  
  bytes_allocated += sizeof(struct solution_pair) + n*sizeof(int);
  if (memory_allocation_limit != -1 &&
      bytes_allocated > memory_allocation_limit)
  {
    printf("  Overallocation detected in Williamson Shmoys LL push! Bytes got to"
           " %lld.\n", bytes_allocated);
    bytes_allocated = -1;
  }

  if (new_solution_pair)
  {
    struct solution_pair const temp_pair =
      {.weight = new_weight, .profit = new_profit, .next=(*head_ref)};
     memcpy(new_solution_pair, &temp_pair, sizeof(struct solution_pair));
    
  }

  /* Define new pair's data */
  new_solution_pair->weight = new_weight;
  new_solution_pair->profit = new_profit;

  /* Connect pair to the head of the list */
  new_solution_pair->next = (*head_ref);
  new_solution_pair->prev = NULL;

  /* Initialise the array to 0's*/
  for(int i = 0; i < n; i++)
    *(new_solution_pair->solution_array+i) = 0;
 
  /* Set the new node to be the new head of the list */
  if ((*head_ref) != NULL)
    (*head_ref)->prev = new_solution_pair;
  (*head_ref) = new_solution_pair;
}

void insert_after(struct solution_pair* reference_pair, int new_weight, int new_profit,
                  int n, const long long int memory_allocation_limit)
{
 /***insert_after*************************************************************
  *  Description:                                                            *
  *    Creates a solution_pair node and puts it after existing solution_pair *
  *    called reference_pair.                                                *
  *  Postconditions:                                                         *
  *    New solution pair with weight new_weight and profit new_profit will   *
  *    be inserted after reference_pair, with pointers 'next' for both being *
  *    updated.                                                              *
  *  Notes:                                                                  *
  *    This doesn't update 'prev' members of the structs, cause (hopefully)  *
  *    we won't need them!                                                   *
  ****************************************************************************/
  struct solution_pair* new_solution_pair = 
     (struct solution_pair*)calloc(sizeof(struct solution_pair) + n,
      sizeof(int));

  bytes_allocated += sizeof(struct solution_pair) + n*sizeof(int);
  if (memory_allocation_limit != -1 &&
      bytes_allocated > memory_allocation_limit)
  {
    printf("  Overallocation detected in Williamson Shmoys LL insert_after! Byt"
           "es got to %lld.\n", bytes_allocated);
    bytes_allocated = -1;
  }

  /* If solution was allocated */
  if (new_solution_pair)
  {
    struct solution_pair const temp_pair =
      {.weight = new_weight, .profit = new_profit, .next=reference_pair->next};
     memcpy(new_solution_pair, &temp_pair, sizeof(struct solution_pair));
    
  }

  /* Define new pair's data (note to self: why does this need to be done like 
     this?) */
  new_solution_pair->weight = new_weight;
  new_solution_pair->profit = new_profit;

  /* Initialise the array to 0's*/
  for(int i = 0; i < n; i++)
    *(new_solution_pair->solution_array+i) = 0;

  /* Insert pair into the list */
  new_solution_pair->next = reference_pair->next;
  new_solution_pair->prev = reference_pair;
  if (reference_pair->next != NULL)
    reference_pair->next->prev = new_solution_pair;
  reference_pair->next = new_solution_pair;
}


/* W&S DP: Remove Dominated Pairs */
void remove_dominated_pairs(struct solution_pair** head_ref, 
                            const long long int memory_allocation_limit, 
                            clock_t *start_time, const int timeout, int n,
                            int *first_iteration_flag)
{
 /***remove_dominated_pairs documentation*************************************
  *  Description:                                                            *
  *    Removes the dominated pairs within a linked list of solution_pairs by *
  *    a two phase approach consistening of a merge sort on each of the      * 
  *    weights and a series of iteratively shortening comparisons based on   *
  *    the profits.                                                          *
  *  Inputs:                                                                 *
  *    struct solution_pair** headRef                                        *
  *      Double pointer to the head of the linked list to allow for          *
  *      reallocation of the head, should it be dominated                    * 
  *  Postconditions:                                                         *
  *    Linked list will have entirely non-dominated pairs.                   *
  *  Notes:                                                                  *
  *    Has function dependencies in merge sort and all of its constituent    *
  *    functions                                                             *
  *    This function assumes that the (0,0) tuple will always be in an input.*
  *    As such, the case where the linked list is of length 0 is not         *
  *    addressed. The algorithm seems resilient enough to cover length=1     *
  *    with its general case.                                                *
  ****************************************************************************/
  //merge_sort(head_ref, 0, start_time, timeout);
  
  /* Merge sort the list on first iteration, by weight */
  if (*first_iteration_flag)
  {
    struct solution_pair* lets_count = *head_ref;
    int list_length = 0;
    for(lets_count = *head_ref; lets_count != NULL; lets_count = lets_count->next)
      list_length++;
    iterative_merge_sort(head_ref, list_length, n);
    *first_iteration_flag = 0;
  }  
  /* Insertion sort the list on other iterations, by weight */
  else
  {
    linked_list_insertion_sort(head_ref);
  }

  /* Exit chute */
  if (bytes_allocated == -1 || *start_time == -1)
    return;

  struct solution_pair* current = (*head_ref)->next;
  struct solution_pair* previous = *head_ref;

  /* Filtration algorithm */
  while (current != NULL)
  {
    if (previous->profit >= current->profit)
    {
      current = current->next;
      bytes_allocated -= sizeof(previous->next);
      free(previous->next);
      previous->next = current;
    }
    else
    {
      current = current->next;
      previous = previous->next;
    }
  }
}

/* Merge sort */
void merge_sort(struct solution_pair** head_ref, long long int merge_sort_memory, clock_t *start_time, const int timeout)
{
 /* merge_sort: my adapation for solution pairs as originally described by    *
  *             geeks for geeks.                                              *
  *             This is strictly designed for linked lists                    *
  *             This algorithm is said to have complexity O(nlogn)            *
  *                                                                           *
  *                                                                           */

  /* Check for timeout */
  clock_t elapsed = clock() - *start_time;
  double time_taken = ((double)elapsed)/CLOCKS_PER_SEC;
  if (timeout != -1 && time_taken > timeout)
  {
    *start_time = -1;
    return;
  }
  
  if(merge_sort_memory >= SYSTEM_STACK_LIMIT)
  {
    printf("merge_sort_memory: %lld\n", merge_sort_memory);
    bytes_allocated = -1;
    return;
  }

  struct solution_pair* head = *head_ref;
  struct solution_pair* a;
  struct solution_pair* b;

  merge_sort_memory += sizeof(struct solution_pair *)*3;

  /* Bases: length 0 or 1 */
  if ((head == NULL) || (head->next == NULL))
    return;

  /* Split head into 'a' and 'b' sublists */
  front_back_split(head, &a, &b);

  /* Recursively sort the sublists */
  merge_sort(&a, merge_sort_memory, start_time, timeout);

  if(bytes_allocated == -1 || *start_time == -1)
  { 
    struct solution_pair* next_one;
    struct solution_pair* temp_one;
    while(a != NULL)
    {
      next_one = a->next;
      free(a);
      a = next_one;
    }
    while(b != NULL)
    {
      next_one = b->next;
      free(b);
      b = next_one;
    }
    /* Implicitly, this nullified head_ref */
    *head_ref = NULL;
    return;
  }
  
  merge_sort(&b, merge_sort_memory, start_time, timeout);

  if(bytes_allocated == -1 || *start_time == -1)
  { 
    struct solution_pair* next_one;
    while(a != NULL)
    {
      next_one = a->next;
      free(a);
      a = next_one;
    }
    while(b != NULL)
    {
      next_one = b->next;
      free(b);
      b = next_one;
    }
    /* Implicitly, this nullified head_ref */
    *head_ref = NULL;
    return;
  }

  /* Merge the two lists of this scope together */
  *head_ref = sorted_merge(a, b, start_time, timeout);
  if(bytes_allocated == -1 || *start_time == -1)
  { 
    struct solution_pair* next_one;
    while(*head_ref != NULL)
    {
      next_one = (*head_ref)->next;
      free(*head_ref);
      *head_ref = next_one;
    }
    return;
  }
}

/* Merge sort aux: sorted merge function */
struct solution_pair* sorted_merge(struct solution_pair* a, 
                                   struct solution_pair* b, clock_t *start_time,
                                   const int timeout)
{
  /* Check for timeout */
  clock_t elapsed = clock() - *start_time;
  double time_taken = ((double)elapsed)/CLOCKS_PER_SEC;
  if (timeout != -1 && time_taken > timeout)
  {
    *start_time = -1;
    /* Complete the merge though because it's easier lol */
  }

  struct solution_pair* result = NULL;

  /* Base cases */
  if (a == NULL) return (b);
  else if (b == NULL) return (a);

  /* Pick the lower of either a or b, and recur */
  if (a->weight <= b->weight)
  {
    /* Corner case: if they're equal weighted, put the one with the higher 
       profit first. This simplifies the cases we have to consider within
       the filtration step of dominated pair removal */
    if (a->weight == b->weight)
    {
      if (a->profit > b->profit) 
      {
        result = a;
        result->next = sorted_merge(a->next, b, start_time, timeout);
      }
      else 
      {
        result = b;
        result->next = sorted_merge(a, b->next, start_time, timeout);
      }
    }
    /* Otherwise just merge as normal */
    else
    {
      result = a;
      result->next = sorted_merge(a->next, b, start_time, timeout);
    }
  }
  else
  {
    result = b;
    result->next = sorted_merge(a, b->next, start_time, timeout); 
  }
  return(result);
}

/* Merge sort aux: front/back split function */
void front_back_split(struct solution_pair* source, 
                      struct solution_pair** front_ref, 
                      struct solution_pair** back_ref)
{
  struct solution_pair* fast;
  struct solution_pair* slow;
  if (source == NULL || source->next == NULL)
  {
    /* Length < 2 cases */
    *front_ref = source;
    *back_ref = NULL; 
  }
  else
  {
    slow = source;
    fast = source->next;

    /* Advance 'fast' by two nodes, and advance 'slow' by one */
    while (fast != NULL)
    {
      fast = fast->next;
      if (fast != NULL)
      {
        slow = slow->next;
        fast = fast->next;
      }
    }
    /* 'slow' is before the midpoint in the list, so split it at that point */
    *front_ref = source;
    *back_ref = slow->next;
    slow->next = NULL;
  }
}

/* Linked list printer function */
void print_list(struct solution_pair* node)
{
  while(node!=NULL)
  {
    printf("%d ", node->profit);
    node = node->next;
  }
}

/* Dynamic Array method: Initialise */
void initialise_dynamic_array(Dynamic_Array **dynamic_array, size_t initial_size)
{
  Dynamic_Array *tmp = *dynamic_array = (Dynamic_Array*)malloc(sizeof(Dynamic_Array));
  tmp->array = (double *)malloc(initial_size * sizeof(double));
  tmp->used = 0;
  tmp->size = initial_size;
}

/* Dynamic Array method: Append */
void append_to_dynamic_array(Dynamic_Array *dynamic_array, double element)
{
 if (dynamic_array->used == dynamic_array->size)
  {
    dynamic_array->size *= 2;
    dynamic_array->array = (double *)realloc(dynamic_array->array, 
                                         dynamic_array->size * sizeof(double));
  }
  dynamic_array->array[dynamic_array->used++] = element; 
}

/* Dynamic Array method: Free */
void free_dynamic_array(Dynamic_Array *dynamic_array)
{
  free(dynamic_array->array);
  dynamic_array->array = NULL;
  dynamic_array->used = dynamic_array->size = 0;
}

int did_timeout_occur(const int timeout, const clock_t start_time)
{
  if (timeout == -1) return FALSE;

  clock_t elapsed = clock() - start_time;
  double elapsed_secs = ((double) elapsed) / CLOCKS_PER_SEC;
  if (elapsed_secs > timeout)
    return TRUE;
  else
    return FALSE;
}


/* Actual merge sort function */
void iterative_merge_sort(struct solution_pair** head_ref, int list_length, int n)
/******************************************************************************
 * iterative_merge_sort:                                                      *
 *  Implements sorted_merge iteratively across the list of items, first with  *
 *   sublists of length 2, then 4, ... until the whole list has had a sorted  *
 *   merge done on it.                                                        *
 *  Sorts in ascending order by weight                                        *
 ******************************************************************************/
{
  for(int current_size = 1; current_size <= list_length-1; current_size *= 2)
    for(int left_start = 0; left_start < list_length-1; 
        left_start += 2*current_size)
    {
      int mid = left_start + current_size - 1;
      int right_end = min(left_start + 2*current_size - 1, list_length-1);
      merge(head_ref, left_start, mid, right_end, n, list_length);
    }
}

/* Sorted merge function */
void merge(struct solution_pair** head_ref, int left_index, int mid_index, 
           int right_index, int n, int list_length)
{
  /* Merges symbolic sublists arr[left_index ... mid_index] and 
      arr[mid_index+1 ... right_index] */
  int n1 = mid_index - left_index + 1;
  int n2 = right_index - mid_index;
  if (n2 < 0) n2 = 0;
  struct solution_pair* current, * left_current, * right_current;

  /* Make empty linked list of length n1 */
  struct solution_pair* left_head =  
       (struct solution_pair*)calloc(sizeof(struct solution_pair) + n,
        sizeof(int));
  current = left_head;
  for (int i = 0; i < n1-1 && left_index+i < list_length-1; i++)
  {
    struct solution_pair* new_solution_pair =
       (struct solution_pair*)calloc(sizeof(struct solution_pair) + n,
        sizeof(int));
    current->next = new_solution_pair;
    current = new_solution_pair;
  }
  current->next = NULL;
  
  /* Make empty linked list of length n2 */
  struct solution_pair *right_head;
  if (n2 > 0)
  {
    right_head =  
         (struct solution_pair*)calloc(sizeof(struct solution_pair) + n,
          sizeof(int));
    current = right_head;
    for (int i = 0; i < n2-1; i++)
    {
      struct solution_pair* new_solution_pair =
         (struct solution_pair*)calloc(sizeof(struct solution_pair) + n,
          sizeof(int));
      current->next = new_solution_pair;
      current = new_solution_pair;
    }
    current->next = NULL;
  }
  else
  {
    right_head = NULL;
  }  

  /* Copy data into temp arrays */
  current = *head_ref;
  for (int i = 0; i < left_index; i++)
    current = current->next; 

  left_current = left_head;
  while(left_current != NULL)
  {
    left_current->weight = current->weight;
    left_current->profit = current->profit;
    for (int j = 0; j < n; j++)
      left_current->solution_array[j] = current->solution_array[j];
    left_current = left_current->next;
    current = current->next;
  }

  /* current should be the right_index'th element */
  right_current = right_head;
  while(right_current != NULL)
  {
    right_current->weight = current->weight;
    right_current->profit = current->profit;
    for (int j = 0; j < n; j++)
      right_current->solution_array[j] = current->solution_array[j];
    right_current = right_current->next;
    current = current->next;
  }

  /* Move current back to left_index */
  current = *head_ref;
  for(int i = 0; i < left_index; i++)
  {
    current = current->next;
  }

  /* Merge them until we hit the end of one of the lists */
  left_current = left_head;
  right_current = right_head;
  while(left_current != NULL && right_current != NULL)
  {
    /* Pick the one with lower weight */
    if (left_current->weight <= right_current->weight)
    {
      /* If they have the same weight, pick the one with higher profit */
      if(left_current->weight == right_current->weight)
      {
        if(left_current->profit >= right_current->profit)
        {
          current->weight = left_current->weight;
          current->profit = left_current->profit;
          for (int j = 0; j < n; j++)
            current->solution_array[j] = left_current->solution_array[j];
          left_current = left_current->next;
        }
        else
        {
          current->weight = right_current->weight;
          current->profit = right_current->profit;
          for (int j = 0; j < n; j++)
            current->solution_array[j] = right_current->solution_array[j];
          right_current = right_current->next;
        }
      }
      /* Left had the lower weight */
      else
      {
        current->weight = left_current->weight;
        current->profit = left_current->profit;
        for (int j = 0; j < n; j++)
          current->solution_array[j] = left_current->solution_array[j];
        left_current = left_current->next;
      }
    }
    /* Right had the lower weight */
    else
    {
      current->weight = right_current->weight;
      current->profit = right_current->profit;
      for (int j = 0; j < n; j++)
        current->solution_array[j] = right_current->solution_array[j];
      right_current = right_current->next;
    }
    current = current->next;
  }
  
  /* Empty any of the remaining elements out of the lists */
  while (left_current != NULL)
  {
    current->weight = left_current->weight;
    current->profit = left_current->profit;
    for (int j = 0; j < n; j++)
      current->solution_array[j] = left_current->solution_array[j];
    left_current = left_current->next;
    current = current->next;
  }
  while (right_current != NULL)
  {
    current->weight = right_current->weight;
    current->profit = right_current->profit;
    for (int j = 0; j < n; j++)
      current->solution_array[j] = right_current->solution_array[j];
    right_current = right_current->next;
    current = current->next;
  }

  /* Free the lists */
  while(left_head != NULL)
  {
    left_current = left_head->next;
    free(left_head);
    left_head = left_current;
  }
  while(right_head != NULL)
  {
    right_current = right_head->next;
    free(right_head);
    right_head = right_current;
  }
}

void linked_list_insertion_sort(struct solution_pair **head_ref)
/******************************************************************************
 *  linked_list_insertion_sort                                                *
 *    implements insertion sort for solution_pair structures                  *
 *    TODO THIS DOESN'Y WORK :(                                               *
 ******************************************************************************/  
{
  struct solution_pair *sorted_head, *current, *to_be_slotted;
  /* Make doubly linked */
  (*head_ref)->prev = NULL;
  for(current = *head_ref; current->next != NULL; current = current->next)
    current->next->prev = current;

  sorted_head = *head_ref;
  //while(sorted_head->next != NULL)
  while(sorted_head != NULL && sorted_head->next != NULL)
  {
    current = sorted_head;
    to_be_slotted = current->next;

    /* Move current to item that is greater than or equal to to_be_slotted */
    while(current != NULL && to_be_slotted->weight < current->weight)
    {
      current = current->prev;
    }

    /* Insertion midlist (insert after current) */
    if (current != NULL)
    {
      /* If is not already in order (if it is already in order, move on)*/
      //if(current->next != to_be_slotted)
      if(sorted_head != current)
      {
        /* Moving to_be_slotted behind sorted_head */
        sorted_head->next = to_be_slotted->next;
        if(to_be_slotted->next != NULL)
          to_be_slotted->next->prev = sorted_head;
        to_be_slotted->next = current->next;
        current->next->prev = to_be_slotted;
        current->next = to_be_slotted;
        to_be_slotted->prev = current;
      }
      else
      {
        sorted_head = sorted_head->next;
      }
    }
    /* Insertion at start of list (current backpedalled to NULL) */
    else
    {
      if (to_be_slotted->next != NULL)
        to_be_slotted->next->prev = to_be_slotted->prev;
      to_be_slotted->prev->next = to_be_slotted->next;
      to_be_slotted->prev = NULL;
      to_be_slotted->next = *head_ref;
      (*head_ref)->prev = to_be_slotted;
      *head_ref = to_be_slotted;
    }
  }
  
  /*Make sure it is sorted*/
  current = *head_ref;
  while (current->next != NULL)
  {
    assert(current->weight <= current->next->weight);
    current = current->next;
  }
}

int dominates(struct solution_pair* A, struct solution_pair* B)
{
/**dominates*******************************************************************
 * Description                                                                *
 *   Simple true/false function, returns TRUE (i.e. 1) if A dominates B, else *
 *   returns FALSE (i.e. 0).                                                  *
 *                                                                            *
 ******************************************************************************/  
  if(A->weight <= B->weight && A->profit >= B->profit)
    return TRUE;
  else
    return FALSE;
}

void remove_from_linked_list(struct solution_pair* to_be_removed)
{
/**remove_from_linked_list*****************************************************
 * Description                                                                *
 *  removes solution_pair to_be_removed from the list which is it a part of   *
 *                                                                            *
 ******************************************************************************/  
  /* Patch up pointers */
  if(to_be_removed->prev != NULL)
    to_be_removed->prev->next = to_be_removed->next;
  if(to_be_removed->next != NULL)
    to_be_removed->next->prev = to_be_removed->prev;

  /* Free the memory */
  bytes_allocated -= sizeof(to_be_removed);
  free(to_be_removed);
}

void remove_linked_list(struct solution_pair** head_reference)
{
/**remove_linked_list**********************************************************
 * Description                                                                *
 *   Goes from head to tail of a linked list, freeing the whole thing.        *
 *                                                                            *
 ******************************************************************************/  
  //struct solution_pair* current = *head_reference;
  struct solution_pair* next = *head_reference;
  while(*head_reference != NULL)
  {
    next = next->next;
    bytes_allocated -= sizeof(*head_reference);
    free(*head_reference);
    *head_reference = next;
    //free(previous->solution_array);//TODO Invalid free?
  }
}

void copy_linked_list(struct solution_pair* old_head, 
                      struct solution_pair** new_head,
                      const int n,
                      const long long int memory_allocation_limit)
{
  /**copy_linked_list**********************************************************
   * Description                                                              *
   *   Copies the linked list starting at old_head onto a new list starting   *
   *    at new_head.                                                          *
   * Notes                                                                    *
   *   new_head is expected to be nothing yet.                                *
   ****************************************************************************/
  struct solution_pair* current_old = old_head;
  struct solution_pair* current_new;
  struct solution_pair* previous_new = NULL;

  if (old_head != NULL)
  {
    /* Make new LL head */
    *new_head = 
       (struct solution_pair*)calloc(sizeof(struct solution_pair) + n,
        sizeof(int));
    current_new = *new_head;
    bytes_allocated += sizeof(struct solution_pair) + n*sizeof(int);
    if (memory_allocation_limit != -1 &&
        bytes_allocated > memory_allocation_limit)
    {
      printf("  Overallocation detected in Williamson Shmoys LL copy_linked_lis"
             "t! Bytes got to %lld.\n", bytes_allocated);
      bytes_allocated = -1;
    }

    copy_solution_pair(current_old, current_new, n);
    previous_new = current_new;
    current_old = current_old->next;
  }
  else *new_head = NULL;

  while(current_old != NULL && bytes_allocated != -1)
  { 	
    /* Allocate for current_new */
    current_new = 
       (struct solution_pair*)calloc(sizeof(struct solution_pair) + n,
        sizeof(int));
    bytes_allocated += sizeof(struct solution_pair) + n*sizeof(int);
    if (memory_allocation_limit != -1 &&
        bytes_allocated > memory_allocation_limit)
    {
      printf("  Overallocation detected in Williamson Shmoys LL copy_linked_lis"
             "t! Bytes got to %lld.\n", bytes_allocated);
      bytes_allocated = -1;
    }

    /* Copy data from old to new */
    copy_solution_pair(current_old, current_new, n);

    /* Connect previous to current */
    previous_new->next = current_new;
    current_new->prev = previous_new;

    /* Set previous to current and progress current_old by one */
    previous_new = previous_new->next;
    current_old = current_old->next;
  }
  
  /* Length of list >= 1 */
  if(previous_new != NULL)
  {
    previous_new->next = NULL;
  }
}

void copy_solution_pair(struct solution_pair* old_pair,
                        struct solution_pair* new_pair,
                        const int n)
{
  /**copy_solution_pair********************************************************
   * Description                                                              *
   *   Copies the struct data from old_pair to new_pair                       *
   * Notes                                                                    *
   *   Assumes that new_pair has already been allocated                       *
   *   DOES NOT copy next and prev pointers, as this is not presumed to be    *
   *    useful in a copying scenario.                                         *
   ****************************************************************************/
  if(old_pair == NULL)
  {
    new_pair = NULL;
  }
  else
  {
    new_pair->weight = old_pair->weight;
    new_pair->profit = old_pair->profit;
    new_pair->next = NULL;
    new_pair->prev = NULL;
    for(int i = 0; i < n; i++)
      new_pair->solution_array[i] = old_pair->solution_array[i];
  }
}

void get_a_lower_bound(const int *profits, const int *weights, int n, int capacity,
                             int *lower_bound, double *upper_bound)
{
  /**get_a_lower_bound*********************************************************
   * Description                                                              *
   *   Lol this is just a copy of the other one.                              *
   * Notes                                                                    *
   *   lower_bound and upper_bound are output parameters.                     *
   ****************************************************************************/
    
  if(capacity <= 0)
    return;

  /* Order by non-increasing profit/weight ratio */
  double ratios[n];
  double indices[n];
  for(int i = 0; i < n; i++) indices[i] = 0;
  for(int i = 0; i < n; i++)
  { 
    if (weights[i] != 0)
      ratios[i] = (double) profits[i]/weights[i];
    else
      ratios[i] = 0;
    indices[i] = (double) i;
  }

  quick_sort_parallel_lists_hey(ratios, indices, 0, n-1);
  
  /* Pick items in that order, one by one until picking an item would 
      overfill the knapsack */
  int current_weight = 0;
  int current_profit = 0;
  int i = 0;
  for(i = 0; i < n && weights[(int)indices[i]]+current_weight <= capacity; i++)
  { 
    current_weight += weights[(int)indices[i]];
    current_profit += profits[(int)indices[i]];
  }
  
  /* Then pick the fractional component of that item that you can fit to get
      the dual bound */
  double scale = 0;
  if(i < n)
  { 
    scale = (double)(capacity - current_weight) / weights[(int)indices[i]];
    *upper_bound = current_profit + scale * profits[(int)indices[i]];
  }
  else *upper_bound = current_profit;

  /* Continue traversing the list until you find the next item that you can
      fit to get the primal bound */
  
  while (i < n && weights[(int)indices[i]]+current_weight > capacity)
    i++;
  if(i < n)
    *lower_bound = profits[(int)indices[i]]+current_profit;
  else
    *lower_bound = current_profit;
}

void quick_sort_parallel_lists_hey(double *list1, double *list2, int lo, int hi)
{
  /**quick_sort_parallel_lists*************************************************
   * Description                                                              *
   *  Sort according to the first list, but also reflect changes in second    *
   *  list in parallel.                                                       *
   *                                                                          *
   ****************************************************************************/

    // Create an auxiliary stack
    double stack[ hi - lo + 1 ];

    // initialize top of stack
    int top = -1;

    // push initial values of l and h to stack
    stack[ ++top ] = lo;
    stack[ ++top ] = hi;

    // Keep popping from stack while is not empty
    while ( top >= 0 )
    {
        // Pop h and l
        hi = stack[ top-- ];
        lo = stack[ top-- ];

        // Set pivot element at its correct position
        // in sorted array
        int p = partition_hey( list1, list2, lo, hi );

        // If there are elements on left side of pivot,
        // then push left side to stack
        if ( p-1 > lo )
        {
            stack[ ++top ] = lo;
            stack[ ++top ] = p - 1;
        }

        // If there are elements on right side of pivot,
        // then push right side to stack
        if ( p+1 < hi )
        {
            stack[ ++top ] = p + 1;
            stack[ ++top ] = hi;
        }
    }
}

/* swap function from geeksforgeeks.org/iterative-quick-sort/ */
void swap_hey( double* a, double* b )
{
    double t = *a;
    *a = *b;
    *b = t;
}

/*  partition function from geeksforgeeks.org/iterative-quick-sort/ */
int partition_hey(double arr1[], double arr2[], int l, int h)
{
    double x = arr1[h];
    int i = (l - 1);

    for (int j = l; j <= h- 1; j++)
    {
        if (arr1[j] >= x)
        {
            i++;
            swap_hey(&arr1[i], &arr1[j]);
            swap_hey(&arr2[i], &arr2[j]);
        }
    }
    swap_hey(&arr1[i + 1], &arr1[h]);
    swap_hey(&arr2[i + 1], &arr2[h]);
    return (i + 1);
}


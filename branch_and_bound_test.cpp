/*****************************************************************************
 * branch_and_bound_test.cpp                                                 *
 * Author: Nelson Frew                                                       *
 * First edit: 19/02/18                                                      *
 * Last edit: 27/02/18                                                       *
 * Description:                                                              *
 *   Unit tests for the branch and bound algorithm which uses an unspecified *
 *   but already tested FPTAS framework.                                     *
 *                                                                           *
 *****************************************************************************/

#define TESTING
#include "branch_and_bound.c"
#include <gtest/gtest.h>
#include <time.h>
#include <math.h>

class branch_and_bound_mechanismTest : public testing::Test
{
  void SetUp(){
  }
  void TearDown(){
  }
};

class problem_instance_queueTest : public testing::Test
{
  void SetUp(){
  }
  void TearDown(){
  }
};

/*
void pisinger_reader(int *n, int *c, int *z, int **p, int **w, int **x,
                     char *problem_file, int problem_no);
*/

/* Queue Test Cases */

TEST(problem_instance_queueTest, create_queueTest){
  /* Make sure that the create_queue algorithm indeed creates an instance of 
   * a queue which has all the expected structure members and is initialised
   * completely empty */
  srand(time(NULL));

  /* Create the queue */
  int capacity = rand() % 10000000;
  Problem_Queue *test_problem_queue = create_queue(capacity);

  /* Check that it has a front, a rear, a size, a capacity and an array which 
   * is NULL. */
  ASSERT_EQ(test_problem_queue->capacity, capacity);
  ASSERT_EQ(test_problem_queue->rear, capacity - 1);
  ASSERT_EQ(test_problem_queue->size, 0);

  free(test_problem_queue);
}

TEST(problem_instance_queueTest, enqueueTest){
  /* Make sure, given a random problem node, the enqueue operation correctly 
   * places the problem at the rear of the queue 
   
  srand(time(NULL));

  /* Create the queue */
  int capacity = rand() % 10000000 + 1;
  Problem_Queue *test_problem_queue = create_queue(capacity);

  /* Create the problem instance */
  int n = capacity;
  Problem_Instance *test_instance = (Problem_Instance *) malloc(sizeof(Problem_Instance));
  test_instance->parent = NULL;
  test_instance->variable_statuses = (int *) malloc(sizeof(int) * n);
  memset(test_instance->variable_statuses, 1, sizeof(int) * n);
  test_instance->lower_bound = 0;
  test_instance->upper_bound = 100;  

  /* Save all the properties of the problem instance */
  int lower_bound = test_instance->lower_bound;
  int upper_bound = test_instance->upper_bound;
  int *variable_statuses = test_instance->variable_statuses;

  /* Place the instance in the queue*/
  enqueue(test_problem_queue, test_instance);

  /* Access the head of the queue and compare the problem characteristics with
   * the saved characteristics */
  int front = test_problem_queue->front;
  ASSERT_EQ(NULL, test_problem_queue->array[front]->parent);
  ASSERT_EQ(test_problem_queue->array[front]->variable_statuses, variable_statuses);
  ASSERT_EQ(test_problem_queue->array[front]->lower_bound, lower_bound);
  ASSERT_EQ(test_problem_queue->array[front]->upper_bound, upper_bound); 

  /* Clean up */
  free(test_instance->variable_statuses);
  free(test_instance);
  free(test_problem_queue);
}

TEST(problem_instance_queueTest, dequeueTest){
  /* Make sure, given a random queue of problem nodes, that the dequeue operation
   * correctly removes the node at the head of the queue. */
  srand(time(NULL));

  /* Create the queue */
  int capacity = rand() % 10000000 + 1;
  Problem_Queue *test_problem_queue = create_queue(capacity);
  int n = capacity;

  /* Create the problem node*/
  Problem_Instance *test_instance = (Problem_Instance *) malloc(sizeof(Problem_Instance));
  test_instance->parent = NULL;
  test_instance->variable_statuses = (int *) malloc(sizeof(int) * n);
  memset(test_instance->variable_statuses, 1, sizeof(int) * n);
  test_instance->lower_bound = 10;
  test_instance->upper_bound = 110;

  /* Put the node in */
  enqueue(test_problem_queue, test_instance);

  /* Create blank slate problem node */
  Problem_Instance *blank_slate_instance;

  /* Dequeue the node into this blank slate */
  blank_slate_instance = dequeue(test_problem_queue);

  /* Compare and assert the original with the dequeued */
  ASSERT_EQ(test_instance->parent, blank_slate_instance->parent);
  ASSERT_EQ(test_instance->variable_statuses, blank_slate_instance->variable_statuses);
  ASSERT_EQ(test_instance->lower_bound, blank_slate_instance->lower_bound);
  ASSERT_EQ(test_instance->upper_bound, blank_slate_instance->upper_bound);

  /* Make sure the queue is now empty */
  ASSERT_EQ(NULL, dequeue(test_problem_queue));
  ASSERT_EQ(test_problem_queue->size, 0);

  /* Clean up */
  free(test_instance->variable_statuses);
  free(test_instance);
  free(test_problem_queue);
}

TEST(problem_instance_queueTest, is_fullTest){
  /* Make sure, given a random series of problem instances pumped 
   * into a randomly sized (i.e. random capacity) queue that the queue correctly
   * recognises when it has reached capacity. */
  srand(time(NULL));

  /* Randomly generate the queue capacity */
  int capacity = rand() % 10000000 + 1;

  /* Create the queue */
  Problem_Queue *test_problem_queue = create_queue(capacity);

  /* Generate as many unique nodes as there is capacity */
  /* Enqueue the nodes and make sure that the queue thinks it is full */
  int n = capacity;
  for(int i = 0; i < n; i++)
  {
    Problem_Instance *test_problem = (Problem_Instance *) malloc(sizeof(Problem_Instance));
    test_problem->parent = NULL;
    test_problem->lower_bound = i*3;
    test_problem->upper_bound = i*3;
    enqueue(test_problem_queue, test_problem);
  }  
  ASSERT_EQ(is_full(test_problem_queue), 1);

  /* Try to append another unique node, hoping that it fails */
  Problem_Instance *test_problem = (Problem_Instance *) malloc(sizeof(Problem_Instance));
  test_problem->parent = NULL;
  test_problem->lower_bound = n*3;
  test_problem->upper_bound = n*3;
  int size_before = test_problem_queue->size;
  enqueue(test_problem_queue, test_problem);
  int size_after = test_problem_queue->size;
  ASSERT_EQ(size_before, size_after);

  /* Check the whole queue and make sure that the node isn't in it */
  for(int i = 0; i < n; i++)
    ASSERT_NE(test_problem_queue->array[i]->lower_bound, test_problem->lower_bound);

  /* Clean up */
  free(test_problem);
  free(test_problem_queue);
}

TEST(problem_instance_queueTest, is_emptyTest){
  /* Make sure, given a randomly populated problem queue of n items, that the
   * queue will correctly recognise when it has no more elements after n 
   * dequeue operations. */
  srand(time(NULL));

  /* Randomly generate the queue capacity */
  int capacity = rand() % 10000000 + 1;

  /* Create the queue */
  Problem_Queue *test_problem_queue = create_queue(capacity);

  /* Assert that its empty */
  ASSERT_EQ(is_empty(test_problem_queue), 1);

  /* Populate it with at most as many unique nodes as there is capacity */
  /* Enqueue the nodes and make sure that the queue thinks it is full */
  int n = rand() % capacity + 1; 
  for(int i = 0; i < n; i++)
  {
    Problem_Instance *test_problem = (Problem_Instance *) malloc(sizeof(Problem_Instance));
    test_problem->parent = NULL;
    test_problem->lower_bound = i*3;
    test_problem->upper_bound = i*3;
    enqueue(test_problem_queue, test_problem);
  }  
  
  /* Dequeue while not empty */
  int counter = 0;
  while(!is_empty(test_problem_queue) && (counter <= 2*n))
  {
    Problem_Instance *test_problem = dequeue(test_problem_queue);
    free(test_problem);
    counter++;
  }

  /* Make sure we dequeued as many as we put in */
  ASSERT_EQ(counter, n);

  /* Clean up */
  free(test_problem_queue);
}

TEST(problem_instance_queueTest, frontTest){
  /* Make sure, given a randomly populated queue of problem nodes, that the 
   * method which identifies the front of the queue operates as expected. */
  /* Randomly generate queue capacity */
  int capacity = rand() % 10000000;

  /* Create queue */
  Problem_Queue *test_problem_queue = create_queue(capacity);  

  /* Generate a node and save its pointer, then enqueue it */
  Problem_Instance *first_test_problem =
                         (Problem_Instance *) malloc(sizeof(Problem_Instance));
  enqueue(test_problem_queue, first_test_problem);

  /* Generate a random number of nodes from 0 to capacity-1 */
  int n = rand() % (capacity - 1);  
  for(int i = 0; i < n; i++)
  {
    Problem_Instance *arbitrary_test_problem = 
                         (Problem_Instance *) malloc(sizeof(Problem_Instance));
    enqueue(test_problem_queue, arbitrary_test_problem); 
  }

  /* Make sure that the front is the same structure as saved before */
  ASSERT_EQ(test_problem_queue->array[test_problem_queue->front], first_test_problem);
  
  /* Cleanup */
  while(!is_empty(test_problem_queue))
  {
    Problem_Instance *dequeued_test_problem = dequeue(test_problem_queue);
    free(dequeued_test_problem);
  }
  free(test_problem_queue);
}

TEST(problem_instance_queueTest, rearTest){
  /* Make sure, given a randomlmy populated queue of problem nodes, that the 
   * method which identifies the rear of the queue operates as expected. */
  /* Randomly generate queue capacity */
  int capacity = rand() % 10000000;

  /* Create queue */
  Problem_Queue *test_problem_queue = create_queue(capacity); 

  /* Generate a random number of nodes from 0 to capacity-1 */
  int n = rand() % (capacity - 1);
  for(int i = 0; i < n; i++)
  {
    Problem_Instance *arbitrary_test_problem = 
                         (Problem_Instance *) malloc(sizeof(Problem_Instance));
    enqueue(test_problem_queue, arbitrary_test_problem);
  }

  /* Generate a node and save its pointer */
  Problem_Instance *last_test_problem = (Problem_Instance *) malloc(sizeof(Problem_Instance)); 
  enqueue(test_problem_queue, last_test_problem);

  /* Make sure that the rear points to the same structure as this saved 
   * pointer */
  ASSERT_EQ(test_problem_queue->array[test_problem_queue->rear], last_test_problem);

  /* Cleanup */
  while(!is_empty(test_problem_queue))
  {
    Problem_Instance *dequeued_test_problem = dequeue(test_problem_queue);
    free(dequeued_test_problem);
  }
  free(test_problem_queue);
}

/* Branch and bound test cases */

TEST(branch_and_bound_mechanismTest, find_heuristic_initial_glbTest){
  /* Make sure, given a problem instance with known optimal objective value
   * that the heuristic initial GLB is indeed less than it. */
  /* Pisinger Reader version */
  int *profits, *weights, *x;
  int n, z, capacity;
  char problem_file[100];

  /* Edit problem file here */
  char file[] = "knapPI_3_50_1000.csv";

  /* Read problem */
  strcpy(problem_file, file);
  pisinger_reader(&n, &capacity, &z, &profits, &weights, &x, problem_file, 1);

  /* Derive initial GLB */
  int GLB = find_heuristic_initial_GLB(profits, weights, x, z, n, capacity, 
                                      problem_file);  

  /* Assert z >= GLB */
  ASSERT_GE(z, GLB);
}

TEST(branch_and_bound_mechanismTest, find_boundsTest){
  /* Make sure, given a problem instance with known optimal objective value
   * that the return UB and LB are in the correct relative positions. */
  /* Input variables */
  int *profits, *weights, *x;
  int n, z, capacity;
  char problem_file[100];
  int DP_method = WILLIAMSON_SHMOY;
  int logging_rule = NO_LOGGING;
  FILE *logging_stream = stdout;
  double eps = 0.02;

  /* Edit problem file here */
  char file[] = "knapPI_3_50_1000.csv";

  /* Read problem */
  strcpy(problem_file, file);
  pisinger_reader(&n, &capacity, &z, &profits, &weights, &x, problem_file, 1);

  /* Define test problem instance */
  Problem_Instance *test_problem = (Problem_Instance *) malloc(sizeof(Problem_Instance));
  test_problem->parent = NULL;
  test_problem->variable_statuses = (int *) malloc(sizeof(int)*n);
  for(int i = 0; i < n; i++) test_problem->variable_statuses[i] = VARIABLE_UNCONSTRAINED;
  test_problem->lower_bound = 0;  
  test_problem->upper_bound = 0;

  /* Derive bounds */ 
  find_bounds(test_problem, profits, weights, x, capacity, n, z, 
              &test_problem->lower_bound, &test_problem->upper_bound, 
              problem_file, DP_method, logging_rule, logging_stream, eps);

  /* Assert UB >= z >= LB*/
  ASSERT_GE(test_problem->upper_bound, z);
  ASSERT_GE(z, test_problem->lower_bound);

  /* Cleanup */
  free(test_problem->variable_statuses);
  free(test_problem);
}

TEST(branch_and_bound_mechanismTest, generate_and_enqueue_nodesTest){
  /* Make sure, given a problem queue and a current problem instance structure,
   * assuming an abitrary variable to branch on, that the algorithm correctly
   * generates two subinstances: one with the variable constrained on and the
   * other constrained off.*/
  /* Generate a problem with a random n */
  //int n = rand() % 10000 + 50;
  int n = 10;
  int queue_capacity = n;
  int count = 0;
  FILE *logging_stream = stdout;
  int logging_rule = NO_LOGGING;

  /* Create an empty queue */
  //Problem_Queue *test_problem_queue = create_queue(queue_capacity);
  LL_Problem_Queue *test_problem_queue = LL_create_queue();

  /* Generate a random number to represent the variable to branch on */
  int branching_var = rand() % n;

  /* Make problem instance to be branched from */
  Problem_Instance *test_problem_instance = 
                          (Problem_Instance *) malloc(sizeof(Problem_Instance));
  test_problem_instance->parent = NULL;
  test_problem_instance->variable_statuses = (int *) malloc(sizeof(int)*n);
  for(int i = 0; i < n; i++) test_problem_instance->variable_statuses[i] = VARIABLE_UNCONSTRAINED; 
  test_problem_instance->lower_bound = 0;
  test_problem_instance->upper_bound = 30;


  /* Make expected versions of the lists */
  /* First case */
  int *expected_variable_statuses_on = (int *) malloc(sizeof(int)*n);
  for(int i = 0; i < n; i++) expected_variable_statuses_on[i] = VARIABLE_UNCONSTRAINED;
  expected_variable_statuses_on[branching_var] = VARIABLE_ON;
  
  /* Just assertions to make sure this worked */
  for(int i = 0; i < n; i++)
  {
    if(i == branching_var)
      ASSERT_EQ(expected_variable_statuses_on[i], VARIABLE_ON);
    else ASSERT_EQ(expected_variable_statuses_on[i], VARIABLE_UNCONSTRAINED);
  }

  /*Second Case*/
  int *expected_variable_statuses_off = (int *) malloc(sizeof(int)*n);
  for(int i = 0; i < n; i++) expected_variable_statuses_off[i] = VARIABLE_UNCONSTRAINED;
  expected_variable_statuses_off[branching_var] = VARIABLE_OFF;

  /* And more assertions to make sure this worked */
  for(int i = 0; i < n; i++)
  {
    if(i == branching_var)
      ASSERT_EQ(expected_variable_statuses_off[i], VARIABLE_OFF);
    else ASSERT_EQ(expected_variable_statuses_off[i], VARIABLE_UNCONSTRAINED);
  }


  /* Run generate_and_enqueue */
  generate_and_enqueue_nodes(test_problem_instance, n, branching_var, test_problem_queue, &count, logging_stream, logging_rule);


  /* Compare the lists, asserting equality */
  /* First list */
  //Problem_Instance *child_problem_instance = dequeue(test_problem_queue);
  Problem_Instance *child_problem_instance = LL_dequeue(test_problem_queue);
  for(int i = 0; i < n; i++)
    ASSERT_EQ(child_problem_instance->variable_statuses[i], expected_variable_statuses_on[i]);
  free(child_problem_instance->variable_statuses);
  free(child_problem_instance); 

  /* Second list  */
  //child_problem_instance = dequeue(test_problem_queue);
  child_problem_instance = LL_dequeue(test_problem_queue);
  for(int i = 0; i < n; i++)
    ASSERT_EQ(child_problem_instance->variable_statuses[i], expected_variable_statuses_off[i]);
  free(child_problem_instance->variable_statuses);
  free(child_problem_instance);

  /* Clean up */
  free(test_problem_queue);
  free(test_problem_instance->variable_statuses);
  free(test_problem_instance);
}

TEST(branch_and_bound_mechanismTest, select_and_dequeue_nodesTest){
  /* Make sure, given a list of nodes, that the select_and_dequeue algorithm 
   * both correctly returns a problem instance structure and ensures it is no
   * longer a part of the node list. One note to self: this is only going to 
   * pass for thee b&b which has a FIFO strategy for node selection. */
  /* Generate a queue and some amount of problems and put them into the queue*/
  int queue_capacity = rand() % 1000000; 
  //Problem_Queue *test_problem_queue = create_queue(queue_capacity);
  LL_Problem_Queue *test_problem_queue = LL_create_queue();
  int no_problems = rand() % queue_capacity;
  Problem_Instance *first_instance;
  LL_enqueue(test_problem_queue, first_instance, stdout, NO_LOGGING);
  for (int i = 0; i < no_problems-1; i++)
  {
    Problem_Instance *arbitrary_instance;
    LL_enqueue(test_problem_queue, arbitrary_instance, stdout, NO_LOGGING);
  } 

  /* Select and dequeue the node */
  Problem_Instance *dequeued_instance = select_and_dequeue_node(test_problem_queue);
 
  /* Ensure that queue returned the expected instance */
  ASSERT_EQ(dequeued_instance, first_instance);

  /* Ensure that it isn't in the list anymore 
  while(!is_empty(test_problem_queue))
  {
    Problem_Instance *a_problem = dequeue(test_problem_queue);
    ASSERT_NE(a_problem, first_instance);
  }*/
}

/* Tests for computing the optimal value*/
TEST(branch_and_bound_mechanismTest, returns_optimal_value_uncorrelated){
  /* Input variables */
  int *profits, *weights, *x;
  int n, z, capacity;
  char problem_file[100];
  int branching_strategy = TRUNCATION_BRANCHING;
  int seed = 0;
  int DP_method = WILLIAMSON_SHMOY;
  int logging_rule = NO_LOGGING;
  FILE *logging_stream = stdout;
  double eps = 0.02;
  int number_of_nodes = -1;
  int memory_allocation_limit = -1;
  clock_t t = clock();
  int timeout = -1;

  /* Edit problem file here */
  char file[] = "knapPI_1_50_1000.csv";

  /* Read problem */
  strcpy(problem_file, file);
  pisinger_reader(&n, &capacity, &z, &profits, &weights, &x, problem_file, 1);

  /* Output variables */
  int z_out = 0;
  int sol_out[n];
  

  branch_and_bound_bin_knapsack(profits, weights, x, capacity, z, &z_out, sol_out, n, problem_file,
                                branching_strategy, seed, DP_method, logging_rule, logging_stream, eps, 
                                &number_of_nodes, memory_allocation_limit, &t, timeout); 

  ASSERT_EQ(z_out, z);
}

TEST(branch_and_bound_mechanismTest, returns_optimal_value_weaklycorrelated){
  /* Input variables */
  int *profits, *weights, *x;
  int n, z, capacity;
  char problem_file[100];
  int branching_strategy = TRUNCATION_BRANCHING;
  int seed = 0;
  int DP_method = WILLIAMSON_SHMOY;
  int logging_rule = NO_LOGGING;
  FILE *logging_stream = stdout;
  double eps = 0.02;
  int number_of_nodes = -1;
  int memory_allocation_limit = -1;
  clock_t t = clock();
  int timeout = -1;

  /* Edit problem file here */
  char file[] = "knapPI_2_50_1000.csv";

  /* Read problem */
  strcpy(problem_file, file);
  pisinger_reader(&n, &capacity, &z, &profits, &weights, &x, problem_file, 1);

  /* Output variables */
  int z_out = 0;
  int sol_out[n];

  /* Solve */
  branch_and_bound_bin_knapsack(profits, weights, x, capacity, z, &z_out, sol_out, n, problem_file,
                                branching_strategy, seed, DP_method, logging_rule, logging_stream, eps, 
                                &number_of_nodes, memory_allocation_limit, &t, timeout); 
  ASSERT_EQ(z_out, z);
}

TEST(branch_and_bound_mechanismTest, returns_optimal_value_stronglycorrelated){
  /* Input variables */
  int *profits, *weights, *x;
  int n, z, capacity;
  char problem_file[100];
  int branching_strategy = TRUNCATION_BRANCHING;
  int seed = 0;
  int DP_method = WILLIAMSON_SHMOY;
  int logging_rule = NO_LOGGING;
  FILE *logging_stream = stdout;
  double eps = 0.02;
  int number_of_nodes = -1;
  int memory_allocation_limit = -1;
  clock_t t = clock();
  int timeout = -1;

  /* Edit problem file here */
  char file[] = "knapPI_3_50_1000.csv";

  /* Read problem */
  strcpy(problem_file, file);
  pisinger_reader(&n, &capacity, &z, &profits, &weights, &x, problem_file, 1);

  /* Output variables */
  int z_out = 0;
  int sol_out[n];

  /* Solve */
  branch_and_bound_bin_knapsack(profits, weights, x, capacity, z, &z_out, sol_out, n, problem_file, 
                                branching_strategy, seed, DP_method, logging_rule, logging_stream, eps, 
                                &number_of_nodes, memory_allocation_limit, &t, timeout); 
  ASSERT_EQ(z_out, z);
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

/* Pisinger instance reader */


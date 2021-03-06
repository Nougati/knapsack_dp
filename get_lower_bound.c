#include <stdio.h>
#include "kellerer_pferschy_fptas.h"

int main(int argc, char *argv[])
{
/*
int profits[] = {15094, 24506, 94416, 40992, 66649, 49237, 96457, 67815, 19446,
                 63422, 88791, 49359, 45667, 31598, 82007, 20544, 85334, 82766,
                 93994, 59893, 62633, 87131, 5428, 76700, 30617, 15874, 77720, 
                 74419, 69794, 28196, 95997, 83116, 15908, 55539, 45707, 38569,
                 25537, 90931, 55726, 75487, 59772, 67513, 52081, 29943, 88058,
                 84303, 13764, 6536, 90724, 63789};

int weights[] = {485, 56326, 79248, 45421, 80322, 15795, 58043, 42845, 24955, 
                49252, 61009, 25901, 81122, 81094, 38738, 88574, 65715, 78882,
                31367, 59984, 73299, 49433, 15682, 90072, 97874, 138, 53856,
                87145, 37995, 91529, 36199, 83277, 80097, 59719, 35242, 36107,
                41122, 41070, 76098, 53600, 36645, 7267, 41972, 9895, 83213, 
                99748, 89487, 71923, 17029, 2567};
  int n = 50;
  int capacity = 99748;
*/
  int profits[] = {94, 506, 416, 992, 649, 237, 457, 815, 446, 422};
  int weights[] = {485, 326, 248, 421, 322, 795, 43, 845, 955, 252};
  int capacity = 1850;
  int n = 10;
  double epsilon = 0.5;

  int lower = 0;
  double upper = 0.0;

  get_knapsack_lowerbound(profits, weights, n, capacity, &lower, &upper);

  printf("Solution is between [%d, %d]\n", lower,(int)upper);
  return 0;
}

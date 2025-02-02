/* Latency tests                                                            */

/* Copyright (c) 2007, 2009-2011, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

#include "simdag/simdag.h"

/*
 * simple latency test
 * send one byte from 0 to 1 
 * 
 * this is a test for multiple platforms
 * see tesh file for expected output
 * 
 */
int main(int argc, char **argv)
{
  double time;
  double communication_amount[] = { 0.0, 1.0, 0.0, 0.0 };
  const double no_cost[] = { 0.0, 0.0 };
  SD_task_t task;
  xbt_dynar_t ret;

  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  task = SD_task_create("Comm 1", NULL, 1.0);

  SD_task_schedule(task, 2, SD_workstation_get_list(), no_cost,
                   communication_amount, -1.0);

  ret = SD_simulate(-1.0);
  xbt_dynar_free(&ret);

  time = SD_get_clock();

  printf("%g\n", time);
  fflush(stdout);

  SD_task_destroy(task);

  SD_exit();

  return 0;
}

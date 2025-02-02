/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

#include <xbt.h>
#include <simgrid/simix.h>

#include "mc_base.h"
#include "mc_record.h"

#ifdef HAVE_MC
#include "mc_private.h"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_record, mc,
  " Logging specific to MC record/replay facility");

char* MC_record_path = NULL;

void MC_record_replay(mc_record_item_t start, size_t len)
{
  MC_wait_for_requests();
  mc_record_item_t end = start + len;

  // Choose the recorded simcall and execute it:
  for (mc_record_item_t item=start;item!=end; ++item) {

    XBT_DEBUG("Executing %i$%i", item->pid, item->value);
/*
    if (xbt_dynar_is_empty(simix_global->process_to_run))
      xbt_die("Unexpected end of application.");
*/

    // Choose a request:
    smx_process_t process = SIMIX_process_from_PID(item->pid);
    if (!process)
      xbt_die("Unexpected process.");
    smx_simcall_t simcall = &(process->simcall);
    if(!simcall || simcall->call == SIMCALL_NONE)
      xbt_die("No simcall for this process.");
    if (!MC_request_is_visible(simcall) || !MC_request_is_enabled(simcall))
      xbt_die("Unexpected simcall.");

    // Execute the request:
    SIMIX_simcall_handle(simcall, item->value);
    MC_wait_for_requests();
  }
}

xbt_dynar_t MC_record_from_string(const char* data)
{
  XBT_INFO("path=%s", data);
  if (!data || !data[0])
    return NULL;

  xbt_dynar_t dynar = xbt_dynar_new(sizeof(s_mc_record_item_t), NULL);

  const char* current = data;
  while (*current) {

    s_mc_record_item_t item = { 0, 0 };
    int count = sscanf(current, "%u/%u", &item.pid, &item.value);
    if(count != 2 && count != 1)
      goto fail;
    xbt_dynar_push(dynar, &item);

    // Find next chunk:
    char* end = strchr(current, ';');
    if(end==NULL)
      break;
    else
      current = end + 1;
  }

  return dynar;

fail:
  xbt_dynar_free(&dynar);
  return NULL;
}

#ifdef HAVE_MC
char* MC_record_stack_to_string(xbt_fifo_t stack)
{
  xbt_fifo_item_t start = xbt_fifo_get_last_item(stack);

  if (!start) {
    char* res = (char*) malloc(1 * sizeof(char));
    res[0] = '\0';
    return res;
  }

  char* buffer;
  size_t size;
  FILE* file = open_memstream(&buffer, &size);

  xbt_fifo_item_t item;
  for (item = start; item; item = xbt_fifo_get_prev_item(item)) {

    // Find (pid, value):
    mc_state_t state = (mc_state_t) xbt_fifo_get_item_content(item);
    int value = 0;
    smx_simcall_t saved_req = MC_state_get_executed_request(state, &value);
    int pid = saved_req->issuer->pid;

    // Serialization the (pid, value) pair:
    const char* sep = (item!=start) ? ";" : "";
    if (value)
      fprintf(file, "%s%u/%u", sep, pid, value);
    else
      fprintf(file, "%s%u", sep, pid);
  }

  fclose(file);
  return buffer;
}

void MC_record_dump_path(xbt_fifo_t stack)
{
  if (MC_record_is_active()) {
    char* path = MC_record_stack_to_string(stack);
    XBT_INFO("Path = %s", path);
    free(path);
  }
}
#endif

void MC_record_replay_from_string(const char* path_string)
{
  xbt_dynar_t path = MC_record_from_string(path_string);
  mc_record_item_t start = &xbt_dynar_get_as(path, 0, s_mc_record_item_t);
  MC_record_replay(start, xbt_dynar_length(path));
  xbt_dynar_free(&path);
}

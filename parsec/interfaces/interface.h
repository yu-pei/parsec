/**
 * Copyright (c) 2016      The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 */

#ifndef PARSEC_INTERFACE_H_HAS_BEEN_INCLUDED
#define PARSEC_INTERFACE_H_HAS_BEEN_INCLUDED

#include "parsec_config.h"
#include "parsec/parsec_internal.h"

/**
 * Generic startup function for DSLs. For more info read comment in
 * parsec/interface/interface.c
 */
PARSEC_DECLSPEC extern const parsec_function_t __parsec_generic_startup;

/* Functions to return tasks to their mempool once their execution is
 * completed. The fist one should be used when counting the tasks is
 * not necessary, while the second one contains the task counting.
 */
parsec_hook_return_t
parsec_release_task_to_mempool(parsec_execution_unit_t *eu,
                              parsec_execution_context_t *this_task);
parsec_hook_return_t
parsec_release_task_to_mempool_update_nbtasks(parsec_execution_unit_t *eu,
                                             parsec_execution_context_t *this_task);

parsec_hook_return_t
parsec_release_task_to_mempool_and_count_as_runtime_tasks(parsec_execution_unit_t *eu,
                                                         parsec_execution_context_t *this_task);
#endif  /* PARSEC_INTERFACE_H_HAS_BEEN_INCLUDED */
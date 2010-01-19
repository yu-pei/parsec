/*
 * Copyright (c) 2009      The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 */

#ifndef _dplasma_h
#define _dplasma_h

typedef struct dplasma_t dplasma_t;

#define MAX_LOCAL_COUNT  5
#define MAX_PRED_COUNT   5
#define MAX_PARAM_COUNT  10

#include <stdint.h>
#include <stdlib.h>
#include "symbol.h"
#include "expr.h"
#include "params.h"
#include "dep.h"
#include "execution_unit.h"
#include "lifo.h"

#ifdef _DEBUG
#   ifdef USE_MPI
#include <mpi.h>
#define DEBUG(ARG)  do { \
    int rank; \
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); \
    printf("[%d]\t", rank); \
    printf ARG ;\
} while(0)
#   else
#define DEBUG(ARG) printf ARG
#   endif
#else
#define DEBUG(ARG)
#endif

/* There is another loop after this one. */
#define DPLASMA_DEPENDENCIES_FLAG_NEXT       0x01
/* This is the final loop */
#define DPLASMA_DEPENDENCIES_FLAG_FINAL      0x02
/* This loops array is allocated */
#define DPLASMA_DEPENDENCIES_FLAG_ALLOCATED  0x04

/* TODO: Another ugly hack. The first time the IN dependencies are
 *       checked leave a trace in order to avoid doing it again.
 */
#define DPLASMA_DEPENDENCIES_HACK_IN         0x80

typedef struct dplasma_dependencies_t dplasma_dependencies_t;
typedef union {
    unsigned int            dependencies[1];
    dplasma_dependencies_t* next[1];
} dplasma_dependencies_union_t;

struct dplasma_dependencies_t {
    int                     flags;
    symbol_t*               symbol;
    int                     min;
    int                     max;
    dplasma_dependencies_t* prev;
    /* keep this as the last field in the structure */
    dplasma_dependencies_union_t u; 
};

typedef struct dplasma_execution_context_t dplasma_execution_context_t;
typedef int (dplasma_hook_t)(struct dplasma_execution_unit_t*, const dplasma_execution_context_t*);

#define DPLASMA_HAS_IN_IN_DEPENDENCIES     0x0001
#define DPLASMA_HAS_OUT_OUT_DEPENDENCIES   0x0002
#define DPLASMA_HAS_IN_STRONG_DEPENDENCIES 0x0004
struct dplasma_t {
    const char*             name;
    uint16_t                flags;
    uint16_t                dependencies_mask;
    uint16_t                nb_locals;
    uint16_t                nb_params;
    symbol_t*               params[MAX_LOCAL_COUNT];
    symbol_t*               locals[MAX_LOCAL_COUNT];
    expr_t*                 preds[MAX_PRED_COUNT];
    param_t*                inout[MAX_PARAM_COUNT];
    dplasma_dependencies_t* deps;
    dplasma_hook_t*         hook;
    char*                   body;
};

struct dplasma_execution_context_t {
    dplasma_list_item_t list_item;
    dplasma_t*   function;
    assignment_t locals[MAX_LOCAL_COUNT];
};

/**
 *
 */
dplasma_context_t* dplasma_init( int nb_cores, int* pargc, char** pargv[] );

/**
 *
 */
int dplasma_fini( dplasma_context_t** );

/**
 * Dump the content of a dplams_t object.
 */
void dplasma_dump(const dplasma_t *d, const char *prefix);

/**
 * Dump all defined dplasma_t obejcts.
 */
void dplasma_dump_all( void );

/**
 * helper to get the index of a dplasma object when dumping in the C-like format
 */
int dplasma_dplasma_index( const dplasma_t *d );

/**
 * Add the dplasma_t object to a global list.
 */
int dplasma_push( const dplasma_t* d );

/**
 * Find a dplasma_t object by name. If no object with such a name
 * exist return NULL.
 */
const dplasma_t* dplasma_find( const char* name );

/**
 * Find a dplasma_t object by name. If no object with such a name
 * exist one will be created and automatically added to the global
 * list.
 */
dplasma_t* dplasma_find_or_create( const char* name );

/**
 * Return the i'th dplasma_t object. If no such element exists
 * return NULL.
 */
const dplasma_t* dplasma_element_at( int i );

/**
 * Returns the number of elements in the dplasma_array
 */
int dplasma_nb_elements( void );

/**
 * Compute the correct initial values for an execution context. These values
 * are in the range and validate all possible predicates. If such values do
 * not exist this function returns -1.
 *
 * @param [INOUT] The execution context to be initialized.
 *
 * @return  0 If the execution context has been correctly setup
 * @return -1 If no suitable values for this execution context can be found.
 */
int dplasma_set_initial_execution_context( dplasma_execution_context_t* exec_context );

/**
 * Release all OUT dependencies for this particular instance of the service.
 * @param [IN] The execution context which just completed
 * @param [IN] The name of the released parameter at the source
 * @param [INOUT] The execution context used as destination
 * @param [IN] The name of the parameter at the destination
 * @param [IN[ Allow forwarding to remote processes of dependency resolution
 */
int dplasma_release_OUT_dependencies( dplasma_execution_unit_t* eu_context,
                                      const dplasma_execution_context_t* origin,
                                      const param_t* origin_param,
                                      dplasma_execution_context_t* exec_context,
                                      const param_t* dest_param, 
                                      int forward_remote );

/**
 * Check is there is any of the input parameters that do depend on some
 * other service.
 *
 * @param [INOUT] The execution context where the check will be started. On return
 *                it will contain the updated values for a valid execution context
 *                where all predicats are valid.
 *
 * @return  0 If a execution context that can be used as startup has been found, and
 *            in this case the parameter contain the right values.
 * @return -1 If no other execution context that can be used as a startup exists.
 */
int dplasma_service_can_be_startup( dplasma_execution_context_t* exec_context );

/**
 * Convert the execution context to a string.
 *
 * @param [IN]    the context to be printed out
 * @param [INOUT] the string where the output will be added
 * @param [IN]    the number of characters available on the string.
 */
char* dplasma_service_to_string( const dplasma_execution_context_t* exec_context,
                                 char* tmp,
                                 size_t length );

/**
 * Convert a dependency to a string under the format X(...) -> Y(...).
 *
 * @param [IN]    the source of the dependency context
 * @param [IN]    the destination of the dependency context
 * @param [INOUT] the string where the output will be added
 * @param [IN]    the number of characters available on the string.
 */
char* dplasma_dependency_to_string( const dplasma_execution_context_t* from,
                                    const dplasma_execution_context_t* to,
                                    char* tmp,
                                    size_t length );

/**
 * Compute the total number of tasks for a dplasma_object. If use_predicates
 * is true then the predicates will be applied, otherwise it compute the
 * total number of tasks.
 */
int dplasma_compute_nb_tasks( const dplasma_t* object, int use_predicates );

void dplasma_load_array( dplasma_t *array, int size );

/**
 * This is the interface used by DPlasma Libraries:
 *
 * Once per process:
 *  The DPLASMA has to be initialized with a call to dplasma_init.
 *  Then, load_dplasma_objects(); will load all data structures and symbols into memory
 *  Then, the library can use dplasma_assign_global_symbol(...); to assign values to symbols that were not defined statically in the jdf file
 *  When this is done, the library callls load_dplasma_hooks(); to assign the hooks to the objects using all symbols to compute expressions
 *  Finally, the library calls enumerate_dplasma_tasks(); to prepare the stopping condition of the scheduling engine
 *  Potentially, the library can call dplasma_profiling_init(...); to initialize profiling
 *  A single thread should also dplasma_find(...) the entry point of scheduling and dplasma_schedule(...); it
 * Then, each thread that participate to this scheduling should call dplasma_progress();
 * When all threads have exited dplasma_progress(); the job is done.
 *
 * load_dplasma_hooks, load_dplasma_objects, enumerate_dplasma_tasks are created by the compiler.
 * all the other functions are part of the dplasma runtime library.
 */

/**
 * Prepare the dplasma data structure into memory.
 * @return 0 if everything was okay
 * Generated by the Dplasma Compiler from a jdf file.
 */
int load_dplasma_objects( dplasma_context_t* context );

/**
 * Assign the hooks to the dplasma objects that have just been loaded into memory
 * @return 0 if everything was okay
 * Generated by the Dplasma Compiler from a jdf file.
 */
int load_dplasma_hooks( dplasma_context_t* context );

/**
 * Enumerate the number of tasks to be run by this processor
 * Store this number to define the stopping condition of the scheduler
 * @return the number of tasks to be run by this processor
 * Generated by the Dplasma Compiler from a jdf file.
 */
int enumerate_dplasma_tasks( void );

#endif

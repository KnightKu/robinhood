/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */
/*
 * Copyright (C) 2004-2009 CEA/DAM
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the CeCILL License.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license (http://www.cecill.info) and that you
 * accept its terms.
 */

/**
 *  \file RobinhoodConfig.h
 *  \brief Module for configuration management and parsing.
 *
 * \addtogroup CONFIG_PARSING
 * @{
 */

#ifndef _ROBINHOOD_CONFIG_H
#define _ROBINHOOD_CONFIG_H

#include "config_parsing.h"
#include <sys/types.h>
#include <stdio.h>
#include <limits.h>

/**
 * Generic definitions for config management
 * \addtogroup MODULE_CONFIG_FUNCTIONS
 * @{
 */
typedef int    ( *set_default_config_func_t ) ( void *module_config, char *msg_out );
typedef int    ( *read_config_func_t ) ( config_file_t config,
                                         void *module_config, char *msg_out, int for_reload );
typedef int    ( *reload_config_func_t ) ( void *module_config );
typedef int    ( *write_config_func_t ) ( FILE * output );
/** @} */

#include "list_mgr.h"
#include "RobinhoodLogs.h"
#include "global_config.h"
#include "entry_processor.h"
#include "policy_rules.h"
#include "policy_run.h"
#include "fs_scan_main.h"
#include "update_params.h"

//#ifdef HAVE_RMDIR_POLICY
//#include "rmdir.h"
//#endif

#ifdef HAVE_CHANGELOGS
#include "chglog_reader.h"
#endif

//#ifdef HAVE_MIGR_POLICY
//#include "migration.h"
//#endif

//#ifdef HAVE_RM_POLICY
//#include "hsm_rm.h"
//#endif

#ifdef _BUDDY_MALLOC
#include "BuddyMalloc.h"
#endif

#ifdef _HSM_LITE
#include "backend_mgr.h"
#endif

typedef struct module_config_def_t
{
    char          *module_name;
    set_default_config_func_t set_default_func;
    read_config_func_t read_func;
    reload_config_func_t reload_func;
    write_config_func_t write_template_func;
    write_config_func_t write_default_func;
    off_t          module_config_offset;
    unsigned int   flags;
} module_config_def_t;

/**
 * This structure groups the configuration of all modules
 */
typedef struct robinhood_config_t
{
    /** Global configuration */
    global_config_t global_config;

    /** Log configuration */
    log_config_t   log_config;

    /** List Manager Configuration */
    lmgr_config_t  lmgr_config;

    /** Database update strategy */
    updt_params_t  db_update_params;

    /** Entry processor pipeline configuration */
    entry_proc_config_t entry_proc_config;

    /** FS scan configuration */
    fs_scan_config_t fs_scan_config;

#ifdef HAVE_CHANGELOGS
    chglog_reader_config_t chglog_reader_config;
#endif

    /** policies (migration, purge, rmdir, unlink...) */
    policies_t              policies;

    /** policy run parameters (same order as policies) */
    policy_run_configs_t    policy_run_cfgs;

#if 0
#ifdef HAVE_RMDIR_POLICY
    /** rmdir parameters */
    rmdir_config_t rmdir_config;
#endif

#ifdef HAVE_RM_POLICY
    /** hsm removal parameters */
    hsm_rm_config_t hsm_rm_config;
#endif
#endif

#ifdef _HSM_LITE
    backend_config_t backend_config;
#endif

} robinhood_config_t;

/* the config file for the current process */
extern char *process_config_file;

/* behavior flags for all modules */
#define FLAG_DRY_RUN    0x00000001
#define FLAG_IGNORE_POL 0x00000002
#define FLAG_ONCE       0x00000004
#define FLAG_NO_LIMIT   0x00000008
#define FLAG_CHECK_ONLY 0x00000010 /* only check triggers, don't purge */
#define FLAG_NO_GC      0x00000020 /* don't clean orphan entries after scan */
#define FLAG_FORCE_RUN  0x00000040 /* force running policy even if no scan was complete */

/* Config module masks:
 * Global, Log, and List Manager are always initialized.
 * Entry processor, Info Collector, policy runs are optional
 * are optionnal.
 */
#define MODULE_MASK_ENTRY_PROCESSOR 0x00000001
#define MODULE_MASK_FS_SCAN         0x00000002
#define MODULE_MASK_EVENT_HDLR      0x00000004
#define MODULE_MASK_POLICY_RUN      0x00000008

#define MODULE_MASK_ALWAYS          0x10000000


/**
 * Array of configuration routines for all modules.
 */
static const module_config_def_t robinhood_module_conf[] = {
    {"Global", SetDefaultGlobalConfig, ReadGlobalConfig, ReloadGlobalConfig,
     WriteGlobalConfigTemplate, WriteGlobalConfigDefault,
     offsetof( robinhood_config_t, global_config ), MODULE_MASK_ALWAYS},
    {"Log", SetDefaultLogConfig, ReadLogConfig, ReloadLogConfig,
     WriteLogConfigTemplate, WriteLogConfigDefault,
     offsetof( robinhood_config_t, log_config ), MODULE_MASK_ALWAYS},
#ifdef _HSM_LITE
    {"Backend", SetDefault_Backend_Config, Read_Backend_Config,
        Reload_Backend_Config, Write_Backend_ConfigTemplate,
        Write_Backend_ConfigDefault,
        offsetof( robinhood_config_t, backend_config ),
        MODULE_MASK_ALWAYS
    },
#endif
    {"List Manager", SetDefaultLmgrConfig, ReadLmgrConfig, ReloadLmgrConfig,
     WriteLmgrConfigTemplate, WriteLmgrConfigDefault,
     offsetof( robinhood_config_t, lmgr_config ), MODULE_MASK_ALWAYS},

    {"Attr Update", set_default_update_params, read_update_params, reload_update_params,
     write_update_params_template, write_default_update_params,
     offsetof(robinhood_config_t, db_update_params), MODULE_MASK_ALWAYS},

    {"Entry Processor", SetDefault_EntryProc_Config, Read_EntryProc_Config,
     Reload_EntryProc_Config, Write_EntryProc_ConfigTemplate,
     Write_EntryProc_ConfigDefault, offsetof( robinhood_config_t,
                                              entry_proc_config ),
     MODULE_MASK_ENTRY_PROCESSOR},

    {"FS Scan", FSScan_SetDefaultConfig, FSScan_ReadConfig, FSScan_ReloadConfig,
     FSScan_WriteConfigTemplate, FSScan_WriteDefaultConfig,
     offsetof( robinhood_config_t, fs_scan_config ), MODULE_MASK_FS_SCAN},
#ifdef HAVE_CHANGELOGS
    {"ChangeLog Reader", ChgLogRdr_SetDefaultConfig, ChgLogRdr_ReadConfig, ChgLogRdr_ReloadConfig,
     ChgLogRdr_WriteConfigTemplate, ChgLogRdr_WriteDefaultConfig,
     offsetof( robinhood_config_t, chglog_reader_config ), MODULE_MASK_EVENT_HDLR},
#endif

    /* TODO manage fileclasses separately? */

    {"Policies", SetDefault_Policies, Read_Policies, Reload_Policies,
     Write_Policy_Template, Write_Policy_Default,
     offsetof(robinhood_config_t, policies),
     MODULE_MASK_ALWAYS},

    {"PolicyRun", policy_run_cfg_set_default, policy_run_cfg_read,
     policy_run_cfg_reload, policy_run_cfg_write_template,
     policy_run_cfg_write_defaults, offsetof(robinhood_config_t, policy_run_cfgs),
     MODULE_MASK_POLICY_RUN},

#if 0
#ifdef HAVE_RMDIR_POLICY
    {"Directory Remover", SetDefault_Rmdir_Config, Read_Rmdir_Config,
     Reload_Rmdir_Config, Write_Rmdir_ConfigTemplate,
     Write_Rmdir_ConfigDefault, offsetof( robinhood_config_t, rmdir_config ),
     MODULE_MASK_RMDIR},
#endif

#ifdef HAVE_RM_POLICY
   {"HSM Remove", SetDefault_HSMRm_Config, Read_HSMRm_Config,
     Reload_HSMRm_Config, Write_HSMRm_ConfigTemplate,
     Write_HSMRm_ConfigDefault, offsetof( robinhood_config_t, hsm_rm_config ),
     MODULE_MASK_UNLINK},
#endif
#endif

    {NULL, NULL, NULL, NULL, NULL, NULL, 0, 0}

};


/**
 * Read robinhood's configuration file and fill config struct.
 * if everything is OK, returns 0 and fills the structure
 * else, returns an error code and sets a contextual error message in err_msg_out.
 */
int            ReadRobinhoodConfig( int module_mask, char *file_path,
                                    char *err_msg_out, robinhood_config_t * config_struct,
                                    int for_reload );

/**
 * Reload robinhood's configuration file (the one used for last call to ReadRobinhoodConfig),
 * and change only parameters that can be modified on the fly.
 */
int            ReloadRobinhoodConfig( int curr_module_mask,
                                      robinhood_config_t * new_config );

/**
 * Write a documented template of configuration file,
 * to the given file path.
 * returns 0 on success, else it returns a posix error code.
 */
int            WriteConfigTemplate( FILE * stream );

/**
 * Write all default configuration values,
 * to the given file path.
 * returns 0 on success, else it returns a posix error code.
 */
int            WriteConfigDefault( FILE * stream );

/**
 *  For debugging.
 */
void           DisplayConfiguration( FILE * stream, robinhood_config_t * config );

/* ==== Tools for writing config templates ==== */

void           print_begin_block( FILE * output, unsigned int indent, const char *blockname,
                                  const char *id );
void           print_end_block( FILE * output, unsigned int indent );
void           print_line( FILE * output, unsigned int indent, const char *format, ... );

/* ==== Tools for retrieving parameters from conf and checking them ==== */

/* constraint flags on parameters */

/* parameter flags */

#define PFLG_MANDATORY             (1 << 0)

#define PFLG_ABSOLUTE_PATH         (1 << 1)
#define PFLG_REMOVE_FINAL_SLASH    (1 << 2)
#define PFLG_NO_WILDCARDS          (1 << 3)
#define PFLG_MAIL                  (1 << 4)
#define PFLG_STDIO_ALLOWED         (1 << 5)

/* for int and float params */
#define PFLG_POSITIVE              (1 << 6)
#define PFLG_NOT_NULL              (1 << 7)

/* for string param */
#define PFLG_NOT_EMPTY             PFLG_NOT_NULL

/* float params only */
#define PFLG_ALLOW_PCT_SIGN        (1 << 8)

/**
 *  Retrieve a string parameter and check its format
 *  @return 0 on success
 *          ENOENT if the parameter does not exist in the block
 *          EINVAL if the parameter does not satisfy restrictions
 */
int            GetStringParam(config_item_t block, const char *block_name,
                              const char *var_name, int flags, char *target,
                              unsigned int target_size, char ***extra_args_tab,
                              unsigned int *nb_extra_args, char *err_msg);
/**
 *  Retrieve a boolean parameter and check its format
 *  @return 0 on success
 *          ENOENT if the parameter does not exist in the block
 *          EINVAL if the parameter does not satisfy restrictions
 */
int            GetBoolParam(config_item_t block, const char *block_name,
                            const char *var_name, int flags, int *target,
                            char ***extra_args_tab, unsigned int *nb_extra_args,
                            char *err_msg );

/**
 *  Retrieve a duration parameter and check its format
 *  @return 0 on success
 *          ENOENT if the parameter does not exist in the block
 *          EINVAL if the parameter does not satisfy restrictions
 */
int            GetDurationParam(config_item_t block, const char *block_name,
                                const char *var_name, int flags, time_t *target,
                                char ***extra_args_tab,
                                unsigned int *nb_extra_args, char *err_msg);
/**
 *  Retrieve a size parameter and check its format
 *  @return 0 on success
 *          ENOENT if the parameter does not exist in the block
 *          EINVAL if the parameter does not satisfy restrictions
 */
int            GetSizeParam(config_item_t block, const char *block_name,
                            const char *var_name, int flags,
                            unsigned long long *target, char ***extra_args_tab,
                            unsigned int *nb_extra_args, char *err_msg);



/**
 *  Retrieve an integer parameter and check its format
 *  @return 0 on success
 *          ENOENT if the parameter does not exist in the block
 *          EINVAL if the parameter does not satisfy restrictions
 */
int            GetIntParam(config_item_t block, const char *block_name,
                           const char *var_name, int flags, int *target,
                           char ***extra_args_tab, unsigned int *nb_extra_args,
                           char *err_msg);

/**
 *  Retrieve a 64 bits integer parameter and check its format.
 *  (a suffix can be used in config file).
 *  @return 0 on success
 *          ENOENT if the parameter does not exist in the block
 *          EINVAL if the parameter does not satisfy restrictions
 */
int GetInt64Param(config_item_t block, const char *block_name,
                  const char *var_name, int flags, uint64_t *target,
                  char ***extra_args_tab, unsigned int *nb_extra_args,
                  char *err_msg);

/**
 *  Retrieve a float parameter and check its format
 *  @return 0 on success
 *          ENOENT if the parameter does not exist in the block
 *          EINVAL if the parameter does not satisfy restrictions
 */
int           GetFloatParam(config_item_t block, const char *block_name,
                            const char *var_name, int flags, double *target,
                            char ***extra_args_tab, unsigned int *nb_extra_args,
                            char *err_msg);


// TODO generic config parsing using structure {type, name, flags, tgtptr}

/**
 * Types and function to parse a list of simple scalar configuration variables (with no extra args).
 */
typedef enum {
    PT_STRING,
    PT_BOOL,
    PT_DURATION,
    PT_SIZE,
    PT_INT,
    PT_INT64,
    PT_FLOAT
} cfg_param_type;

typedef struct cfg_param_t {
    const char     *name; /* NULL for last name */
    cfg_param_type  type;
    int             flags;
    void           *ptr;
    size_t          ptrsize;
} cfg_param_t;
#define END_OF_PARAMS {NULL, 0, 0, NULL, 0}

int read_scalar_params(config_item_t block, const char *block_name, 
                       const cfg_param_t * params, char *msgout);

/**
 * Build a policy boolean expression from the given block
 */
int            GetBoolExpr( config_item_t block, const char *block_name,
                            bool_node_t * p_bool_node,
                            int *p_attr_mask, char *err_msg );

/**
 * Build a policy boolean expression from a union/intersection of filesets
 */
int            GetSetExpr(config_item_t block, const char *block_name,
                          bool_node_t * p_bool_node,
                          int *p_attr_mask, const policies_t *policies,
                          char *err_msg);


/**
 * Free a boolean expression structure
 */
int            FreeBoolExpr( bool_node_t * p_expr, int free_top_node );


/** give the  string for a compare oparation */
const char    *op2str( compare_direction_t comp );

/** give the string for an object type */
const char    *type2str( obj_type_t type );

/** give the  name for a criteria */
const char    *criteria2str( compare_criteria_t crit );


/** Create a boolean condition */
int CreateBoolCond(bool_node_t * p_out_node, compare_direction_t compar,
                   compare_criteria_t  crit, compare_value_t val);

/** Append a boolean condition with bool op = AND */
int AppendBoolCond(bool_node_t * p_in_out_node, compare_direction_t compar,
                   compare_criteria_t  crit, compare_value_t val);


/**
 * Print a boolean expression to a string.
 */
int            BoolExpr2str( bool_node_t * p_bool_node, char *out_str, size_t str_size );


/**
 * Check that no unknown parameter or block is found.
 * @param param_array NULL terminated array of allowed parameters.
 */
void           CheckUnknownParameters( config_item_t block, const char *block_name,
                                       const char **param_array );



#endif


/** @} */

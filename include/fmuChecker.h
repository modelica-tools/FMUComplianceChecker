/*
    Copyright (C) 2012 Modelon AB <http://www.modelon.com>

	You should have received a copy of the LICENCE-FMUChecker.txt
    along with this program. If not, contact Modelon AB.
*/
/**
	\file fmuChecker.h
	Header file for fmuChecker application.
*/

#ifndef FMUCHECKER_H_
#define FMUCHECKER_H_

#include <stdio.h>
#include <JM/jm_portability.h>
#include <fmilib.h>

#include "fmi1_input_reader.h"
#include "fmi2_input_reader.h"

/** string constant used for logging. */
extern const char* fmu_checker_module;

/** Common exit routine */
void do_exit(int code);

/** Global variable used for checking alloc/free consistency */
extern int allocated_mem_blocks;

/** calloc proxy */
void* check_calloc(size_t nobj, size_t size);

/** free proxy */
void  check_free(void* obj);

/** Print information on command line options */
void print_usage() ;

#define MAX_URL_LENGTH 10000

/**  Checker data structure is used to pass information between different routines */
struct fmu_check_data_t {
	/** FMU file */
	const char* FMUPath;	
	/** Temporary directory with unique name where FMU is unpacked */
	char* tmpPath;
    
    /** Same as tmpPath unless specified by an option */
    char unzipPathBuf[10000];
    char* unzipPath;

	/** Directory to be used for temporary files. Either user specified or system-wide*/
	const char* temp_dir;

	/** Counter for the warnings */
	unsigned int num_warnings;

	/** counter for the non-fatal errors */
	unsigned int num_errors;

	/** counter for the non-fatal errors */
	unsigned int num_fatal;

	/** counter for the non-info messages from FMU */
	unsigned int num_fmu_messages;

	/** A flag that makes instance name error appear only once */
	int printed_instance_name_error_flg;

	/** FMIL callbacks*/
	jm_callbacks callbacks;
	/** FMIL context */
	fmi_import_context_t* context;

	/** Model information */
	const char* modelIdentifierFMI1;
	const char* modelIdentifierME;
	const char* modelIdentifierCS;
	const char* modelName;
	const char*  GUID;
	const char* instanceNameSavedPtr;
	const char* instanceNameToCompare;

	/** Simulation stop time */
	double stopTime;
	/** Step size for the simulation*/
	double stepSize;
    /** Flag indicating if step size is user defined in command line */
    int stepSizeSetByUser;
    
#define DEFAULT_MAX_OUTPUT_PTS 500
#define DEFAULT_MAX_OUTPUT_PTS_STR "500"

    /** Maximum number of steps to save to output */
    size_t maxOutputPts;
    /** Flag indicating if max number of steps is user defined in command line */
    int maxOutputPtsSetByUser;
    /** Next output time */
    double nextOutputTime;
    /** Next output step number*/
    double nextOutputStep;
	/** separator character to use */
	char CSV_separator;

/** this feature is currently off */
#undef SUPPORT_out_enum_as_int_flag
#ifdef SUPPORT_out_enum_as_int_flag
	/** print enums and bools as integers */
	char out_enum_as_int_flag;
#endif

	/** Name of the output file (NULL is stdout)*/
	char* output_file_name;
	/** Output file stream */
	FILE* out_file;
	/** Name of the log file (NULL is stderr)*/
	char* log_file_name;
	/** Log file stream */
	FILE* log_file;

    /** input data file name */
    char* inputFileName;

	/** Should simulation be done (or only XML checking) */
	int do_simulate_flg;

	/** Should we simulate ME if available? */
    int do_test_me;

	/** Should we simulate CS if available? */
    int do_test_cs;

	/** Should we require ME to be available? */
    int require_me;

	/** Should we require CS to be available? */
    int require_cs;

    /** should variable names be mangled to avoid quoting (-m switch) */
    int do_mangle_var_names;

    /** should all variables be printed (-f switch) */
    int do_output_all_vars;

	/** should variables be printed before event handling (-d switch) */
    int print_all_event_vars;

	/** FMI standard version of the FMU */
	fmi_version_enu_t version;

	/** FMI1 main struct */
	fmi1_import_t* fmu1;
	/** Kind of the FMI */
	fmi1_fmu_kind_enu_t fmu1_kind;
    /** input data for FMU 1.0 */
    fmi1_csv_input_t fmu1_inputData;
	/** model variables */
	fmi1_import_variable_list_t* vl;

	/** FMI2 main struct */
	fmi2_import_t* fmu2;
	/** Kind of the FMI */
	fmi2_fmu_kind_enu_t fmu2_kind;
	/** input data for FMU 2.0 */
    fmi2_csv_input_t fmu2_inputData;
	/** model variables */
	fmi2_import_variable_list_t* vl2;
} ;


/** Global pointer is necessary to support FMI 1.0 logging */
extern fmu_check_data_t* cdata_global_ptr;

/** parse command line options and set fields in cdata accordingly */
void parse_options(int argc, char *argv[], fmu_check_data_t* cdata);

/** Init checker data with defaults */ 
void init_fmu_check_data(fmu_check_data_t* cdata);

/** Release allocated resources */ 
void clear_fmu_check_data(fmu_check_data_t* cdata, int close_log);

/** Logger function for FMI library */
void checker_logger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message);

/** Check an FMI 1.0 FMU */
jm_status_enu_t fmi1_check(fmu_check_data_t* cdata);

/** Check if the fmi status is ok or warning */
static int fmi1_status_ok_or_warning(fmi1_status_t fmistatus) {
	return (fmistatus == fmi1_status_ok) || (fmistatus == fmi1_status_warning);
}

/** Print the string in double quotes replacing any occurrence of '"' in the string with \' */
jm_status_enu_t checked_print_quoted_str(fmu_check_data_t* cdata, const char* str);

/** Write out the data into the output file */
jm_status_enu_t checked_fprintf(fmu_check_data_t* cdata, const char* fmt, ...);

/** Write out separator and variable name. Variable name is quoted/mangled if needed */
jm_status_enu_t check_fprintf_var_name(fmu_check_data_t* cdata, const char* vn);

/** Simulate an FMI 1.0 ME FMU */
jm_status_enu_t fmi1_me_simulate(fmu_check_data_t* cdata);

/** Simulate an FMI 1.0 CS FMU */
jm_status_enu_t fmi1_cs_simulate(fmu_check_data_t* cdata);

jm_status_enu_t fmi1_write_csv_header(fmu_check_data_t* cdata);

jm_status_enu_t fmi1_write_csv_data(fmu_check_data_t* cdata, double time);

/** Check an FMI 2.0 FMU */
jm_status_enu_t fmi2_check(fmu_check_data_t* cdata);

/** Simulate an FMI 2.0 ME FMU */
jm_status_enu_t fmi2_me_simulate(fmu_check_data_t* cdata);

/** Simulate an FMI 2.0 CS FMU */
jm_status_enu_t fmi2_cs_simulate(fmu_check_data_t* cdata);

jm_status_enu_t fmi2_write_csv_header(fmu_check_data_t* cdata);

jm_status_enu_t fmi2_write_csv_data(fmu_check_data_t* cdata, double time);

/* Check that it's possible to get all variables with causality output */
jm_status_enu_t fmi2_check_get_INIT(fmu_check_data_t* cdata);

/** Check if the fmi status is ok or warning */
static int fmi2_status_ok_or_warning(fmi2_status_t fmistatus) {
	return (fmistatus == fmi2_status_ok) || (fmistatus == fmi2_status_warning);
}

/* Prepare the time step, time end and number of steps info
    for the simulation.
    Input/output: information from default experiment
*/
void prepare_time_step_info(fmu_check_data_t* cdata, double* timeEnd, double* timeStep);

/* Calls the FMI 1.0 get functions with zero length arrays and logs any problems and
   returns either fmi ok/warning status when successful and other fmi status otherwise */
fmi1_status_t check_fmi1_get_with_zero_len_array(fmi1_import_t* fmu, jm_callbacks* cb);

/* Calls the FMI 1.0 set functions with zero length arrays and logs any problems and
   returns either fmi ok/warning status when successful and other fmi status otherwise */
fmi1_status_t check_fmi1_set_with_zero_len_array(fmi1_import_t* fmu, jm_callbacks* cb);

#endif

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

/** string constant used for logging. */
extern const char* fmu_checker_module;

/** Common exit routine */
void do_exit(int code);

/** Global variable used for checking alloc/free consistency */
extern int allocated_mem_blocks;

/** malloc proxy */
void* check_malloc(size_t size);

/** calloc proxy */
void* check_calloc(size_t nobj, size_t size);

/** free proxy */
void  check_free(void* obj);

/** Print information on command line options */
void print_usage() ;

/**  Checker data structure is used to pass information between different routines */
typedef struct fmu_check_data_t {
	/** FMU file */
	const char* FMUPath;	
	/** Temporary directory with unique name where FMU is unpacked */
	char* tmpPath;
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

	/** FMIL callbacks*/
	jm_callbacks callbacks;
	/** FMIL context */
	fmi_import_context_t* context;

	/** Model information */
	const char* modelIdentifier;
	const char* modelName;
	const char*  GUID;
	const char* instanceName;

	/** Simulation stop time */
	double stopTime;
	/** Step size for the simulation*/
	double stepSize;
	/** Number of steps to take */
	size_t numSteps;
	/** separator character to use */
	char CSV_separator;
	/** Name of the output file (NULL is stdout)*/
	char* output_file_name;
	/** Output file stream */
	FILE* out_file;
	/** Name of the log file (NULL is stderr)*/
	char* log_file_name;
	/** Log file stream */
	FILE* log_file;

	/** Should simulation be done (or only XML checking) */
	int do_simulate_flg;

	/** FMI standard version of the FMU */
	fmi_version_enu_t version;

	/** FMI1 main struct */
	fmi1_import_t* fmu1;
	/** Kind of the FMI */
	fmi1_fmu_kind_enu_t fmu1_kind;
	/** model variables */
	fmi1_import_variable_list_t* vl;
} fmu_check_data_t;


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

/** Simulate an FMI 1.0 ME FMU */
jm_status_enu_t fmi1_me_simulate(fmu_check_data_t* cdata);

/** Simulate an FMI 1.0 CS FMU */
jm_status_enu_t fmi1_cs_simulate(fmu_check_data_t* cdata);

jm_status_enu_t fmi1_write_csv_header(fmu_check_data_t* cdata);

jm_status_enu_t fmi1_write_csv_data(fmu_check_data_t* cdata, double time);


#endif
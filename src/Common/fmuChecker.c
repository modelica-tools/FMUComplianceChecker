/*
    Copyright (C) 2012 Modelon AB <http://www.modelon.com>

	You should have received a copy of the LICENSE-FMUChecker.txt
    along with this program. If not, contact Modelon AB.
*/
/**
	\file fmuChecker.c
	Main function and command line options handling of the FMUChecker application.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>

#if defined(_WIN32) || defined(WIN32)
	#include <Shlwapi.h>
#endif

// #include <config_test.h>
#include <JM/jm_portability.h>
#include <fmilib.h>
#include <fmuChecker.h>
#include <fmu_checker_version.h>
#include <fmilib_config.h>

const char* fmu_checker_module = "FMUCHK";

#define BUFFER 1000

void do_exit(int code)
{
	/* when running on Windows this may be useful:
		printf("Press 'Enter' to exit\n");
		getchar(); */
	exit(code);
}

int allocated_mem_blocks = 0;

void* check_calloc(size_t nobj, size_t size) {
	void* ret = calloc(nobj, size);
	if(ret) allocated_mem_blocks++;
	jm_log_verbose(&cdata_global_ptr->callbacks, fmu_checker_module,
		"allocateMemory( %u, %u) called. Returning pointer: %p",nobj,size,ret);
	return ret;
}

void  check_free(void* obj) {
	jm_log_verbose(&cdata_global_ptr->callbacks, fmu_checker_module, "freeMemory(%p) called", obj);
	if(obj) {
		free(obj);
		allocated_mem_blocks--;
	}
}

void checker_logger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
	fmu_check_data_t* cdata = (fmu_check_data_t*)c->context;
	int ret;

	if(log_level == jm_log_level_warning)
		cdata->num_warnings++;
	else if(log_level == jm_log_level_error)
		cdata->num_errors++;
	else if(log_level == jm_log_level_fatal)
		cdata->num_fatal++;

	if(log_level)
		ret = fprintf(cdata->log_file, "[%s][%s] %s\n", jm_log_level_to_string(log_level), module, message);
	else
		ret = fprintf(cdata->log_file, "%s\n", message);

	fflush(cdata->log_file);

	if(ret <= 0) {
		fclose(cdata->log_file);
		cdata->log_file = stderr;
		fprintf(stderr, "[%s][%s] %s\n", jm_log_level_to_string(log_level), module, message);
		fprintf(stderr, "[%s][%s] %s\n", jm_log_level_to_string(jm_log_level_fatal), module, "Error writing to the log file");
		cdata->num_fatal++;
	}
}

void print_version() {
    printf("FMI compliance checker " FMUCHK_VERSION " [FMILibrary: "FMIL_VERSION"] build date: "__DATE__
#ifdef FMILIB_ENABLE_LOG_LEVEL_DEBUG
        " "__TIME__
#endif
        "\n");
}


void print_usage( ) {
    print_version();
	printf(	"Usage: fmuCheck." FMI_PLATFORM " [options] <model.fmu>\n\n"
		"Options:\n\n"
		"-c <separator>   Separator character to be used in CSV output. Default is ','.\n\n"
        "-d               Print also left limit values at event points to the output\n"
        "                 file to investigate event behaviour. Default is to only print\n"
        "                 values after event handling.\n\n"
        "-e <filename>    Error log file name. Default is to use standard error.\n\n"
        "-f               Print all variables to the output file. Default is to only\n"
        "                 print outputs.\n\n"
        "-h <stepSize>    For ME simulation: Decides step size to use in forward Euler.\n"
        "                 For CS simulation: Decides communication step size for the\n"
        "                 stepping.\n"
        "                 Observe that if a small stepSize is used the number of saved\n"
        "                 outputs will still be limited by the number of output points.\n"
        "                 Default is to calculated a step size from the number of output\n"
        "                 points. See the -n option for how the number of outputs is\n"
        "                 set.\n\n"
        "-i <infile>      Name of the CSV file name with input data.\n\n"
        "-l <log level>   Log level: 0 - no logging, 1 - fatal errors only, 2 - errors, \n"
        "                 3 - warnings, 4 - info, 5 - verbose, 6 - debug.\n\n"
        "-m               Mangle variable names to avoid quoting (needed for some CSV\n"
        "                 importing applications, but not according to the CrossCheck\n"
        "                 rules).\n\n"
        "-n <numSteps>    Maximum number of output points. \"-n 0\" means output at every\n"
        "                 step and the number of outputs are decided by the -h option.\n"
        "                 Observe that no interpolation is used, output points are taken\n"
        "                 at the steps.\n"
        "                 Default is " DEFAULT_MAX_OUTPUT_PTS_STR ".\n\n"
        "-o <filename>    Simulation result output CSV file name. Default is to use\n"
        "                 standard output.\n\n"
        "-s <stopTime>    Simulation stop time, default is to use information from\n"
        "                 'DefaultExperiment' as specified in the model description XML.\n\n"
        "-t <tmp-dir>     Temporary dir to use for unpacking the FMU.\n"
        "                 Default is to use system-wide directory, e.g., C:\\Temp or /tmp.\n\n"
        "-v               Print the checker version information.\n\n"
        "-k xml           Check XML only.\n"
        "-k me            Check XML and ME simulation.\n"
        "-k cs            Check XML and CS simulation.\n"
        "                 Multiple -k options add up.\n"
        "                 No -k option: test XML, simulate ME and CS respectively if\n"
        "                 supported.\n\n"
        "-x               Check XML only. Same as -k xml.\n\n"
        "-z <unzip-dir>   Do not create and remove a temp directory but instead use the\n"
        "                 specified one for unpacking the FMU. The option takes \n"
        "                 precendence over -t.\n\n"
        "Command line examples:\n\n"
        "fmuCheck." FMI_PLATFORM " model.fmu\n"
        "       The checker will process 'model.fmu'  with default options.\n\n"
        "fmuCheck." FMI_PLATFORM " -e log.txt -o result.csv -c ; -s 2 -h 1e-3 -l 5 -t . model.fmu \n"
        "       The checker will process 'model.fmu'.\n"
        "       Log messages will be saved in log.txt, simulation output in \n"
        "       result.csv and semicolon will be used for field separation in the CSV\n"
        "       file. The checker will simulate the FMU until 2 seconds with \n"
        "       time step 1e-3 seconds. Verbose messages will be generated.\n"
        "       Temporary files will be created in the current directory.\n"
        );
}

void parse_options(int argc, char *argv[], fmu_check_data_t* cdata) {
	int do_test_everything=1;
	size_t i;
 	if(argc < 2) {
		print_usage();
		do_exit(0);
	}
    if((argc == 2) && strcmp(argv[1], "-v") == 0) {
        print_version();
        do_exit(0);
    }

	i=1;
	while(i < (size_t)(argc - 1)) {
		const char* option = argv[i];
		if((option[0] != '-') || (option[2] != 0)) {
			jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Error parsing command line. Expected a single character option but got %s.\nRun without arguments to see help.", option);
			do_exit(1);
		}
		option++;
		switch(*option) {
		case 'x': /* same as -k xml */
            do_test_everything = 0;
			break;
        case 'k':
            do_test_everything = 0;

			i++;
			option = argv[i];

            {
                /* convert option to lowecase */
                char *ch = (char *)option;
                while (*ch != 0) {
                    *ch = tolower(*ch);
                    ch++;
                }
            }

            if      (strcmp(option, "me")  == 0) cdata->require_me = 1;
            else if (strcmp(option, "cs")  == 0) cdata->require_cs = 1;
            else if (strcmp(option, "xml") == 0) {}
            else {
				jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Unsupported option '-k %s'.\nRun without arguments to see help.", option);
				do_exit(1);
            }
            break;
		case 's': {
			/* <stopTime>\t Simulation stop time, default is to use information from 'DefaultExperiment'\n" */
			double endTime;

			i++;
			option = argv[i];
			if((sscanf(option, "%lg", &endTime) != 1) || (endTime <= 0)) {
				jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Error parsing command line. Expected positive stop time after '-e'.\nRun without arguments to see help.");
				do_exit(1);
			}
			cdata->stopTime = endTime;
			break;
		}
		case 'h':  { /*<stepSize>\t Step size to use in forward Euler. Default is to used step size based on the number of output points.\n" */
			double h;
			i++;
			option = argv[i];
			if((sscanf(option, "%lg", &h) != 1) ||(h <= 0)) {
				jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Error parsing command line. Expected positive step size after '-h'.\nRun without arguments to see help.");
				do_exit(1);
			}
			cdata->stepSize = h;
            cdata->stepSizeSetByUser = 1;
			break;
				   }
		case 'n': {/*<num_steps>\t Maximum number of output points. Zero means output in every step. Default is " #DEFAULT_MAX_OUTPUT_PTS ".\n"*/
			int n;
			i++;
			option = argv[i];
			if((sscanf(option, "%d", &n) != 1) || (n < 0)) {
				jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Error parsing command line. Expected number of steps after '-n'.\nRun without arguments to see help.");
				do_exit(1);
			}
            cdata->maxOutputPts = (size_t)n;
            cdata->maxOutputPtsSetByUser = 1;
			break;
				  }
		case 'l': { /*log level>\t Log level: 0 - no logging, 1 - fatal errors only,\n\t 2 - errors, 3 - warnings, 4 - info, 5 - verbose, 6 - debug\n"*/
			int log_level;
			i++;
			option = argv[i];
			if((sscanf(option, "%d", &log_level) != 1) || (log_level < jm_log_level_nothing) ||(log_level > jm_log_level_all) ) {
				jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Error parsing command line. Expected log level after '-l'.\nRun without arguments to see help.");
				do_exit(1);
			}
			cdata->callbacks.log_level = (jm_log_level_enu_t)log_level;
			break;
		}
		case 'c': {/*csvSeparator>\t Separator character to be used. Default is ','.\n"*/
			i++;
			option = argv[i];
			if(option[1] != 0) {
				jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Error parsing command line. Expected single separator character after '-s'.\nRun without arguments to see help.");
				do_exit(1);
			}
			cdata->CSV_separator = *option;
			break;
				  }
		case 'o': {/*output-file-name>\t Default is to print output to standard output.\n"*/
			i++;
			cdata->output_file_name = argv[i];
			break;
				  }
		case 'e': {/*log-file-name>\t Default is to print log to standard error.\n"*/
			i++;
			cdata->log_file_name = argv[i];
			break;
				  }
        case 'f': {   /*    "-f\t\t Print all variables to the output file. Default is to only print outputs.\n\n" */
            cdata->do_output_all_vars = 1;
            break;
                  }
		case 't': {/*tmp-dir>\t Temporary dir to use for unpacking the fmu.\n\t Default is to create an unique temporary dir.\n"*/
			i++;
			cdata->temp_dir = argv[i];
			break;
                  }
		case 'i': {   /*     "-i <infile>\t Name of the CSV file name with input data.\n\n" */
            i++;
            cdata->inputFileName = argv[i];
            break;
         }
        case 'm':{ /* "Print enums and booleans as integers (default is to print item names, true and false)." */
            cdata->do_mangle_var_names = 1;
            break;
        }
		case 'd': {   /*    "-d\t\t Print also left limit values at event points to the output file to investigate event behaviour. Default is to only print values after event handling.\n\n" */
            cdata->print_all_event_vars = 1;
            break;
                  }
        case 'v': {
            print_version();
                break;
                  }
        case 'z': { /* "-z <unzip-dir>\t Do not create temp directory but use the specified one\n\t\t for unpacking the FMU The option takes precendence over -t." */
			char cwd[10000];
            i++;
			if( jm_portability_get_current_working_directory(cwd, 9999) ||
                jm_portability_set_current_working_directory(argv[i]) ||
			    jm_portability_get_current_working_directory(cdata->unzipPathBuf, 9999) ||
			    jm_portability_set_current_working_directory(cwd)
				) {
				clear_fmu_check_data(cdata, 1);
				do_exit(1);
			}
            cdata->unzipPath = cdata->unzipPathBuf;
            break;
        }
        default:
			jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Unsupported command line option %s.\nRun without arguments to see help.", option);
			do_exit(1);
		}
		i++;
	}
	if(i != argc - 1) {
		jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Error parsing command line. Last argument must be an FMU filename.\nRun without arguments to see help.");
		do_exit(1);
	}
	cdata->FMUPath = argv[i];

    cdata->do_test_me = cdata->require_me || do_test_everything;
    cdata->do_test_cs = cdata->require_cs || do_test_everything;
    cdata->do_simulate_flg = cdata->do_test_me || cdata->do_test_cs;

	if(cdata->log_file_name) {
		cdata->log_file = fopen(cdata->log_file_name, "wb");
		if(!cdata->log_file) {
			cdata->log_file = stderr;
			jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Could not open %s for writing", cdata->log_file_name);
			clear_fmu_check_data(cdata, 1);
			do_exit(1);
		}
	}

	{
		jm_log_level_enu_t log_level = cdata->callbacks.log_level;
		jm_log_verbose(&cdata->callbacks,fmu_checker_module,"Setting log level to [%s]", jm_log_level_to_string(log_level));
#ifndef FMILIB_ENABLE_LOG_LEVEL_DEBUG
		if(log_level == jm_log_level_debug) {
			jm_log_verbose(&cdata->callbacks,fmu_checker_module,"This binary is build without debug log messages."
			"\n Reconfigure with FMUCHK_ENABLE_LOG_LEVEL_DEBUG=ON and rebuild to enable debug level logging");
		}
#endif
	}
	{
		FILE* tryFMU = fopen(cdata->FMUPath, "r");
		if(tryFMU == 0) {
			jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Cannot open FMU file (%s)", strerror(errno));
			clear_fmu_check_data(cdata, 1);
			do_exit(1);
		}
		fclose(tryFMU);
	}
    {

        if(cdata->inputFileName) {
            FILE* infile = fopen(cdata->inputFileName, "rb");
            if(infile) {
                fclose(infile);
            }
            else {
    			jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Cannot open input data file (%s)", strerror(errno));
	    		clear_fmu_check_data(cdata, 1);
		    	do_exit(1);
            }
        }
    }

	if(!cdata->temp_dir) {
		cdata->temp_dir = jm_get_system_temp_dir();
		if(!cdata->temp_dir) cdata->temp_dir = "./";
	}
    if(cdata->unzipPath) {
        cdata->tmpPath = cdata->unzipPath;
    }
    else {
		cdata->tmpPath = fmi_import_mk_temp_dir(&cdata->callbacks, cdata->temp_dir, "fmucktmp");
    }
	if(!cdata->tmpPath) {
		do_exit(1);
	}
	if(cdata->output_file_name) {
		cdata->out_file = fopen(cdata->output_file_name, "wb");
		if(!cdata->out_file) {
			jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Could not open %s for writing", cdata->output_file_name);
			clear_fmu_check_data(cdata, 1);
			do_exit(1);
		}
	}
}

jm_status_enu_t checked_print_quoted_str(fmu_check_data_t* cdata, const char* str) {
	jm_status_enu_t status = jm_status_success;
    if(!str) return status;
	if(strchr(str, '"')) {
		/* replace double quotes with single quotes */
#ifdef _MSC_VER
		char* buf = _strdup(str);
#else
		char* buf = strdup(str);
#endif
		char * ch = strchr(buf, '"');
		while(ch) {
			*ch = '\'';
			ch = strchr(ch + 1, '"');
		}
		status = checked_fprintf(cdata, "\"%s\"", buf);
		free(buf);
	}
	else
		status = checked_fprintf(cdata, "\"%s\"", str);

	return status;
}


jm_status_enu_t checked_fprintf(fmu_check_data_t* cdata, const char* fmt, ...) {
	jm_status_enu_t status = jm_status_success;
	va_list args;
    va_start (args, fmt);
	if(vfprintf(cdata->out_file, fmt, args) <= 0) {
		jm_log_fatal(&cdata->callbacks, fmu_checker_module, "Error writing output file (%s)", strerror(errno));
		status = jm_status_error;
	}
    va_end (args);
	return status;
}

jm_status_enu_t check_fprintf_var_name(fmu_check_data_t* cdata, const char* vn) {
    char buf[10000], *cursrc, *curdest;
    int need_quoting = 1;
    jm_status_enu_t status = jm_status_success;
   	char replace_sep = ':';

	if(cdata->CSV_separator == ':') {
		replace_sep = '|';
	}

    if(cdata->do_mangle_var_names) {
        /* skip spaces ans repace separator character in column names */
        sprintf(buf, "%s", vn);
        curdest = cursrc = buf;
        while(*cursrc) {
            if(*cursrc != ' ') {
                if(*cursrc == cdata->CSV_separator)
                    *curdest = replace_sep;
                else if(curdest != cursrc)
                    *curdest = *cursrc;
                curdest++;
            }
            cursrc++;
        }
        *curdest = 0;
    }
    else {
        int j = 0;
        while(vn[j]) {
            char ch = vn[j];
            if((ch == cdata->CSV_separator)
                || (ch == '"')
                || (ch == ' ')
                || (ch == '\n')
                || (ch == '\t')
                || (ch == '\r')) {
                    need_quoting = 1;
                    break;
            }
            j++;
        }
        if(need_quoting) {
            curdest = buf;
            *curdest = '"';
            curdest++;
            j = 0;
            while(vn[j]) {
                char ch = vn[j];
                if(ch == '"') {
                    *curdest = ch;
                    curdest++;
                }
                *curdest = ch;
                curdest++;
                j++;
            }
            *curdest = '"';
            curdest++;
            *curdest = 0;
        }
        else {
            sprintf(buf, "%s", vn);
        }
    }
    status = checked_fprintf(cdata, "%c%s", cdata->CSV_separator, buf);
    if(status != jm_status_success) {
        return jm_status_error;
    }
    return status;
}

void init_fmu_check_data(fmu_check_data_t* cdata) {
	cdata->FMUPath = 0;
	cdata->tmpPath = 0;
	cdata->temp_dir = 0;
    cdata->unzipPath = 0;

	cdata->num_errors = 0;
	cdata->num_warnings = 0;
	cdata->num_fatal = 0;
	cdata->num_fmu_messages = 0;
	cdata->printed_instance_name_error_flg = 0;

	cdata->callbacks.malloc = malloc;
    cdata->callbacks.calloc = calloc;
    cdata->callbacks.realloc = realloc;
    cdata->callbacks.free = free;
    cdata->callbacks.logger = checker_logger;
	cdata->callbacks.log_level = jm_log_level_info;
    cdata->callbacks.context = cdata;

	cdata->context = 0;

	cdata->modelIdentifierFMI1 = 0;
	cdata->modelIdentifierME = 0;
	cdata->modelIdentifierCS = 0;
	cdata->modelName = 0;
	cdata->GUID = 0;
	cdata->instanceNameSavedPtr = 0;
	cdata->instanceNameToCompare = 0;

	cdata->stopTime = 0.0;
	cdata->stepSize = 0.0;
    cdata->stepSizeSetByUser = 0;
    cdata->maxOutputPts = DEFAULT_MAX_OUTPUT_PTS;
    cdata->maxOutputPtsSetByUser = 0;
    cdata->nextOutputTime = 0.0;
    cdata->nextOutputStep = 0;
	cdata->CSV_separator = ',';
#ifdef SUPPORT_out_enum_as_int_flag
	cdata->out_enum_as_int_flag = 0;
#endif
	cdata->output_file_name = 0;
	cdata->out_file = stdout;
	cdata->log_file_name = 0;
	cdata->log_file = stderr;
    cdata->inputFileName = 0;
	cdata->do_simulate_flg = 1;
    cdata->do_test_me = 1;
    cdata->do_test_cs = 1;
    cdata->require_me = 0;
    cdata->require_cs = 0;
    cdata->do_mangle_var_names = 0;
    cdata->do_output_all_vars = 0;
	cdata->print_all_event_vars = 0;

	cdata->version = fmi_version_unknown_enu;

	cdata->fmu1 = 0;
	cdata->fmu1_kind = fmi1_fmu_kind_enu_unknown;
	cdata->vl = 0;

    cdata->fmu2 = 0;
	cdata->fmu2_kind = fmi2_fmu_kind_unknown;
    cdata->vl2 = 0;
	assert(cdata_global_ptr == 0);
	cdata_global_ptr = cdata;
}

void clear_fmu_check_data(fmu_check_data_t* cdata, int close_log) {
	if(cdata->fmu1) {
		if (cdata->do_simulate_flg) {
			fmi1_free_input_data(&cdata->fmu1_inputData);
		}
		fmi1_import_free(cdata->fmu1);
		cdata->fmu1 = 0;
	}
	if(cdata->fmu2) {
		if (cdata->do_simulate_flg) {
			fmi2_free_input_data(&cdata->fmu2_inputData);
		}
		fmi2_import_free(cdata->fmu2);
		cdata->fmu2 = 0;
	}
	if(cdata->context) {
		fmi_import_free_context(cdata->context);
		cdata->context = 0;
	}
    if(cdata->tmpPath && (cdata->tmpPath != cdata->unzipPath)) {
		jm_rmdir(&cdata->callbacks,cdata->tmpPath);
		cdata->callbacks.free(cdata->tmpPath);
		cdata->tmpPath = 0;
	}
	if(cdata->out_file && (cdata->out_file != stdout)) {
		fclose(cdata->out_file);
	}
	if(cdata->vl) {
		fmi1_import_free_variable_list(cdata->vl);
		cdata->vl = 0;
	}
    if(cdata->vl2) {
		fmi2_import_free_variable_list(cdata->vl2);
		cdata->vl2 = 0;
	}
	if(close_log && cdata->log_file && (cdata->log_file != stderr)) {
		fclose(cdata->log_file);
		cdata->log_file = stderr;
	}
	cdata_global_ptr = 0;
}

/* Prepare the time step, time end and number of steps info
    for the simulation.
    Input/output: information from default experiment
*/
void prepare_time_step_info(fmu_check_data_t* cdata, double* timeEnd, double* timeStep) {
    if(cdata->stopTime > 0) {
        *timeEnd = cdata->stopTime;
    }
    else {
        cdata->stopTime = *timeEnd;
    }

    if(cdata->stepSizeSetByUser) {
        *timeStep = cdata->stepSize;
    }
    else if(cdata->maxOutputPtsSetByUser) {
        if(cdata->maxOutputPts) {
            *timeStep = cdata->stopTime / cdata->maxOutputPts;
        }
        else {
            *timeStep = cdata->stopTime / DEFAULT_MAX_OUTPUT_PTS;
        }
    }
    else {
        *timeStep = cdata->stopTime / cdata->maxOutputPts;
    }
}

fmu_check_data_t* cdata_global_ptr = 0;

static int direxists(const char* dirname) {
    struct stat s;
    return stat(dirname, &s) == 0 && (S_IFDIR & s.st_mode);
}

static int check_dir_structure(fmu_check_data_t *cdata)
{
    char *bindir;
    char *srcdir;
    int is_valid = 0;

    size_t pathlen = strlen(cdata->tmpPath);
    size_t binlen = pathlen + strlen(FMI_FILE_SEP) + 8; /*strlen("binaries") == 8*/
    size_t srclen = pathlen + strlen(FMI_FILE_SEP) + 7; /*strlen("sources") == 7*/

    bindir = cdata->callbacks.calloc(binlen + 1, sizeof(char));
    srcdir = cdata->callbacks.calloc(srclen + 1, sizeof(char));
    if (bindir == NULL || srcdir == NULL) {
        jm_log_fatal(&cdata->callbacks,
                     fmu_checker_module,
                     "Failed to allocate memory");
        clear_fmu_check_data(cdata, 1);
        do_exit(1);
    }

    jm_snprintf(bindir, binlen + 1, "%s" FMI_FILE_SEP "binaries", cdata->tmpPath);
    jm_snprintf(srcdir, srclen + 1, "%s" FMI_FILE_SEP "sources", cdata->tmpPath);

    is_valid = direxists(bindir) || direxists(srcdir);

    cdata->callbacks.free(bindir);
    cdata->callbacks.free(srcdir);

    return is_valid;
}

int main(int argc, char *argv[])
{
	fmu_check_data_t cdata;
	jm_status_enu_t status = jm_status_success;
	jm_log_level_enu_t log_level = jm_log_level_info;
	jm_callbacks* callbacks;
	int i = 0;
    int cnt;
	char clopts[JM_MAX_ERROR_MESSAGE_SIZE];

	init_fmu_check_data(&cdata);
	callbacks = &cdata.callbacks;
	parse_options(argc, argv, &cdata);

#ifdef FMILIB_GENERATE_BUILD_STAMP
	jm_log_debug(callbacks,fmu_checker_module,"FMIL build stamp:\n%s\n", fmilib_get_build_stamp());
#endif

#ifdef FMILIB_ENABLE_LOG_LEVEL_DEBUG
	jm_log_info(callbacks,fmu_checker_module,"FMI compliance checker " FMUCHK_VERSION " [FMILibrary: "FMIL_VERSION"] build date: "__DATE__ " "__TIME__);
#else
	jm_log_info(callbacks,fmu_checker_module,"FMI compliance checker " FMUCHK_VERSION " [FMILibrary: "FMIL_VERSION"] build date: "__DATE__ );
#endif

	/*Print commad line arguments to log.*/
	strcpy(clopts, argv[0]);
	for( cnt = 1; cnt < argc; cnt++ ){
		strcat(clopts," ");
		strcat(clopts,argv[cnt]);
	}
	strcat(clopts,"\0");
	jm_log_info(callbacks,fmu_checker_module,"Called with following options:");
	jm_log_info(callbacks,fmu_checker_module,clopts);


	jm_log_info(callbacks,fmu_checker_module,"Will process FMU %s",cdata.FMUPath);

	cdata.context = fmi_import_allocate_context(callbacks);
    fmi_import_set_configuration(cdata.context, FMI_IMPORT_NAME_CHECK);

	cdata.version = fmi_import_get_fmi_version(cdata.context, cdata.FMUPath, cdata.tmpPath);
	if(cdata.version == fmi_version_unknown_enu) {
		jm_log_fatal(callbacks,fmu_checker_module,"Error in FMU version detection");
		do_exit(1);
	}

    if (!check_dir_structure(&cdata)) {
        jm_log_error(&cdata.callbacks,
                     fmu_checker_module,
                     "FMU must contain either a \"sources\" or a \"binaries\" folder");
    }

	switch(cdata.version) {
	case  fmi_version_1_enu:
		status = fmi1_check(&cdata);
		break;
	case  fmi_version_2_0_enu:
		status = fmi2_check(&cdata);
		break;
	default:
		clear_fmu_check_data(&cdata, 1);
		jm_log_fatal(callbacks,fmu_checker_module,"Only FMI version 1.0 and 2.0 are supported so far");
		do_exit(1);
	}

	clear_fmu_check_data(&cdata, 0);

	if(allocated_mem_blocks)  {
		if(allocated_mem_blocks > 0) {
			jm_log_error(callbacks,fmu_checker_module,
				"Memory leak: freeMemory was not called for %d block(s) allocated by allocateMemory",
				allocated_mem_blocks);
		}
		else {
			jm_log_error(callbacks,fmu_checker_module,
				"Memory mamagement: freeMemory was called without allocateMemory for %d block(s)",
				-allocated_mem_blocks);
		}
	}

	jm_log(callbacks, fmu_checker_module, jm_log_level_nothing, "FMU check summary:");

	jm_log(callbacks, fmu_checker_module, jm_log_level_nothing, "FMU reported:\n\t%u warning(s) and error(s)\nChecker reported:", cdata.num_fmu_messages);

	if(callbacks->log_level < jm_log_level_error) {
		jm_log(callbacks, fmu_checker_module, jm_log_level_nothing,
			"\tWarnings and non-critical errors were ignored (log level: %s)", jm_log_level_to_string( callbacks->log_level ));
	}
	else {
		if(callbacks->log_level < jm_log_level_warning) {
			jm_log(callbacks, fmu_checker_module, jm_log_level_nothing,
				"\tWarnings were ignored (log level: %s)", jm_log_level_to_string( callbacks->log_level ));
		}
		else
		jm_log(callbacks, fmu_checker_module, jm_log_level_nothing, "\t%u Warning(s)", cdata.num_warnings);
		if ((cdata.num_fatal > 0)) cdata.num_errors=cdata.num_errors+cdata.num_fatal;
		jm_log(callbacks, fmu_checker_module, jm_log_level_nothing, "\t%u Error(s)", cdata.num_errors);
	}
	if((status == jm_status_success) && (cdata.num_fatal == 0)) {
		if(cdata.log_file && (cdata.log_file != stderr))
			fclose(cdata.log_file);
		do_exit(0);
	}
	else {
		jm_log(callbacks, fmu_checker_module, jm_log_level_nothing,
			"\t%u Fatal error(s) occurred during processing",cdata.num_fatal);
		if(cdata.log_file && (cdata.log_file != stderr))
			fclose(cdata.log_file);
		do_exit(1);
	}
	return 0;
}

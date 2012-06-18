/*
    Copyright (C) 2012 Modelon AB

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// #include <config_test.h>
#include <JM/jm_portability.h>
#include <fmilib.h>
#include <fmuChecker.h>

const char* fmu_checker_module = "FMUCHK";

#define BUFFER 1000

void do_exit(int code)
{
	/* when running on Windows this may be useful:
		printf("Press 'Enter' to exit\n");
		getchar(); */
	exit(code);
}

size_t allocated_mem_blocks = 0;

void* check_malloc(size_t size) {
	void* ret = malloc(size);
	if(ret) allocated_mem_blocks++;
	return ret;
}

void* check_calloc(size_t nobj, size_t size) {
	void* ret = calloc(nobj, size);
	if(ret) allocated_mem_blocks++;
	return ret;
}

void  check_free(void* obj) {
	if(obj) {
		free(obj);
		allocated_mem_blocks--;
	}
}

void checker_logger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
	fmu_check_data_t* cdata = (fmu_check_data_t*)c->context;

	if(log_level < cdata->status ) 
		 cdata->status = log_level;
	
	fprintf(stderr, "[%s][%s] %s\n", jm_log_level_to_string(log_level), module, message);

}


void print_usage() {
	printf("FMI compliance checker. Usage: fmuCheck [options] <model.fmu>\n"
		"Options:\n\n"
		"-x\t\t Check XML and stop, default is to load the DLL and simulate\n\t\t after this.\n\n"
		"-e <stopTime>\t Simulation stop time, default is to use information from\n\t\t'DefaultExperiment' as specified in the model description XML.\n\n"
		"-h <stepSize>\t Step size to use in forward Euler. Takes preference over '-n'.\n\n"
		"-n <num_steps>\t Number of steps in forward Euler until time end.\n\t\t Default is 100 steps simulation between start and stop time.\n\n"
		"-l <log level>\t Log level: 0 - no logging, 1 - fatal errors only,\n\t\t 2 - errors, 3 - warnings, 4 - info, 5 - verbose, 6 - debug.\n\n"
		"-s <separator>\t Separator character to be used in CSV output. Default is ';'.\n\n"
		"-o <filename>\t Output file name. Default is to use standard output.\n\n"
		"-t <tmp-dir>\t Temporary dir to use for unpacking the FMU.\n\t\t Default is to create an unique temporary dir.\n\n"
		);
}

void parse_options(int argc, char *argv[], fmu_check_data_t* cdata) {
	size_t i;
	if(argc < 2) {
		print_usage();
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
		case 'x':
			cdata->do_simulate_flg = 0;
			break;
		case 'e': {
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
		case 'h':  { /*stepSize>\t Step size to use in forward Euler. If provided takes preference over '-n' below.\n" */
			double h;
			i++;
			option = argv[i];
			if((sscanf(option, "%lg", &h) != 1) ||(h <= 0)) {
				jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Error parsing command line. Expected positive step size after '-h'.\nRun without arguments to see help.");
				do_exit(1);
			}
			cdata->stepSize = h;
			break;
				   }
		case 'n': {/*num_steps>\t Number of steps in forward Euler until time end.\n\t Default is 100 steps simulation between time start and time end.\n"*/
			int n;
			i++;
			option = argv[i];
			if((sscanf(option, "%d", &n) != 1) || (n < 0)) {
				jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Error parsing command line. Expected number of steps after '-n'.\nRun without arguments to see help.");
				do_exit(1);
			}
			cdata->numSteps = (size_t)n;			
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
			jm_log_verbose(&cdata->callbacks,fmu_checker_module,"Setting log level to [%s]", jm_log_level_to_string((jm_log_level_enu_t)log_level));
			break;
		}
		case 's': {/*csvSeparator>\t Separator character to be used. Default is ';'.\n"*/
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
		case 't': {/*tmp-dir>\t Temporary dir to use for unpacking the fmu.\n\t Default is to create an unique temporary dir.\n"*/
			i++;
			cdata->temp_dir = argv[i];
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
	if(!cdata->temp_dir) {
		cdata->temp_dir = jm_get_system_temp_dir();
		if(!cdata->temp_dir) cdata->temp_dir = "./";
	}
	{
		const char * tmpl = "/fmutmpXXXXXX";
		size_t len;
		len = strlen(cdata->temp_dir);
		if((cdata->temp_dir[len-1] == tmpl[0]) || (cdata->temp_dir[len-1] == '/')) 
			tmpl++;
		cdata->tmpPath = (char*)cdata->callbacks.malloc(strlen(cdata->temp_dir) + strlen(tmpl) + 1);
		if(!cdata->tmpPath) {
			jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Could not allocate memory");
			do_exit(1);
		}
		sprintf(cdata->tmpPath,"%s%s",cdata->temp_dir,tmpl);
		if(!jm_mktemp(cdata->tmpPath)) {
			jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Could not create a unique temporary directory name");
			cdata->callbacks.free(cdata->tmpPath);
			do_exit(1);
		}
		if(jm_mkdir(&cdata->callbacks,cdata->tmpPath) != jm_status_success) {
			cdata->callbacks.free(cdata->tmpPath);
			do_exit(1);
		}
	}
	if(cdata->do_simulate_flg && cdata->output_file_name) {
		cdata->out_file = fopen(cdata->output_file_name, "w");
		if(!cdata->out_file) {
			jm_log_fatal(&cdata->callbacks,fmu_checker_module,"Could not open %s for writing", cdata->output_file_name);
			clear_fmu_check_data(cdata);
			do_exit(1);
		}
	}
}

void init_fmu_check_data(fmu_check_data_t* cdata) {
	cdata->FMUPath = 0;
	cdata->tmpPath = 0;
	cdata->temp_dir = 0;
	cdata->status = jm_log_level_info;

	cdata->callbacks.malloc = check_malloc;
    cdata->callbacks.calloc = check_calloc;
    cdata->callbacks.realloc = realloc;
    cdata->callbacks.free = check_free;
    cdata->callbacks.logger = checker_logger;
	cdata->callbacks.log_level = jm_log_level_info;
    cdata->callbacks.context = cdata;

	cdata->context = 0;

	cdata->modelIdentifier = 0;
	cdata->modelName = 0;
	cdata->GUID = 0;

	cdata->stopTime = 0.0;
	cdata->stepSize = 0.0;
	cdata->numSteps = 100;
	cdata->CSV_separator = ';';
	cdata->output_file_name = 0;
	cdata->out_file = stdout;
	cdata->do_simulate_flg = 1;

	cdata->version = fmi_version_unknown_enu;

	cdata->fmu1 = 0;
	cdata->fmu1_kind = fmi1_fmu_kind_enu_unknown;
	cdata->vl = 0;
}

void clear_fmu_check_data(fmu_check_data_t* cdata) {
	if(cdata->fmu1) {
		fmi1_import_free(cdata->fmu1);
		cdata->fmu1 = 0;
	}
	if(cdata->context) {
		fmi_import_free_context(cdata->context);
		cdata->context = 0;
	}
	if(cdata->tmpPath) {
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
}


int main(int argc, char *argv[])
{
	fmu_check_data_t cdata;
	jm_status_enu_t status = jm_status_success;
	jm_log_level_enu_t log_level = jm_log_level_info;
	jm_callbacks* callbacks;
	int i = 0;


	init_fmu_check_data(&cdata);
	callbacks = &cdata.callbacks;
	parse_options(argc, argv, &cdata);

#ifdef FMILIB_GENERATE_BUILD_STAMP
	jm_log_debug(callbacks,fmu_checker_module,"FMIL build stamp:\n%s\n", fmilib_get_build_stamp());
#endif
	jm_log_info(callbacks,fmu_checker_module,"Will process FMU %s",cdata.FMUPath);

	cdata.context = fmi_import_allocate_context(callbacks);

	cdata.version = fmi_import_get_fmi_version(cdata.context, cdata.FMUPath, cdata.tmpPath);
	if(cdata.version == fmi_version_unknown_enu) {
		jm_log_fatal(callbacks,fmu_checker_module,"Error in FMU version detection");
		do_exit(1);
	}

	switch(cdata.version) {
	case  fmi_version_1_enu:
		status = fmi1_check(&cdata);
		break;
	default:
		clear_fmu_check_data(&cdata);
		jm_log_fatal(callbacks,fmu_checker_module,"Only FMI version 1.0 is supported so far");
		do_exit(1);
	}

	clear_fmu_check_data(&cdata);

	if(status == jm_status_success) {
		if(cdata.status >= jm_log_level_info)
			jm_log_info(callbacks,fmu_checker_module, "FMU seems to be OK since you got this far =)!");
		else if(cdata.status >= jm_log_level_warning)
			jm_log_info(callbacks,fmu_checker_module, "FMU is mostly OK since you got this far =)!");
		else 
			jm_log_info(callbacks,fmu_checker_module, "No critical errors found since you got this far =)!");
		do_exit(0);
	}
	else
		do_exit(1);

	return 0;
}



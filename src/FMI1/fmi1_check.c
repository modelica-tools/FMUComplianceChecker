#include <errno.h>

#include <fmuChecker.h>
#include <fmilib.h>

jm_status_enu_t fmi1_check(fmu_check_data_t* cdata) {
	fmi1_callback_functions_t callBackFunctions;
	jm_callbacks* cb = &cdata->callbacks;
	jm_status_enu_t status;

	cdata->fmu1 = fmi1_import_parse_xml(cdata->context, cdata->tmpPath);

	if(!cdata->fmu1) {
		jm_log_fatal(cb,fmu_checker_module,"Error parsing XML, exiting");
		return jm_status_error;
	}

	cdata->modelIdentifier = fmi1_import_get_model_identifier(cdata->fmu1);
	cdata->modelName = fmi1_import_get_model_name(cdata->fmu1);
	cdata->GUID = fmi1_import_get_GUID(cdata->fmu1);

	jm_log_info(cb, fmu_checker_module,"Model name: %s", cdata->modelName);
    jm_log_info(cb, fmu_checker_module,"Model identifier: %s", cdata->modelIdentifier);
    jm_log_info(cb, fmu_checker_module,"Model GUID: %s", cdata->GUID);
    jm_log_info(cb, fmu_checker_module,"Model version: %s", fmi1_import_get_model_version(cdata->fmu1));

	cdata->fmu1_kind = fmi1_import_get_fmu_kind(cdata->fmu1);

    jm_log_info(cb, fmu_checker_module,"FMU kind: %s", fmi1_fmu_kind_to_string(cdata->fmu1_kind));

	cdata->vl = fmi1_import_get_variable_list(cdata->fmu1);
	if(!cdata->vl) {
		jm_log_fatal(cb, fmu_checker_module,"Could not construct model variables list");
		return jm_status_error;
	}

	if(cb->log_level >= jm_log_level_info) {
		fmi1_import_model_counts_t counts;
		fmi1_import_collect_model_counts(cdata->fmu1, &counts);
		jm_log_info(cb, fmu_checker_module,
			"The FMU contains:\n"
			"%u constants\n"
			"%u parameters\n"
			"%u discrete variables\n"
			"%u continuous variables\n"
			"%u inputs\n"
			"%u outputs\n"
			"%u internal variables\n"
			"%u variables with causality 'none'\n"
			"%u real variables\n"
			"%u integer variables\n"
			"%u enumeration variables\n"
			"%u boolean variables\n"
			"%u string variables\n",
			counts.num_constants,
			counts.num_parameters,
			counts.num_discrete,
			counts.num_continuous,
			counts.num_inputs,
			counts.num_outputs,
			counts.num_internal,
			counts.num_causality_none,
			counts.num_real_vars,
			counts.num_integer_vars,
			counts.num_enum_vars,
			counts.num_bool_vars,
			counts.num_string_vars);
	}

	jm_log_info(cb, fmu_checker_module,"Printing output file header");
	if(fmi1_write_csv_header(cdata) != jm_status_success) {
		return jm_status_error;
	}

	if(!cdata->do_simulate_flg) {
		jm_log_verbose(cb, fmu_checker_module,"Simulation was not requested");
		return jm_status_success;
	}


	callBackFunctions.allocateMemory = cb->calloc;
	callBackFunctions.freeMemory = cb->free;
	callBackFunctions.logger = fmi1_log_forwarding;
	callBackFunctions.stepFinished = 0;

	status = fmi1_import_create_dllfmu(cdata->fmu1, callBackFunctions, 1);

	if (status == jm_status_error) {
		jm_log_fatal(cb,fmu_checker_module,"Could not create the DLL loading mechanism(C-API).");
		return jm_status_error;
	}

	jm_log_info(cb,fmu_checker_module,"Version returned from FMU:   %s\n", fmi1_import_get_version(cdata->fmu1));

	{
		const char* platform;

		if(cdata->fmu1_kind == fmi1_fmu_kind_enu_me)
			platform= fmi1_import_get_model_types_platform(cdata->fmu1);
		else
			platform= fmi1_import_get_types_platform(cdata->fmu1);
		if(strcmp(platform, fmi1_get_platform())) 
			jm_log_error(cb,fmu_checker_module,"Platform type returned from FMU %s does not match the checker  %s\n",platform, fmi1_get_platform() );
	}

	if(cdata->fmu1_kind == fmi1_fmu_kind_enu_me)
		return fmi1_me_simulate(cdata);
	else 
		return fmi1_cs_simulate(cdata);
}

jm_status_enu_t fmi1_write_csv_header(fmu_check_data_t* cdata) {
	fmi1_import_variable_list_t * vl = cdata->vl;
	size_t i;
	size_t n = fmi1_import_get_variable_list_size(vl);
	char fmt[10];
	sprintf(fmt, "%s%c", "%s", cdata->CSV_separator);
	
	if(fprintf(cdata->out_file, fmt, "time") <= 0) {
		jm_log_fatal(&cdata->callbacks, fmu_checker_module, "Error writing output file (%s)", strerror(errno));
		return jm_status_error;
	}

	for(i = 0; i < n; i++) {
		if(fprintf(cdata->out_file, fmt, fmi1_import_get_variable_name(fmi1_import_get_variable(vl, i)))<=0) {
			jm_log_fatal(&cdata->callbacks, fmu_checker_module, "Error writing output file (%s)", strerror(errno));
			return jm_status_error;
		}
	}
	if(fprintf(cdata->out_file, "\n")<=0) {
		jm_log_fatal(&cdata->callbacks, fmu_checker_module, "Error writing output file (%s)", strerror(errno));
		return jm_status_error;
	}
	return jm_status_success;
}

jm_status_enu_t fmi1_write_csv_data(fmu_check_data_t* cdata, double time) {
	fmi1_import_t* fmu = cdata->fmu1;
	fmi1_import_variable_list_t * vl = cdata->vl;
	jm_callbacks* cb = &cdata->callbacks;
	fmi1_status_t fmistatus = fmi1_status_ok;
	int outstatus = 1;
	size_t i;
	size_t n = fmi1_import_get_variable_list_size(vl);
	char fmt_r[20];
	char fmt_i[20];
	char fmt_s[20];
	char fmt_true[20];
	char fmt_false[20];
	sprintf(fmt_r, "%s%c", "%g", cdata->CSV_separator);
	sprintf(fmt_i, "%s%c", "%d", cdata->CSV_separator);
	sprintf(fmt_s, "%s%c", "%s", cdata->CSV_separator);
	sprintf(fmt_true, "true%c", cdata->CSV_separator);
	sprintf(fmt_false, "false%c", cdata->CSV_separator);

	outstatus = fprintf(cdata->out_file, fmt_r, time);

	for(i = 0; i < n; i++) {
		fmi1_import_variable_t* v = fmi1_import_get_variable(vl, i);
		fmi1_value_reference_t vr = fmi1_import_get_variable_vr(v); 
		switch(fmi1_import_get_variable_base_type(v)) {
		case fmi1_base_type_real:
			{
				double val;
				fmistatus = fmi1_import_get_real(fmu,&vr, 1, &val);
				outstatus = fprintf(cdata->out_file, fmt_r, val);
				break;
			}
		case fmi1_base_type_int:
			{
				int val;
				fmistatus = fmi1_import_get_integer(fmu,&vr, 1, &val);
				outstatus = fprintf(cdata->out_file, fmt_i, val);
				break;
			}
		case fmi1_base_type_bool:
			{
				fmi1_boolean_t val;
				char* fmt;
				fmistatus = fmi1_import_get_boolean(fmu,&vr, 1, &val);
				fmt = (val == fmi1_true) ? fmt_true:fmt_false;
				outstatus = fprintf(cdata->out_file, fmt);
				break;
			}
		case fmi1_base_type_str:
			{
				fmi1_string_t val;
				fmistatus = fmi1_import_get_string(fmu,&vr, 1, &val);
				outstatus = fprintf(cdata->out_file, fmt_s, val);
				break;
			}
		case fmi1_base_type_enum:
			{
				int val;
				fmi1_import_variable_typedef_t* t = fmi1_import_get_variable_declared_type(v);
				fmi1_import_enumeration_typedef_t* et = 0;
				unsigned int item = 0;
				const char* itname = 0;
				if(t) et = fmi1_import_get_type_as_enum(t);

				fmistatus = fmi1_import_get_integer(fmu,&vr, 1, &val);
				if(et) itname = fmi1_import_get_enum_type_item_name(et, val);
				if(!itname) {
					jm_log_error(cb, fmu_checker_module, "Could not get item name for enum variable %s", fmi1_import_get_variable_name(v));
					outstatus = fprintf(cdata->out_file, fmt_i, val);
				}
				else {
					outstatus = fprintf(cdata->out_file, fmt_s, itname);
				}
				break;
			}
		}
		if(  fmistatus != fmi1_status_ok) {
			jm_log_warning(cb, fmu_checker_module, "fmiGetXXX returned status: %s for variable %s", 
				fmi1_status_to_string(fmistatus), fmi1_import_get_variable_name(v));
		}

		if(outstatus<=0) {
			jm_log_fatal(&cdata->callbacks, fmu_checker_module, "Error writing output file (%s)", strerror(errno));
			return jm_status_error;
		}
	}
	if(fprintf(cdata->out_file, "\n")<=0) {
		jm_log_fatal(&cdata->callbacks, fmu_checker_module, "Error writing output file (%s)", strerror(errno));
		return jm_status_error;
	}
	return jm_status_success;
}
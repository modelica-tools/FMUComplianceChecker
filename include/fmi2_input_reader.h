/*
    Copyright (C) 2012 Modelon AB <http://www.modelon.com>

	You should have received a copy of the LICENCE-FMUChecker.txt
    along with this program. If not, contact Modelon AB.
*/
/**
	\file fmi2_input_reader.h
	Structure and functions supporting input files.
*/

#ifndef fmi2_input_reader_h
#define fmi2_input_reader_h


#include <JM/jm_vector.h>
#include <fmilib.h>

/** Structure incapsulating information on input data*/
typedef struct fmi2_csv_input_t {
    jm_callbacks* cb;
   	/** FMI2 main struct */
	fmi2_import_t* fmu;

    jm_vector(double) timeStamps;
    size_t numTimeStamps;

    fmi2_import_variable_list_t* allInputs;

    fmi2_import_variable_list_t* realInputVariabilities;
    jm_vector(jm_voidp)* realInputVariabilityData;

    fmi2_import_variable_list_t* realInputs;
    jm_vector(jm_voidp)* realInputData;

	//	fmi2_import_variable_list_t* discreteRealInputs;
//    jm_vector(jm_voidp)* discsrealInputData;

//	fmi2_import_variable_list_t* continuousRealInputs;
//    jm_vector(jm_voidp)* contrealInputData;

    fmi2_import_variable_list_t* intInputs;
    jm_vector(jm_voidp)* intInputData;

    fmi2_import_variable_list_t* boolInputs;
    jm_vector(jm_voidp)* boolInputData;

    /** interpolation data for doubles. */
    /*  v[t] = v[i1]*lambda+v[i2](1-lambda) */
    double interpTime; /** time instance where the coeff is calculated */
    size_t discreteIndex; /** current data element index for discrete inputs */
    size_t interpIndex1; /** first data element index for interpolation */
    size_t interpIndex2; /** second data element index for interpolation */
    double interpLambda; /** interpolation coefficient */
    fmi2_real_t* interpData; /** interpolated inputs */

	/*input event check data*/
	size_t eventIndex1; /** first data element index for interpolation */
	size_t eventIndex2; /** first data element index for interpolation */

} fmi2_csv_input_t;

//typedef struct fmu_check_data_t fmu_check_data_t;

/** initialize the fmi2_csv_input_t strucuture */
jm_status_enu_t fmi2_init_input_data(fmi2_csv_input_t* indata, jm_callbacks* cb, fmi2_import_t* fmu);

/** free memory allocated for the input data */
void fmi2_free_input_data(fmi2_csv_input_t* indata);

/** update the interpolation coefficients inside the input data */
void fmi2_update_input_interpolation(fmi2_csv_input_t* indata, double t);

/** set inputs on the fmu */ 
fmi2_status_t fmi2_set_inputs(fmu_check_data_t* cdata, double time);

/** read input data from the file */ 
jm_status_enu_t fmi2_read_input_file( fmu_check_data_t* cdata);

/** check input data interval for event trigger from data */
jm_status_enu_t fmi2_check_external_events(fmi2_real_t tcur, fmi2_real_t tnext,fmi2_event_info_t* eventInfo,fmi2_csv_input_t* indata);
#endif

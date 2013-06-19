/*
    Copyright (C) 2012 Modelon AB <http://www.modelon.com>

	You should have received a copy of the LICENCE-FMUChecker.txt
    along with this program. If not, contact Modelon AB.
*/
/**
	\file fmi1_input_reader.h
	Structure and functions supporting input files.
*/

#ifndef fmi1_input_reader_h
#define fmi1_input_reader_h


#include <JM/jm_vector.h>
#include <fmilib.h>

/** Structure incapsulating information on input data*/
typedef struct fmi1_csv_input_t {
    jm_callbacks* cb;
   	/** FMI1 main struct */
	fmi1_import_t* fmu;

    jm_vector(double) timeStamps;
    size_t numTimeStamps;

    fmi1_import_variable_list_t* allInputs;

    fmi1_import_variable_list_t* realInputs;
    jm_vector(jm_voidp)* realInputData;

    fmi1_import_variable_list_t* intInputs;
    jm_vector(jm_voidp)* intInputData;

    fmi1_import_variable_list_t* boolInputs;
    jm_vector(jm_voidp)* boolInputData;

    /** interpolation data for doubles. */
    /*  v[t] = v[i1]*lambda+v[i2](1-lambda) */
    double interpTime; /** time instance where the coeff is calculated */
    size_t interpIndex1; /** first data element index for interpolation */
    size_t interpIndex2; /** second data element index for interpolation */
    double interpLambda; /** interpolation coefficient */
    fmi1_real_t* interpData; /** interpolated inputs */
} fmi1_csv_input_t;

typedef struct fmu_check_data_t fmu_check_data_t;

/** initialize the fmi1_csv_input_t strucuture */
jm_status_enu_t fmi1_init_input_data(fmi1_csv_input_t* indata, jm_callbacks* cb, fmi1_import_t* fmu);

/** free memory allocated for the input data */
void fmi1_free_input_data(fmi1_csv_input_t* indata);

/** update the interpolation coefficients inside the input data */
void fmi1_update_input_interpolation(fmi1_csv_input_t* indata, double t);

/** set inputs on the fmu */ 
fmi1_status_t fmi1_set_inputs(fmu_check_data_t* cdata, double time);

/** read input data from the file */ 
jm_status_enu_t fmi1_read_input_file( fmu_check_data_t* cdata);

#endif

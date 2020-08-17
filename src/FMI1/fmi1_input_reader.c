/*
    Copyright (C) 2012 Modelon AB <http://www.modelon.com>

	You should have received a copy of the LICENSE-FMUChecker.txt
    along with this program. If not, contact Modelon AB.
*/
/**
	\file fmuInputReader.c
	Main function and command line options handling of the FMUChecker application.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#include <JM/jm_portability.h>
#include <JM/jm_vector.h>
#include <fmilib.h>
#include <fmuChecker.h>

#define BUFFER 1000

jm_status_enu_t fmi1_init_input_data(fmi1_csv_input_t* indata, jm_callbacks* cb, fmi1_import_t* fmu) {
    int err;
    indata->cb = cb;
    indata->fmu = fmu;
    jm_vector_init(double)(&indata->timeStamps, 0, cb);

    indata->numTimeStamps = 0;
    err = ((indata->allInputs = fmi1_import_alloc_variable_list(fmu, 0)) == 0);
    err |= ((indata->realInputs = fmi1_import_alloc_variable_list(fmu, 0)) == 0);
    err |= ((indata->continuousInputs = fmi1_import_alloc_variable_list(fmu, 0)) == 0);
    err |= ((indata->intInputs = fmi1_import_alloc_variable_list(fmu, 0)) == 0);
    err |= ((indata->boolInputs = fmi1_import_alloc_variable_list(fmu, 0)) == 0);

    err |= ((indata->realInputData = jm_vector_alloc(jm_voidp)(0,100,cb)) == 0);
    err |= ((indata->intInputData  = jm_vector_alloc(jm_voidp)(0,100,cb)) == 0);
    err |= ((indata->boolInputData = jm_vector_alloc(jm_voidp)(0,100,cb)) == 0);

    indata->interpTime = 0;
    indata->discreteIndex = 0;
    indata->interpIndex1 = 0;
    indata->interpIndex2 = 0;
    indata->interpLambda = 0.0;
    indata->interpData = 0;
    indata->interpContinuousData = 0;

    indata->eventIndex1 = indata->eventIndex2 = 0;

    if(err){
        jm_log_error(cb, fmu_checker_module, "Cannot allocate memory");
        return jm_status_error;
    }

    return jm_status_success;
}

void fmi1_free_input_data(fmi1_csv_input_t* indata) {
    size_t i;
    if(!indata || !indata->fmu) return;
    jm_vector_free_data(double)(&indata->timeStamps);

    fmi1_import_free_variable_list(indata->allInputs);
    indata->allInputs = 0;
    if(indata->boolInputData) {
        for(i=0; i < jm_vector_get_size(jm_voidp)(indata->boolInputData); i++) {
            void** data = jm_vector_get_itemp(jm_voidp)(indata->boolInputData, i);
            free(*data);
            *data = 0;
        }
        jm_vector_free(jm_voidp)(indata->boolInputData);
        indata->boolInputData = 0;
    }
    fmi1_import_free_variable_list(indata->boolInputs);
    indata->boolInputs = 0;
    if(indata->intInputData) {
        for(i=0; i < jm_vector_get_size(jm_voidp)(indata->intInputData); i++) {
            void** data = jm_vector_get_itemp(jm_voidp)(indata->intInputData, i);
            free(*data);
            *data = 0;
        }
        jm_vector_free(jm_voidp)(indata->intInputData);
        indata->intInputData = 0;
    }
    fmi1_import_free_variable_list(indata->intInputs);
    indata->intInputs = 0;
    if(indata->realInputData) {
        for(i=0; i < jm_vector_get_size(jm_voidp)(indata->realInputData); i++) {
            void** data = jm_vector_get_itemp(jm_voidp)(indata->realInputData, i);
            free(*data);
            *data = 0;
        }
        jm_vector_free(jm_voidp)(indata->realInputData);
        indata->realInputData = 0;
    }
    fmi1_import_free_variable_list(indata->realInputs);
    indata->realInputs = 0;
    fmi1_import_free_variable_list(indata->continuousInputs);
    indata->continuousInputs = 0;
    free(indata->interpData);
    indata->interpData = 0;
    free(indata->interpContinuousData);
    indata->interpContinuousData = 0;
}

void fmi1_update_input_interpolation(fmi1_csv_input_t* indata, double t) {
    size_t i;
    size_t cont_i;
    if( (t == indata->interpTime) ||
        !jm_vector_get_size(double)(&indata->timeStamps)) {
            return;
    }

    indata->interpTime = t;
    if(t <= jm_vector_get_item(double)(&indata->timeStamps,0)) {
            /* handle extrapolation on the left */
            indata->interpIndex1 = indata->interpIndex2 = indata->discreteIndex = 0;
            indata->interpLambda = 0.0;
    }
    else if(t >= jm_vector_get_last(double)(&indata->timeStamps)) {
            /* handle extrapolation on the right */
            indata->interpIndex1 = indata->interpIndex2 = indata->discreteIndex
                = jm_vector_get_size(double)(&indata->timeStamps) - 1;
            indata->interpLambda = 1.0;
    }
    else {
        /* linear interpolation, note that time is always increasing and so indices are growing */
        double t1;
        double t2 = jm_vector_get_item(double)(&indata->timeStamps, indata->interpIndex2);
        while( t2 < t) {
            indata->interpIndex2++;
            t2 = jm_vector_get_item(double)(&indata->timeStamps, indata->interpIndex2);
        }
        if (t2 == t) {
            /* If we are exactly on the input time then use it for integers/booleans */
            indata->discreteIndex = indata->interpIndex2;
        } else {
            indata->discreteIndex = indata->interpIndex2 - 1;
        }
        indata->interpIndex1 = indata->interpIndex2 - 1;
        t1 = jm_vector_get_item(double)(&indata->timeStamps, indata->interpIndex1);
        indata->interpLambda = (t - t1)/(t2 -t1);
    }

    for (i = cont_i = 0; i < fmi1_import_get_variable_list_size(indata->realInputs); i++) {
        fmi1_import_variable_t* v = fmi1_import_get_variable(indata->realInputs, i);
        fmi1_variability_enu_t variability = fmi1_import_get_variability(v);
        if (variability == fmi1_variability_enu_continuous) {
            fmi1_real_t* v1 = (fmi1_real_t*)jm_vector_get_item(jm_voidp)(indata->realInputData,indata->interpIndex1);
            fmi1_real_t* v2 = (fmi1_real_t*)jm_vector_get_item(jm_voidp)(indata->realInputData,indata->interpIndex2);
            indata->interpData[i] = v1[i] * (1.0 - indata->interpLambda) + v2[i] * indata->interpLambda;
            indata->interpContinuousData[cont_i++] = indata->interpData[i];
        } else {
            /* discrete real, no interpolation */
            fmi1_real_t* v1 = (fmi1_real_t*)jm_vector_get_item(jm_voidp)(indata->realInputData,indata->discreteIndex);
            indata->interpData[i] = v1[i];
        }
    }
}

fmi1_status_t fmi1_set_continuous_inputs(fmu_check_data_t* cdata, double time) {
    fmi1_status_t fmiStatus = fmi1_status_ok;
    fmi1_csv_input_t* indata = &cdata->fmu1_inputData;

    if (!jm_vector_get_size(double)(&indata->timeStamps))
        return fmi1_status_ok;

    fmi1_update_input_interpolation(indata, time);

    if (indata->realInputData && fmi1_import_get_variable_list_size(indata->continuousInputs)) {
        const fmi1_value_reference_t* bv = fmi1_import_get_value_referece_list(indata->continuousInputs);
        if(!bv) return fmi1_status_error;
        fmiStatus = fmi1_import_set_real(cdata->fmu1, bv,
                   fmi1_import_get_variable_list_size(indata->continuousInputs),
                   indata->interpContinuousData);
    }
    return fmiStatus;
}

fmi1_status_t fmi1_set_inputs(fmu_check_data_t* cdata, double time) {
    fmi1_status_t fmiStatus = fmi1_status_ok;
    fmi1_csv_input_t* indata = &cdata->fmu1_inputData;

    if (!jm_vector_get_size(double)(&indata->timeStamps))
        return fmi1_status_ok;

    fmi1_update_input_interpolation(indata, time);

    if(indata->realInputData && fmi1_import_get_variable_list_size(indata->realInputs)) {
        const fmi1_value_reference_t* bv = fmi1_import_get_value_referece_list(indata->realInputs);
        if(!bv) return fmi1_status_error;
        fmiStatus = fmi1_import_set_real(cdata->fmu1, bv, fmi1_import_get_variable_list_size(indata->realInputs),
                        indata->interpData);
    }
    if (!fmi1_status_ok_or_warning(fmiStatus)) {
        return fmiStatus;
    }

    if(indata->boolInputData && fmi1_import_get_variable_list_size(indata->boolInputs)) {
        const fmi1_value_reference_t* bv = fmi1_import_get_value_referece_list(indata->boolInputs);
        if(!bv) return fmi1_status_error;
        fmiStatus = fmi1_import_set_boolean(cdata->fmu1, bv, fmi1_import_get_variable_list_size(indata->boolInputs),
                        (const fmi1_boolean_t*)jm_vector_get_item(jm_voidp)(indata->boolInputData,indata->discreteIndex));
    }
    if(!fmi1_status_ok_or_warning(fmiStatus)) {
        return fmiStatus;
    }

    if(indata->intInputData && fmi1_import_get_variable_list_size(indata->intInputs)) {
        const fmi1_value_reference_t* bv = fmi1_import_get_value_referece_list(indata->intInputs);
        if(!bv) return fmi1_status_error;
        fmiStatus = fmi1_import_set_integer(cdata->fmu1, bv, fmi1_import_get_variable_list_size(indata->intInputs),
                        (const fmi1_integer_t*)jm_vector_get_item(jm_voidp)(indata->intInputData,indata->discreteIndex));
    }

    return fmiStatus;
}

jm_status_enu_t fmi1_read_input_file(fmu_check_data_t* cdata) {
    FILE* infile;
    fmi1_csv_input_t* indata = &cdata->fmu1_inputData;
    fmi1_import_t* fmu = cdata->fmu1;
    int buf;
    char sep;
#define NAMEBUFSIZE 10000
    char namebuffer[NAMEBUFSIZE+1];
    size_t namelen = 0;
    int quotedFlg = 0;
    size_t lineCnt = 0, varCnt = 0;
    const char* fname = cdata->inputFileName;

    if(!fname) return jm_status_success;

    jm_log_info(&cdata->callbacks, fmu_checker_module,"Opening input file %s", fname);

    infile = fopen(fname, "rb");

    if(!infile) {
        jm_log_error(&cdata->callbacks, fmu_checker_module, "Cannot open input file %s", fname);
        return jm_status_error;
    }

    /* first column must be time */
    buf = fgetc(infile);
    if(    (buf == 't' ) && (fscanf(infile, "ime%c",&sep) != 1)
        || (buf == '"' ) && (fscanf(infile, "time\"%c",&sep) != 1)
        || (buf != '"' ) && (buf != 't' )) {
        jm_log_error(&cdata->callbacks, fmu_checker_module, "Input file must be a CSV file with a header. First column must be time.");
        fclose(infile);
        return jm_status_error;
    }
    jm_log_info(&cdata->callbacks, fmu_checker_module,"Detected separator character in input file: %c", sep);

    /* read the header */
    buf = fgetc(infile);
    while(buf != '\n') {
        if(feof(infile)) {
            jm_log_error(&cdata->callbacks, fmu_checker_module, "Unexpected end of file or error processing input file header");
            fclose(infile);
            return jm_status_error;
        }
        /* read in next variable name */
        namelen = 0;
        namebuffer[0] = 0;
        while( ((buf != sep) && (buf != '\n')) || quotedFlg) {
            if(feof(infile)) {
                namebuffer[namelen] = 0;
                jm_log_error(&cdata->callbacks, fmu_checker_module, "Unexpected end of file or error processing input file when reading variable: %s", namebuffer);
                fclose(infile);
                return jm_status_error;
            }
            /* handle quoting */
            if((namelen == 0) && (buf == '"')) {
                buf = fgetc(infile);
                quotedFlg = 1;
            }
            if(quotedFlg && (buf == '"')) {
                buf = fgetc(infile);
                if(buf == '"') {
                    /* double quote twice is just a double quote */
                }
                else if((buf == sep)||(buf == '\r') ||(buf == '\n')){
                    /* quoted name ended */
                    quotedFlg = 0;
                    if (buf == '\r') {
                        buf = fgetc(infile);
                        if(buf != '\n') {
                            fclose(infile);
                            jm_log_error(&cdata->callbacks, fmu_checker_module, "Expected CR+LF or just LF as end of line in input file. Got: CR+[%X]",buf);
                            return jm_status_error;
                        }
                    }
                    continue;
                }
                else {
                    fclose(infile);
                    namebuffer[namelen] = 0;
                    jm_log_error(&cdata->callbacks, fmu_checker_module, "Variable name in input file is not correctly quoted: %s", namebuffer);
                    return jm_status_error;
                }
            }
            namebuffer[namelen++] = buf;

            if(namelen >= NAMEBUFSIZE) {
                fclose(infile);
                namebuffer[namelen] = 0;
                jm_log_error(&cdata->callbacks, fmu_checker_module, "Variable name in input file is too long: %s",namebuffer);
                return jm_status_error;
            }

            buf = fgetc(infile);
            if((buf == '\r') && !quotedFlg) {
                buf = fgetc(infile);
                if(buf != '\n') {
                    fclose(infile);
                    jm_log_error(&cdata->callbacks, fmu_checker_module, "Expected CR+LF or just LF as end of line in input file. Got: CR+[%X]",buf);
                    return jm_status_error;
                }
            }
        }
        namebuffer[namelen++] = 0;
        {
            /* add the variable to the lists */
            fmi1_import_variable_t* v = fmi1_import_get_variable_by_name(fmu, namebuffer);
            fmi1_variability_enu_t variability;
            fmi1_causality_enu_t causality;
            fmi1_base_type_enu_t type;
            if(!v) {
                fclose(infile);
                jm_log_error(&cdata->callbacks, fmu_checker_module, "Cannot find input variable '%s' in the model description", namebuffer);
                return jm_status_error;
            }
            causality = fmi1_import_get_causality(v);
            variability = fmi1_import_get_variability(v);
            if (!((variability == fmi1_variability_enu_parameter)
                    || (causality == fmi1_causality_enu_input))) {
                fclose(infile);
                jm_log_error(&cdata->callbacks, fmu_checker_module, "Variables in the input file must be either parameters or inputs. '%s' is neither.", namebuffer);
                return jm_status_error;
            }
            type = fmi1_import_get_variable_base_type(v);
            switch(type) {
                case fmi1_base_type_real:
                    fmi1_import_var_list_push_back(indata->realInputs,v);
                    if (variability == fmi1_variability_enu_continuous) {
                        fmi1_import_var_list_push_back(indata->continuousInputs, v);
                    }
                    break;
                case fmi1_base_type_int:
                case fmi1_base_type_enum:
                    fmi1_import_var_list_push_back(indata->intInputs,v);
                    break;
                case fmi1_base_type_bool:
                    fmi1_import_var_list_push_back(indata->boolInputs,v);
                    break;
                default:
                    fclose(infile);
                    jm_log_error(&cdata->callbacks, fmu_checker_module, "Inputs must be real, integer, enum or boolean. Cannot process variable '%s'", namebuffer);
                    return jm_status_error;
            }
            fmi1_import_var_list_push_back(indata->allInputs, v);
        }
        if(buf != '\n') buf = fgetc(infile);
    }
    if(
        !(indata->interpData = (fmi1_real_t*)malloc(sizeof(fmi1_real_t) * fmi1_import_get_variable_list_size(indata->realInputs))) ||
        !(indata->interpContinuousData = (fmi1_real_t*)malloc(sizeof(fmi1_real_t) * fmi1_import_get_variable_list_size(indata->continuousInputs))) ||
        (fmi1_import_get_variable_list_size(indata->allInputs) !=
        fmi1_import_get_variable_list_size(indata->realInputs)+
        fmi1_import_get_variable_list_size(indata->intInputs)+
        fmi1_import_get_variable_list_size(indata->boolInputs))) {
        fclose(infile);
        jm_log_error(&cdata->callbacks, fmu_checker_module, "Internal error trying to create input variable lists. Possibly out of memory");
        return jm_status_error;
    }

    /* read input data */
    lineCnt = 0, varCnt = 0;
    while(!feof(infile)) {
        size_t realVarCnt, intVarCnt, boolVarCnt;
        int memErr = 0;
        fmi1_real_t* realData = 0;
        fmi1_integer_t* intData = 0;
        fmi1_boolean_t* boolData = 0;
        double time;
        lineCnt++;
        /* first column is time */
        if(fscanf(infile,"%lg",&time) != 1) break;
        /* allocate memory for the data and store pointers */
        memErr |= (jm_vector_push_back(double)(&indata->timeStamps, time) == 0);
        memErr |= (jm_vector_push_back(jm_voidp)(indata->realInputData,realData) == 0);
        memErr |= (jm_vector_push_back(jm_voidp)(indata->intInputData,intData) == 0);
        memErr |= (jm_vector_push_back(jm_voidp)(indata->boolInputData,boolData) == 0);
        if(!memErr) {
            realData = (fmi1_real_t*)malloc(sizeof(fmi1_real_t) * fmi1_import_get_variable_list_size(indata->realInputs));
            intData = (fmi1_integer_t*)malloc(sizeof(fmi1_integer_t) * fmi1_import_get_variable_list_size(indata->intInputs));
            boolData = (fmi1_boolean_t*)malloc(sizeof(fmi1_boolean_t) * fmi1_import_get_variable_list_size(indata->boolInputs));
        }
        if(    (realData == 0)
            || (intData == 0)
            || (boolData == 0)
            || memErr) {
            free(realData);
            free(intData);
            free(boolData);
            fclose(infile);
            jm_log_error(&cdata->callbacks, fmu_checker_module, "Out of memory while reading input file line %d", lineCnt);
            return jm_status_error;
        }
        *(fmi1_real_t**)jm_vector_get_lastp(jm_voidp)(indata->realInputData) = realData;
        *(fmi1_integer_t**)jm_vector_get_lastp(jm_voidp)(indata->intInputData) = intData;
        *(fmi1_boolean_t**)jm_vector_get_lastp(jm_voidp)(indata->boolInputData) = boolData;

        for(varCnt = realVarCnt = intVarCnt = boolVarCnt = 0;
            varCnt < fmi1_import_get_variable_list_size(indata->allInputs);
            varCnt++) {
            fmi1_import_variable_t* v = fmi1_import_get_variable(indata->allInputs, varCnt);
            fmi1_base_type_enu_t type = fmi1_import_get_variable_base_type(v);
            int negated = (fmi1_import_get_variable_alias_kind(v) == fmi1_variable_is_negated_alias);
            int err = 0;
            if(fgetc(infile) != sep) {
                jm_log_error(&cdata->callbacks, fmu_checker_module, "Expected separator character, got '%c'[%x] instead. Parsing line %i", sep,sep,lineCnt);
                break;
            }
            switch(type) {
            case fmi1_base_type_real:
                {
                    double dbl;
                    err = (fscanf(infile,"%lg",&dbl) != 1);
                    if(negated) dbl = -dbl;
                    realData[realVarCnt++] = dbl;
                    break;
                }

            case fmi1_base_type_int:
            case fmi1_base_type_enum:
                {
                    int intbuf;
                    err = (fscanf(infile,"%d",&intbuf) != 1);
                    if(negated) intbuf = -intbuf;
                    intData[intVarCnt++] = intbuf;
                    break;
                }
            case fmi1_base_type_bool:
                {
                    int intbuf;
                    err = (fscanf(infile,"%d",&intbuf) != 1) || (intbuf != 0) && (intbuf != 1);
                    if(negated) intbuf = intbuf ^ 1;
                    boolData[boolVarCnt++] = intbuf;
                    break;
                }
            default:
                err = 1;
                break;
            }
            if(err) {
                fclose(infile);
                jm_log_error(indata->cb, fmu_checker_module, "Error parsing input file data [line %d, time '%g', variable '%s']",
                    lineCnt+1, time, fmi1_import_get_variable_name(v));
                return jm_status_error;
            }
        }
        buf = fgetc(infile);
        if(buf == '\r') {
            buf = fgetc(infile);
        }
    }
    if(!feof(infile) || ferror(infile)) {
        fclose(infile);
        jm_log_error(&cdata->callbacks, fmu_checker_module, "Could not process input file past line %d.", lineCnt+1);
        return jm_status_error;
    }
    if(jm_vector_get_size(double)(&indata->timeStamps)) {
        fmi1_update_input_interpolation(indata, jm_vector_get_item(double)(&indata->timeStamps,0)-1);
    }
    return jm_status_success;
}

/** Local helper function, checks if it makes seens to look for external events */
static int fmi1_not_possible_to_have_external_event(fmi1_real_t tcur, fmi1_real_t tnext, fmi1_csv_input_t* indata) {
    return jm_vector_get_size(double)(&indata->timeStamps) < 2             ||
           tnext <= jm_vector_get_item(double)(&indata->timeStamps, 0)     ||
           tcur  >= jm_vector_get_last(double)(&indata->timeStamps);
}

jm_status_enu_t fmi1_check_external_events(fmi1_real_t tcur, fmi1_real_t tnext, fmi1_event_info_t* eventInfo, fmi1_csv_input_t* indata){
    size_t numberOfBools, numberOfInt, numberOfReals, cnt, timeIndex1;
    double t1;
    fmi1_integer_t *i1, *i2;
    fmi1_real_t *r1, *r2;
    fmi1_boolean_t *b1, *b2;

    if (fmi1_not_possible_to_have_external_event(tcur, tnext, indata)) {
        return jm_status_success;
    }

    timeIndex1 = indata->eventIndex1;
    if (timeIndex1 == 0) timeIndex1++; /* Need a previous value to check against */
    t1 = jm_vector_get_item(double)(&indata->timeStamps, timeIndex1);

    /* Loop until we find a time after the current time */
    while (t1 < tcur) {
        timeIndex1++;
        t1 = jm_vector_get_item(double)(&indata->timeStamps, timeIndex1);
    }

    /* Setup for loop */
    numberOfBools = fmi1_import_get_variable_list_size(indata->boolInputs);
    numberOfInt = fmi1_import_get_variable_list_size(indata->intInputs);
    numberOfReals = fmi1_import_get_variable_list_size(indata->realInputs);
    b1 = (fmi1_boolean_t*)jm_vector_get_item(jm_voidp)(indata->boolInputData,timeIndex1-1);
    b2 = (fmi1_boolean_t*)jm_vector_get_item(jm_voidp)(indata->boolInputData,timeIndex1);
    i1 = (fmi1_integer_t*)jm_vector_get_item(jm_voidp)(indata->intInputData,timeIndex1-1);
    i2 = (fmi1_integer_t*)jm_vector_get_item(jm_voidp)(indata->intInputData,timeIndex1);
    r1 = (fmi1_real_t*)jm_vector_get_item(jm_voidp)(indata->realInputData,timeIndex1-1);
    r2 = (fmi1_real_t*)jm_vector_get_item(jm_voidp)(indata->realInputData,timeIndex1);

    /* Check for any changes in discrete inputs occurring before the next time */
    while (t1 <= tnext) {
        /*boolean*/
        for (cnt = 0; cnt < numberOfBools; cnt++){
            if (b1[cnt] != b2[cnt]) {
                indata->eventIndex1 = timeIndex1 + 1;
                eventInfo->upcomingTimeEvent = fmi1_true;
                eventInfo->nextEventTime = t1;
                return jm_status_success;
            }
        }
        /*integer*/
        for (cnt = 0; cnt < numberOfInt; cnt++) {
            if (i1[cnt] != i2[cnt]) {
                indata->eventIndex1 = timeIndex1 + 1;
                eventInfo->upcomingTimeEvent = fmi1_true;
                eventInfo->nextEventTime = t1;
                return jm_status_success;
            }
        }
        /*discrete real*/
        for (cnt = 0; cnt < numberOfReals; cnt++) {
            fmi1_import_variable_t* v = fmi1_import_get_variable(indata->realInputs, cnt);
            if ((r1[cnt] != r2[cnt]) && (fmi1_import_get_variability(v) == fmi1_variability_enu_discrete)) {
                indata->eventIndex1 = timeIndex1 + 1;
                eventInfo->upcomingTimeEvent = fmi1_true;
                eventInfo->nextEventTime = t1;
                return jm_status_success;
            }
        }

        if (jm_vector_get_size(double)(&indata->timeStamps) == timeIndex1 + 1) {
            /* At last time index, finished */
            indata->eventIndex1 = timeIndex1;
            return jm_status_success;
        }

        /* Save cur as prev for next itr */
        b1 = b2;
        i1 = i2;
        r1 = r2;

        /* Increase time index and update values */
        timeIndex1++;
        t1 = jm_vector_get_item(double)(&indata->timeStamps, timeIndex1);
        b2 = (fmi1_boolean_t*)jm_vector_get_item(jm_voidp)(indata->boolInputData,timeIndex1);
        i2 = (fmi1_integer_t*)jm_vector_get_item(jm_voidp)(indata->intInputData,timeIndex1);
        r2 = (fmi1_real_t*)jm_vector_get_item(jm_voidp)(indata->realInputData,timeIndex1);
    }

    timeIndex1 = indata->eventIndex1 = timeIndex1 - 1;
    return jm_status_success;
}

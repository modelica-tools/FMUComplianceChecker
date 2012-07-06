/*
    Copyright (C) 2012 Modelon AB <http://www.modelon.com>

	You should have received a copy of the LICENCE-FMUChecker.txt
    along with this program. If not, contact Modelon AB.
*/
/**
	\file fmi1_me_sim.c
	Simulation loop for the FMI 1.0 model exchange FMUs
*/
#include <fmuChecker.h>
#include <fmilib.h>
  
jm_status_enu_t fmi1_me_simulate(fmu_check_data_t* cdata)
{	
	fmi1_status_t fmistatus;
	jm_status_enu_t jmstatus = jm_status_success;
	jm_callbacks* cb = &cdata->callbacks;

	fmi1_import_t* fmu = cdata->fmu1;
	fmi1_real_t tstart = fmi1_import_get_default_experiment_start(fmu);
	fmi1_real_t tcur;
	fmi1_real_t hcur;
	fmi1_real_t hdef;
	fmi1_real_t tend = fmi1_import_get_default_experiment_stop(fmu);
	size_t n_states;
	size_t n_event_indicators;
	fmi1_real_t* states = 0;
	fmi1_real_t* states_der = 0;
	fmi1_real_t* event_indicators = 0;
	fmi1_real_t* event_indicators_prev = 0;
	fmi1_boolean_t callEventUpdate;
	fmi1_boolean_t toleranceControlled = fmi1_false;
	fmi1_real_t relativeTolerance = fmi1_import_get_default_experiment_tolerance(fmu);
	fmi1_event_info_t eventInfo;
	fmi1_boolean_t intermediateResults = fmi1_false;

	if(cdata->stopTime > 0) {
		tend = cdata->stopTime;
	}

	if(cdata->stepSize > 0) {
		hdef = cdata->stepSize;
	}
	else {
		hdef = (tend - tstart) / cdata->numSteps;
	}

	n_states = fmi1_import_get_number_of_continuous_states(fmu);	
	n_event_indicators = fmi1_import_get_number_of_event_indicators(fmu);

	states = cb->calloc(n_states, sizeof(double));
	states_der = cb->calloc(n_states, sizeof(double));
	event_indicators = cb->calloc(n_event_indicators, sizeof(double));
	event_indicators_prev = cb->calloc(n_event_indicators, sizeof(double));

	if(!states || !states_der || !event_indicators || !event_indicators_prev) {
		cb->free(states);
		cb->free(states_der);
		cb->free(event_indicators);
		cb->free(event_indicators_prev);
	}

	cdata->instanceName = "Test FMI 1.0 ME";

	jmstatus = fmi1_import_instantiate_model(fmu, cdata->instanceName);
	if (jmstatus == jm_status_error) {
		jm_log_fatal(cb, fmu_checker_module, "Could not instantiate the model");
		cb->free(states);
		cb->free(states_der);
		cb->free(event_indicators);
		cb->free(event_indicators_prev);		
		return jm_status_error;
	}

	if( 
		(((fmistatus = fmi1_import_set_time(fmu, tstart)) == fmi1_status_ok) || (fmistatus == fmi1_status_warning)) &&
		(((fmistatus = fmi1_import_initialize(fmu, toleranceControlled, relativeTolerance, &eventInfo)) == fmi1_status_ok) || (fmistatus == fmi1_status_warning)) &&
		(((fmistatus = fmi1_import_get_continuous_states(fmu, states, n_states)) == fmi1_status_ok) || (fmistatus == fmi1_status_warning)) &&
		(((fmistatus = fmi1_import_get_event_indicators(fmu, event_indicators_prev, n_event_indicators)) == fmi1_status_ok) || (fmistatus == fmi1_status_warning)) 
		) {
			jm_log_info(cb, fmu_checker_module, "Initialized FMU for simulation starting at time %g", tstart);
	}
	else {
			jm_log_fatal(cb, fmu_checker_module, "Failed to initialize FMU for simulation (FMU status: %s)", fmi1_status_to_string(fmistatus));
			fmistatus = fmi1_status_fatal;
			jmstatus = jm_status_error;
	}

	tcur = tstart;
	hcur = hdef;
	callEventUpdate = fmi1_false;

	while ((tcur < tend) && (fmistatus != fmi1_status_error) && (fmistatus != fmi1_status_fatal) ) {
		size_t k;
		int zero_crossning_event = 0;

		jm_log_verbose(cb, fmu_checker_module, "Simulation time: %g", tcur);
		if(  ((fmistatus = fmi1_import_set_time(fmu, tcur)) != fmi1_status_ok) && (fmistatus != fmi1_status_warning)) break;
		if(  ((fmistatus = fmi1_import_get_event_indicators(fmu, event_indicators, n_event_indicators)) != fmi1_status_ok) && (fmistatus != fmi1_status_warning)) break;

		/* Check if an event inidcator has triggered */
		for (k = 0; k < n_event_indicators; k++) {
			if (event_indicators[k]*event_indicators_prev[k] < 0) {
				zero_crossning_event = 1;
				break;
			}
		}

		/* Handle any events */
		if (callEventUpdate || zero_crossning_event || (eventInfo.upcomingTimeEvent && tcur == eventInfo.nextEventTime)) {
			jm_log_verbose(cb, fmu_checker_module, "Handling an event");
			if(  ((fmistatus = fmi1_import_eventUpdate(fmu, intermediateResults, &eventInfo)) != fmi1_status_ok) && (fmistatus != fmi1_status_warning)) break;
			if(  ((fmistatus = fmi1_import_get_continuous_states(fmu, states, n_states)) != fmi1_status_ok) && (fmistatus != fmi1_status_warning)) break;
			if(  ((fmistatus = fmi1_import_get_event_indicators(fmu, event_indicators, n_event_indicators)) != fmi1_status_ok) && (fmistatus != fmi1_status_warning)) break;
			if(  ((fmistatus = fmi1_import_get_event_indicators(fmu, event_indicators_prev, n_event_indicators)) != fmi1_status_ok) && (fmistatus != fmi1_status_warning)) break;
		}

		/* Updated next time step */
		if (eventInfo.upcomingTimeEvent) {
			if (tcur + hdef < eventInfo.nextEventTime) {
				hcur = hdef;
			} else {
				hcur = eventInfo.nextEventTime - tcur;
			}
		} else {
			hcur = hdef;
		}
		tcur += hcur;
		/* adjust time step to get tend exactly */ 
		if(tcur > tend - hcur/1e16) {
			tcur -= hcur;
			hcur = (tend - tcur);
			tcur = tend;				
		}
		/* Get derivatives */
		if(  ((fmistatus = fmi1_import_get_derivatives(fmu, states_der, n_states)) != fmi1_status_ok) && (fmistatus != fmi1_status_warning)) break;
		/* print current variable values*/
		if(fmi1_write_csv_data(cdata, tcur) != jm_status_success) {
			jmstatus = jm_status_error;
			break;
		}

		/* integrate */
		for (k = 0; k < n_states; k++) {
			states[k] = states[k] + hcur*states_der[k];	
		}

		/* Set states */
		if(  ((fmistatus = fmi1_import_set_continuous_states(fmu, states, n_states)) != fmi1_status_ok) && (fmistatus != fmi1_status_warning)) break;
		/* Step is complete */
		if(  ((fmistatus = fmi1_import_completed_integrator_step(fmu, &callEventUpdate)) != fmi1_status_ok) && (fmistatus != fmi1_status_warning)) break;
	} /* while */

	 if((fmistatus != fmi1_status_ok) && (fmistatus != fmi1_status_warning)) {
		 jm_log_fatal(cb, fmu_checker_module, "Simulation loop terminated since FMU returned status: %s", fmi1_status_to_string(fmistatus));
		jmstatus = jm_status_error;
	 }
	 else if(jmstatus != jm_status_error) {
 		 jm_log_info(cb, fmu_checker_module, "Simulation finished successfully");
	 }

	if(  (fmistatus = fmi1_import_terminate(fmu)) != fmi1_status_ok) {
		 jm_log_error(cb, fmu_checker_module, "fmiTerminate returned status: %s", fmi1_status_to_string(fmistatus));
	}
	
	fmi1_import_free_model_instance(fmu);

	cb->free(states);
	cb->free(states_der);
	cb->free(event_indicators);
	cb->free(event_indicators_prev);

	return 	jmstatus;
}

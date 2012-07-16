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
	fmi1_real_t tcur, tnext;
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

	if(n_states) {
		states = cb->calloc(n_states, sizeof(double));
		states_der = cb->calloc(n_states, sizeof(double));
		if(!states || !states_der) {
			cb->free(states);
			cb->free(states_der);
			jm_log_fatal(cb, fmu_checker_module, "Could not allocated memory");
			return jm_status_error;
		}
	}
	if(n_event_indicators) {
		event_indicators = cb->calloc(n_event_indicators, sizeof(double));
		event_indicators_prev = cb->calloc(n_event_indicators, sizeof(double));

		if( !event_indicators || !event_indicators_prev) {
			cb->free(states);
			cb->free(states_der);
			cb->free(event_indicators);
			cb->free(event_indicators_prev);
			jm_log_fatal(cb, fmu_checker_module, "Could not allocated memory");
			return jm_status_error;
		}
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

	if( fmi1_status_ok_or_warning(fmistatus = fmi1_import_set_time(fmu, tstart)) &&
		fmi1_status_ok_or_warning(fmistatus = fmi1_import_initialize(fmu, toleranceControlled, relativeTolerance, &eventInfo)) &&
		( (n_states == 0) || 
		  fmi1_status_ok_or_warning(fmistatus = fmi1_import_get_continuous_states(fmu, states, n_states))
		) &&
		( (n_event_indicators == 0) || 
		  fmi1_status_ok_or_warning(fmistatus = fmi1_import_get_event_indicators(fmu, event_indicators_prev, n_event_indicators))
		)
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

	jm_log_verbose(cb, fmu_checker_module, "Writing simulation output for start time");
	if(fmi1_write_csv_data(cdata, tstart) != jm_status_success) {
		jmstatus = jm_status_error;
	}
	else while ((tcur < tend) && fmi1_status_ok_or_warning(fmistatus) ) {
		size_t k;
		int zero_crossning_event = 0;
		int time_event = 0;

		/* Get derivatives */
		if( (n_states > 0) &&  !fmi1_status_ok_or_warning(fmistatus = fmi1_import_get_derivatives(fmu, states_der, n_states))) {
			jm_log_fatal(cb, fmu_checker_module, "Could not retrieve time derivatives");
			break;
		}

		/* Choose time step and advance tcur */
		tnext = tcur + hdef;

		/* adjust tnext step to get tend exactly */ 
		if(tnext > tend - hdef/1e16) {
			tnext = tend;				
		}

		/* adjust for time events */ 
		if (eventInfo.upcomingTimeEvent && (tnext >= eventInfo.nextEventTime)) {
				tnext = eventInfo.nextEventTime;
				time_event = 1;
		}

		hcur = tnext - tcur;
		tcur = tnext;

		jm_log_verbose(cb, fmu_checker_module, "Simulation time: %g", tcur);
		if(  !fmi1_status_ok_or_warning(fmistatus = fmi1_import_set_time(fmu, tcur))) {
			jm_log_fatal(cb, fmu_checker_module, "Could not set simulation time to %g", tcur);
			break;
		}

		/* integrate */
		for (k = 0; k < n_states; k++) {
			states[k] = states[k] + hcur*states_der[k];	
		}

		/* Set states */
		if( (n_states > 0) && !fmi1_status_ok_or_warning(fmistatus = fmi1_import_set_continuous_states(fmu, states, n_states))) {
			jm_log_fatal(cb, fmu_checker_module, "Could not set continuous states");
			break;
		}
		/* Step is completed */
		if(  !fmi1_status_ok_or_warning(fmistatus = fmi1_import_completed_integrator_step(fmu, &callEventUpdate))){
			jm_log_fatal(cb, fmu_checker_module, "Could not complete integrator step");
			break;
		}

		/* Check if an event indicator has triggered */
		if( (n_event_indicators > 0) && 
			!fmi1_status_ok_or_warning(fmistatus = fmi1_import_get_event_indicators(fmu, event_indicators, n_event_indicators))
			) {
			jm_log_fatal(cb, fmu_checker_module, "Could not get event indicators");
			break;
		}

		for (k = 0; k < n_event_indicators; k++) {
			if (event_indicators[k]*event_indicators_prev[k] < 0) {
				zero_crossning_event = 1;
				break;
			}
		}

		/* Handle events */
		if (callEventUpdate || zero_crossning_event || time_event) {
			const char* eventKind;
			if(callEventUpdate) eventKind = "step";
			else if(zero_crossning_event) eventKind = "state";
			else eventKind = "time";
			jm_log_verbose(cb, fmu_checker_module, "Handling a %s event", eventKind);
			if( !fmi1_status_ok_or_warning(fmistatus = fmi1_import_eventUpdate(fmu, intermediateResults, &eventInfo))) {
				jm_log_fatal(cb, fmu_checker_module, "Event update call failed");
				break;
			}
			if(!eventInfo.iterationConverged) {
				jm_log_fatal(cb, fmu_checker_module, "FMU could not converge in event update");
				jmstatus = jm_status_error;
				break;
			}
			if( eventInfo.stateValuesChanged &&
				!fmi1_status_ok_or_warning(fmistatus = fmi1_import_get_continuous_states(fmu, states, n_states))) {
				jm_log_fatal(cb, fmu_checker_module, "Could not get continuous states");
				break;
			}
			if( (n_event_indicators > 0) && 
				!fmi1_status_ok_or_warning(fmistatus = fmi1_import_get_event_indicators(fmu, event_indicators_prev, n_event_indicators))) {
				jm_log_fatal(cb, fmu_checker_module, "Could not get event indicators");
				break;
			}
		}

		/* print current variable values*/
		if(fmi1_write_csv_data(cdata, tcur) != jm_status_success) {
			jmstatus = jm_status_error;
			break;
		}
		if(eventInfo.terminateSimulation) {
			jm_log_info(cb, fmu_checker_module, "FMU requested simulation termination");
			break;
		}
	} /* while */

	if(!fmi1_status_ok_or_warning(fmistatus)) {
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

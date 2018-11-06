/*
Copyright (C) 2012 Modelon AB <http://www.modelon.com>

You should have received a copy of the LICENSE-FMUChecker.txt
along with this program. If not, contact Modelon AB.
*/
/**
\file fmi2_me_sim.c
Simulation loop for the FMI 2.0 model exchange FMUs
*/
#include <fmuChecker.h>
#include <fmilib.h>


/*Helper event iteration*/
fmi2_status_t do_event_iteration(fmi2_import_t *fmu, fmi2_event_info_t *eventInfo)
{
	fmi2_status_t fmistatus = fmi2_status_ok;
	eventInfo->newDiscreteStatesNeeded = fmi2_true;
	eventInfo->terminateSimulation     = fmi2_false;
	while (eventInfo->newDiscreteStatesNeeded && !eventInfo->terminateSimulation) {
		fmistatus = fmi2_import_new_discrete_states(fmu, eventInfo);
	}
	return fmistatus;
}


jm_status_enu_t fmi2_me_simulate(fmu_check_data_t* cdata)
{	
	fmi2_status_t fmistatus;
	jm_status_enu_t jmstatus = jm_status_success;
	jm_callbacks* cb = &cdata->callbacks;

	fmi2_import_t* fmu = cdata->fmu2;
	fmi2_real_t tstart = fmi2_import_get_default_experiment_start(fmu);
	fmi2_real_t tcur, tnext;
	fmi2_real_t hcur;
	fmi2_real_t hdef;
	fmi2_real_t tend = fmi2_import_get_default_experiment_stop(fmu);
	size_t n_states;
	size_t n_event_indicators;
	fmi2_real_t* states = 0;
	fmi2_real_t* states_der = 0;
	fmi2_real_t* event_indicators = 0;
	fmi2_real_t* event_indicators_prev = 0;
	fmi2_boolean_t enterEventMode;
	fmi2_boolean_t terminateSimulation = fmi2_false;
	fmi2_boolean_t toleranceControlled = fmi2_false;
	fmi2_real_t relativeTolerance = fmi2_import_get_default_experiment_tolerance(fmu);
	fmi2_event_info_t eventInfo;
	fmi2_boolean_t intermediateResults = fmi2_false;

	prepare_time_step_info(cdata, &tend, &hdef);

	n_states = fmi2_import_get_number_of_continuous_states(fmu);	
	n_event_indicators = fmi2_import_get_number_of_event_indicators(fmu);

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

	cdata->instanceNameSavedPtr = 0;
	cdata->instanceNameToCompare = "Test FMI 2.0 ME";

	jmstatus = fmi2_import_instantiate(fmu, cdata->instanceNameToCompare,fmi2_model_exchange,0,0);

	cdata->instanceNameSavedPtr = cdata->instanceNameToCompare;

	if (jmstatus == jm_status_error) {
		jm_log_fatal(cb, fmu_checker_module, "Could not instantiate the model");
		cb->free(states);
		cb->free(states_der);
		cb->free(event_indicators);
		cb->free(event_indicators_prev);		
		return jm_status_error;
	}
	
	if (
		fmi2_status_ok_or_warning(fmistatus = fmi2_set_inputs(cdata, tstart)) &&
		fmi2_status_ok_or_warning(fmistatus =  fmi2_import_setup_experiment(fmu, toleranceControlled,relativeTolerance, tstart, fmi2_false, 0.0)) && 
		fmi2_status_ok_or_warning(fmistatus = fmi2_import_enter_initialization_mode(fmu)) &&
		fmi2_status_ok_or_warning(fmi2_import_exit_initialization_mode(fmu))) {

			tcur = tstart;
			hcur = hdef;
			enterEventMode = fmi2_false;

			eventInfo.newDiscreteStatesNeeded           = fmi2_false;
			eventInfo.terminateSimulation               = fmi2_false;
			eventInfo.nominalsOfContinuousStatesChanged = fmi2_false;
			eventInfo.valuesOfContinuousStatesChanged   = fmi2_true;
			eventInfo.nextEventTimeDefined              = fmi2_false;
			eventInfo.nextEventTime                     = -0.0;

			/* fmiExitInitializationMode leaves FMU in event mode */
			do_event_iteration(fmu, &eventInfo);

			if (!fmi2_status_ok_or_warning( fmistatus = fmi2_import_enter_continuous_time_mode(fmu))){
				jm_log_fatal(cb, fmu_checker_module, "Could not enter continuous time mode");
				jmstatus = jm_status_error;
			}

			if(( (n_states == 0) || 
				fmi2_status_ok_or_warning(fmistatus = fmi2_import_get_continuous_states(fmu, states, n_states))
				) &&
				( (n_event_indicators == 0) || 
				fmi2_status_ok_or_warning(fmistatus = fmi2_import_get_event_indicators(fmu, event_indicators_prev, n_event_indicators))
				)){
					jm_log_info(cb, fmu_checker_module, "Initialized FMU for simulation starting at time %g", tstart);
			}
			else {
				jm_log_fatal(cb, fmu_checker_module, "Failed to retrieve initial FMU states (FMU status: %s)", fmi2_status_to_string(fmistatus));
				fmistatus = fmi2_status_fatal;
				jmstatus = jm_status_error;
			}
	}
	else {
		jm_log_fatal(cb, fmu_checker_module, "Failed to initialize FMU for simulation (FMU status: %s)", fmi2_status_to_string(fmistatus));
		fmistatus = fmi2_status_fatal;
		jmstatus = jm_status_error;
	}


	if((jmstatus != jm_status_error) && (fmi2_write_csv_data(cdata, tstart) != jm_status_success)) {
		jmstatus = jm_status_error;
	}
	else while ((tcur < tend) && (!(eventInfo.terminateSimulation || terminateSimulation)) && fmi2_status_ok_or_warning(fmistatus) ) {
		size_t k;
		int zero_crossning_event = 0;
		int time_event = 0;

		/* Get derivatives */
		if( (n_states > 0) &&  !fmi2_status_ok_or_warning(fmistatus = fmi2_import_get_derivatives(fmu, states_der, n_states))) {
			if(fmistatus != fmi2_status_discard)
				jm_log_fatal(cb, fmu_checker_module, "Could not retrieve time derivatives");
			else
				jm_log_warning(cb, fmu_checker_module, "Could not retrieve time derivatives since FMU returned fmiDiscard");
			break;
		}

		/* Choose time step and advance tcur */
		tnext = tcur + hdef;

		/* adjust tnext step to get tend exactly */ 
		if(tnext > tend - hdef/1e16) {
			tnext = tend;				
		}

		/*Check for eternal events*/
		jmstatus = fmi2_check_external_events(tcur,tnext, &eventInfo, &cdata->fmu2_inputData);
		if( jmstatus > jm_status_warning) {
			jm_log_fatal(cb, fmu_checker_module, "Detection of input data events failed for Simtime %g", tcur);
			break;
		}

		/* adjust for time events */ 
		if (eventInfo.nextEventTimeDefined && (tnext >= eventInfo.nextEventTime)) {
			tnext = eventInfo.nextEventTime;
			time_event = 1;
		}

		hcur = tnext - tcur;
		tcur = tnext;

        /* Set time */
        jm_log_verbose(cb, fmu_checker_module, "Simulation time: %g", tcur);
        if (!fmi2_status_ok_or_warning(fmistatus = fmi2_import_set_time(fmu, tcur))) {
            jm_log_fatal(cb, fmu_checker_module, "Could not set simulation time to %g", tcur);
            break;
        }

		/* Set inputs */
		if(!fmi2_status_ok_or_warning(fmistatus = fmi2_set_inputs(cdata, tcur))) {
            jmstatus = jm_status_error;
            break;
        }

		/* integrate */
		for (k = 0; k < n_states; k++) {
			states[k] = states[k] + hcur*states_der[k];	
		}

		/* Set states */
		if( (n_states > 0) && !fmi2_status_ok_or_warning(fmistatus = fmi2_import_set_continuous_states(fmu, states, n_states))) {
			if(fmistatus != fmi2_status_discard)
				jm_log_fatal(cb, fmu_checker_module, "Could not set continuous states");
			else
				jm_log_warning(cb, fmu_checker_module, "Could not set continuous states since FMU returned fmiDiscard");
			break;
		}

		/* Check if an event indicator has triggered */
		if( (n_event_indicators > 0) && 
			!fmi2_status_ok_or_warning(fmistatus = fmi2_import_get_event_indicators(fmu, event_indicators, n_event_indicators))
			) {
				if(fmistatus != fmi2_status_discard)
					jm_log_fatal(cb, fmu_checker_module, "Could not get event indicators");
				else
					jm_log_warning(cb, fmu_checker_module, "Could not get event indicators since FMU returned fmiDiscard");
				break;
		}

		for (k = 0; k < n_event_indicators; k++) {
			if (event_indicators[k]*event_indicators_prev[k] < 0) {
				zero_crossning_event = 1;
				break;
			}
		}

		/* Step is completed */
		if(  !fmi2_status_ok_or_warning(fmistatus = fmi2_import_completed_integrator_step(fmu, fmi2_true, &enterEventMode, &terminateSimulation))){
			jm_log_fatal(cb, fmu_checker_module, "Could not complete integrator step");
			break;
		}

		/* Handle events */
		if (enterEventMode || zero_crossning_event || time_event) {
			const char* eventKind;
			if(enterEventMode) eventKind = "step";
			else if(zero_crossning_event) eventKind = "state";
			else eventKind = "time";
			jm_log_verbose(cb, fmu_checker_module, "Handling a %s event", eventKind);

			if(cdata->print_all_event_vars){
				/* print variable values before event handling*/
				if(fmi2_write_csv_data(cdata, tcur) != jm_status_success) {
				jmstatus = jm_status_error;
				}
			}

			if( !fmi2_status_ok_or_warning(fmistatus = fmi2_import_enter_event_mode(fmu))){
				jm_log_fatal(cb, fmu_checker_module, "Could not enter event mode");
				break;
			}

			if(!fmi2_status_ok_or_warning(fmistatus = do_event_iteration(fmu, &eventInfo))){
				jm_log_fatal(cb, fmu_checker_module, "Event iteration failed event mode");
				break;
			}

			if( eventInfo.valuesOfContinuousStatesChanged &&
				!fmi2_status_ok_or_warning(fmistatus = fmi2_import_get_continuous_states(fmu, states, n_states))) {
					jm_log_fatal(cb, fmu_checker_module, "Could not get continuous states");
					break;
			}
			if( eventInfo.nominalsOfContinuousStatesChanged &&
				!fmi2_status_ok_or_warning(fmistatus = fmi2_import_get_nominals_of_continuous_states(fmu, states, n_states))) {
					jm_log_fatal(cb, fmu_checker_module, "Could not get nominals of continuous states");
					break;
			}
			if( (n_event_indicators > 0) && 
				!fmi2_status_ok_or_warning(fmistatus = fmi2_import_get_event_indicators(fmu, event_indicators_prev, n_event_indicators))) {
				jm_log_fatal(cb, fmu_checker_module, "Could not get event indicators");
				break;
			}
			if( !fmi2_status_ok_or_warning(fmistatus = fmi2_import_enter_continuous_time_mode(fmu))){
				jm_log_fatal(cb, fmu_checker_module, "Could not enter continuous time mode");
				break;
			}
		}	
		/* print current variable values*/
		if(fmi2_write_csv_data(cdata, tcur) != jm_status_success) {
			jmstatus = jm_status_error;
			break;
		}
		if(eventInfo.terminateSimulation) {
			jm_log_info(cb, fmu_checker_module, "FMU requested simulation termination");
			break;
		}
	} /* while */

	if(fmistatus == fmi2_status_discard) {
		jm_log_warning(cb, fmu_checker_module, "Simulation loop terminated at time %g since FMU returned fmiDiscard. Running with shorter time step may help.", tcur);
	}
	else if(!fmi2_status_ok_or_warning(fmistatus)) {
		jm_log_fatal(cb, fmu_checker_module, "Simulation loop terminated at time %g since FMU returned status: %s", tcur, fmi2_status_to_string(fmistatus));
		jmstatus = jm_status_error;
	}
	else if(jmstatus != jm_status_error) {
		jm_log_info(cb, fmu_checker_module, "Simulation finished successfully at time %g", tcur);
	}

	if(fmistatus != fmi2_status_fatal) {
		if(  (fmistatus = fmi2_import_terminate(fmu)) != fmi2_status_ok) {
			jm_log_error(cb, fmu_checker_module, "fmiTerminate returned status: %s", fmi2_status_to_string(fmistatus));
		}

		fmi2_import_free_instance(fmu);
	}

	cb->free(states);
	cb->free(states_der);
	cb->free(event_indicators);
	cb->free(event_indicators_prev);

	return 	jmstatus;
}

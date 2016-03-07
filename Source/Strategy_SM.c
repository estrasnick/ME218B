/****************************************************************************
 Module
   Strategy_SM.c

 Description
   This is the top level state machine that controls overall game strategy,
	 including the selection of new targets and the the directing of the 
	 robot to search and capture stations.

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "helpers.h"
#include "definitions.h"
#include "Master_SM.h"
#include "Helpers.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "Strategy_SM.h"
#include "PositionLogic_Service.h"
#include "PeriscopeControl_Service.h"
#include "GameInfo.h"
#include "EnemyCaptureQueue.h"
#include "PWM_Service.h"
#include "HallEffect_SM.h"
#include "DriveTrainControl_Service.h"
#include "AttackStrategy_SM.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE Wait4Start_t


#define MANUALLY_SET_DESTINATION 4
#define MANUALLY_SET_MY_X 29.1f
#define MANUALLY_SET_MY_Y 75.8f
#define MANUALLY_SET_MY_THETA 180


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringWait4Start_t( ES_Event Event);
static ES_Event DuringWait4Zero_t( ES_Event Event);
static ES_Event DuringChooseDestination_t( ES_Event Event);
static ES_Event DuringFaceTarget_t( ES_Event Event);
static ES_Event DuringTravel_t( ES_Event Event);
static ES_Event DuringStationCapture_t( ES_Event Event);
static ES_Event DuringHandleCollision_t( ES_Event Event);

static void ChooseDestination(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static StrategyState_t CurrentState;

static uint8_t TargetStation = NULL_STATION;

static uint8_t timePeriod = 0;

static uint8_t retryCount;

static uint8_t PositionTimeoutCount;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunStrategySM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
****************************************************************************/
ES_Event RunStrategySM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   StrategyState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

	 // If we are waiting for position
	 if ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == POSITION_CHECK))
	 {
		 // if we don't have position
		 if (!IsAbsolutePosition())
		 {
			 // increment a timeout counter, and execute a backup if we are stalled
			 PositionTimeoutCount++;
			 if (PositionTimeoutCount >= POSITION_TIMEOUT_THRESHOLD)
			 {
				 PositionTimeoutCount = 0;
				 ExecuteBackup();
			 }
		 }
		 else
		 {
			 PositionTimeoutCount = 0;
		 }
	 }
	 
	 // If we are moving on to the next game phase
	 // Note: because framework timers are uint16_t types, we must restart the timer 2 times
	 //  to time a full 2:18 game time
	 if ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == GAME_TIMER))
	 {
		 if (timePeriod == 0)
		 {
			 ES_Timer_InitTimer(GAME_TIMER, GAME_TIMER_T);
			 ES_Timer_InitTimer(ATTACK_PHASE_TIMER, REV_T);
			 timePeriod = 1;
		 }
		 else if (timePeriod == 1)
		 {
			 ES_Timer_InitTimer(GAME_TIMER, GAME_TIMER_T);
			 timePeriod = 2;
		 }
		 // otherwise, game is over
		 else
		 {
			 // Return to the wait4start state
			 NextState = Wait4Start_t;
			 MakeTransition = true;
			 timePeriod = 0;
		 }
	 }
	 // If we need a new destination
	 else if (CurrentEvent.EventType == ES_RESET_DESTINATION)
	 {
		 // resume our positioning and choose a new destination
		 ResumePositioning();
		 NextState = ChooseDestination_t;
		 MakeTransition = true;
	 }
	 // if we've had a collision, go to the collision state
	 else if (CurrentEvent.EventType == ES_COLLISION)
	 {
		 NextState = HandleCollision_t;
		 MakeTransition = true;
	 }
	 
	 else
	 {
		 switch ( CurrentState )
		 {
			
			 case Wait4Start_t : 
			 {			 
				 // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
				 CurrentEvent = DuringWait4Start_t(CurrentEvent);
			 
				 //Process Any Events
				 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
				 {	
						//Check for Specific Events
					 // If we are starting the game
						if (CurrentEvent.EventType == ES_GAME_STARTED || CurrentEvent.EventType == ES_MANUAL_START)
						{
							NextState = Wait4Zero_t;
							MakeTransition = true;
						}
				 }
				 break;
			 }
			 
			 case Wait4Zero_t : 
			 {			 
				 // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
				 CurrentEvent = DuringWait4Zero_t(CurrentEvent);
			 
				 //Process Any Events
				 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
				 {	
					 // If we've zeroed our periscope
						if (CurrentEvent.EventType == ES_ZEROED)
						{
							NextState = ChooseDestination_t;
							MakeTransition = true;
						}
				 }
				 break;
			 }
			 
			 case ChooseDestination_t :
			 {			 
				 // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
				 CurrentEvent = DuringChooseDestination_t(CurrentEvent);
			 
				 //Process Any Events
				 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
				 {	
						//Check for Specific Events
					 // If we have a new destination
						if (CurrentEvent.EventType == ES_NEW_DESTINATION)
						{
							// pause positioning and get ready to rotate
							PausePositioning();
							NextState = FaceTarget_t; //ie. we want to exit and reenter
							MakeTransition = true;
						}
				 }
				 break;
			 }
			 
			 case FaceTarget_t :
			 {			 
				 // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
				 CurrentEvent = DuringFaceTarget_t(CurrentEvent);
			 
				 //Process Any Events
				 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
				 {	
						//Check for Specific Events
					 
					 // If our rotation is complete
						if (CurrentEvent.EventType == ES_ARRIVED)
						{
							NextState = Travel_t; //ie. we want to exit and reenter
							MakeTransition = true;
						}
						// If we are interrupted by the detection of a station
						else if (CurrentEvent.EventType == ES_PS_DETECTED)
						{
							// resume positioning, so we can start getting our position while capturing
							ResumePositioning();
							
							NextState = StationCapture_t; //ie. we want to exit and reenter
							MakeTransition = true;
						}
				 }
				 break;
			 }
			 
			 case Travel_t :
			 {			 
				 // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
				 CurrentEvent = DuringTravel_t(CurrentEvent);
			 
				 //Process Any Events
				 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
				 {	
						//Check for Specific Events
					 // If we are interrupted by the detection of a station
						if (CurrentEvent.EventType == ES_PS_DETECTED)
						{
							// resume positioning, so we can start getting our position while capturing
							ResumePositioning();
							
							NextState = StationCapture_t; //ie. we want to exit and reenter
							MakeTransition = true;		
						}
						// If we've arrived at our target (and we haven't detection a polling station)
						else if (CurrentEvent.EventType == ES_ARRIVED)
						{
							// Resume positioning
							ResumePositioning();
							
							// Choose new destination
							NextState = ChooseDestination_t;
							MakeTransition = true;
						}
				 }
				 break;
			 }
					 
			 case StationCapture_t :
			 {			 
				 // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
				 CurrentEvent = DuringStationCapture_t(CurrentEvent);
			 
				 //Process Any Events
				 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
				 {	
						//Check for Specific Events
					 // If we captured the station, choose a new destination
						if (CurrentEvent.EventType == ES_PS_CAPTURED)
						{
							NextState = ChooseDestination_t; //ie. we want to exit and reenter
							MakeTransition = true;
						}
						// otherwise, if we timed out, choose a new destination
						else if ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == CAPTURE_TIMEOUT_TIMER))
						{
						  NextState = ChooseDestination_t;
						  MakeTransition = true;
						}
				 }
				 break;
			 }
			 
			 case HandleCollision_t:
			 {
				 // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
				 CurrentEvent = DuringHandleCollision_t(CurrentEvent);
				 // If we are interrupted by the detection of a station
				 if (CurrentEvent.EventType == ES_PS_DETECTED)
				 {
				  // resume positioning, so we can start getting our position while capturing
					ResumePositioning();
				 	
					NextState = StationCapture_t; //ie. we want to exit and reenter
				 	MakeTransition = true;
				 }
				 else if (CurrentEvent.EventType == ES_ARRIVED)
				 {
				 	ResumePositioning();
				 	//printf("Arrived at target\r\n");
				 	NextState = ChooseDestination_t;
				 	MakeTransition = true;
				 }
			 }
		 }
	 }
	//   If we are making a state transition
	if (MakeTransition == true)
	{
		 //   Execute exit function for current state
		 CurrentEvent.EventType = ES_EXIT;
		 RunStrategySM(CurrentEvent);

		 CurrentState = NextState; //Modify state variable

		 //   Execute entry function for new state
		 // this defaults to ES_ENTRY
		 RunStrategySM(EntryEventKind);
	 }
	 return(ReturnEvent);
}
/****************************************************************************
 Function
     StartStrategySM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 2/18/99, 10:38AM
****************************************************************************/
void StartStrategySM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunStrategySM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryStrategySM

 Parameters
     None

 Returns
     RequestState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
StrategyState_t QueryStrategySM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringWait4Start_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
			
        // implement any entry actions required for this state machine
			
				// Set the game status light off
				GPIO_Clear(GAME_BASE, GAME_STATUS_PIN);
				
				// Attempt to zero the periscope
				RequireZero();
				PausePositioning();
			
        // after that start any lower level machines that run in this state
        
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
				
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        
        // repeat for any concurrently running state machines
        // now do any local exit functionality
			
				// Light the game status LED
				GPIO_Set(GAME_BASE, GAME_STATUS_PIN);
			
				// Start the game timer
				ES_Timer_InitTimer(GAME_TIMER, GAME_TIMER_T);
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringWait4Zero_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
				
        // implement any entry actions required for this state machine
			
				// if zeroed
				if (IsZeroed())
				{
					// begin calculating our position
					ResumePositioning();
					
					ES_Event ZeroEvent;
					ZeroEvent.EventType = ES_ZEROED;
					PostMasterSM(ZeroEvent);
				}
				else
				{
					// Else wait and check again
					ES_Timer_InitTimer(CHECK_ZERO_TIMER, CHECK_ZERO_T);
				}
        // after that start any lower level machines that run in this state
        
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
				
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				enableCaptureInterrupt(HALLSENSOR_INNER_LEFT_INTERRUPT_PARAMATERS);
				enableCaptureInterrupt(HALLSENSOR_INNER_RIGHT_INTERRUPT_PARAMATERS);
				enableCaptureInterrupt(HALLSENSOR_OUTER_LEFT_INTERRUPT_PARAMATERS);
				enableCaptureInterrupt(HALLSENSOR_OUTER_RIGHT_INTERRUPT_PARAMATERS);
			  
				enableCaptureInterrupt(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
			if ((Event.EventType == ES_TIMEOUT) && (Event.EventParam == CHECK_ZERO_TIMER))
			{
				// If zeroed
				if (IsZeroed())
				{
					// begin trying to find our position
					ResumePositioning();
					
					ES_Event ZeroEvent;
					ZeroEvent.EventType = ES_ZEROED;
					PostMasterSM(ZeroEvent);
				}
				else
				{
					// else wait and check again
					ES_Timer_InitTimer(CHECK_ZERO_TIMER, CHECK_ZERO_T);
				}
			}
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}




static ES_Event DuringChooseDestination_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
			
			// If we have our position, and the attack machine is not going
			if (IsAbsolutePosition() && (QueryAttackStrategySM() != Attack_t))
			{
				// choose a new destination
				ChooseDestination();
			}
			// Else wait for our position
			else if (QueryAttackStrategySM() != Attack_t)
			{
				ES_Timer_InitTimer(POSITION_CHECK, POSITION_CHECK_T);
			}
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
				
				//Post a Send Command 
				
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
			
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
				
				if ((Event.EventType == ES_TIMEOUT) && (Event.EventParam == POSITION_CHECK))
				{
					// If we have our position and we aren't attacking
					if (IsAbsolutePosition() && (QueryAttackStrategySM() != Attack_t))
					{
						// Choose a destination
						ChooseDestination();
					}
					// Else wait for position
					else if (QueryAttackStrategySM() != Attack_t)
					{
						ES_Timer_InitTimer(POSITION_CHECK, POSITION_CHECK_T);
					}
				}
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringFaceTarget_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				
				ES_Event NewEvent;
				NewEvent.EventType = ES_FACE_TARGET;
				PostPositionLogicService(NewEvent);

        // after that start any lower level machines that run in this state
        
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
				
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state

    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringTravel_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				ES_Event NewEvent;
				NewEvent.EventType = ES_DRIVE_TO_TARGET;
				PostPositionLogicService(NewEvent);
			
			// Note: relative positioning was disabled for the competition version
			//  of the software
				//ES_Timer_InitTimer(RELATIVE_POSITION_TIMER, RELATIVE_POSITION_T);
			
        // after that start any lower level machines that run in this state
        
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
				
				
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				ResetEncoderTicks(); 
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringStationCapture_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
			
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringHandleCollision_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				
				// execute backup
        setTargetEncoderTicks(BACK_UP_TICKS, BACK_UP_TICKS, true, true);
			
				// Record that the given target was obstructed
				MarkObstructed(TargetStation);
			
        // after that start any lower level machines that run in this state
        
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
			
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static void ChooseDestination(void)
{
	uint8_t bestStation = 0;
	uint32_t bestPriority = 0xffffffff;
	
	// For each station, compare their current priorities
	for (int i = 0; i < NUM_STATIONS; i++)
	{
		uint32_t priority;
		
		//If we own the station or it is undefined, ie. not used, we should never
		// go to it
		if ((GetStationOwner(i) == MyColor()) | (GetStationOwner(i) == Undefined_b))
		{
			priority = 0xffffffff;
		}
		// otherwise, if we remember that we were obstructed trying to reach this station
		else if (IsObstructed(i))
		{
			priority = 0xffffffff - 1;
		}
		else
		{			
			// otherwise, compute the priority. We weigh the following factors:
			priority = (PRI_CAPTURE_HISTORY_MULTIPLIER * PositionInQueue(i)) // has the enemy captured it?
			+ (RETRY_MULTIPLIER * ((i == TargetStation) ? retryCount : 0)) // have we tried to get it and failed?
			+ (PRI_DISTANCE_MULTIPLIER * DistanceToPoint(GetStationX(i), GetStationY(i)) // how far are we from it?
			+ GetIntrinsicPriority(i)); // do we instrinsically weigh some stations over others? (i.e. they are 
																	// 	in more favorable locations)
		}
		
		// If this is a better choice than our previous best, store it
		if (priority < bestPriority)
		{
			bestPriority = priority;
			bestStation = i;
		}
		
	}
	
	// If we just tried for this station, increment the retry count
	if (bestStation == TargetStation)
	{
		retryCount++;
	}
	else
	{
		// otherwise, reset the retry count
		retryCount = 0;
	}
	
	// Set our new target
	TargetStation = bestStation;
	SetTargetLocation(GetStationX(bestStation), GetStationY(bestStation));
	
	ES_Event NewEvent;
	NewEvent.EventType = ES_NEW_DESTINATION;
	PostMasterSM(NewEvent);
}

// Return the current station target
uint8_t GetTargetStation(void)
{
	return TargetStation;
}

// Stop positioning
void PausePositioning(void)
{
	// note our intent to stop the periscope
	SetAttemptingToStop(true);
	
	// raise the servo latch
	LatchPeriscope();
	
	// disable interrupts
	disableCaptureInterrupt(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);
	
}

// resume positioning
void ResumePositioning(void)
{
	// note that we do not want the periscope to stop
	SetAttemptingToStop(false);
	
	// enable interrupts
	enableCaptureInterrupt(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);
	
	// raise the periscope latch
	UnlatchPeriscope();
	
	// start the periscope
	SetPWM_Periscope(PERISCOPE_PWM_DUTY);
}

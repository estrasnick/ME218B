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
#include "DriveTrainControl_Service.h"

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
static ES_Event DuringChooseDestination_t( ES_Event Event);
static ES_Event DuringFaceTarget_t( ES_Event Event);
static ES_Event DuringTravel_t( ES_Event Event);
static ES_Event DuringStationCapture_t( ES_Event Event);

static void ChooseDestination(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static StrategyState_t CurrentState;

static uint8_t TargetStation;

static uint8_t timePeriod = 0;

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

	 if ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == GAME_TIMER))
	 {
		 if (timePeriod == 0)
		 {
			 ES_Timer_InitTimer(GAME_TIMER, GAME_TIMER_T);
			 ES_Timer_InitTimer(ATTACK_PHASE_TIMER, ATTACK_PHASE_T);
			 timePeriod = 1;
		 }
		 else if (timePeriod == 1)
		 {
			 ES_Timer_InitTimer(GAME_TIMER, GAME_TIMER_T);
			 timePeriod = 2;
		 }
		 else
		 {
			 NextState = Wait4Start_t;
			 MakeTransition = true;
			 timePeriod = 0;
		 }
	 }
	 else if (CurrentEvent.EventType == ES_RESET_DESTINATION)
	 {
		 ResumePositioning();
		 NextState = ChooseDestination_t;
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
						if (CurrentEvent.EventType == ES_TRANSACTION_COMPLETE)
						{
							if (isGameStarted())
							{
								NextState = ChooseDestination_t;
								MakeTransition = true;
							}
						}
						else if (CurrentEvent.EventType == ES_MANUAL_START)
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
						if (CurrentEvent.EventType == ES_NEW_DESTINATION)
						{
							PausePositioning();
							NextState = FaceTarget_t; //ie. we want to exit and reenter
							MakeTransition = true;
							printf("Transitioning to FaceTarget_t\r\n");
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
						if (CurrentEvent.EventType == ES_ARRIVED)
						{
							printf("ES_Arrived in FaceTarget\r\n");
							NextState = Travel_t; //ie. we want to exit and reenter
							MakeTransition = true;
						}
						else if (CurrentEvent.EventType == ES_PS_DETECTED)
						{
							printf("ES_PS_DETECTED in FaceTarget\r\n");
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
						if (CurrentEvent.EventType == ES_PS_DETECTED)
						{
							NextState = StationCapture_t; //ie. we want to exit and reenter
							MakeTransition = true;
							ResumePositioning();
						}
						else if (CurrentEvent.EventType == ES_ARRIVED)
						{
							ResumePositioning();
							printf("Arrived at target\r\n");
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
						if (CurrentEvent.EventType == ES_PS_CAPTURED)
						{
							if (!checkResponseReadyByte()){
								NextState = ChooseDestination_t; //ie. we want to exit and reenter
								MakeTransition = true;
							}
						}
				 }
				 break;
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
				GPIO_Clear(GAME_BASE, GAME_STATUS_PIN);
        // after that start any lower level machines that run in this state
        
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
				
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				GPIO_Set(GAME_BASE, GAME_STATUS_PIN);
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

static ES_Event DuringChooseDestination_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
			if (IsAbsolutePosition())
				{
					ChooseDestination();
				// after that start any lower level machines that run in this state
				}
				else
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
					if (IsAbsolutePosition())
					{
						ChooseDestination();
					}
					else
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
				
				printf("Posting a Face_Target event to position logic \r\n");
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
			
				ES_Timer_InitTimer(RELATIVE_POSITION_TIMER, RELATIVE_POSITION_T);
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

static void ChooseDestination(void)
{
	printf("Choosing destination\r\n");
	uint8_t bestStation = NULL_STATION;
	uint32_t bestPriority = 0xffffffff;
	
	for (int i = 0; i < NUM_STATIONS; i++)
	{
		uint32_t priority;
		
		if (GetStationOwner(i) == MyColor())
		{
			priority = 0xffffffff;
		}
		/*else if (IsObstructed(i))
		{
			priority = 0xffffffff - 1;
		}*/
		else
		{
			printf("position in queue: %d\r\n", PositionInQueue(i));
			
			priority = (PRI_CAPTURE_HISTORY_MULTIPLIER * PositionInQueue(i))
			+ (PRI_DISTANCE_MULTIPLIER * DistanceToPoint(GetStationX(i), GetStationY(i)));
		}
		
		if (priority < bestPriority)
		{
			bestPriority = priority;
			bestStation = i;
		}
		
		printf("Location: %d, priority: %d\r\n", i , priority);
	}
	
	TargetStation = bestStation;
	SetTargetLocation(GetStationX(bestStation), GetStationY(bestStation));
	switch (TargetStation)
	{
		case (0):
		{
			printf("Choosing Destination: Sacramento\r\n");
			break;
		}
		case (1):
		{
			printf("Choosing Destination: Seattle\r\n");
			break;
		}
		case (2):
		{
			printf("Choosing Destination: Billings\r\n");
			break;
		}
		case (3):
		{
			printf("Choosing Destination: Denver\r\n");
			break;
		}
		case (4):
		{
			printf("Choosing Destination: Dallas\r\n");
			break;
		}
		case (5):
		{
			printf("Choosing Destination: Chicago\r\n");
			break;
		}
		case (6):
		{
			printf("Choosing Destination: Miami\r\n");
			break;
		}
		case (7):
		{
			printf("Choosing Destination: Washington\r\n");
			break;
		}
		case (8):
		{
			printf("Choosing Destination: Concord\r\n");
			break;
		}
	}
	ES_Event NewEvent;
	NewEvent.EventType = ES_NEW_DESTINATION;
	PostMasterSM(NewEvent);
}

uint8_t GetTargetStation(void)
{
	return TargetStation;
}

void PausePositioning(void)
{
	printf("Disabling positioning\r\n");
	LatchPeriscope();
	disableCaptureInterrupt(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);
}

void ResumePositioning(void)
{
	printf("Enabling positioning\r\n");
	enableCaptureInterrupt(PHOTOTRANSISTOR_INTERRUPT_PARAMATERS);
	UnlatchPeriscope();
}

/****************************************************************************
 Module
   CapturePS_SM.c

 Description
   Handles the various phases of trying to capture a polling station

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "Definitions.h"
#include "Helpers.h"
#include "GameInfo.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "Master_SM.h"
#include "PACLogic_SM.h"
#include "CapturePS_SM.h"
#include "Request_SM.h"
#include "SendingCMD_SM.h"
#include "SendingByte_SM.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE Request1_t

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringMeasuring1_t( ES_Event Event);
static ES_Event DuringRequest1_t( ES_Event Event);
static ES_Event DuringMeasuring2_t( ES_Event Event);
static ES_Event DuringRequest2_t( ES_Event Event);
static ES_Event DuringMeasuring3_t( ES_Event Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static CapturePSState_t CurrentState;

static uint8_t storedLocation;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunCapturePSSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
****************************************************************************/
ES_Event RunCapturePSSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   CapturePSState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
	 
   switch ( CurrentState )
   {
        case Measuring1_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringMeasuring1_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//If we received an EV_PS Detected
						if (CurrentEvent.EventType == ES_PS_DETECTED)
						{									
								NextState = Request1_t;
								MakeTransition = true;
						}
					}
         break;
			
				case Request1_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringRequest1_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//If the Transactin is Complete and the Response was Ready
						if ((CurrentEvent.EventType == ES_TRANSACTION_COMPLETE) && (checkResponseReadyByte()))
						{									
								//Check if we were acknowledged and the location is correct
								if ((checkAcknowledged() == ACK_b) && (checkLocation()))
								{
									//printf("Polling Station Confirmed 1: Switching Frequencies \n\r");
									NextState = Measuring2_t;
									MakeTransition = true;
								} else {	//if not exit and go back to measuring									
									//printf("Got a NACK, go back to measuring1_t \n\r");
									NextState = Measuring1_t;
									MakeTransition = true;
								}
						}
         }
         break;
				 
				case Measuring2_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringMeasuring2_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//If we received an EV_PS Detected
						if (CurrentEvent.EventType == ES_PS_DETECTED)
						{									
								NextState = Request2_t;
								MakeTransition = true;
						}
         }
         break;
				 
				case Request2_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringRequest2_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//If the Transactin is Complete and the Response was Ready
						if ((CurrentEvent.EventType == ES_TRANSACTION_COMPLETE) && (checkResponseReadyByte()))
						{									
								//Check if we were acknowledged and the location is correct
								if ((checkAcknowledged() == ACK_b) && (checkLocation()))
								{
									//printf("Polling Station Confirmed 2: Captured, time to move on \n\r");
								
									storedLocation = getLocation();
									SetStationOwner(storedLocation, MyColor());
									NextState = Measuring3_t;
									MakeTransition = true;
								} else {	//if not exit and re-enter as measuring 1 to see if we can get it right the second time	around			
									NextState = Measuring1_t;
									MakeTransition = true;
								}
						}
         }
         break;	 
				 
				 //Measure a Third Time to prevent recapture attempts
				 case Measuring3_t :     
				// Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringMeasuring3_t(CurrentEvent); 
				 
				 //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//If we received an EV_PS Detected
						if (CurrentEvent.EventType == ES_PS_DETECTED)
						{		
									//Update Our Own Frequency with what we just remeasured
									updateCapturedFrequency(storedLocation, GetTargetFrequencyIndex());
									//printf("Setting station owner for location: %d to color %d\r\n", getLocation(), MyColor());
									
									//Now Post that we Captured and it's time to move on
									ES_Event ThisEvent;
									ThisEvent.EventType = ES_PS_CAPTURED;								
									PostMasterSM(ThisEvent);
									
								//No Need to Transition Out
								MakeTransition = false;
						}
         }
         break;
    } 
	 
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunCapturePSSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunCapturePSSM(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartCapturePSSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
****************************************************************************/
void StartCapturePSSM ( ES_Event CurrentEvent )
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
   RunCapturePSSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryCapturePSSM

 Returns
     CapturePSState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
****************************************************************************/
CapturePSState_t QueryCapturePSSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/
static ES_Event DuringMeasuring1_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				ES_Timer_InitTimer(MEASURING_TIMEOUT_TIMER, MEASURING_TIMEOUT_T);
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				ES_Timer_StopTimer(MEASURING_TIMEOUT_TIMER);
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringRequest1_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        // after that start any lower level machines that run in this state
        StartRequestSM(Event);
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunRequestSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality												
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_PS_MEASURING;								
				PostMasterSM(ThisEvent);
				
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        ReturnEvent = RunRequestSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


static ES_Event DuringMeasuring2_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				ES_Timer_InitTimer(MEASURING_TIMEOUT_TIMER, MEASURING_TIMEOUT_T);
			
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				ES_Timer_StopTimer(MEASURING_TIMEOUT_TIMER);
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


static ES_Event DuringRequest2_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        // after that start any lower level machines that run in this state
         StartRequestSM(Event);
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunRequestSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				
				//Start Measuring Again
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_PS_MEASURING;								
				PostMasterSM(ThisEvent);
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        ReturnEvent = RunRequestSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state

    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringMeasuring3_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				ES_Timer_InitTimer(MEASURING_TIMEOUT_TIMER, MEASURING_TIMEOUT_T);
			
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
			
				//Start Measuring Again
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_PS_MEASURING;								
				PostMasterSM(ThisEvent);
			
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				ES_Timer_StopTimer(MEASURING_TIMEOUT_TIMER);
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

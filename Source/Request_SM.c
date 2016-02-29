/****************************************************************************
 Module
   Request.c

 Description
   This is the top level state machine of the PAC Logic controlling communication with the SUPER PAC

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "helpers.h"
#include "definitions.h"
#include "Master_SM.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "PACLogic_SM.h"
#include "CapturePS_SM.h"
#include "Request_SM.h"
#include "SendingCMD_SM.h"
#include "SendingByte_SM.h"
#include "GameInfo.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE Request_t

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringRequest_t( ES_Event Event);
static ES_Event DuringQuery_t( ES_Event Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static RequestState_t CurrentState;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunRequestSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
****************************************************************************/
ES_Event RunRequestSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   RequestState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

   switch ( CurrentState )
   {
    
		 case Request_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringRequest_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//Check for Specific Events
            if (CurrentEvent.EventType == ES_TRANSACTION_COMPLETE)
            {
							NextState = Query_t;
							MakeTransition = true;
            }
         }
         break;    
		 
		 case Query_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringQuery_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//Check for Specific Events
            if (CurrentEvent.EventType == ES_TRANSACTION_COMPLETE)
            {
							if (!checkResponseReadyByte()){
								NextState = Query_t; //ie. we want to exit and reenter
								MakeTransition = true;
							}
            }
         }
         break;
		 
   }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunRequestSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunRequestSM(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartRequestSM

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
void StartRequestSM ( ES_Event CurrentEvent )
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
   RunRequestSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryRequestSM

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
RequestState_t QueryRequestSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringRequest_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        StartSendingCMDSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
				
				//Post a Send Command 
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_SEND_CMD;
				ThisEvent.EventParam = GetRequestCommand(MyColor(), MyColor(), GetTargetFrequencyIndex());
				printf("Color: %d \n\r", MyColor());
				printf("Target Frequency Indexed: %d \n\r", GetTargetFrequencyIndex());
				printf("Frequency Code Sent: %x \n\r", PS_Frequency_Codes[GetTargetFrequencyIndex()]);
				PostMasterSM(ThisEvent);
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunSendingCMDSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
				//Get the Response Array and print it
				uint8_t *byte;
				byte = getResponseArray();

				printf("Byte Received from Request Request:  %x, %x, %x, %x, %x \n\r", 
				*(byte + 0), 
				*(byte + 1), 
				*(byte + 2), 
				*(byte + 3), 
				*(byte + 4));
			
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        ReturnEvent = RunSendingCMDSM(Event);
      
        // repeat for any concurrent lower level machines

      
			
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringQuery_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
			
        // after that start any lower level machines that run in this state
        StartSendingCMDSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        
				//Post a Send Command 
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_SEND_CMD;
				ThisEvent.EventParam = QUERY_COMMAND;
				PostMasterSM(ThisEvent);
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunSendingCMDSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
							//Get the Response Array and print it
			
				uint8_t *byte;
				byte = getResponseArray();
			
			/*
				printf("Querying:  %x, %x, %x, %x, %x \n\r", 
				*(byte + 0), 
				*(byte + 1), 
				*(byte + 2), 
				*(byte + 3), 
				*(byte + 4));
			*/
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
				ReturnEvent = RunSendingCMDSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
			        // repeat for any concurrent lower level machines

			

    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

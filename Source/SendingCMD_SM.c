/****************************************************************************
 Module
   SendingCMD.c

 Description
   Sends a command via SPI

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "Definitions.h"
#include "Helpers.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "Master_SM.h"
#include "PACLogic_SM.h"
#include "CapturePS_SM.h"
#include "Request_SM.h"
#include "SendingCMD_SM.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE Waiting_t

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringWaiting_t( ES_Event Event);
static ES_Event DuringWaiting4Interrupt_t( ES_Event Event);
static ES_Event DuringWaiting4Timeout_t( ES_Event Event);

static void checkForPACError(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static SendingCMDState_t CurrentState;

static uint8_t responseArray[5];
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunSendingCMDSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
****************************************************************************/
ES_Event RunSendingCMDSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   SendingCMDState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
 
   switch ( CurrentState )
   {
  
		 case Waiting_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringWaiting_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//We've been commanded to Send a Command
            if (CurrentEvent.EventType == ES_SEND_CMD)
            {
								EnterCritical();
								//Write to the Data Register our First Command
								WriteDataRegister(CurrentEvent.EventParam);
							
								for (int i = 0; i < 4; i++){
									WriteDataRegister(0x00);
								}
								ExitCritical();
							
								NextState = Waiting4Interrupt_t;
								MakeTransition = true;
            }
         }
         break;
				 
			case Waiting4Interrupt_t :     
				 // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
				 CurrentEvent = DuringWaiting4Interrupt_t(CurrentEvent);
			 
				 //Process Any Events
				 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
				 {	
						//Check for Specific Events
						if (CurrentEvent.EventType == ES_EOT)
						{							
								EnterCritical();
								for (int i = 0; i < 5; i++)
								{
									responseArray[i] = ReadDataRegister();
								}
								ExitCritical();
								NextState = Waiting4Timeout_t;
								MakeTransition = true;
						}
				 }
				 break;
				 
			case Waiting4Timeout_t :     
				 // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
				 CurrentEvent = DuringWaiting4Timeout_t(CurrentEvent);
			 
				 //Process Any Events
				 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
				 {	
						//Check for Specific Events
						if ((CurrentEvent.EventType == ES_TIMEOUT) && CurrentEvent.EventParam == SSI_TIMER)
						{
								ES_Event DoneEvent;
								DoneEvent.EventType = ES_TRANSACTION_COMPLETE;
								PostMasterSM(DoneEvent);
								NextState = Waiting_t;
								MakeTransition = true;					
						}
				 }
				 break;
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunSendingCMDSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunSendingCMDSM(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartSendingCMDSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
****************************************************************************/
void StartSendingCMDSM ( ES_Event CurrentEvent )
{
	//Initialize our PAC Error LED 
	GPIO_Init(PACERROR_SYSTCL, PACERROR_BASE, PAC_ERROR_PIN, OUTPUT);
	GPIO_Clear(PACERROR_BASE,PAC_ERROR_PIN);
	
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunSendingCMDSM(CurrentEvent);
}

/****************************************************************************
 Function
     QuerySendingCMDSM

 Returns
     SendingCMDState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
****************************************************************************/
SendingCMDState_t QuerySendingCMDSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
During Functions
 ***************************************************************************/
static ES_Event DuringWaiting_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
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

static ES_Event DuringWaiting4Interrupt_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
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
				checkForPACError();
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

static ES_Event DuringWaiting4Timeout_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				//Start the Timer that will ultimately get us our desired timeout
				ES_Timer_InitTimer(SSI_TIMER, SSI_TIMER_LENGTH);
			
			
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

/***************************************************************************
Query Response Array and Information
 ***************************************************************************/

uint8_t * getResponseArray(){
	return responseArray;
}

static void checkForPACError(void){
	//If the first byte received was FF, the PAC is in an error state
	if (responseArray[0] == 0xff){
		GPIO_Set(PACERROR_BASE, PAC_ERROR_PIN);
	} else {
		GPIO_Clear(PACERROR_BASE, PAC_ERROR_PIN);
	}
}





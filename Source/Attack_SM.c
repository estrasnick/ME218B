/****************************************************************************
 Module
   PAC Logic.c

 Description
   This is the top level state machine of the PAC Logic controlling communication with the SUPER PAC

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as we00ll as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "Attack_SM.h"
#include "CapturePS_SM.h"
#include "Request_SM.h"
#include "SendingCMD_SM.h"
#include "SendingByte_SM.h"
#include "CannonControl_Service.h"
#include "PositionLogic_Service.h"
#include "PWM_Service.h"
#include "DEFINITIONS.h"
#include "Strategy_SM.h"
#include "PeriscopeControl_Service.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE Align_and_StartCannon_t

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringAlign_and_StartCannon_t( ES_Event Event);
static ES_Event DuringAligned_t( ES_Event Event);
static ES_Event DuringCannonReady_t( ES_Event Event);
static ES_Event DuringFire_t( ES_Event Event);
static void LoadChamber(void);
static void DriveHammer(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static AttackState_t CurrentState;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunAttackSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
****************************************************************************/
ES_Event RunAttackSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   AttackState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

	 if ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == HOPPER_LOAD_TIMER))
	 {
		 DriveHammer();
	 }
	 
   switch ( CurrentState )
   {
       case Align_and_StartCannon_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringAlign_and_StartCannon_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//Check for Specific Events
            if (CurrentEvent.EventType == ES_CANNON_READY /*|| ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == CANNON_READY_TIMER))*/) //comes from Cannon Control Service when we reach our an RPM error of zero
            {
							printf("Cannon ready\r\n");
							NextState = CannonReady_t;
							MakeTransition = true;
            }
						else if (CurrentEvent.EventType == ES_ALIGNED_TO_BUCKET)
						{
							printf("Aligned to bucket\r\n");
							NextState = Aligned_t;
							MakeTransition = true;
						}
         }
         break;
        
			 case Aligned_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringAligned_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//Check for Specific Events
            if (CurrentEvent.EventType == ES_CANNON_READY /*|| ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == CANNON_READY_TIMER))*/)
            {
							printf("Cannon ready\r\n");
							NextState = Fire_t;
							MakeTransition = true;
            }
         }
         break;
           
				case CannonReady_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringCannonReady_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//Check for Specific Events
            if (CurrentEvent.EventType == ES_ALIGNED_TO_BUCKET)
						{
							printf("Aligned to bucket\r\n");
							NextState = Fire_t;
							MakeTransition = true;
						}
         }
         break;
 			
				case Fire_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringFire_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//Check for Specific Events
            
         }
         break;     
				 
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunAttackSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunAttackSM(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartAttackSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
****************************************************************************/
void StartAttackSM ( ES_Event CurrentEvent )
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
   RunAttackSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryAttackSM

 Parameters
     None

 Returns
     AttackState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
AttackState_t QueryAttackSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringAlign_and_StartCannon_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
			// implement any entry actions required for this state machine
			//Pause our Positioning using the Periscope
			printf("entering attack state machine\r\n");
			LatchPeriscope();
			SetAttemptingToStop(true);
			
			//Post an Align Event in order to use the latch to align the periscope
			ES_Event AlignEvent;
			AlignEvent.EventType = ES_ALIGN_TO_BUCKET;
			PostPeriscopeControlService(AlignEvent);
			
			//Post to Start the Cannon
			ES_Event StartCannonEvent;
			StartCannonEvent.EventType = ES_START_CANNON;
			PostCannonControlService(StartCannonEvent);
			
			//ES_Timer_InitTimer(CANNON_READY_TIMER, CANNON_READY_T);
			
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


static ES_Event DuringAligned_t( ES_Event Event)
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

static ES_Event DuringCannonReady_t( ES_Event Event)
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


static ES_Event DuringFire_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        ES_Timer_InitTimer(ATTACK_COMPLETE_TIMER, ATTACK_COMPLETE_T);
				LoadChamber();
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
				ES_Event StopCannonEvent;
				StopCannonEvent.EventType = ES_STOP_CANNON;
				PostCannonControlService(StopCannonEvent);
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

static void LoadChamber(void)
{
	printf("Load Hopper \n\r");
	SetPWM_Hopper(HOPPER_LOAD_DUTY);
	ES_Timer_InitTimer(HOPPER_LOAD_TIMER, HOPPER_LOAD_T);
}

static void DriveHammer(void)
{
	SetPWM_Hopper(HOPPER_DEFAULT_DUTY);
}

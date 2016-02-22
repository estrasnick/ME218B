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

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "AttackStrategy_SM.h"
#include "CapturePS_SM.h"
#include "Request_SM.h"
#include "SendingCMD_SM.h"
#include "SendingByte_SM.h"
#include "DriveTrainControl_Service.h"
#include "Master_SM.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE Wait4AttackPhase_t

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringWait4AttackPhase_t( ES_Event Event);
static ES_Event DuringAttack_t( ES_Event Event);
static ES_Event DuringWait4NextAttack_t( ES_Event Event);


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static AttackStrategyState_t CurrentState;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunAttackStrategySM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
****************************************************************************/
ES_Event RunAttackStrategySM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   AttackStrategyState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

   switch ( CurrentState )
   {
       case Wait4AttackPhase_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringWait4AttackPhase_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//Check for Specific Events
            if ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == ATTACK_PHASE_TIMER))
						{
							NextState = Attack_t;
							MakeTransition = true;
						}
         }
         break;
        
			 case Attack_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringAttack_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//Check for Specific Events
					 if ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == ATTACK_COMPLETE_TIMER))
            {
							NextState = Wait4NextAttack_t;
							MakeTransition = true;
            }
         }
         break;
           
				case Wait4NextAttack_t :     
         // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
         CurrentEvent = DuringWait4NextAttack_t(CurrentEvent);
			 
         //Process Any Events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {	
						//Check for Specific Events
            if ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == ATTACK_PHASE_TIMER))
            {
							NextState = Attack_t;
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
       RunAttackStrategySM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunAttackStrategySM(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartAttackStrategySM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
****************************************************************************/
void StartAttackStrategySM ( ES_Event CurrentEvent )
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
   RunAttackStrategySM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryAttackStrategySM

 Parameters
     None

 Returns
     AttackStrategyState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
AttackStrategyState_t QueryAttackStrategySM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringWait4AttackPhase_t( ES_Event Event)
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


static ES_Event DuringAttack_t( ES_Event Event)
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
			ES_Timer_InitTimer(ATTACK_PHASE_TIMER, NEXT_SHOT_T);
			
			ES_Event ResetEvent;
			ResetEvent.EventType = ES_RESET_DESTINATION;
			PostMasterSM(ResetEvent);
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

static ES_Event DuringWait4NextAttack_t( ES_Event Event)
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

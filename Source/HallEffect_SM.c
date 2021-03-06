/****************************************************************************
 Module
   HallEffect_SM.c

 Description
   Manages the hall effect sensors and the identification of polling stations

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "Helpers.h"
#include "Definitions.h"
#include "GameInfo.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "Master_SM.h"
#include "HallEffect_SM.h"
#include "CapturePS_SM.h"
#include "Request_SM.h"
#include "SendingCMD_SM.h"
#include "SendingByte_SM.h"
#include "PACLogic_SM.h"
#include "DriveTrainControl_Service.h"
#include "AttackStrategy_SM.h"
/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE Measure_t

#define PERIOD_MEASURING_ERROR_TOLERANCE 8 //in micr
#define NUMBER_PULSES_TO_STOP 4
#define NUMBER_HALL_EFFECT_SENSORS 4
#define NUMBER_FREQUENCIES 16

//Pound Define Index in Array Corresponding to Hall Effect Position
#define LEFT_OUTER_INDEX 0
#define LEFT_INNER_INDEX 1
#define RIGHT_INNER_INDEX 2
#define RIGHT_OUTER_INDEX 3

#define MICROSECONDS 1000000
#define NULL_PERIOD_INDEX 100 //we want to return this if we found no frequencies
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringMeasure_t( ES_Event Event);
static ES_Event DuringRequesting_t( ES_Event Event);

static void initializeHEInterrupts(void);
static void enableHEInterrupts(void);
static void disableHEInterrupts(void);

static void updateBuckets(uint32_t CurrentPeriod);
static bool toleranceCheck(uint32_t value, uint32_t target, uint32_t tol);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static HallEffectState_t CurrentState;

static uint8_t buckets[NUMBER_FREQUENCIES];

//Create Array with different Possibile Periods in Microseconds (note although it says _f these are actually directly periods in Microseconds
uint32_t HallEffect_P[] = {	
	HE_f_1000, 
	HE_f_947, 
	HE_f_893,
	HE_f_840, 
	HE_f_787, 
	HE_f_733,
	HE_f_680, 
	HE_f_627, 
	HE_f_573,
	HE_f_520, 
	HE_f_467, 
	HE_f_413,
	HE_f_360,
	HE_f_307, 
	HE_f_253, 
	HE_f_200};

//Last Capture from Interrupts
	uint32_t lastCaptureOuterLeft;
	uint32_t lastCaptureInnerLeft;
	uint32_t lastCaptureInnerRight;
	uint32_t lastCaptureOuterRight;
	
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunHallEffectSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
****************************************************************************/
ES_Event RunHallEffectSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   HallEffectState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

	 // If we stop seeing pulses on the hall sensor, reset
	 if ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == HALL_EFFECT_TIMEOUT_TIMER))
	 {
	 	 for (int i = 0; i < NUMBER_FREQUENCIES; i++)
	 	 {
	 	 	 buckets[i] = 0;
	 	 }
		 
		 // Go back to the measuring state
		 ES_Event MeasureEvent;
		 MeasureEvent.EventType = ES_PS_MEASURING;
		 PostMasterSM(MeasureEvent);
	 }
	 else
	 {
		 switch ( CurrentState )
		 {
				 case Measure_t :     
					 // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
					 CurrentEvent = DuringMeasure_t(CurrentEvent);
				 
					 //Process Any Events
					 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
					 {	
							//Check for Specific Events
							// If we detect a polling station frequency
							if (CurrentEvent.EventType == ES_PS_DETECTED)
							{
								MakeTransition = true;
								NextState = Requesting_t;
							}
					 }
					 break;
					
				 case Requesting_t :     
					 // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
					 CurrentEvent = DuringRequesting_t(CurrentEvent);
				 
					 //Process Any Events
					 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
					 {	
							//Check for Specific Events
							// If we are supposed to return to measuring
							if (CurrentEvent.EventType == ES_PS_MEASURING)
							{
								MakeTransition = true;
								NextState = Measure_t;
							}
					 }
					 break;
			
			}
		}
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunHallEffectSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunHallEffectSM(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartHallEffectSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
****************************************************************************/
void StartHallEffectSM ( ES_Event CurrentEvent )
{
  //Initialize the Hall Effect Interrupts
	initializeHEInterrupts();
	
	// Disable the hall interrupts. They will restart when the game starts
	disableHEInterrupts();
	
	// to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunHallEffectSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryHallEffectSM

 Parameters
     None

 Returns
     HallEffectState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
HallEffectState_t QueryHallEffectSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
During Functions
 ***************************************************************************/

static ES_Event DuringMeasure_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				//Enable all our Hall Effect Interrupts
				enableHEInterrupts();
					
			
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
			
				//disable interrupts
				disableHEInterrupts();
      
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


static ES_Event DuringRequesting_t( ES_Event Event)
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

/***************************************************************************
Interrupt Helper Functinos
 ***************************************************************************/
//Initializes all the Hall Effect Sensor Interrupts
void initializeHEInterrupts(void){
		InitInputCapture(HALLSENSOR_OUTER_LEFT_INTERRUPT_PARAMATERS);
		InitInputCapture(HALLSENSOR_INNER_LEFT_INTERRUPT_PARAMATERS);
		InitInputCapture(HALLSENSOR_INNER_RIGHT_INTERRUPT_PARAMATERS);
		InitInputCapture(HALLSENSOR_OUTER_RIGHT_INTERRUPT_PARAMATERS);
}

//Enables all the Hall Effect Sensor Interrupts
void enableHEInterrupts(void){
		enableCaptureInterrupt(HALLSENSOR_OUTER_LEFT_INTERRUPT_PARAMATERS);
		enableCaptureInterrupt(HALLSENSOR_INNER_LEFT_INTERRUPT_PARAMATERS);
		enableCaptureInterrupt(HALLSENSOR_INNER_RIGHT_INTERRUPT_PARAMATERS);
		enableCaptureInterrupt(HALLSENSOR_OUTER_RIGHT_INTERRUPT_PARAMATERS);	
}

//Disables all the Hall Effect Sensor Interrupts
void disableHEInterrupts(void){
		disableCaptureInterrupt(HALLSENSOR_OUTER_LEFT_INTERRUPT_PARAMATERS);
		disableCaptureInterrupt(HALLSENSOR_INNER_LEFT_INTERRUPT_PARAMATERS);
		disableCaptureInterrupt(HALLSENSOR_INNER_RIGHT_INTERRUPT_PARAMATERS);
		disableCaptureInterrupt(HALLSENSOR_OUTER_RIGHT_INTERRUPT_PARAMATERS);	
}


/***************************************************************************
Interrupt Responses
 ***************************************************************************/
//OUTER LEFT 
void HE_OuterLeft_InterruptResponse(void){
	//Clear the Source of the Interrupt
	clearCaptureInterrupt(HALLSENSOR_OUTER_LEFT_INTERRUPT_PARAMATERS);
	//printf("outerleft\r\n");
	// Determine Capture time
	uint32_t ThisCapture = captureInterrupt(HALLSENSOR_OUTER_LEFT_INTERRUPT_PARAMATERS);
	
	// Calculate period ( in microseconds)
	uint32_t Period = ((ThisCapture - lastCaptureOuterLeft) * MICROSECONDS_DIVISOR) / TICKS_PER_MS;
	
	//Update Our Last Capture
	lastCaptureOuterLeft = ThisCapture;
	
	// Update our knowledge of the most recent periods
	updateBuckets(Period);
}

//INNER LEFT
void HE_InnerLeft_InterruptResponse(void){
	//Clear the Source of the Interrupt
	clearCaptureInterrupt(HALLSENSOR_INNER_LEFT_INTERRUPT_PARAMATERS);
	//printf("innerleft\r\n");
	// Determine Capture time
	uint32_t ThisCapture = captureInterrupt(HALLSENSOR_INNER_LEFT_INTERRUPT_PARAMATERS);
	
	// Calculate period ( in microseconds)
	uint32_t Period = ((ThisCapture - lastCaptureInnerLeft) * MICROSECONDS_DIVISOR) / TICKS_PER_MS;
	
	//Update Our Last Capture
	lastCaptureInnerLeft = ThisCapture;
	
	// Update our knowledge of the most recent periods
	updateBuckets(Period);
}

//INNER RIGHT
void HE_InnerRight_InterruptResponse(void){
	//Clear the Source of the Interrupt
	clearCaptureInterrupt(HALLSENSOR_INNER_RIGHT_INTERRUPT_PARAMATERS);
	//printf("innerright\r\n");
	// Determine Capture time
	uint32_t ThisCapture = captureInterrupt(HALLSENSOR_INNER_RIGHT_INTERRUPT_PARAMATERS);
	
	// Calculate period ( in microseconds)
	uint32_t Period = ((ThisCapture - lastCaptureInnerRight) * MICROSECONDS_DIVISOR) / TICKS_PER_MS;
	
	//Update Our Last Capture
	lastCaptureInnerRight = ThisCapture;
	
	// Update our knowledge of the most recent periods
	updateBuckets(Period);
}

//OUTER RIGHT
void HE_OuterRight_InterruptResponse(void){
	//Clear the Source of the Interrupt
	clearCaptureInterrupt(HALLSENSOR_OUTER_RIGHT_INTERRUPT_PARAMATERS);
	//printf("outerright\r\n");
	// Determine Capture time
	uint32_t ThisCapture = captureInterrupt(HALLSENSOR_OUTER_RIGHT_INTERRUPT_PARAMATERS);
	
	// Calculate period ( in microseconds)
	uint32_t Period = ((ThisCapture - lastCaptureOuterRight) * MICROSECONDS_DIVISOR) / TICKS_PER_MS;
	
	//Update Our Last Capture
	lastCaptureOuterRight = ThisCapture;
	
	// Update our knowledge of the most recent periods
	updateBuckets(Period);
}






/***************************************************************************
Update the Array with Periods to shift everything down whenever we get a new one
 ***************************************************************************/
void updateBuckets(uint32_t CurrentPeriod){
	
	if (!CheckGameStarted())
	{
		return;
	}
	
	//Shift Periods Down
	for (int i = 0; i < NUMBER_FREQUENCIES; i++){
			// If the period matches a legitimate period per the field spec
			if (toleranceCheck(CurrentPeriod, HallEffect_P[i], PERIOD_MEASURING_ERROR_TOLERANCE))
			{
				// Increment the number of periods seen for that frequency
				buckets[i]++;
				
				// If we have seen enough of a given frequency to consider it a match
				if (buckets[i] > NUMBER_PULSES_TO_STOP)
				{
					// If we do not own this frequency
					if (!checkOwnFrequency(i)){
						//Disable the Interrupts
						disableHEInterrupts();
						
						// Stop the timeout timer, as we have a match
						ES_Timer_StopTimer(HALL_EFFECT_TIMEOUT_TIMER);
						 
						//Post to master that we detected a polling station
						ES_Event ThisEvent;
						ThisEvent.EventType = ES_PS_DETECTED;
						ThisEvent.EventParam = i; //Pass Index Over
						
						//set the target frequency index
						SetTargetFrequencyIndex(i);
						
						PostMasterSM(ThisEvent);
						
					}
					
					// reset all buckets
					for (int i = 0; i < NUMBER_FREQUENCIES; i++){
						buckets[i] = 0;
					}
				}
				else
				{
					// Restart the hall effect timeout timer
					ES_Timer_InitTimer(HALL_EFFECT_TIMEOUT_TIMER, HALL_EFFECT_TIMEOUT_T);
				}
				
				return;
			}
	}
}

// Returns true if the value matches the target within a specified tolerance
static bool toleranceCheck(uint32_t value, uint32_t target, uint32_t tol){
	//Check if within Tolerance
	if ( (value <= target + tol) && (value >= target - tol)){
		return true;
	} else {
		return false;
	}
}	

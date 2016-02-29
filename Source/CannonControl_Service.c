/****************************************************************************
 Module
   CannonControl_Service.c

 Description
		
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "CannonControl_Service.h"
#include "Helpers.h"
#include "Definitions.h"
#include "PWM_Service.h"
#include "Math.h"
#include "Master_SM.h"
#include "PositionLogic_Service.h"

/*----------------------------- Module Defines ----------------------------*/
//Define Gains
#define STARTUP_P_GAIN 2.5f

#define CONTROL_P_GAIN .000095f
#define CONTROL_D_GAIN 0.0035f
#define I_GAIN .000075f

#define INTEGRAL_CLAMP_MIN 0
#define INTEGRAL_CLAMP_MAX 250

//Cannon Test Speeds in RPM
#define CANNON_STOP_SPEED 0
#define CANNON_TEST_PWM 20
#define CANNON_TEST_RPM 2500

#define CANNON_RPM_TOLERANCE 2000

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

static void calculateControlResponse(float currentRPM);
static float CalculateRPM(void);
static bool SpeedCheck(float rpm);
static float DetermineCannonSpeed(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

//Encoder Input Capture Variables
static uint32_t Period;
static uint32_t LastCapture;

//Target RPM
static float RPMTarget;

//Controls
float integralTerm = 0.0; /* integrator control effort */

bool Revving = false;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitCannonControlService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

****************************************************************************/
bool InitCannonControlService ( uint8_t Priority )
{
  ES_Event ThisEvent;
	
  MyPriority = Priority;
	
	GPIO_Init(CANNONANGLE_SYSCTL, CANNONANGLE_BASE, CANNON_DIRECTION_PIN, OUTPUT);
	GPIO_Clear(CANNONANGLE_BASE, CANNON_DIRECTION_PIN);
	
	//Initialize Our Input Captures for Encoder
	InitInputCapture(CANNON_ENCODER_INTERRUPT_PARAMATERS);
	
	//Initialize Periodic Interrupt for Control Laws
	InitPeriodic(CANNON_CONTROL_INTERRUPT_PARAMATERS);
	
	//Start the Cannon at Rest
	setTargetCannonSpeed(CANNON_TEST_RPM);	//thsi should be zero but we are experimenting for testing
	
	//Set Hopper to proper position
	SetPWM_Hopper(HOPPER_DEFAULT_DUTY);
	
	// post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService( MyPriority, ThisEvent) == true)
  {
      return true;
  }else
  {
      return false;
  }
}

/****************************************************************************
 Function
     PostCannonControlService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise
****************************************************************************/
bool PostCannonControlService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunCannonControlService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

****************************************************************************/
ES_Event RunCannonControlService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

	if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == CANNON_STOPPED_TIMER)
	{
		Period = 0;
	}
	
	//If we receive any of these events (from keyboard presses)
		switch (ThisEvent.EventType){
			case (ES_START_CANNON):
				{
					//For Actual Implementation
					Revving = true;
					//setTargetCannonSpeed(DetermineCannonSpeed());
					
					//For Testing 
					setTargetCannonSpeed(CANNON_TEST_RPM);
				}
				break;
			case (ES_STOP_CANNON):
				{
					//For Actual Implementation
					Revving = false;
					//setTargetCannonSpeed(CANNON_STOP_PWM);
					
					//For Testing 
					setTargetCannonSpeed(0);
				}
				break;
				case (ES_TIMEOUT):
				{
					//if the cannon is stoped
					if (ThisEvent.EventParam == CANNON_STOPPED_TIMER){
						printf(" \n\r \n\r CANNON MOTOR STOPPED!!!! \n\r\n\r");
						
						//Set the Period to zero
						Period = 0;
					}
				}
				break;
			}
			
			
  return ReturnEvent;
}

/***************************************************************************
Interrupt Responses
 ***************************************************************************/
void CannonEncoder_InterruptResponse(void){
	uint32_t ThisCapture;

	// start by clearing the source of the interrupt, the input capture event
	clearCaptureInterrupt(CANNON_ENCODER_INTERRUPT_PARAMATERS);

	// now grab the captured value and calculate the period
	ThisCapture = captureInterrupt(CANNON_ENCODER_INTERRUPT_PARAMATERS);


  //Update the Period based on the difference between the two rising edges
	uint32_t tempPeriod = ThisCapture - LastCapture;

	//Tracker for how many times in sequence we think we have missed encoder ticks
	static uint8_t numTimesMissed;
	
	//Check to See if We Have Missed any Encoder Ticks by comparing the last period to the new period and making sure this hasn't happened twice
	if ((tempPeriod > 1.5 * Period) && numTimesMissed < 3){
		//printf(" \n\r I think the cannon missed an encoder tick \n\r");
		numTimesMissed++;
		//don't update the period
	} else {
		//Set the Period equal to the new Period
		Period = tempPeriod;
		
		//Set the error flag to zero
		numTimesMissed = 0;
	}
		
	// update LastCapture to prepare for the next edge
	LastCapture = ThisCapture;
	
	//Restart the Timer for the Cannon being Stopped
	ES_Timer_InitTimer(CANNON_STOPPED_TIMER, CANNON_STOPPED_T);
}

//Interrupt Response to Manage our Control Feedback loop to the motors
void CannonControl_PeriodicInterruptResponse(void){
	// start by clearing the source of the interrupt
	clearPeriodicInterrupt(CANNON_CONTROL_INTERRUPT_PARAMATERS);
	
	//Calculate RPM
	static float currentRPM;
	currentRPM = CalculateRPM();
	
	if (Revving)
	{
		if (SpeedCheck(currentRPM))
		{
			ES_Event NewEvent;
			NewEvent.EventType = ES_CANNON_READY;
			PostMasterSM(NewEvent);
			Revving = false;
		}
	}
	
	//Calculate Control Response individually
	calculateControlResponse(currentRPM);
}

/***************************************************************************
Control Law
 ***************************************************************************/
static void calculateControlResponse(float currentRPM){
	static float RPMError; /* make static for speed */
	static float LastError; /* for Derivative Control */

	
	//Calculate Error (absolute)
	RPMError = fabs(RPMTarget) - currentRPM; //fabs is absolute value

	/*
	//If the RPM error is zero and hasn't been before then post that we are at the correct speed
	if ((RPMError == 0) & (LastError != 0)){
		printf("Post ES_CANNON_READY \n\r"); 
		ES_Event ThisEvent;
		ThisEvent.EventType = ES_CANNON_READY;
		PostMasterSM(ThisEvent);
	}*/

	
	//Determine Integral Term
	integralTerm += I_GAIN * RPMError;
	integralTerm = clamp(integralTerm, INTEGRAL_CLAMP_MIN, INTEGRAL_CLAMP_MAX); /* anti-windup */
	
	
	//if we are below the target use D gain if not set it to zero
	static float D_GAIN ;
	if (RPMError > 0)
	{
		D_GAIN = CONTROL_D_GAIN;
	} else {
		D_GAIN = 0;
	}
	
	//if the RPM Error is less than 50% of the target RPM then assume we are in start up
	static float P_GAIN ;
	if (RPMError > .25 * RPMTarget)
	{
		P_GAIN = STARTUP_P_GAIN;
		D_GAIN = 0; //don't want any derviative gain
	} else {
		P_GAIN = CONTROL_P_GAIN;
	}
	

	
	
	//As we cannot brake, ie. only of one direction of control, lets
	uint8_t RequestedDuty = 0;
//	if (RPMError > 0){
		//Calculate Desired Duty Cycle
		RequestedDuty = P_GAIN*RPMError + D_GAIN * (RPMError-LastError) + 	integralTerm;		//(P_GAIN * ((RPMError)+integralTerm+(D_GAIN * (RPMError-LastError))));
//	}

	/*
	static int i;
	if (i++ >= 100)
	{
		printf("CANNON RPM: %f, duty: %d\r\n", currentRPM, RequestedDuty);
		i = 0;
	}*/
	
	
		//For Printing
	static uint8_t i;
	i++; //add to i
	if (i > 150){
		printf("TargetRPM: %f, 		CurrentRPM: %f,     RPMError: %f,      duty: %d, 		integralTerm: %f \n\r", RPMTarget, currentRPM, RPMError, RequestedDuty, integralTerm);
			i = 0; //reset i
	}
	
	
	//Save the Last Error
	LastError = RPMError;
		
	//Call the Set PWM Function on the clamped RequestedDuty Value
	SetPWM_Cannon(clamp(RequestedDuty, 0, 100));
	//SetPWM_Cannon(CANNON_TEST_PWM);
	//SetPWM_Cannon(0);
}



/****************************************************************************
 Function
     Set the New Target Speed Function
****************************************************************************/
void setTargetCannonSpeed(uint32_t newCannonRPM){
	RPMTarget = newCannonRPM;
	ES_Timer_InitTimer(CANNON_STOPPED_TIMER, CANNON_STOPPED_T);
}

//Returns the RPM
static float CalculateRPM(void) 
{
	if (Period == 0)
	{
		return 0;
	}
	else
	{
		return ((TICKS_PER_MS * SECS_PER_MIN * MS_PER_SEC) / ((float) Period * FLYWHEEL_GEAR_RATIO * ENCODER_PULSES_PER_REV));
	}
}

// Return true if the cannon has reached its target RPM
static bool SpeedCheck(float rpm)
{
	return (rpm <= RPMTarget + CANNON_RPM_TOLERANCE) && (rpm >= RPMTarget - CANNON_RPM_TOLERANCE);
}

static float DetermineCannonSpeed(void)
{	return DetermineDistanceToBucket() * CANNON_DISTANCE_MULTIPLIER;
}

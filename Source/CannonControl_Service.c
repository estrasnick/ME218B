/****************************************************************************
 Module
   CannonControl_Service.c

 Description
		Controls the flywheel motor on our cannon
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
#define STARTUP_THRESH .2f

#define CONTROL_P_GAIN_BELOW .00145f
#define CONTROL_D_GAIN_BELOW 0.0000f
#define I_GAIN_BELOW .000095f

#define CONTROL_P_GAIN_ABOVE .0000001f
#define CONTROL_D_GAIN_ABOVE 0.000015f
#define I_GAIN_ABOVE .00055f

#define INTEGRAL_CLAMP_MIN -100
#define INTEGRAL_CLAMP_MAX 100

//Cannon Test Speeds in RPM
#define CANNON_TEST_PWM 25
#define CANNON_TEST_RPM 3500
#define CANNON_RPM_TOLERANCE 200

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

static void calculateControlResponse(float currentRPM);
static float CalculateRPM(void);
static bool SpeedCheck(float rpm);
static float DetermineCannonSpeed(void);
static uint16_t SpeedCheckTimeoutCounter;

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
	setTargetCannonSpeed(0);
		
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

	// If the flywheel has stopped, set a zero period
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
					setTargetCannonSpeed(DetermineCannonSpeed());
					
					//For Testing 
					//setTargetCannonSpeed(CANNON_TEST_RPM);
				}
				break;
			case (ES_STOP_CANNON):
				{
					//For Actual Implementation
					Revving = false;
					setTargetCannonSpeed(REV_SPEED);
					
					//For Testing 
					//setTargetCannonSpeed(0);
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
	Period = ThisCapture - LastCapture;
		
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
	
	// If we're supposed to get the cannon up to speed
	if (Revving)
	{
		// If the cannon is at its target speed, post it
		if (SpeedCheck(currentRPM))
		{
			ES_Event NewEvent;
			NewEvent.EventType = ES_CANNON_READY;
			PostMasterSM(NewEvent);
			Revving = false;
			SpeedCheckTimeoutCounter = 0;
		}
		else
		{
			// Otherwise, increment a timeout counter
			SpeedCheckTimeoutCounter++;
			if (SpeedCheckTimeoutCounter > SPEED_CHECK_LIMIT)
			{
				ES_Event NewEvent;
				NewEvent.EventType = ES_CANNON_READY;
				PostMasterSM(NewEvent);
				Revving = false;
				SpeedCheckTimeoutCounter = 0;
			}
		}
	}
	
	//Calculate Control Response
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

	//Depending on if we are below or above our target, change the value of our paramaters
	bool above = (RPMError <= 0);
	

	//Determine Integral Term
	integralTerm += (above ? I_GAIN_ABOVE : I_GAIN_BELOW ) * RPMError;
	integralTerm = clamp(integralTerm, INTEGRAL_CLAMP_MIN, INTEGRAL_CLAMP_MAX); /* anti-windup */
	
	
	//if we are below the target use D gain if not set it to zero
	static float D_GAIN ;
	//if (RPMError > 0)
	//{
	D_GAIN = above ? CONTROL_D_GAIN_ABOVE : CONTROL_D_GAIN_BELOW;
	//} else {
	//	D_GAIN = 0;
	//}
	
	//if the RPM Error is less than 50% of the target RPM then assume we are in start up
	static float P_GAIN ;
	if (RPMError > (STARTUP_THRESH) * RPMTarget)
	{
		P_GAIN = STARTUP_P_GAIN;
	} else {
		P_GAIN = above ? CONTROL_P_GAIN_ABOVE*fabs(RPMError) : CONTROL_P_GAIN_BELOW;
	}
	
	
	float proportionalResponse = P_GAIN*RPMError;
	float derviativeResponse = D_GAIN * (RPMError-LastError);
	float integralResponse = integralTerm;
	
	if(RPMError-LastError != 0){
	}
	
	float RequestedDuty = proportionalResponse + derviativeResponse + 	integralTerm;		//(P_GAIN * ((RPMError)+integralTerm+(D_GAIN * (RPMError-LastError))));

	
	//Save the Last Error
	LastError = RPMError;
	
	//Call the Set PWM Function on the clamped RequestedDuty Value
	if (RPMTarget != 0)
	{
		SetPWM_Cannon(clamp(RequestedDuty, -50, 50));
	}
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

// Determine how fast the cannon should rev given its distance to the bucket
// Use a quadratic relationship
static float DetermineCannonSpeed(void)
{	
	float dist = (DetermineDistanceToBucket() + DISTANCE_FROM_BEACON_TO_BUCKET_CENTER);
	return dist * dist * CANNON_SLOPE_MULTIPLIER;
}

/****************************************************************************
 Module
   DriveTrainControl_Service.c

 Description
		
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "DriveTrainControl_Service.h"
#include "Helpers.h"
#include "Definitions.h"
#include "PWM_Service.h"
#include "Math.h" 
#include "Master_SM.h"

#include "inc/hw_timer.h"
#include "PositionLogic_Service.h"

/*----------------------------- Module Defines ----------------------------*/
//Define Gains
#define P_GAIN 1.92f
#define D_GAIN  0.0f //2.5f
#define I_GAIN .25f

//Test Conditions
#define FULL_SPEED 100.0f
#define HALF_SPEED_L 70.0f
#define HALF_SPEED_R 63.0f
#define STOP_SPEED 0.0f
#define ROTATE_90_TIME 500
#define ROTATE_45_TIME 300

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

static uint8_t calculateControlResponse(uint32_t ThisPeriod, float integralTerm, float targetSpeed, bool isRight);
static void implementControlResponse(uint8_t left, uint8_t right);
static float CalculateRPM(uint32_t period);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

//Encoder Input Capture Variables
static uint32_t Left_Period;
static uint32_t Left_LastCapture;
static uint32_t Right_Period;
static uint32_t Right_LastCapture;
static uint32_t Stored_Left_Period;
static uint32_t Stored_Left_LastCapture;
static uint32_t Stored_Right_Period;
static uint32_t Stored_Right_LastCapture;

//Encoder Tick Counters
static uint32_t LeftEncoderTicks;
static uint32_t RightEncoderTicks;
static uint32_t Stored_LeftEncoderTicks;
static uint32_t Stored_RightEncoderTicks;

//Motor Stopped Timer (milliseconds)
static uint8_t motor_stopped_timer = 150; //ie, if no encoder ticks received within this amount we post an event

//Target RPM (negative implies backwards)
static float RPMTarget_Left;
static float RPMTarget_Right;

//TargetTicks
static uint32_t TargetTicks_Left;
static uint32_t TargetTicks_Right;

//Controls
static float integralTerm_Left = 0.0; /* integrator control effort */
static float integralTerm_Right = 0.0; /* integrator control effort */
static float LastError_Left = 0;
static float LastError_Right = 0;

static bool isMoving = false; //initialize to false




/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitDriveTrainControlService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

****************************************************************************/
bool InitDriveTrainControlService ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;

	//Initialize Our Input Captures for Each Powertrain Encoder
	InitInputCapture(DRIVE_LEFT_ENCODER_INTERRUPT_PARAMATERS);
	InitInputCapture(DRIVE_RIGHT_ENCODER_INTERRUPT_PARAMATERS);
	
	//Initialize Periodic Interrupt for Control Laws
	InitPeriodic(DRIVE_CONTROL_INTERRUPT_PARAMATERS);
  
	//Initialize Direction Pins
	GPIO_Init(DRIVE_SYSCTL, DRIVE_BASE, DRIVE_DIRECTION_LEFT_PIN, OUTPUT);
	GPIO_Init(DRIVE_SYSCTL, DRIVE_BASE, DRIVE_DIRECTION_RIGHT_PIN, OUTPUT);

	SetPWM_DriveLeft(0.0, LEFT_DRIVE_FORWARD_PIN_DIRECTION);
	SetPWM_DriveRight(0.0, RIGHT_DRIVE_FORWARD_PIN_DIRECTION);
	
	printf("Drive Control Service Initialized \n\r");
	
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
     PostDriveTrainControlService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise
****************************************************************************/
bool PostDriveTrainControlService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunDriveTrainControlService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

****************************************************************************/
ES_Event RunDriveTrainControlService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
	//printf("Drive train run function \r\n");
	//If we are in testing mode
	if(TESTING_MODE){
		
		//If we received a timeout from our rotate timer post a ES_STOP_DRIVE event
		if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == ROTATE_TESTING_TIMER)){
			ES_Event NewEvent;
			NewEvent.EventType = ES_STOP_DRIVE;
			PostDriveTrainControlService(NewEvent); //post to stop driving
		}
		
		//If we receive any of these events (from keyboard presses)
		switch (ThisEvent.EventType){
			case (ES_DRIVE_FULL_SPEED):
				printf("Driving forward full speed\r\n");
				SetPWM_DriveLeft(FULL_SPEED, LEFT_DRIVE_FORWARD_PIN_DIRECTION);
				SetPWM_DriveRight(FULL_SPEED, RIGHT_DRIVE_FORWARD_PIN_DIRECTION);
				break;
			case (ES_DRIVE_HALF_SPEED):
				printf("Driving forward half speed\r\n");
				setTargetEncoderTicks(250, 250, 0, 0);
				//setTargetDriveSpeed(80, 80);
				//SetPWM_DriveLeft(HALF_SPEED_L, LEFT_DRIVE_FORWARD_PIN_DIRECTION);
				//SetPWM_DriveRight(HALF_SPEED_R, RIGHT_DRIVE_FORWARD_PIN_DIRECTION);		
				
				break;
			case (ES_REVERSE_HALF_SPEED):
				printf("Driving reverse half speed\r\n");
				SetPWM_DriveLeft(HALF_SPEED_L, LEFT_DRIVE_BACKWARD_PIN_DIRECTION);
				SetPWM_DriveRight(HALF_SPEED_R, RIGHT_DRIVE_BACKWARD_PIN_DIRECTION);
				break;
			case (ES_STOP_DRIVE):
				setTargetDriveSpeed(0, 0);
				//SetPWM_DriveLeft(STOP_SPEED, LEFT_DRIVE_FORWARD_PIN_DIRECTION);
				//SetPWM_DriveRight(STOP_SPEED, RIGHT_DRIVE_FORWARD_PIN_DIRECTION);
				break;
			case (ES_ROTATE_45):
				SetPWM_DriveLeft(HALF_SPEED_L, LEFT_DRIVE_FORWARD_PIN_DIRECTION);
				SetPWM_DriveRight(HALF_SPEED_R, RIGHT_DRIVE_BACKWARD_PIN_DIRECTION);
				ES_Timer_InitTimer(ROTATE_TESTING_TIMER, ROTATE_45_TIME);
				break;
			case (ES_ROTATE_90):
				SetPWM_DriveLeft(HALF_SPEED_L, LEFT_DRIVE_FORWARD_PIN_DIRECTION);
				SetPWM_DriveRight(HALF_SPEED_R, RIGHT_DRIVE_BACKWARD_PIN_DIRECTION);
				ES_Timer_InitTimer(ROTATE_TESTING_TIMER, ROTATE_90_TIME);
				break;	
			default:
				break;
		}
	}
	
	if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == MOTOR_STOPPED_L))
	{
		//printf("MOTOR_STOPPED left timer\r\n");
		EnterCritical();
		Left_Period = 0;
		Left_LastCapture = 0;
		ExitCritical();
	}
	else if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == MOTOR_STOPPED_R))
	{
		//printf("MOTOR_STOPPED right timer\r\n");
		EnterCritical();
		Right_Period = 0;
		Right_LastCapture = 0;
		ExitCritical();
	}
	
  return ReturnEvent;
}

/***************************************************************************
Interrupt Responses
 ***************************************************************************/
void DriveEncoder_Left_InterruptResponse(void){
	uint32_t ThisCapture;

	// start by clearing the source of the interrupt, the input capture event
	clearCaptureInterrupt(DRIVE_LEFT_ENCODER_INTERRUPT_PARAMATERS);
	//printf("Left Encoder interrupt\r\n");
	
	// increment the encoder ticks that we have seen
	LeftEncoderTicks++;
	
	// Check if we've reached our target, and if so, stop
	if (LeftEncoderTicks >= TargetTicks_Left)
	{
		setTargetDriveSpeed(0, RPMTarget_Right);
		if (RPMTarget_Right == 0)
		{
			ES_Event NewEvent;
			NewEvent.EventType = ES_ARRIVED;
			PostMasterSM(NewEvent);
			isMoving = false;
			ResetEncoderTicks();
		}
	}
	
	// now grab the captured value and calculate the period
	ThisCapture = captureInterrupt(DRIVE_LEFT_ENCODER_INTERRUPT_PARAMATERS);

  //Update the Period based on the difference between the two rising edges
	Left_Period = ThisCapture - Left_LastCapture;
 //printf("ThisCapture: %d, LastCapture: %d, Period: %d\r\n", ThisCapture, Left_LastCapture, Left_Period);
	// update LastCapture to prepare for the next edge
	Left_LastCapture = ThisCapture;
	
	//Start the Motor Stopped Timer Again
	ES_Timer_InitTimer(MOTOR_STOPPED_L, motor_stopped_timer);
	
}


void DriveEncoder_Right_InterruptResponse(void){
	uint32_t ThisCapture;

	// start by clearing the source of the interrupt, the input capture event
	clearCaptureInterrupt(DRIVE_RIGHT_ENCODER_INTERRUPT_PARAMATERS);

	//printf("Right Encoder interrupt\r\n");
	
	// increment the encoder ticks that we have seen
	RightEncoderTicks++;
	
	// Check if we've reached our target, and if so, stop
	if (RightEncoderTicks >= TargetTicks_Right)
	{
		setTargetDriveSpeed(RPMTarget_Left, 0);
		if (RPMTarget_Left == 0)
		{
			ES_Event NewEvent;
			NewEvent.EventType = ES_ARRIVED;
			PostMasterSM(NewEvent);
			isMoving = false;
			ResetEncoderTicks();
		}
	}
	
	// now grab the captured value and calculate the period
	ThisCapture = captureInterrupt(DRIVE_RIGHT_ENCODER_INTERRUPT_PARAMATERS);

  //Update the Period based on the difference between the two rising edges
	Right_Period = ThisCapture - Right_LastCapture;
	//printf("ThisCapture: %d, LastCapture: %d, Period: %d\r\n", ThisCapture, Right_LastCapture, Right_Period);

	// update LastCapture to prepare for the next edge
	Right_LastCapture = ThisCapture;
	
	//Start the Motor Stopped Timer Again
	ES_Timer_InitTimer(MOTOR_STOPPED_R, motor_stopped_timer);
	
	

}

//Interrupt Response to Manage our Control Feedback loop to the motors
void DriveControl_PeriodicInterruptResponse(void){
	//Print for testing
	
	// start by clearing the source of the interrupt
	clearPeriodicInterrupt(DRIVE_CONTROL_INTERRUPT_PARAMATERS);
	
	//printf("RPM Targets: %f, %f \n\r", RPMTarget_Left, RPMTarget_Right);
	//Calculate Control Response individually
	uint8_t RequestedDuty_Left = calculateControlResponse(Left_Period, integralTerm_Left, RPMTarget_Left, false);
	uint8_t RequestedDuty_Right = calculateControlResponse(Right_Period, integralTerm_Right, RPMTarget_Right, true);

	/*
		static int i;
		if (i++ > 100)
		{
			printf("Implementing control response: left=%d, right=%d\r\n", RequestedDuty_Left, RequestedDuty_Right);
			printf("Left period was: %d, right period was: %d\r\n", Left_Period, Right_Period);
			i=0;
		}*/
	
	//printf("Req. duty: %d, %d\r\n", RequestedDuty_Left, RequestedDuty_Right);
	//Implement Control Response Similtaneously
	implementControlResponse(RequestedDuty_Left, RequestedDuty_Right);
}

/***************************************************************************
Control Law
 ***************************************************************************/
static uint8_t calculateControlResponse(uint32_t ThisPeriod, float integralTerm, float targetSpeed, bool isRight){
	static float RPMError; /* make static for speed */
	static float currentRPM;
	static float lastError;
	
	//Calculate RPM (Need new formula
	currentRPM = CalculateRPM(ThisPeriod);
	
	//Calculate Error (absolute)
	RPMError = fabs(targetSpeed) - currentRPM; //fabs is absolute value
	
	lastError = isRight ? LastError_Right : LastError_Left;
	
	//Determine Integral Term
	integralTerm += I_GAIN * RPMError;
	integralTerm = clamp(integralTerm, 0, 100); /* anti-windup */
	
	//Calculate Desired Duty Cycle
	uint8_t RequestedDuty = (P_GAIN * ((RPMError)+integralTerm+(D_GAIN * (RPMError-lastError))));
	if (isRight)
	{
		LastError_Right = RPMError;
		integralTerm_Right = integralTerm;
	}
	else
	{
		LastError_Left = RPMError;
		integralTerm_Left = integralTerm;
	}
	
	/*
		if (displayinfo)
	{
	static int i;
		if (i++ > 501)
		{
			if (isRight)
			{
				printf("\r\nRIGHT");
			}
			else
			{
				printf("\r\n LEFT");
			}
			printf("\n\r period %d", ThisPeriod);
			printf("\n\r targetSpeed %f", targetSpeed);
			printf("\n\r currentRPM %f", currentRPM);
			printf("\n\r RPMError %f", RPMError);
			printf("\r\n LastError %f", lastError);
			printf("\n\r Integral Term %f", integralTerm);
			printf("\r\n Requested duty %d", RequestedDuty);
			printf("\n\r");
			i = 0;
		}
		
	}*/
	
	
	return clamp(RequestedDuty, 0, 100);
}

//Actually Command the PWM Changes
static void implementControlResponse(uint8_t left, uint8_t right){
	//Use turnary operator to go forward if RPM > 0 and backwards if less than zero
	SetPWM_DriveLeft(left, (RPMTarget_Left > 0) ? LEFT_DRIVE_FORWARD_PIN_DIRECTION : LEFT_DRIVE_BACKWARD_PIN_DIRECTION);
	SetPWM_DriveRight(right, (RPMTarget_Right > 0) ? RIGHT_DRIVE_FORWARD_PIN_DIRECTION : RIGHT_DRIVE_BACKWARD_PIN_DIRECTION);
}

static float CalculateRPM(uint32_t period) 
{
	if (period == 0)
	{
		return 0;
	}
	else
	{
		return ((TICKS_PER_MS * SECS_PER_MIN * MS_PER_SEC) / (period * DRIVE_GEAR_RATIO * ENCODER_PULSES_PER_REV));
	}
}
	

/****************************************************************************
 Function
     Set the New Target Speed Function
****************************************************************************/
void setTargetDriveSpeed(float newRPMTarget_left, float newRPMTarget_right){
	//printf("Setting drive speed to: %f, %f\r\n", newRPMTarget_left, newRPMTarget_right);
	RPMTarget_Left = newRPMTarget_left;
	RPMTarget_Right = newRPMTarget_right;
	
	if (newRPMTarget_left != 0 && newRPMTarget_right != 0)
	{
		ES_Timer_InitTimer(MOTOR_STOPPED_L, motor_stopped_timer);
		ES_Timer_InitTimer(MOTOR_STOPPED_R, motor_stopped_timer);
		ResetAbsolutePosition();
	}
}

/****************************************************************************
 Function
     Set the New Target Encoder Ticks
****************************************************************************/
void setTargetEncoderTicks(uint32_t leftTicks, uint32_t rightTicks, bool negativeLeft, bool negativeRight){
	printf("Encoder ticks set to: %d, %d, direction %d, %d\r\n", leftTicks, rightTicks, negativeLeft, negativeRight);
	isMoving = true;
	
	TargetTicks_Left = leftTicks;
	TargetTicks_Right = rightTicks;
	
	if (leftTicks == rightTicks)
	{
		setTargetDriveSpeed(negativeLeft ? -DEFAULT_DRIVE_RPM : DEFAULT_DRIVE_RPM, negativeRight ? -DEFAULT_DRIVE_RPM : DEFAULT_DRIVE_RPM);
	}
	else
	{
		if (leftTicks > rightTicks)
		{
			setTargetDriveSpeed(DEFAULT_DRIVE_RPM, (((float) rightTicks) / leftTicks) * DEFAULT_DRIVE_RPM);
		}
		else
		{
			setTargetDriveSpeed(DEFAULT_DRIVE_RPM, (((float) leftTicks) / rightTicks) * DEFAULT_DRIVE_RPM);
		}
	}
}

/****************************************************************************
 Function
     Return the number of left encoder ticks since our last absolute position update
****************************************************************************/
uint32_t GetLeftEncoderTicks()
{
	return LeftEncoderTicks;
}

/****************************************************************************
 Function
     Return the number of right encoder ticks since our last absolute position update
****************************************************************************/
uint32_t GetRightEncoderTicks()
{
	return RightEncoderTicks;
}

/****************************************************************************
 Function
     Reset the encoder ticks
****************************************************************************/
void ResetEncoderTicks()
{
	LeftEncoderTicks = 0;
	RightEncoderTicks = 0;
}

/****************************************************************************
 Function
     Return true iff we are moving
****************************************************************************/
bool IsMoving(void)
{
	return isMoving;
}


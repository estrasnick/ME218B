/****************************************************************************
 Module
   PWM_Service.c

 Description
		Initializes the PWM of the Power Train and provides getter and setter fuctions
		
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "PWM_Service.h"
#include "Helpers.h"
#include "DEFINITIONS.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;


/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitPWMService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

****************************************************************************/
bool InitPWMService ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;
	
	//Initialize All of Our PWM Functions
	
	InitPWM(LEFT_DRIVE_PWM_PARAMATERS);
	InitPWM(RIGHT_DRIVE_PWM_PARAMATERS);
	InitPWM(PERISCOPE_PWM_PARAMATERS);
	InitPWM(CANNON_PWM_PARAMATERS);
	InitPWM(HOPPER_PWM_PARAMATERS);
	InitPWM(PERISCOPE_LATCH_PWM_PARAMATERS);
	
	printf("PWM Initialized \n\r");

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
     PostPWMService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise
****************************************************************************/
bool PostPWMService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunPWMService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

****************************************************************************/
ES_Event RunPWMService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors	
	
  return ReturnEvent;
}

/***************************************************************************
PWM Setter Functions
 ***************************************************************************/
//Drive Left
void SetPWM_DriveLeft(float duty, uint32_t direction){
	// Invert duty cycle if direction is reverse
	if (direction == LEFT_DRIVE_BACKWARD_PIN_DIRECTION)
	{
		duty = 100 - duty;
		GPIO_Set(DRIVE_BASE, DRIVE_DIRECTION_LEFT_PIN);
	}
	else
	{
		GPIO_Clear(DRIVE_BASE, DRIVE_DIRECTION_LEFT_PIN);
	}
	
	setPWM_value(duty, LEFT_DRIVE_PWM_PARAMATERS);
}

//Drive Right
void SetPWM_DriveRight(float duty, uint32_t direction){
	// Invert duty cycle if direction is reverse
	if (direction == RIGHT_DRIVE_BACKWARD_PIN_DIRECTION)
	{
		duty = 100 - duty;
		GPIO_Set(DRIVE_BASE, DRIVE_DIRECTION_RIGHT_PIN);
	}
	else
	{
		GPIO_Clear(DRIVE_BASE, DRIVE_DIRECTION_RIGHT_PIN);
	}
	
	setPWM_value(duty, RIGHT_DRIVE_PWM_PARAMATERS);
}

//Periscope Duty Cycle
void SetPWM_Periscope(float duty){
	//Invert the value of the duty cycle passed
	setPWM_value(100 - duty, PERISCOPE_PWM_PARAMATERS);
}
	
//Hopper Duty Cycle
void SetPWM_Hopper(float duty){
	printf("Setting PWM Hopper Duty to: %f \n\r", duty);
	setPWM_value(duty, HOPPER_PWM_PARAMATERS);
}
	
//Cannon Duty Cycle
void SetPWM_Cannon(float duty){
	printf("Setting PWM SetPWM_Cannon Duty to: %f \n\r", duty);
	setPWM_value(100-duty, CANNON_PWM_PARAMATERS);
}

//Periscope Latch Duty Cycle
void SetPWM_PeriscopeLatch(float duty){
	printf("Setting PWM Latch Duty to: %f \n\r", duty);
	setPWM_value(duty, PERISCOPE_LATCH_PWM_PARAMATERS);
}

/****************************************************************************
 Module
   PeriscopeControl_Service.c

 Description
		
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "PeriscopeControl_Service.h"
#include "Helpers.h"
#include "Definitions.h"
#include "PWM_Service.h"
#include "PositionLogic_Service.h"
#include "DriveTrainControl_Service.h"
#include "PhotoTransistor_Service.h"
#include <Math.h>

/*----------------------------- Module Defines ----------------------------*/


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

//Encoder Input Capture Variables
static uint32_t numTicks;

static bool AligningToBucket = false;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitPeriscopeControlService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

****************************************************************************/
bool InitPeriscopeControlService ( uint8_t Priority )
{
  ES_Event ThisEvent;
	
  MyPriority = Priority;
  printf("Periscope Service Attempt Initialization \n\r");
	
	//Initialize Our Input Captures for Encoder
	InitInputCapture(PERISCOPE_ENCODER_INTERRUPT_PARAMATERS_1);
	InitInputCapture(PERISCOPE_ENCODER_INTERRUPT_PARAMATERS_2);
	
	/*
	//Start the Periscope
	printf("Initialize the Periscope as Not Spinning\r\n");
	SetPWM_Periscope(0); 
	*/
	
	//Initialize the Periscope to be Unlatched
	printf("Initialize the periscope as unlatched \n\r");
	UnlatchPeriscope();
	
	ES_Timer_InitTimer(START_PERISCOPE_TIMER, START_PERISCOPE_T);
	
	printf("Periscope Service Initialized \n\r");
	
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
     PostPeriscopeControlService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise
****************************************************************************/
bool PostPeriscopeControlService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunPeriscopeControlService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

****************************************************************************/
ES_Event RunPeriscopeControlService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

	if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == START_PERISCOPE_TIMER))
	{
		SetPWM_Periscope(PERISCOPE_PWM_DUTY);
	}
	else if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == PERISCOPE_STOPPED_TIMER))
	{
		SetPWM_Periscope(0);
		if (AligningToBucket)
		{
			ES_Event RotateEvent;
			RotateEvent.EventType = ES_ALIGN_TO_BUCKET;
			setTargetDriveSpeed(DEFAULT_DRIVE_RPM, -DEFAULT_DRIVE_RPM);
			PostPhotoTransistorService(RotateEvent);
			AligningToBucket = false;
		}
	}
	else if (ThisEvent.EventType == ES_ALIGN_TO_BUCKET)
	{
		AligningToBucket = true;
	}

	//If we are in testing mode
	if(TESTING_MODE){
	
		//If we receive any of these events (from keyboard presses)
		switch (ThisEvent.EventType){
			case (ES_START_PERISCOPE):
				SetPWM_Periscope(PERISCOPE_PWM_DUTY);
				break;
			case (ES_STOP_PERISCOPE):
				SetPWM_Periscope(0);
				break;
		}
	}
	
  return ReturnEvent;
}

/***************************************************************************
Interrupt Responses
 ***************************************************************************/
void PeriscopeEncoder_InterruptResponse_1(void){
	// start by clearing the source of the interrupt, the input capture event
	clearCaptureInterrupt(PERISCOPE_ENCODER_INTERRUPT_PARAMATERS_1);
	numTicks++;
	ES_Timer_InitTimer(PERISCOPE_STOPPED_TIMER, PERISCOPE_STOPPED_T);
	if (numTicks >= 1000)
	{
		numTicks = 0;
	}
	//printf("Encoder angle is: %f\r\n", GetPeriscopeAngle());
}

void PeriscopeEncoder_InterruptResponse_2(void){
	// start by clearing the source of the interrupt, the input capture event
	clearCaptureInterrupt(PERISCOPE_ENCODER_INTERRUPT_PARAMATERS_2);
	//printf("Num ticks is: %d\r\n", numTicks);
	numTicks++;
	if (numTicks >= 1000)
	{
		numTicks = 0;
	}
}
/*
void PeriscopeTapeSensor_InterruptResponse(void) {
	clearCaptureInterrupt(PERISCOPE_TAPE_SENSOR_INTERRUPT_PARAMATERS);
	
	numTicks = 0;
}*/

float GetPeriscopeAngle(void)
{
	return (numTicks * 180.0) / (ENCODER_PULSES_PER_REV * PERISCOPE_GEAR_RATIO);
}

void ResetPeriscopeEncoderTicks(void)
{
	numTicks = 0;
}

void LatchPeriscope(void)
{
	SetPWM_PeriscopeLatch(PERISCOPE_LATCH_DUTY);
}

void UnlatchPeriscope(void)
{
	SetPWM_PeriscopeLatch(PERISCOPE_UNLATCH_DUTY);
	SetPWM_Periscope(PERISCOPE_PWM_DUTY);
}

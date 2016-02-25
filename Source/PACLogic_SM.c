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
#include "DEFINITIONS.h"
#include "Helpers.h"
#include "ES_DeferRecall.h"
#include "Master_SM.h"
#include "inc/hw_ssi.h"
#include "inc/hw_nvic.h"

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

#define ENTRY_STATE Waiting4Command_t
#define CAMPAIGN_STATUS_REQUEST_TIMER 100 //set to 100 ms

#define BITS_PER_NIBBLE 4
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringWaiting4Command_t( ES_Event Event);
static ES_Event DuringCampaignStatus_t( ES_Event Event);
static ES_Event DuringCapture_t( ES_Event Event);

//SPI Initialization
void SPI_Init(void);

//Initiate Deferral Queue (for the PAC Logic)
static ES_Event DeferralQueue[3+1]; // initialize our deferral queue

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static PACLogicState_t CurrentState;

static uint8_t TargetFrequencyIndex;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunPACLogicSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
****************************************************************************/
ES_Event RunPACLogicSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   PACLogicState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

	 //If we are in our testing mode then we are going to ignore everything from the PAC
	// if (TESTING_MODE == false){	 
	 
	 if ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == CAPTURE_TIMEOUT_TIMER))
	 {
		 NextState = Waiting4Command_t;
		 MakeTransition = true;
	 }
	 else
	 {
		 switch ( CurrentState )
		 {
				 case Waiting4Command_t :     
					 // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
					 CurrentEvent = DuringWaiting4Command_t(CurrentEvent);
				 
					 //Process Any Events
					 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
					 {	
							//If we received a timeout telling us to request the campaign status
							if ((CurrentEvent.EventType == ES_TIMEOUT) && CurrentEvent.EventParam == CAMPAIGN_STATUS_CHECK)
							{									
									NextState = CampaignStatus_t;
									MakeTransition = true;
							//If we detect a polling station move to the request state
							} else if (CurrentEvent.EventType == ES_PS_DETECTED){
									TargetFrequencyIndex = CurrentEvent.EventParam;
									NextState = Capture_t;
									MakeTransition = true;
							}
					 }
					 break;
					
				 case CampaignStatus_t :     
					 // ES_ENTRY & ES_EXIT are processed here
					 CurrentEvent = DuringCampaignStatus_t(CurrentEvent);
				 
					 //Process Any Events
					 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
					 {	
							//If transaction is complete go back to the waiting state
							if (CurrentEvent.EventType == ES_TRANSACTION_COMPLETE)
							{									
									NextState = Waiting4Command_t;
									MakeTransition = true;
							//If we detect a polling station move to the request state
							} else if (CurrentEvent.EventType == ES_PS_DETECTED)
							{
								TargetFrequencyIndex = CurrentEvent.EventParam;
								//Defer the Event
								ES_DeferEvent(DeferralQueue, CurrentEvent); 
							}
					 }
					 break;
						 
					case Capture_t :     
					 // Execute During function for state one. ES_ENTRY & ES_EXIT are processed here
					 CurrentEvent = DuringCapture_t(CurrentEvent);
				 
					 //Process Any Events
					 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
					 {	
							//If polling station is captured go back to the waiting state
							if (CurrentEvent.EventType == ES_PS_CAPTURED)
							{
									NextState = Waiting4Command_t;
									MakeTransition = true;
							}
							else if ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == MEASURING_TIMEOUT_TIMER))
							{
								printf("TAKING TOO LONG TO CAPTURE, MOVE ON! \n\r");
								
								ES_Event ResetDestinationEvent;
								ResetDestinationEvent.EventType = ES_RESET_DESTINATION;
								PostMasterSM(ResetDestinationEvent);
								
								NextState = Waiting4Command_t;
								MakeTransition = true;
							}
					 }
					 break;
				
				 }
			}
		//}
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunPACLogicSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunPACLogicSM(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartPACLogicSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
****************************************************************************/
void StartPACLogicSM ( ES_Event CurrentEvent )
{
   	//Initiate Deferral Queue
		ES_InitDeferralQueueWith(DeferralQueue, ARRAY_SIZE(DeferralQueue));
		
		printf("Attempt SPI Initialized \n\r");
		//Perform SPI Initialization
		SPI_Init();
		printf("SPI Initialized \n\r");
		
		// to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunPACLogicSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryPACLogicSM

 Parameters
     None

 Returns
     PACLogicState_t The current state of the Template state machine
****************************************************************************/
PACLogicState_t QueryPACLogicSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringWaiting4Command_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
				//printf("Entering Waiting4Command \n\r");
			
        // implement any entry actions required for this state machine
				//Recall Deferred Events
				ES_RecallEvents(MASTER_PRIORITY, DeferralQueue);       
			
				//Start the Timer for the Campaign Status
				ES_Timer_InitTimer(CAMPAIGN_STATUS_CHECK, CAMPAIGN_STATUS_REQUEST_TIMER);
			
			
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


static ES_Event DuringCampaignStatus_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        //printf("Entering DuringCampaignStatus \n\r");
				// implement any entry actions required for this state machine
				//Post to an ES_SendCmd
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_SEND_CMD;
				ThisEvent.EventParam = CAMPAIGN_STATUS_COMMAND;
				PostMasterSM(ThisEvent);
			
        // after that start any lower level machines that run in this state, ie our sending CMD SM
        StartSendingCMDSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
				//We Need to update the enemy queue now!!!!!
			
        // on exit, give the lower levels a chance to clean up first
        RunSendingCMDSM( Event );
			
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				UpdateADStatus();
				updatePSStatuses();
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
				ReturnEvent = RunSendingCMDSM( Event );
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringCapture_t( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) || (Event.EventType == ES_ENTRY_HISTORY) )
    {
        printf("Entering DuringCapture_t \n\r");
				
				// implement any entry actions required for this state machine
        ES_Timer_InitTimer(CAPTURE_TIMEOUT_TIMER, CAPTURE_TIMEOUT_T);
        // after that start any lower level machines that run in this state
        StartCapturePSSM(Event);
			
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
				//We Need to update the enemy queue now!!!!!
			
        // on exit, give the lower levels a chance to clean up first
        RunCapturePSSM(Event);
			
			
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        ReturnEvent = RunCapturePSSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


/***************************************************************************
SPI Initialization 
 ***************************************************************************/
void SPI_Init(){
	// Set ExpectingCommand to false
	//ExpectingCommand = false;
	
	//	Enable clock to GPIO Port
	HWREG(SYSCTL_RCGCGPIO) |= PAC_SYSCTL;
	
	//	Enable clock to SSI module
	HWREG(SYSCTL_RCGCSSI) |= PAC_SYSCTL;
	
	//	Wait for GPIO to be ready
	while ((HWREG(SYSCTL_PRGPIO) & PAC_SYSCTL) != PAC_SYSCTL);   
	
	//	Set alternate functionality for SSI pins
	HWREG(PAC_BASE+GPIO_O_AFSEL) |= PAC_CLOCK_PIN | PAC_FSS_PIN | PAC_RX_PIN | PAC_TX_PIN;
	
	//	Set the alternate function for SSI pins to be SSI
	HWREG(PAC_BASE+GPIO_O_PCTL) = (HWREG(PAC_BASE+GPIO_O_PCTL) & 0xff0000ff) 
		+ (2<<(2*BITS_PER_NIBBLE)) 
		+ (2<<(3*BITS_PER_NIBBLE)) 
		+ (2<<(4*BITS_PER_NIBBLE)) 
		+ (2<<(5*BITS_PER_NIBBLE));
	
	//	Set pins for digital I/O
	HWREG(PAC_BASE+GPIO_O_DEN) |= PAC_CLOCK_PIN | PAC_FSS_PIN | PAC_RX_PIN | PAC_TX_PIN;
	
	//	Set data direction for each pin
	HWREG(PAC_BASE+GPIO_O_DIR) |= PAC_CLOCK_PIN | PAC_FSS_PIN | PAC_TX_PIN;
	HWREG(PAC_BASE+GPIO_O_DIR) &= PAC_RX_PIN;
	
	//	Set pullup on clock line
	HWREG(PAC_BASE+GPIO_O_PUR) |= PAC_CLOCK_PIN;
		
	//	Wait for SSI ready register
	while ((HWREG(SYSCTL_PRSSI) & SYSCTL_PRSSI_R0) != SYSCTL_PRSSI_R0)
			;
	
	//	Disable SSI
	HWREG(SSI0_BASE+SSI_O_CR1) &= ~SSI_CR1_SSE;
	
	//	Select master mode and EOT interrupt
	HWREG(SSI0_BASE+SSI_O_CR1) |= SSI_CR1_EOT;
	
	//	Configure SSI clock source to the system clock
	HWREG(SSI0_BASE+SSI_O_CC) = (HWREG(SSI0_BASE+SSI_O_CC) & 0xfffffff0) + SSI_CC_CS_SYSPLL;
	
	//	Configure clock prescalar
	HWREG(SSI0_BASE+SSI_O_CPSR) = (HWREG(SSI0_BASE+SSI_O_CPSR) & 0xffffff00) + 0x14;
	
	//	Configure SCR, SPH, SPO, FRF (frame format), DSS (data size)
	HWREG(SSI0_BASE+SSI_O_CR0) |= (0x8B << 8) | SSI_CR0_SPO | SSI_CR0_SPH | SSI_CR0_DSS_8 | SSI_CR0_FRF_MOTO;
	
	//	Locally enable interrupts
	HWREG(SSI0_BASE+SSI_O_IM) |= SSI_IM_TXIM;
	
	//Set Priority to be less important than phototransistor
	//Interrupt number 23 corresponds to PRI5, and INTD becuase (4n + 3) hence Bit29 must be high
	HWREG(NVIC_PRI5) = BIT29HI;
	
	//	Globally enable interrupts
	__enable_irq();
	
	//	Enable SSI
	HWREG(SSI0_BASE+SSI_O_CR1) |= SSI_CR1_SSE;
	
	//	Enable interrupt in NVIC
	HWREG(NVIC_EN0) = BIT7HI;
}

/***************************************************************************
SPI Functions 
 ***************************************************************************/
 uint8_t ReadDataRegister(void)
{
	return HWREG(SSI0_BASE+SSI_O_DR);
}

void WriteDataRegister(uint8_t data)
{
	HWREG(SSI0_BASE+SSI_O_DR) = data;
}


void SSI_InterruptResponse(void)
{
	HWREG(SSI0_BASE+SSI_O_ICR) = SSI_ICR_EOTIC;
	
	ES_Event NewEvent;
	NewEvent.EventType = ES_EOT;
	PostMasterSM(NewEvent);
};

/***************************************************************************
GetTargetFrequencyIndex
	Returns the frequency of the most recently encountered polling station
 ***************************************************************************/
uint8_t GetTargetFrequencyIndex()
{
	return TargetFrequencyIndex;
}

void SetTargetFrequencyIndex(uint8_t index)
{
	TargetFrequencyIndex  = index;
}

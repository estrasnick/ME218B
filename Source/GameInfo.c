/****************************************************************************
 Module
   GameInfo.c

 Description
   Manages various information about the current state of the game

 Notes

****************************************************************************/

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"
#include "inc/hw_pwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"	// Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"
#include "BITDEFS.H"
#include "string.h"

#include "Helpers.h"
#include "DEFINITIONS.h"
#include "PositionLogic_Service.h"
#include "Master_SM.h"


/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define NULL_F 20

//Polling Station Structure
typedef struct {
	uint8_t location_code;
	float location_x;
	float location_y;
	Claimed_b claimed_status; // who owns it
	uint8_t byte; // which byte of the status reply contains it
	uint8_t bit1; // bit1 of status reply
	uint8_t bit2;	// bit2 of status reply
	uint8_t f_index; // currently known frequency
	bool obstructed; // is it known to be obstructed?
	uint32_t instrinsicPriority; // intrinsic priority -- higher is worse
} PS_Struct;

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static void updateClaimedStatus(uint8_t index);
static void ClearObstructions(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static bool MyBlockStatus;
static bool EnemyBlockStatus;

static PS_Struct PS_Array[] = {
	{.location_code = SACREMENTO_CODE, .location_x = 6.0f, .location_y = 55.8f, 	.claimed_status = Unclaimed_b, .byte = 2, .bit1 = BIT7HI, .bit2 = BIT6HI, .f_index = NULL_F, .obstructed = false, .instrinsicPriority = 0},
	{.location_code = SEATTLE_CODE, .location_x = 9.0f, .location_y = 88.5f, 			.claimed_status = Unclaimed_b, .byte = 2, .bit1 = BIT5HI, .bit2 = BIT4HI, .f_index = NULL_F, .obstructed = false, .instrinsicPriority = 0},
	{.location_code = BILLINGS_CODE, .location_x = 29.1f, .location_y = 75.8f, 		.claimed_status = Unclaimed_b, .byte = 2, .bit1 = BIT3HI, .bit2 = BIT2HI, .f_index = NULL_F, .obstructed = false, .instrinsicPriority = 0},
	{.location_code = DENVER_CODE, .location_x = 33.3f, .location_y = 53.8f, 			.claimed_status = Unclaimed_b, .byte = 2, .bit1 = BIT1HI, .bit2 = BIT0HI, .f_index = NULL_F, .obstructed = false, .instrinsicPriority = 0},
	{.location_code = DALLAS_CODE, .location_x = 45.3f, .location_y = 29.4f, 			.claimed_status = Unclaimed_b, .byte = 3, .bit1 = BIT7HI, .bit2 = BIT6HI, .f_index = NULL_F, .obstructed = false, .instrinsicPriority = 0},
	{.location_code = CHICAGO_CODE, .location_x = 61.8f, .location_y = 60.8f, 		.claimed_status = Unclaimed_b, .byte = 3, .bit1 = BIT5HI, .bit2 = BIT4HI, .f_index = NULL_F, .obstructed = false, .instrinsicPriority = 0},
	{.location_code = MIAMI_CODE, .location_x = 80.8f, .location_y = 9.2f, 				.claimed_status = Unclaimed_b, .byte = 3, .bit1 = BIT3HI, .bit2 = BIT2HI, .f_index = NULL_F, .obstructed = false, .instrinsicPriority = 0},
	{.location_code = WASHINGTON_CODE, .location_x = 81.5f, .location_y = 55.2f, 	.claimed_status = Unclaimed_b, .byte = 3, .bit1 = BIT1HI, .bit2 = BIT0HI, .f_index = NULL_F, .obstructed = false, .instrinsicPriority = 0},
	{.location_code = CONCORD_CODE, .location_x = 87.6f, .location_y = 72.6f, 		.claimed_status = Unclaimed_b, .byte = 4, .bit1 = BIT7HI, .bit2 = BIT6HI, .f_index = NULL_F, .obstructed = false, .instrinsicPriority = 0}
};

static bool GameStarted = false;

/*------------------------------ Module Code ------------------------------*/

//Check my color
uint8_t MyColor()
{
	//Color Pin isn't equal to one so need to explicitly say Red and Blue
	if ((HWREG(GAME_BASE + (GPIO_O_DATA + ALL_BITS)) & COLOR_PIN) == 0){
		return RED;
	} else {
		return BLUE;
	}
}	

//Check if Game Started
void UpdateGameStarted()
{
	uint8_t *byte;
	byte = getResponseArray();
	
	GameStarted = ((*(byte + 4) & BIT0HI) == BIT0HI) ;
	
	// If started, post it
	if (GameStarted)
	{
		ES_Event StartEvent;
		StartEvent.EventType = ES_GAME_STARTED;
		PostMasterSM(StartEvent);
	}
}

// return true iff the game is started
bool CheckGameStarted(void)
{
	return GameStarted;
}

// Update our knowledge of who is disabled
void UpdateADStatus(void)
{
	uint8_t *byte;
	byte = getResponseArray();
	uint8_t RS = *(byte + 4);
	
	// Color pin is high for Blue, low for Red
	if (MyColor() == COLOR_RED)
	{
		MyBlockStatus = RS & BIT2HI;
		EnemyBlockStatus = RS & BIT1HI;
	}
	else
	{
		MyBlockStatus = RS & BIT1HI;
		EnemyBlockStatus = RS & BIT2HI;		
	}
}

// return true iff we are disabled
bool AmIBlocked(void)
{
	return MyBlockStatus;
}

// return true iff enemy is disabled
bool IsEnemyBlocked(void)
{
	return EnemyBlockStatus;
}

// set the owner of a station
void SetStationOwner(uint8_t which, Claimed_b owner)
{
	PS_Array[which].claimed_status = owner;
}

// returns the owner of a station
uint8_t GetStationOwner(uint8_t which)
{
	return PS_Array[which].claimed_status;
}

// returns the X location of a station
float GetStationX(uint8_t which)
{
	return PS_Array[which].location_x;
}

// returns the Y location of a station
float GetStationY(uint8_t which)
{
	return PS_Array[which].location_y;
}

// returns true iff a station is known to be obstructed
bool IsObstructed(uint8_t which)
{
	return PS_Array[which].obstructed;
}

// marks that a station is obstructed
void MarkObstructed(uint8_t which)
{
	PS_Array[which].obstructed = true;
}

// returns the intrinsic priority we have assigned to a station
uint32_t GetIntrinsicPriority(uint8_t which)
{
	return PS_Array[which].instrinsicPriority;
}

// Returns true iff we are not currently near a station that we own
bool NotByOurStation(void)
{
	for (int i = 0; i < NUM_STATIONS; i++)
	{
		if (PS_Array[i].claimed_status == MyColor())
		{
			if (DistanceToPoint(PS_Array[i].location_x, PS_Array[i].location_y) <= PROXIMITY_TO_OUR_STATION_THRESHOLD)
			{
				return false;
			}
		}
	}
	return true;
}

//Update polling station Statuses
void updatePSStatuses(){
	for (int i = 0; i < NUM_STATIONS; i++){
		//Store Last Claimed Status
		uint8_t lastClaimedStatus = PS_Array[i].claimed_status;
		
		//Update the Claimed Status according to new information
		updateClaimedStatus(i);
		
		//Check if it changed to someone else's color
		if ((PS_Array[i].claimed_status != lastClaimedStatus) && (PS_Array[i].claimed_status != MyColor())){
			//Set the Frequency to the NUll Frequency
			PS_Array[i].f_index = NULL_F;
			
			ClearObstructions();
		}
	}
}

//Update the claimed status for a given station
static void updateClaimedStatus(uint8_t index){
	uint8_t *RA;
	RA = getResponseArray();	
	
	uint8_t byte = *(RA + PS_Array[index].byte);
	uint8_t bit1 = PS_Array[index].bit1;
	uint8_t bit2 = PS_Array[index].bit2;

	PS_Array[index].claimed_status = (byte & bit1) ? ((byte & bit2) ?  Undefined_b : RED_b ) : ((byte & bit2) ? BLUE_b : Unclaimed_b );
}

//Update the known frequency at a station we have just captured
// We use this information to make sure that we don't attempt a response
//  to any frequencies that we own
void updateCapturedFrequency(uint8_t station, uint8_t f_index){
	PS_Array[station].f_index = f_index;
}

//Check if the frequency is owned, returns true if we own the frequency
bool checkOwnFrequency(uint8_t f_index){
	for (int i = 0; i < NUM_STATIONS; i++){		
			//Check if it changed to someone else's color
			if (PS_Array[i].f_index == f_index && PS_Array[i].claimed_status == MyColor()){
					return true;
			}
	}	
	
	return false;
}

//Mark all stations as unobstructed
static void ClearObstructions(void)
{
	for (int i = 0; i < NUM_STATIONS; i++){
		PS_Array[i].obstructed = false;
	}
}

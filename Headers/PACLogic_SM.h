/****************************************************************************
PACLogic Header File
 ****************************************************************************/

#ifndef PACLogic_H
#define PACLogic_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { Waiting4Command_t, CampaignStatus_t, Capture_t } PACLogicState_t ;


// Public Function Prototypes

ES_Event RunPACLogicSM( ES_Event CurrentEvent );
void StartPACLogicSM ( ES_Event CurrentEvent );
PACLogicState_t QueryPACLogicSM ( void );

uint8_t ReadDataRegister(void);
void WriteDataRegister(uint8_t data);
void SSI_InterruptResponse(void);

uint8_t GetTargetFrequencyIndex(void);
void SetTargetFrequencyIndex(uint8_t index);

#endif /*SHMTemplate_H */


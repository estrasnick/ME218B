/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts

 ****************************************************************************/

#ifndef TopHSMTemplate_H
#define TopHSMTemplate_H

// State definitions for use with the query function
typedef enum { STATE_ONE, STATE_TWO, STATE_THREE } MasterTemplateState_t ;

// Public Function Prototypes

ES_Event RunMasterTemplateSM( ES_Event CurrentEvent );
void StartMasterTemplateSM ( ES_Event CurrentEvent );
bool PostMasterTemplateSM( ES_Event ThisEvent );
bool InitMasterTemplateSM ( uint8_t Priority );

#endif /*TopHSMTemplate_H */


/****************************************************************************
Strategy Header File
 ****************************************************************************/

#ifndef Strategy_H
#define Strategy_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { Wait4Start_t, ChooseDestination_t, FaceTarget_t, Travel_t, StationCapture_t } StrategyState_t ;


// Public Function Prototypes

ES_Event RunStrategySM( ES_Event CurrentEvent );
void StartStrategySM ( ES_Event CurrentEvent );
StrategyState_t QueryStrategySM ( void );

uint8_t GetTargetStation(void);

void PausePositioning(void);
void ResumePositioning(void);

#endif /*Strategy_H */


/****************************************************************************
 
  Header file for template service 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef Indicators_H
#define Indicators_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitIndicatorsService ( uint8_t Priority );
bool PostIndicatorsService( ES_Event ThisEvent );
ES_Event RunIndicatorsService( ES_Event ThisEvent );

#endif 


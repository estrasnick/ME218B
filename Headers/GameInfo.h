/****************************************************************************
GameInfo Header File
 ****************************************************************************/

#include "ES_Types.h"

#ifndef GameInfo_H
#define GameInfo_H

// Public Function Prototypes
uint8_t MyColor(void);
bool isGameStarted(void);
void UpdateADStatus(void);
bool AmIBlocked(void);
bool IsEnemyBlocked(void);

uint8_t GetStationOwner(uint8_t which);

float GetStationX(uint8_t which);
float GetStationY(uint8_t which);

void updatePSStatuses(void);

#endif /*GameInfo_H */


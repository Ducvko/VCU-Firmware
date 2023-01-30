/*
 * APPS.c
 *
 *  Created on: Jan 19, 2023
 *      Author: Matt
 */

#include "APPS.h"
#include "utils.h"
#include "CAN1.h"
#include <string.h>

#define AVG_WINDOW			3
#define APPS1_MIN 			409
#define APPS1_MAX			1300
#define APPS_DIFF_THRESH	90


APPS_Data_Struct APPS_Data;

volatile uint16_t ADC1_buff[ADC1_BUFF_LEN];

void startAPPSTask() {

	//used for averaging the apps signal
	uint32_t apps1Avg = 0;
	uint32_t apps2Avg = 0;



	CANMsg txMsg;

	//circular buffers for moving average
	uint32_t apps1PrevMesurments[AVG_WINDOW];
	uint32_t apps2PrevMesurments[AVG_WINDOW];

	//position in circular buffer used for moving average
	uint8_t circBuffPos = 0;

	uint32_t tick;

	tick = osKernelGetTickCount();

	while (1) {

		//Averages samples in DMA buffer
		apps1Avg = 0;
		for (int i = 0; i < ADC1_BUFF_LEN; i++) {
			if(i%2 ==0){
				apps1Avg += ADC1_buff[i];
			} else {
				apps2Avg += ADC1_buff[i];
			}
		}
		apps1Avg = (apps1Avg + (1<<8)) >> 10;
		apps2Avg = (apps2Avg + (1<<8)) >> 10;


		//Calculates moving average of previous measurements
		if(++circBuffPos == AVG_WINDOW){
			circBuffPos = 0;
		}
		apps1PrevMesurments[circBuffPos] = apps1Avg;
		apps2PrevMesurments[circBuffPos] = apps2Avg;

		apps1Avg = 0;
		apps2Avg = 0;

		for (int i = 0; i < AVG_WINDOW; i++) {
			apps1Avg += apps1PrevMesurments[i];
			apps2Avg += apps2PrevMesurments[i];
		}
		apps1Avg = apps1Avg/AVG_WINDOW;
		apps2Avg = apps2Avg/AVG_WINDOW;

		//TODO compare APPS signal to detect plausibility error;

		//Formatting sample can message
		//TODO format can message as motor controller torque command
		txMsg.aData[3] = apps1Avg & 0xFFU;
		txMsg.aData[2] = apps1Avg >> 8 & 0xFFU;
		txMsg.aData[1] = apps1Avg >> 16 & 0xFFU;
		txMsg.aData[0] = apps1Avg >> 24 & 0xFFU;

		txMsg.header.DLC = 4;
		txMsg.header.StdId = 0x69U;

		txMsg.header.ExtId = 0;
		txMsg.header.IDE = 0;
		txMsg.header.RTR = CAN_RTR_DATA;
		txMsg.header.TransmitGlobalTime = DISABLE;

		myprintf("APPS1:%d, APPS2:%d\n\r", apps1Avg, apps2Avg);
;

		osMessageQueuePut(CAN1_QHandle, &txMsg, 0, APPS_PERIOD-2);


		tick+= APPS_PERIOD;
		osDelayUntil(tick);
	}
}

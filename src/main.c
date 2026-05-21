/*==================================================================================================
 * Project : RTD AUTOSAR 4.9
 * Platform : CORTEXM
 * Peripheral : S32K3XX
 * Dependencies : none
 *
 * Autosar Version : 4.9.0
 * Autosar Revision : ASR_REL_4_9_REV_0000
 * Autosar Conf.Variant :
 * SW Version : 7.0.0
 * Build Version : S32K3_RTD_7_0_0_QLP03_D2512_ASR_REL_4_9_REV_0000_20251210
 *
 * Copyright 2020 - 2026 NXP
 *
 *   NXP Proprietary. This software is owned or controlled by NXP and may only be
 *   used strictly in accordance with the applicable license terms. By expressly
 *   accepting such terms or by downloading, installing, activating and/or otherwise
 *   using the software, you are agreeing that you have read, and that you agree to
 *   comply with and are bound by, such license terms. If you do not agree to be
 *   bound by the applicable license terms, then you may not retain, install,
 *   activate or otherwise use the software.
 ==================================================================================================*/

/*==================================================================================================
 *                                        INCLUDE FILES
 ==================================================================================================*/
#include "Clock_Ip.h"
#include "IntCtrl_Ip.h"
#include "Adc_Sar_Ip.h"
#include "Siul2_Port_Ip.h"
#include "OsIf.h"
#include "Flexio_Mcl_Ip.h"
#include "WS2812_utils.h"

/*==================================================================================================
 *                                      DEFINES AND MACROS
 ==================================================================================================*/
/* ADC Configuration */
#define ADC_SAR_USED_CH                 0U
#define ADC_VREFH_mV                    5000U
#define ADC_VREFL_mV                    0U

/* ADC Threshold values in mV */
#define ADC_THRESHOLD_GREEN             500U
#define ADC_THRESHOLD_YELLOW            1000U
#define ADC_THRESHOLD_ORANGE            2000U
#define ADC_THRESHOLD_RED            	2500U

/*==================================================================================================
 *                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
 ==================================================================================================*/

/*==================================================================================================
 *                                       LOCAL MACROS
 ==================================================================================================*/

/*==================================================================================================
 *                                      LOCAL CONSTANTS
 ==================================================================================================*/

/*==================================================================================================
 *                                      LOCAL VARIABLES
 ==================================================================================================*/

/*==================================================================================================
 *                                      GLOBAL CONSTANTS
 ==================================================================================================*/

/*==================================================================================================
 *                                      GLOBAL VARIABLES
 ==================================================================================================*/
/* WS2812 LED Strip State */
WS2812_Strip_t ledStrip;

/* ADC Variables */
volatile boolean notif_triggered = FALSE;
volatile uint16 data;
uint16_t ui16AdcRawValue = 0u;
uint16_t ui16AdcMax = (1 << 12);
float fAdcValue;

/*==================================================================================================
 *                                   LOCAL FUNCTION PROTOTYPES
 ==================================================================================================*/

/*==================================================================================================
 *                                   EXTERNAL FUNCTION PROTOTYPES
 ==================================================================================================*/
extern void Adc_Sar_0_Isr(void);

/*==================================================================================================
 *                                       LOCAL FUNCTIONS
 ==================================================================================================*/

/**
 * @brief Performs initialization and test sequence for WS2812 LED strip
 *
 * This function runs a visual test sequence that:
 * 1. Lights up each LED sequentially in white
 * 2. Cycles all LEDs through RED, GREEN, and BLUE colors
 * 3. Clears all LEDs to prepare for normal operation
 *
 * @param strip  Pointer to the WS2812 strip structure
 */
void WS2812_InitSequence(WS2812_Strip_t *strip) {
	// Step 1: Light up all LEDs one by one in WHITE
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		WS2812_SetPixel(strip, i, 255, 255, 255);
		WS2812_Show(strip);
		delay_cycles(5000000);  // Delay between each LED
	}

	delay_cycles(5000000);  // Hold all white
	WS2812_Clear(strip);
	WS2812_Show(strip);
	// Step 2: Cycle through RGB colors on all LEDs
	// RED
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		WS2812_SetPixel(strip, i, 255, 0, 0);
	}
	WS2812_Show(strip);
	delay_cycles(10000000);

	// GREEN
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		WS2812_SetPixel(strip, i, 0, 255, 0);
	}
	WS2812_Show(strip);
	delay_cycles(10000000);

	// BLUE
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		WS2812_SetPixel(strip, i, 0, 0, 255);
	}
	WS2812_Show(strip);
	delay_cycles(10000000);

	// Step 4: Clear all LEDs
	WS2812_Clear(strip);
	WS2812_Show(strip);
	delay_cycles(5000000);
}

/*==================================================================================================
 *                                       GLOBAL FUNCTIONS
 ==================================================================================================*/
/**
 * @brief ADC End of Chain Notification Callback
 */
void AdcEndOfChainNotif(void) {
	data = Adc_Sar_Ip_GetConvData(ADCHWUNIT_0_INSTANCE, ADC_SAR_USED_CH);
	notif_triggered = TRUE;
}

/*==================================================================================================
 *                                           MAIN FUNCTION
 ==================================================================================================*/
int main(void) {
	/*==============================================================================================
	 *                                 SYSTEM INITIALIZATION
	 ==============================================================================================*/

	/* 1. Initialize System Clocks using the pure Clock IP driver */
	Clock_Ip_Init(&Clock_Ip_aClockConfig[0]);

	/* 2. Initialize Pins using the pure SIUL2 Port IP driver */
	Siul2_Port_Ip_Init(
	NUM_OF_CONFIGURED_PINS_PortContainer_0_BOARD_InitPeripherals,
			g_pin_mux_InitConfigArr_PortContainer_0_BOARD_InitPeripherals);

	/* 3. Initialize OS Interface (Needed for timing functions) */
	OsIf_Init(NULL_PTR);

	/*==============================================================================================
	 *                                 WS2812 LED STRIP INITIALIZATION
	 ==============================================================================================*/

	/* 4. Turn on the FlexIO module globally using the MCL IP driver */
	Flexio_Mcl_Ip_InitDevice(&Flexio_Ip_Sa_xFlexioInit);

	/* 5. Initialize WS2812 Hardware Routing (Pin=1, Shifter=0, Timer=0) */
	WS2812_Init(&ledStrip, 8, 0, 0);

	/* 6. Adjust brightness (0-255): 25 = ~10%, 50 = ~20%, 128 = 50% */
	ledStrip.brightness = 50;  // Set to 20% brightness

	/* 7. Run LED initialization sequence to test all LEDs */
	WS2812_InitSequence(&ledStrip);

	/*==============================================================================================
	 *                                 ADC INITIALIZATION
	 ==============================================================================================*/

	/* 8. Initialize ADC Hardware Unit */
	Adc_Sar_Ip_Init(ADCHWUNIT_0_INSTANCE, &AdcHwUnit_0);

	/* 9. Install and enable ADC interrupt handler */
	IntCtrl_Ip_InstallHandler(ADC0_IRQn, Adc_Sar_0_Isr, NULL_PTR);
	IntCtrl_Ip_EnableIrq(ADC0_IRQn);

	/* 10. Perform ADC Calibration (multiple times for accuracy) */
	for (uint8 Index = 0; Index <= 5; Index++) {
		Adc_Sar_Ip_DoCalibration(ADCHWUNIT_0_INSTANCE);
	}

	/* 11. Enable ADC End of Chain notifications */
	Adc_Sar_Ip_EnableNotifications(ADCHWUNIT_0_INSTANCE,
	ADC_SAR_IP_NOTIF_FLAG_NORMAL_ENDCHAIN);

	/*==============================================================================================
	 *                                 MAIN LOOP - ADC MONITORING
	 ==============================================================================================*/

	while (1) {
		/* Start ADC Conversion */
		Adc_Sar_Ip_StartConversion(ADCHWUNIT_0_INSTANCE,
				ADC_SAR_IP_CONV_CHAIN_NORMAL);

		/* Wait for the notification to be triggered */
		while (notif_triggered != TRUE)
			;
		notif_triggered = FALSE;

		/* Read ADC raw value */
		ui16AdcRawValue = data;

		/* Convert ADC raw value to millivolts */
		fAdcValue = ((float) ui16AdcRawValue * (ADC_VREFH_mV - ADC_VREFL_mV))
				/ ui16AdcMax;

		/* Clear all LEDs first */
		WS2812_Clear(&ledStrip);

		/* Light up columns progressively based on ADC thresholds */
		for (uint16_t i = 0; i < NUM_LEDS; i++) {
			uint8_t column = (i % 4) + 1;  // Identify column (1-4)

			/* Column 1: Green (if voltage >= ADC_THRESHOLD_GREEN) */
			if (column == 1 && fAdcValue >= ADC_THRESHOLD_GREEN) {
				WS2812_SetPixel(&ledStrip, i, 0, 255, 0);  // Green
			}

			/* Column 2: Yellow (if voltage >= ADC_THRESHOLD_YELLOW) */
			if (column == 2 && fAdcValue >= ADC_THRESHOLD_YELLOW) {
				WS2812_SetPixel(&ledStrip, i, 255, 255, 0);  // Yellow
			}

			/* Column 3: Orange (if voltage >= ADC_THRESHOLD_ORANGE) */
			if (column == 3 && fAdcValue >= ADC_THRESHOLD_ORANGE) {
				WS2812_SetPixel(&ledStrip, i, 255, 165, 0);  // Orange
			}

			/* Column 4: Red (if voltage >= ADC_THRESHOLD_RED) */
			if (column == 4 && fAdcValue >= ADC_THRESHOLD_RED) {
				WS2812_SetPixel(&ledStrip, i, 255, 0, 0);  // Red
			}
			// LEDs that don't meet threshold remain off (already cleared)
		}

		/* Update LED strip with new colors */
		WS2812_Show(&ledStrip);
	}

	return 0;
}

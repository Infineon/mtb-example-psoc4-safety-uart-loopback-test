/******************************************************************************
* File Name:   main.c
* 
* Description:
* This file provides example usage of SCB-UART self tests for PSoC 4.
*
* Related Document: See README.md
*
********************************************************************************
* Copyright 2023-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
********************************************************************************/

/*******************************************************************************
* Includes
********************************************************************************/
#include <stdio.h>
#include "cy_pdl.h"
#include "cybsp.h"

#include "SelfTest_UART_SCB.h"

/*******************************************************************************
* Macros
********************************************************************************/

#define MAX_INDEX_VAL 0xFFF0u

/*******************************************************************************
* Global Variables
*******************************************************************************/

static cy_stc_scb_uart_context_t CYBSP_DUT_UART_context;
char uart_print_buff[100]={0};

/*******************************************************************************
* Function Prototypes
********************************************************************************/

static void SelfTest_UART_SCB_Init(void);

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* The main function performs the following tasks:
*    1. Initializes the device, board peripherals, and retarget-io for prints.
*    2. Calls the SelfTest_UART_SCB_Init function to initialize the UART component.
*    3. Initializes the Smart-IO based on the design.modus file.
*    4. Bypasses all SmartIO configurations to enable the normal operation of 
*       the UART block.
*    5. Calls abort APIs from the UART PDL to prevent interrupt-driven UART APIs 
*       from triggering during the self-test. 
*    6. Enables loopback by disabling the bypass on all SmartIO configurations 
*       to test the UART block.
*    7. Calls the SelfTest_UART_SCB API to test the UART SCB IP.
*    8. Disables loopback and bypasses SmartIO configurations to restore normal 
*       operation of the UART block.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/

int main(void)
{
    cy_rslt_t result;
    cy_en_smartio_status_t smart_res;
    cy_stc_scb_uart_context_t CYBSP_UART_context;
    uint16_t count = 0u;
    
    uint8_t ret;
    
    uint8_t bypassMask;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();
    
    /* Configure and enable the UART peripheral */
    Cy_SCB_UART_Init(CYBSP_UART_HW, &CYBSP_UART_config, &CYBSP_UART_context);
    Cy_SCB_UART_Enable(CYBSP_UART_HW);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\x1b[2J\x1b[;H");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\r\nClass-B Safety Test: UART Loopback\r\n");

    /* Init UART SelfTest*/
    SelfTest_UART_SCB_Init();

    /* Configure the Smart I/O block 2 */
    if(CY_SMARTIO_SUCCESS != Cy_SmartIO_Init(CYBSP_SMARTIO_UART_LOOPBACK_HW, &CYBSP_SMARTIO_UART_LOOPBACK_config))
    {
        /* Insert error handling */
        CY_ASSERT(0);
    }
    /* Enable Smart I/O block 2 */
    Cy_SmartIO_Enable(CYBSP_SMARTIO_UART_LOOPBACK_HW);


    /* Bypass all the SmartIO configuration for normal mode */
    bypassMask = 0xFFu;
    Cy_SmartIO_Disable(CYBSP_SMARTIO_UART_LOOPBACK_HW);
    smart_res = Cy_SmartIO_SetChBypass(CYBSP_SMARTIO_UART_LOOPBACK_HW, bypassMask);
    if(smart_res != CY_SMARTIO_SUCCESS)
    {
        CY_ASSERT(0);
    }       
    Cy_SmartIO_Enable(CYBSP_SMARTIO_UART_LOOPBACK_HW);

    /* Need to clear buffer after MUX switch */
    Cy_SysLib_DelayUs(100u);
    Cy_SCB_UART_ClearRxFifo(CYBSP_DUT_UART_HW);
    Cy_SCB_UART_ClearTxFifo(CYBSP_DUT_UART_HW);

    for (;;)
    {
        /* If using high level UART APIs with context, run the appropriate stop and abort APIS
         * to ensure interrupt driven high level UART APIs do not trigger during self test*/
        Cy_SCB_UART_StopRingBuffer(CYBSP_DUT_UART_HW, &CYBSP_DUT_UART_context);
        Cy_SCB_UART_AbortReceive(CYBSP_DUT_UART_HW, &CYBSP_DUT_UART_context);
        Cy_SCB_UART_AbortTransmit(CYBSP_DUT_UART_HW, &CYBSP_DUT_UART_context);

        /* Turn on loopback by disabling the bypass on all the SmartIO configuration for test mode */
        Cy_SmartIO_Disable(CYBSP_SMARTIO_UART_LOOPBACK_HW);
        smart_res = Cy_SmartIO_SetChBypass(CYBSP_SMARTIO_UART_LOOPBACK_HW, CY_SMARTIO_CHANNEL_NONE);
        if(smart_res != CY_SMARTIO_SUCCESS)
        {
            CY_ASSERT(0);
        }               
        Cy_SmartIO_Enable(CYBSP_SMARTIO_UART_LOOPBACK_HW);
        
        /* Clear RX, TX buffers */
        Cy_SCB_UART_ClearRxFifo(CYBSP_DUT_UART_HW);
        Cy_SCB_UART_ClearTxFifo(CYBSP_DUT_UART_HW);

        /*******************************/
        /* Run UART Self Test...       */
        /*******************************/
        ret = SelfTest_UART_SCB(CYBSP_DUT_UART_HW);

        /* Turn off loopback by enabling the bypass on all the SmartIO configuration for normal mode */
        Cy_SmartIO_Disable(CYBSP_SMARTIO_UART_LOOPBACK_HW);
        smart_res = Cy_SmartIO_SetChBypass(CYBSP_SMARTIO_UART_LOOPBACK_HW, CY_SMARTIO_CHANNEL_ALL);
        if(smart_res != CY_SMARTIO_SUCCESS)
        {
            CY_ASSERT(0);
        }               
        Cy_SmartIO_Enable(CYBSP_SMARTIO_UART_LOOPBACK_HW);

        /* Need to clear buffer after MUX switch */
        Cy_SysLib_DelayUs(100u);
        Cy_SCB_UART_ClearRxFifo(CYBSP_DUT_UART_HW);
        Cy_SCB_UART_ClearTxFifo(CYBSP_DUT_UART_HW);

        if ((PASS_COMPLETE_STATUS != ret) && (PASS_STILL_TESTING_STATUS != ret))
        {
            /* Process error */
            Cy_SCB_UART_PutString(CYBSP_UART_HW, "\r\nUART SCB test: error ");
            while (1);
        }

        /* Print test counter */
        sprintf(uart_print_buff, "\rUART SCB loopback testing using Smart-IO... count=%d", count);
        Cy_SCB_UART_PutString(CYBSP_UART_HW, uart_print_buff);

        count++;
        if (count > MAX_INDEX_VAL)
        {
            count = 0u;
        }

    }

}

/*****************************************************************************
* Function Name: SelfTest_UART_SCB_Init
******************************************************************************
*
* Summary:
*  Init UART component and clear internal UART buffers
*  Should be called once before tests
*
* Parameters:
*  NONE
*
* Return:
*  NONE
*
* Note:
*
*****************************************************************************/
static void SelfTest_UART_SCB_Init(void)
{

    cy_en_scb_uart_status_t initstatus;

    /* Initialize the UART */
    initstatus = Cy_SCB_UART_Init(CYBSP_DUT_UART_HW, &CYBSP_DUT_UART_config, &CYBSP_DUT_UART_context);

    /* Initialization failed. Handle error */
    if(initstatus != CY_SCB_UART_SUCCESS)
    {
        CY_ASSERT(0);
    }

    Cy_SCB_UART_Enable(CYBSP_DUT_UART_HW);

    /* Clear RX, TX buffers */
    Cy_SCB_UART_ClearRxFifo(CYBSP_DUT_UART_HW);
    Cy_SCB_UART_ClearTxFifo(CYBSP_DUT_UART_HW);
}

/* [] END OF FILE */

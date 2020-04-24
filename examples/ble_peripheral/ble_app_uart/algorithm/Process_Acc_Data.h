/* EasyCASE V6.5 26/05/2009 17:43:20 */
/* EasyCASE O
If=horizontal
LevelNumbers=no
LineNumbers=no
Colors=16777215,0,12582912,12632256,0,0,0,16711680,8388736,0,33023,32768,0,0,0,0,0,32768,12632256,255,65280,255,255,16711935
ScreenFont=Courier New,Regular,80,4,-11,0,400,0,0,0,0,0,0,3,2,1,49,96,96
PrinterFont=Courier,,100,65530,-83,0,400,0,0,0,0,0,0,3,2,1,49,600,600
LastLevelId=2 */
/* EasyCASE ( 1 */
/*  $Date: 2008/29/09 16:05:10 $
 *  $Revision: 1.1 $
 *  
 */

/*
* Copyright (C) 2007 Bosch Sensortec GmbH
*
* Pedometer Algorithm Header file 
* 
* Usage:        Application Programming Interface for Pedometer Algorithm has to include this as headerfile
*
* Author:       aibin.paul@in.bosch.com
*/
/* EasyCASE ( 2
   Disclaimer */
/* Disclaimer
*
* Common:
* Bosch Sensortec products are developed for the consumer goods industry. They may only be used 
* within the parameters of the respective valid product data sheet.  Bosch Sensortec products are 
* provided with the express understanding that there is no warranty of fitness for a particular purpose. 
* They are not fit for use in life-sustaining, safety or security sensitive systems or any system or device 
* that may lead to bodily harm or property damage if the system or device malfunctions. In addition, 
* Bosch Sensortec products are not fit for use in products which interact with motor vehicle systems.  
* The resale and/or use of products are at the purchaser’s own risk and his own responsibility. The 
* examination of fitness for the intended use is the sole responsibility of the Purchaser. 
*
* The purchaser shall indemnify Bosch Sensortec from all third party claims, including any claims for 
* incidental, or consequential damages, arising from any product use not covered by the parameters of 
* the respective valid product data sheet or not approved by Bosch Sensortec and reimburse Bosch 
* Sensortec for all costs in connection with such claims.
*
* The purchaser must monitor the market for the purchased products, particularly with regard to 
* product safety and inform Bosch Sensortec without delay of all security relevant incidents.
*
* Engineering Samples are marked with an asterisk (*) or (e). Samples may vary from the valid 
* technical specifications of the product series. They are therefore not intended or fit for resale to third 
* parties or for use in end products. Their sole purpose is internal client testing. The testing of an 
* engineering sample may in no way replace the testing of a product series. Bosch Sensortec 
* assumes no liability for the use of engineering samples. By accepting the engineering samples, the 
* Purchaser agrees to indemnify Bosch Sensortec from all claims arising from the use of engineering 
* samples.
*
* Special:
* This software module (hereinafter called "Software") and any information on application-sheets 
* (hereinafter called "Information") is provided free of charge for the sole purpose to support your 
* application work. The Software and Information is subject to the following terms and conditions: 
*
* The Software is specifically designed for the exclusive use for Bosch Sensortec products by 
* personnel who have special experience and training. Do not use this Software if you do not have the 
* proper experience or training. 
*
* This Software package is provided `` as is `` and without any expressed or implied warranties, 
* including without limitation, the implied warranties of merchantability and fitness for a particular 
* purpose. 
*
* Bosch Sensortec and their representatives and agents deny any liability for the functional impairment 
* of this Software in terms of fitness, performance and safety. Bosch Sensortec and their 
* representatives and agents shall not be liable for any direct or indirect damages or injury, except as 
* otherwise stipulated in mandatory applicable law.
* 
* The Information provided is believed to be accurate and reliable. Bosch Sensortec assumes no 
* responsibility for the consequences of use of such Information nor for any infringement of patents or 
* other rights of third parties which may result from its use. No license is granted by implication or 
* otherwise under any patent or patent rights of Bosch. Specifications mentioned in the Information are 
* subject to change without notice.
*
* It is not allowed to deliver the source code of the Software to any third party without permission of 
* Bosch Sensortec.
*/
/* EasyCASE ) */
/*! \file Process_Acc_Data.h
\brief This file contains all #define constants and function prototype
    
    Details.
*/
#ifndef __PROCESSDAT__
#define __PROCESSDAT__


#include "smb380.h"
//#include "../../../../../../Include/boards/Target_board.h"

      /*=============================================================================*/
      /***********************Function Prototype**************************************/
      /*=============================================================================*/
      /* start the detections */
      /**
       \brief This function will start the Pedometer Algorithm if pedometer is in 
        sleep mode. If Pedometer Algorithm is running already, It will be restarted by
       this function and step count will be reset.
       \param None
       \return None
      */

      void startDetection();
      /* Stop the detections */
      /**
        \brief This function will put the Pedometer Algorithm to Sleep mode.
        \param None
        \return None
      */
      void stopDetection();
      /* Reset the step count */
      /**
        \brief Thsi API will just reset the step count to zero. Pedometer Algorithm mode
        is not affected by this function.
        \param  None
        \return None
      */
      void resetStepCount();
			void clearStepCount();
      /* Function processes the given acceleration data and verifies if a step is detected from the same */
      /**
        \brief  This API calculates Composite value from  X,Y,Z Axis acceleration
        values and updates the step count. It also classifies the type of step
        whether it is a Jog/Walk/Slow Walk step. This function returns the calculated
        composite value.
        \param short f_x_i16 : X-Axis acceleration value from the sensor \n
               short f_y_i16 : Y-Axis acceleration value from the sensor \n
               short f_z_i16 : Z-Axis acceleration value from the sensor \n
        \return Composite value
      */
      short processAccelarationData(short f_x_i16, short f_y_i16, short f_z_i16);
      /* Get the step count */
       /**
        \brief This function will return the number of steps counted to the
          calling function. 
        \param None
        \return Number of steps counted
      */

      unsigned long getStepCount();
      /* Initialization of the pedometer algorithm */
      /**
       \brief This function will initialze the variables that are used in the
       Pedometer Algorithm. It should be called in Power On Init.0 is passed as parameter for 2G,
       1 for 4G and 2 for 8G\n
       <b>Calling instance:<b>\n Call the function after giving a delay (10msec) after power on
        \param unsigned char v_GRange_u8r : Parameter used to set the division factor for threshold.\n
       0-->2G\n
       1-->4G\n
       2-->8G\n
      \return None\n
     
      */
     void InitAlgo(unsigned char);
      /* Get the Activity of the pedometer */
      /**
        \brief This function will Return the nature of the step \n
         Whether the step is of Jogg(0x12)/Walk(0x11)/Slow Walk(0x10) nature\n
        \param None
        \return Step Nature
      */

      unsigned char getActivity(void);
      
/**
  \brief This function will start the Pedometer Algorithm with robustness
  feature enabled.By default robustness feature will be there in the algorithm.
  If any time the robustness feature is disabled then user need to call this
  functionto enable the robustness feature. 
  
  \param None
  \return None
*/

void enableRobustness(void);

/**
  \brief This function will start the Pedometer Algorithm with robustness
  feature disabled.By default robustness feature will be there in the algorithm.
  If any time the robustness feature need to be disabled then user need to call this
  function 
  
  \param None
  \return None
*/

void disableRobustness(void);
      
      /*=============================================================================*/

#endif
/* EasyCASE ) */

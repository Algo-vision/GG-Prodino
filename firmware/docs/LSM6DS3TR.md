

# **LSM6DS3** 

### iNEMO inertial module: always-on 3D accelerometer and 3D gyroscope 

**Datasheet** - **production data** 



###### **LGA-14L (2.5 x 3 x 0.83 mm) typ.** 

#### **Features** 

- Power consumption: 0.9 mA in combo normal mode and 1.25 mA in combo high-performance mode up to 1.6 kHz. 

- “Always-on” experience with low power consumption for both accelerometer and gyroscope 

- Smart FIFO up to 8 kbyte based on features set 

- Compliant with Android K and L 

- Hard, soft ironing for external magnetic sensor corrections 

- ±2/±4/±8/±16 _g_ full scale 

- ±125/±245/±500/±1000/±2000 dps full scale 

- Analog supply voltage: 1.71 V to 3.6 V 

- Independent IOs supply (1.62 V) 

- Compact footprint, 2.5 mm x 3 mm x 0.83 mm 

- SPI/I<sup>2</sup> C serial interface with main processor data synchronization feature 

- Embedded temperature sensor 

#### **Description** 

The LSM6DS3 is a system-in-package featuring a 3D digital accelerometer and a 3D digital gyroscope performing at 1.25 mA (up to 1.6 kHz ODR) in highperformance mode and enabling always-on low-power features for an optimal motion experience for the consumer. 

The LSM6DS3 supports main OS requirements, offering real, virtual and batch sensors with 8 kbyte for dynamic data batching. 

ST’s family of MEMS sensor modules leverages the robust and mature manufacturing processes already used for the production of micromachined accelerometers and gyroscopes. 

The various sensing elements are manufactured using specialized micromachining processes, while the IC interfaces are developed using CMOS technology that allows the design of a dedicated circuit which is trimmed to better match the characteristics of the sensing element. 

The LSM6DS3 has a full-scale acceleration range of ±2/±4/±8/±16 _g_ and an angular rate range of ±125/±245/±500/±1000/±2000 dps. 

High robustness to mechanical shock makes the LSM6DS3 the preferred choice of system designers for the creation and manufacturing of reliable products. 

The LSM6DS3 is available in a plastic land grid array (LGA) package. 

- ECOPACK<sup>®</sup> , RoHS and “Green” compliant 

**Table 1. Device summary** 

#### **Applications** 

- Pedometer, step detector and step counter 

- Significant motion and tilt functions 

- Indoor navigation 

|**Part number**|**Temperature**<br>**range [°C]**|**Package**|**Packing**|
|---|---|---|---|
|LSM6DS3|-40 to +85|LGA14L|Tray|
|LSM6DS3TR|-40 to +85|-<br>(2.5 x 3 x 0.83 mm)|Tape &<br>Reel|



- Tap and double-tap detection 

- IoT and connected devices 

- Intelligent power saving for handheld devices 

- Vibration monitoring and compensation 

- Free-fall detection 

- 6D orientation detection 

October 2015 

1/99 

DocID026899 Rev 7 

This is information on a product in full production. 

_www.st.com_ 

**Contents** 

**LSM6DS3** 

## **Contents** 

|**1**|**Over**|**view  .**|**. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 15**|
|---|---|---|---|
|**2**|**Emb**|**edded l**|**ow-power features  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 16**|
||2.1|Tilt det|ection . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 16|
|**3**|**Pin**|**descript**|**ion  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 17**|
|**4**|**Mod**|**ule spe**|**cifications . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 19**|
||4.1|Mecha|nical characteristics . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 19|
||4.2|Electri|cal characteristics . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 21|
||4.3|Tempe|rature sensor characteristics . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 22|
||4.4|Comm|unication interface characteristics  . . . . . . . . . . . . . . . . . . . . . . . . . 23|
|||4.4.1|SPI - serial peripheral interface . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 23|
|||4.4.2|I<sup>2</sup>C - inter-IC control interface  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 24|
||4.5|Absolu|te maximum ratings . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 25|
||4.6|Termin|ology  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 26|
|||4.6.1|Sensitivity . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 26|
|||4.6.2|Zero-g and zero-rate level . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 26|
|**5**|**Func**|**tionalit**|**y  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 27**|
||5.1|Operat|ing modes  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 27|
||5.2|Gyrosc|ope power modes . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 27|
||5.3|Accele|rometer power modes  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 27|
||5.4|FIFO .|. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 27|
|||5.4.1|Bypass mode . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 28|
|||5.4.2|FIFO mode . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 28|
|||5.4.3|Continuous mode . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 29|
|||5.4.4|Continuous-to-FIFO mode . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 29|
|||5.4.5|Bypass-to-Continuous mode . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 29|
|||5.4.6|FIFO reading procedure  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 29|
|||5.4.7|Filter block diagrams  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 30|
|**6**|**Digit**|**al inter**|**faces . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 32**|



2/99 

DocID026899 Rev 7 

|**LSM6DS3**|**Contents**|
|---|---|
|6.1|I<sup>2</sup>C serial interface  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 32|
||6.1.1<br>I<sup>2</sup>C operation  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 33|
|6.2|SPI bus interface  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 34|
||6.2.1<br>SPI read . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 35|
||6.2.2<br>SPI write  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 36|
||6.2.3<br>SPI read in 3-wire mode  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 37|
|**7**<br>**Appli**|**cation hints . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 38**|
|7.1|LSM6DS3 electrical connections in Mode 1 . . . . . . . . . . . . . . . . . . . . . . . 38|
|7.2|LSM6DS3 electrical connections in Mode 2 . . . . . . . . . . . . . . . . . . . . . . . 39|
|7.3|LSM6DS3 electrical connections in Mode 3 . . . . . . . . . . . . . . . . . . . . . . . 40|
|**8**<br>**Regi**|**ster mapping  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 41**|
|**9**<br>**Regi**|**ster description  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 45**|
|9.1|FUNC_CFG_ACCESS (01h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 45|
|9.2|SENSOR_SYNC_TIME_FRAME (04h)  . . . . . . . . . . . . . . . . . . . . . . . . . . 45|
|9.3|FIFO_CTRL1 (06h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 45|
|9.4|FIFO_CTRL2 (07h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 46|
|9.5|FIFO_CTRL3 (08h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 46|
|9.6|FIFO_CTRL4 (09h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 47|
|9.7|FIFO_CTRL5 (0Ah)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 48|
|9.8|ORIENT_CFG_G (0Bh)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 49|
|9.9|INT1_CTRL (0Dh)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 50|
|9.10|INT2_CTRL (0Eh)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 51|
|9.11|WHO_AM_I (0Fh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 51|
|9.12|CTRL1_XL (10h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 51|
|9.13|CTRL2_G (11h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 53|
|9.14|CTRL3_C (12h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 54|
|9.15|CTRL4_C (13h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 55|
|9.16|CTRL5_C (14h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 55|
|9.17|CTRL6_C (15h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 57|
|9.18|CTRL7_G (16h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 57|
|9.19|CTRL8_XL (17h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 58|



3/99 

DocID026899 Rev 7 

**Contents** 

**LSM6DS3** 

|9.20|CTRL9_XL (18h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 59|
|---|---|
|9.21|CTRL10_C (19h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 59|
|9.22|MASTER_CONFIG (1Ah) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 60|
|9.23|WAKE_UP_SRC (1Bh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 60|
|9.24|TAP_SRC (1Ch) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 61|
|9.25|D6D_SRC (1Dh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 62|
|9.26|STATUS_REG (1Eh)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 62|
|9.27|OUT_TEMP_L (20h), OUT_TEMP(21h)  . . . . . . . . . . . . . . . . . . . . . . . . . 63|
|9.28|OUTX_L_G (22h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 63|
|9.29|OUTX_H_G (23h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 63|
|9.30|OUTY_L_G (24h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 63|
|9.31|OUTY_H_G (25h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 64|
|9.32|OUTZ_L_G (26h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 64|
|9.33|OUTZ_H_G (27h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 64|
|9.34|OUTX_L_XL (28h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 64|
|9.35|OUTX_H_XL (29h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 65|
|9.36|OUTY_L_XL (2Ah) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 65|
|9.37|OUTY_H_XL (2Bh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 65|
|9.38|OUTZ_L_XL (2Ch) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 65|
|9.39|OUTZ_H_XL (2Dh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 66|
|9.40|SENSORHUB1_REG (2Eh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 66|
|9.41|SENSORHUB2_REG (2Fh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 66|
|9.42|SENSORHUB3_REG (30h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 66|
|9.43|SENSORHUB4_REG (31h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 67|
|9.44|SENSORHUB5_REG (32h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 67|
|9.45|SENSORHUB6_REG (33h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 67|
|9.46|SENSORHUB7_REG (34h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 67|
|9.47|SENSORHUB8_REG(35h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 68|
|9.48|SENSORHUB9_REG (36h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 68|
|9.49|SENSORHUB10_REG (37h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 68|
|9.50|SENSORHUB11_REG (38h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 68|
|9.51|SENSORHUB12_REG(39h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 69|
|9.52|FIFO_STATUS1 (3Ah)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 69|



4/99 

DocID026899 Rev 7 

|**LSM6DS3**|**Contents**|
|---|---|
|9.53|FIFO_STATUS2 (3Bh)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 69|
|9.54|FIFO_STATUS3 (3Ch)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 70|
|9.55|FIFO_STATUS4 (3Dh)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 70|
|9.56|FIFO_DATA_OUT_L (3Eh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 70|
|9.57|FIFO_DATA_OUT_H (3Fh)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 71|
|9.58|TIMESTAMP0_REG (40h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 71|
|9.59|TIMESTAMP1_REG (41h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 71|
|9.60|TIMESTAMP2_REG (42h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 71|
|9.61|STEP_TIMESTAMP_L (49h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 72|
|9.62|STEP_TIMESTAMP_H (4Ah) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 72|
|9.63|STEP_COUNTER_L (4Bh)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 72|
|9.64|STEP_COUNTER_H (4Ch)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 72|
|9.65|SENSORHUB13_REG (4Dh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 73|
|9.66|SENSORHUB14_REG (4Eh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 73|
|9.67|SENSORHUB15_REG (4Fh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 73|
|9.68|SENSORHUB16_REG (50h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 73|
|9.69|SENSORHUB17_REG (51h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 74|
|9.70|SENSORHUB18_REG (52h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 74|
|9.71|FUNC_SRC (53h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 74|
|9.72|TAP_CFG (58h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 75|
|9.73|TAP_THS_6D (59h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 75|
|9.74|INT_DUR2 (5Ah)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 76|
|9.75|WAKE_UP_THS (5Bh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 76|
|9.76|WAKE_UP_DUR (5Ch)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 77|
|9.77|FREE_FALL (5Dh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 77|
|9.78|MD1_CFG (5Eh)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 78|
|9.79|MD2_CFG (5Fh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 79|
|9.80|OUT_MAG_RAW_X_L (66h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 79|
|9.81|OUT_MAG_RAW_X_H (67h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 79|
|9.82|OUT_MAG_RAW_Y_L (68h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80|
|9.83|OUT_MAG_RAW_Y_H (69h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80|
|9.84|OUT_MAG_RAW_Z_L (6Ah) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80|
|9.85|OUT_MAG_RAW_Z_H (6Bh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80|



5/99 

DocID026899 Rev 7 

|**Contents**||**LSM6DS3**|
|---|---|---|
|**10**|**Embe**|**dded functions register mapping . . . . . . . . . . . . . . . . . . . . . . . . . 81**|
|**11**|**Embe**|**dded functions registers description  . . . . . . . . . . . . . . . . . . . . . 83**|
||11.1|SLV0_ADD (02h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 83|
||11.2|SLV0_SUBADD (03h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 83|
||11.3|SLAVE0_CONFIG (04h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 83|
||11.4|SLV1_ADD (05h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 84|
||11.5|SLV1_SUBADD (06h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 84|
||11.6|SLAVE1_CONFIG (07h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 85|
||11.7|SLV2_ADD (08h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 85|
||11.8|SLV2_SUBADD (09h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 85|
||11.9|SLAVE2_CONFIG (0Ah)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 86|
||11.10|SLV3_ADD (0Bh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 86|
||11.11|SLV3_SUBADD (0Ch)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 86|
||11.12|SLAVE3_CONFIG (0Dh)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 87|
||11.13|DATAWRITE_SRC_MODE_SUB_SLV0 (0Eh) . . . . . . . . . . . . . . . . . . . . . 87|
||11.14|PEDO_THS_REG (0Fh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 88|
||11.15|SM_THS (13h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 88|
||11.16|PEDO_DEB_REG (14h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 89|
||11.17|STEP_COUNT_DELTA (15h) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 90|
||11.18|MAG_SI_XX (24h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 90|
||11.19|MAG_SI_XY (25h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 90|
||11.20|MAG_SI_XZ (26h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 90|
||11.21|MAG_SI_YX (27h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 91|
||11.22|MAG_SI_YY (28h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 91|
||11.23|MAG_SI_YZ (29h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 91|
||11.24|MAG_SI_ZX (2Ah) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 91|
||11.25|MAG_SI_ZY (2Bh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 92|
||11.26|MAG_SI_ZZ (2Ch) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 92|
||11.27|MAG_OFFX_L (2Dh)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 92|
||11.28|MAG_OFFX_H (2Eh) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 92|
||11.29|MAG_OFFY_L (2Fh)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 93|
||11.30|MAG_OFFY_H (30h)  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 93|



6/99 

DocID026899 Rev 7 

|**LSM6D**|**S3**|**Contents**|
|---|---|---|
||11.31 MAG_OFFZ_L (31h)  . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . . . . . . . . 93|
||11.32 MAG_OFFZ_H (32h)  . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . . . . . . . . 93|
|**12**|**Soldering information . . . . . . . . . . . . . . . . . . . . .**|**. . . . . . . . . . . . . . . . . . 94**|
|**13**|**Package information . . . . . . . . . . . . . . . . . . . . . .**|**. . . . . . . . . . . . . . . . . . 95**|
||13.1<br>LGA-14 package information  . . . . . . . . . . . . . .|. . . . . . . . . . . . . . . . . . . . 95|
||13.2<br>LGA-14 packing information . . . . . . . . . . . . . . .|. . . . . . . . . . . . . . . . . . . . 96|
|**14**|**Revision history  . . . . . . . . . . . . . . . . . . . . . . . . .**|**. . . . . . . . . . . . . . . . . . 98**|



7/99 

DocID026899 Rev 7 

**List of tables** 

**LSM6DS3** 

## **List of tables** 

|Table 1.|Device summary . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . 1|
|---|---|---|
|Table 2.|Pin description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 18|
|Table 3.|Mechanical characteristics . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 19|
|Table 4.|Electrical characteristics . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 21|
|Table 5.|Temperature sensor characteristics . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 22|
|Table 6.|SPI slave timing values. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .<br>|. . . . 23<br>|
|Table 7.|I<sup>2</sup>C slave timing values . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 24|
|Table 8.|Absolute maximum ratings . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 25|
|Table 9.|Serial interface pin description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .<br>|. . . . 32<br>|
|Table 10.|I<sup>2</sup>C terminology . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 32|
|Table 11.|SAD+Read/Write patterns . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 33|
|Table 12.|Transfer when master is writing one byte to slave . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 33|
|Table 13.|<br>Transfer when master is writing multiple bytes to slave . . . . . . . . . . . . . . . . . . . . . . .|<br>. . . . 33|
|Table 14.|Transfer when master is receiving (reading) one byte of data from slave . . . . . . . . .|. . . . 34|
|Table 15.|Transfer when master is receiving (reading) multiple bytes of data from slave . . . . .|. . . . 34|
|Table 16.|Registers address map. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 41|
|Table 17.|FUNC_CFG_ACCESS register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 45|
|Table 18.|FUNC_CFG_ACCESS register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 45|
|Table 19.|SENSOR_SYNC_TIME_FRAME register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 45|
|Table 20.|SENSOR_SYNC_TIME_FRAME register description . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 45|
|Table 21.|FIFO_CTRL1 register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 45|
|Table 22.|FIFO_CTRL1 register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 45|
|Table 23.|FIFO_CTRL2 register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 46|
|Table 24.|FIFO_CTRL2 register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 46|
|Table 25.|FIFO_CTRL3 register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 46|
|Table 26.|FIFO_CTRL3 register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 46|
|Table 27.|Gyro FIFO decimation setting. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 47|
|Table 28.|Accelerometer FIFO decimation setting . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 47|
|Table 29.|<br>FIFO_CTRL4 register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|<br>. . . . 47|
|Table 30.|<br>FIFO_CTRL4 register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|<br>. . . . 47|
|Table 31.|Fourth FIFO data set decimation setting. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 48|
|Table 32.|Third FIFO data set decimation setting. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 48|
|Table 33.|FIFO_CTRL5 register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 48|
|Table 34.|FIFO_CTRL5 register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 48|
|Table 35.|FIFO ODR selection . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 49|
|Table 36.|FIFO mode selection. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 49|
|Table 37.|ORIENT_CFG_G register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 49|
|Table 38.|ORIENT_CFG_G register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 50|
|Table 39.|Settings for orientation of axes . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 50|
|Table 40.|INT1_CTRL register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 50|
|Table 41.|INT1CTRL register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 50|
|Table 42.|_<br>INT2_CTRL register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|<br>. . . . 51|
|Table 43.|INT2CTRL register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 51|
|Table 44.|_<br>WHOAMI register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|<br>. . . . 51|
|Table 45.|__<br>CTRL1_XL register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|<br>. . . . 51|
|Table 46.|CTRL1_XL register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 52|
|Table 47.|Accelerometer ODR register setting . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 52|
|Table 48.|BW and ODR (high-performance mode). . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . 52|



8/99 

DocID026899 Rev 7 

**LSM6DS3** 

**List of tables** 

|Table 49.<br>|CTRL2_G register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 53<br>|
|---|---|
|Table 50.|CTRL2_G register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 53|
|Table 51.|<br>Gyroscope ODR configuration setting . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 53|
|Table 52.|CTRL3_C register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 54|
|Table 53.|CTRL3_C register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 54|
|Table 54.|CTRL4_C register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 55|
|Table 55.|CTRL4_C register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 55|
|Table 56.<br>|CTRL5_C register  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 55<br>|
|Table 57.<br>|CTRL5_C register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 56<br>|
|Table 58.|Output registers rounding pattern . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 56|
|Table 59.|Angular rate sensor self-test mode selection . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 56|
|Table 60.|Linear acceleration sensor self-test mode selection. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 56|
|Table 61.|CTRL6_C register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 57|
|Table 62.|CTRL6_C register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 57|
|Table 63.|CTRL7_G register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 57|
|Table 64.|CTRL7_G register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 57|
|Table 65.<br>|Gyroscope high-pass filter mode configuration. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 58<br>|
|Table 66.|CTRL8_XL register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 58|
|Table 67.|<br>CTRL8_XL register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 58|
|Table 68.|Accelerometer slope and high-pass filter selection and cutoff frequency. . . . . . . . . . . . . . 58|
|Table 69.|Accelerometer LPF2 cutoff frequency. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 58|
|Table 70.|<br>CTRL9_XL register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 59|
|Table 71.|CTRL9_XL register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 59|
|Table 72.|CTRL10_C register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 59|
|Table 73.|CTRL10_C register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 59|
|Table 74.|MASTER_CONFIG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 60|
|Table 75.|MASTER_CONFIG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 60|
|Table 76.|WAKE_UP_SRC register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 60|
|Table 77.|WAKE_UP_SRC register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 61|
|Table 78.|TAP_SRC register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 61|
|Table 79.|TAP_SRC register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 61|
|Table 80.|D6D_SRC register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 62|
|Table 81.|D6D_SRC register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 62|
|Table 82.|STATUS_REG register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 62|
|Table 83.|STATUS_REG register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 62|
|Table 84.|OUT_TEMP_L register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 63|
|Table 85.|OUT_TEMP_H register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 63|
|Table 86.|<br>OUT_TEMP register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 63|
|Table 87.|OUTX_L_G register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 63|
|Table 88.|OUTX_L_G register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 63|
|Table 89.|OUTX_H_G register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 63|
|Table 90.|OUTX_H_G register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 63|
|Table 91.|<br>OUTY_L_G register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 63|
|Table 92.|OUTY_L_G register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 63|
|Table 93.|OUTY_H_G register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 64|
|Table 94.|<br>OUTYHG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 64|
|Table 95.|__<br>OUTZ_L_G register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 64|
|Table 96|OUTZLG register description                                              64|
|.<br>Table 97.|__   . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .<br>OUTZHG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 64|
|Table 98.|__<br>OUTZ_H_G register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 64|
|Table 99.|OUTX_L_XL register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 64|
|Table 100.|DocID026899 Rev 7<br>9/99<br>OUTX_L_XL register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 64|



9/99 

DocID026899 Rev 7 

**List of tables** 

**LSM6DS3** 

|Table 101.<br>|OUTX_H_XL register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 65<br>|
|---|---|
|Table 102.|OUTX_H_XL register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 65|
|Table 103.|<br>OUTY_L_XL register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 65|
|Table 104.|OUTY_L_XL register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 65|
|Table 105.|OUTY_H_G register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 65|
|Table 106.|OUTY_H_G register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 65|
|Table 107.|OUTZ_L_XL register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 65|
|Table 108.<br>|OUTZ_L_XL register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 65<br>|
|Table 109.|OUTZ_H_XL register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 66|
|Table 110.|OUTZ_H_XL register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 66|
|Table 111.|SENSORHUB1_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 66|
|Table 112.|SENSORHUB1_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 66|
|Table 113.|SENSORHUB2_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 66|
|Table 114.|SENSORHUB2_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 66|
|Table 115.|SENSORHUB3_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 66|
|Table 116.|SENSORHUB3_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 66|
|Table 117.|SENSORHUB4_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 67|
|Table 118.|<br>SENSORHUB4_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 67|
|Table 119.|SENSORHUB5_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 67|
|Table 120.|SENSORHUB5_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 67|
|Table 121.|SENSORHUB6REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 67|
|Table 122.|_<br>SENSORHUB6_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 67|
|Table 123.|SENSORHUB7_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 67|
|Table 124.|SENSORHUB7_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 67|
|Table 125.|SENSORHUB8_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 68|
|Table 126.|SENSORHUB8REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 68|
|Table 127.|_<br>SENSORHUB9_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 68|
|Table 128.|SENSORHUB9_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 68|
|Table 129.|SENSORHUB10_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 68|
|Table 130.|SENSORHUB10_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 68|
|Table 131.|SENSORHUB11_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 68|
|Table 132.|<br>SENSORHUB11_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 68|
|Table 133.|<br>SENSORHUB12_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 69|
|Table 134.|SENSORHUB12_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 69|
|Table 135.|FIFO_STATUS1 register  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 69|
|Table 136.|FIFO_STATUS1 register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 69|
|Table 137.|FIFO_STATUS2 register  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 69|
|Table 138.|<br>FIFO_STATUS2 register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 69|
|Table 139.|FIFO_STATUS3 register  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 70|
|Table 140.|FIFO_STATUS3 register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 70|
|Table 141.|FIFO_STATUS4 register  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 70|
|Table 142.|<br>FIFO_STATUS4 register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 70|
|Table 143.|<br>FIFO_DATA_OUT_L register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 70|
|Table 144.|FIFO_DATA_OUT_L register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 70|
|Table 145.|FIFODATAOUTH register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 71|
|Table 146.|___<br>FIFODATAOUTH register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 71|
|Table 147.|___<br>TIMESTAMP0_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 71|
|Table 148.|TIMESTAMP0REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 71|
|<br>Table 149.|_<br>TIMESTAMP1REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 71|
|Table 150.|_<br>TIMESTAMP1_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 71|
|Table 151.|TIMESTAMP2_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 71|
|10/99<br>Table 152.|DocID026899 Rev 7<br>TIMESTAMP2_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 71|



10/99 

DocID026899 Rev 7 

**LSM6DS3** 

**List of tables** 

|Table 153.<br>|STEP_TIMESTAMP_L register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 72<br>|
|---|---|
|Table 154.|STEPTIMESTAMPL register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 72|
|Table 155.|__<br>STEP_TIMESTAMP_H register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 72|
|Table 156.|STEP_TIMESTAMP_H register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 72|
|Table 157.|STEP_COUNTER_L register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 72|
|Table 158.|STEP_COUNTER_L register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 72|
|Table 159.|STEP_COUNTER_H register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 72|
|Table 160.<br>|STEP_COUNTER_H register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 72<br>|
|Table 161.|SENSORHUB13_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 73|
|Table 162.|SENSORHUB13_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 73|
|Table 163.|SENSORHUB14_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 73|
|Table 164.|<br>SENSORHUB14_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 73|
|Table 165.|SENSORHUB15_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 73|
|Table 166.|SENSORHUB15_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 73|
|Table 167.|SENSORHUB16_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 73|
|Table 168.|SENSORHUB16_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 73|
|Table 169.|SENSORHUB17_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 74|
|Table 170.|SENSORHUB17_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 74|
|Table 171.|SENSORHUB18_REG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 74|
|Table 172.|SENSORHUB18_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 74|
|Table 173.|FUNC_SRC register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 74|
|Table 174.|<br>FUNC_SRC register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 74|
|Table 175.|TAP_CFG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 75|
|Table 176.|TAP_CFG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 75|
|Table 177.|TAP_THS_6D register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 75|
|Table 178.|TAP_THS_6D register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 75|
|Table 179.|<br>Threshold for D4D/D6D function. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 76|
|Table 180.|INT_DUR2 register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 76|
|Table 181.|INT_DUR2 register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 76|
|Table 182.|WAKE_UP_THS register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 76|
|Table 183.|WAKE_UP_THS register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 77|
|Table 184.|<br>WAKE_UP_DUR register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 77|
|Table 185.|WAKE_UP_DUR register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 77|
|Table 186.|FREE_FALL register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 77|
|Table 187.|FREE_FALL register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 77|
|Table 188.|Threshold for free-fall function . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 78|
|Table 189.|MD1_CFG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 78|
|Table 190.|<br>MD1_CFG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 78|
|Table 191.|MD2_CFG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 79|
|Table 192.|MD2_CFG register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 79|
|Table 193.|OUT_MAG_RAW_X_L register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 79|
|Table 194.|<br>OUT_MAG_RAW_X_L register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 79|
|Table 195.|<br>OUT_MAG_RAW_X_H register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 79|
|Table 196.|OUT_MAG_RAW_X_H register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80|
|Table 197.|OUTMAGRAWYL register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80|
|Table 198.|____<br>OUTMAGRAWYL register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80|
|Table 199.|____<br>OUT_MAG_RAW_Y_H register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80|
|Table 200.|OUTMAGRAWYH register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80|
|<br>Table 201.|____<br>OUTMAGRAWZL register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80|
|Table 202.|____<br>OUT_MAG_RAW_Z_L register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80|
|Table 203.|OUT_MAG_RAW_Z_H register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80|
|Table 204.|DocID026899 Rev 7<br>11/99<br>OUT_MAG_RAW_Z_H register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80|



11/99 

DocID026899 Rev 7 

**List of tables** 

**LSM6DS3** 

|Table 205.<br>|Registers address map - embedded functions . . . . . . . . . . . . . . . . . . . . .<br>|. . . . . . . . . . . . . 81<br>|
|---|---|---|
|Table 206.|SLV0ADD register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 83|
|Table 207.|_<br>SLV0_ADD register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|<br>. . . . . . . . . . . . . 83|
|Table 208.|SLV0_SUBADD register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 83|
|Table 209.|SLV0_SUBADD register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 83|
|Table 210.|SLAVE0_CONFIG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 83|
|Table 211.|SLAVE0_CONFIG register description. . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 84|
|Table 212.<br>|SLV1_ADD register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .<br>|. . . . . . . . . . . . . 84<br>|
|Table 213.<br>|SLV1_ADD register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .<br>|. . . . . . . . . . . . . 84<br>|
|Table 214.|SLV1_SUBADD register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 84|
|Table 215.<br>|SLV1_SUBADD register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . .<br>|. . . . . . . . . . . . . 84<br>|
|Table 216.|SLAVE1_CONFIG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 85|
|Table 217.|SLAVE1_CONFIG register description. . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 85|
|Table 218.|SLV2_ADD register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 85|
|Table 219.|SLV2_ADD register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 85|
|Table 220.|SLV2_SUBADD register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 85|
|Table 221.|SLV2_SUBADD register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 85|
|Table 222.|SLAVE2_CONFIG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 86|
|Table 223.|SLAVE2_CONFIG register description. . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 86|
|Table 224.|SLV3_ADD register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 86|
|Table 225.|SLV3_ADD register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 86|
|Table 226.|<br>SLV3_SUBADD register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|<br>. . . . . . . . . . . . . 86|
|Table 227.|SLV3_SUBADD register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 86|
|Table 228.|SLAVE3_CONFIG register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 87|
|Table 229.|SLAVE3_CONFIG register description. . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 87|
|Table 230.|DATAWRITE_SRC_MODE_SUB_SLV0 register . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 87|
|Table 231.|DATAWRITE_SRC_MODE_SUB_SLV0 register description. . . . . . . . . .|. . . . . . . . . . . . . 87|
|Table 232.|PEDO_THS_REG register default values. . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 88|
|Table 233.|PEDO_THS_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 88|
|Table 234.|SM_THS register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 88|
|Table 235.|SM_THS register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 88|
|Table 236.|PEDO_DEB_REG register default values . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 89|
|Table 237.|PEDO_DEB_REG register description . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 89|
|Table 238.|STEP_COUNT_DELTA register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 90|
|Table 239.|STEP_COUNT_DELTA register description. . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 90|
|Table 240.|MAG_SI_XX register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 90|
|Table 241.|MAG_SI_XX register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 90|
|Table 242.|MAG_SI_XY register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 90|
|Table 243.|MAG_SI_XY register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 90|
|Table 244.|MAG_SI_XZ register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 90|
|Table 245.|MAG_SI_XZ register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 90|
|Table 246.|<br>MAG_SI_YX register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|<br>. . . . . . . . . . . . . 91|
|Table 247.|MAG_SI_YX register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 91|
|Table 248.|MAG_SI_YY register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 91|
|Table 249.|MAG_SI_YY register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 91|
|Table 250.|<br>MAGSIYZ register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|<br>. . . . . . . . . . . . . 91|
|Table 251.<br>|__<br>MAG_SI_YZ register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .<br>|<br>. . . . . . . . . . . . . 91<br>|
|Table 252.|MAGSIZX register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 91|
|Table 253.|__<br>MAG_SI_ZX register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|<br>. . . . . . . . . . . . . 91|
|Table 254.|MAG_SI_ZY register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 92|
|Table 255.|MAG_SI_ZY register description . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 92|
|12/99<br>Table 256.|DocID026899 Rev 7<br>MAG_SI_ZZ register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . . . . . 92|



12/99 

DocID026899 Rev 7 

**LSM6DS3** 

**List of tables** 

|Table 257.|MAG_SI_ZZ register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 92|
|---|---|
|Table 258.|MAG_OFFX_L register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 92|
|Table 259.|MAG_OFFX_L register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 92|
|Table 260.|MAG_OFFX_H register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 92|
|Table 261.|MAG_OFFX_L register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 92|
|Table 262.|MAG_OFFY_L register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 93|
|Table 263.|MAG_OFFY_L register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 93|
|Table 264.|MAG_OFFY_H register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 93|
|Table 265.|MAG_OFFY_L register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 93|
|Table 266.|MAG_OFFZ_L register . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 93|
|Table 267.|MAG_OFFZ_L register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 93|
|Table 268.|MAG_OFFZ_H register. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 93|
|Table 269.|MAG_OFFX_L register description. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 93|
|Table 270.|Reel dimensions for carrier tape of LGA-14 package. . . . . . . . . . . . . . . . . . . . . . . . . . . . . 97|
|Table 271.|Document revision history. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 98|



13/99 

DocID026899 Rev 7 

**List of figures** 

**LSM6DS3** 

## **List of figures** 

|Figure 1.|Pin connections . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 17|
|---|---|---|
|Figure 2.|SPI slave timing diagram . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .<br>|. . . . . . . . . 23<br>|
|Figure 3.|I<sup>2</sup>C slave timing diagram  . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 24|
|Figure 4.|Accelerometer chain . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 30|
|Figure 5.|Accelerometer composite filter . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 30|
|Figure 6.|Gyroscope chain. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 31|
|Figure 7.|Read and write protocol . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 34|
|Figure 8.|SPI read protocol . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 35|
|Figure 9.|Multiple byte SPI read protocol (2-byte example). . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 36|
|Figure 10.|SPI write protocol . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 36|
|Figure 11.|Multiple byte SPI write protocol (2-byte example). . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 36|
|Figure 12.|SPI read protocol in 3-wire mode . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 37|
|Figure 13.|LSM6DS3 electrical connections in Mode 1 . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 38|
|Figure 14.|LSM6DS3 electrical connections in Mode 2 . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 39|
|Figure 15.|LSM6DS3 electrical connections in Mode 3 . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 40|
|Figure 16.|LGA-14 2.5x3x0.86 mm 14L package outline and mechanical data. . . . . . . . .|. . . . . . . . . 95|
|Figure 17.|Carrier tape information for LGA-14 package. . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 96|
|Figure 18.|LGA-14 package orientation in carrier tape . . . . . . . . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 96|
|Figure 19.|Reel information for carrier tape of LGA-14 package . . . . . . . . . . . . . . . . . . . .|. . . . . . . . . 97|



14/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Overview** 

### **1 Overview** 

The LSM6DS3 is a system-in-package featuring a high-performance 3-axis digital accelerometer and 3-axis digital gyroscope. 

The integrated power-efficient modes are able to reduce the power consumption down to 1.25 mA in high-performance mode, combining always-on low-power features with superior sensing precision for an optimal motion experience for the consumer thanks to ultra-low noise performance for both the gyroscope and accelerometer. 

The LSM6DS3 delivers best-in-class motion sensing that can detect orientation and gestures in order to empower application developers and consumers with features and capabilities that are more sophisticated than simply orienting their devices to portrait and landscape mode. 

The event-detection interrupts enable efficient and reliable motion tracking and contextual awareness, implementing hardware recognition of free-fall events, 6D orientation, tap and double-tap sensing, activity or inactivity, and wakeup events. 

The LSM6DS3 supports main OS requirements, offering real, virtual and batch mode sensors. In addition, the LSM6DS3 can efficiently run the sensor-related features specified in Android, saving power and enabling faster reaction time. In particular, the LSM6DS3 has been designed to implement hardware features such as significant motion, tilt, pedometer functions, time stamping and to support the data acquisition of an external magnetometer with ironing correction (hard, soft). 

The LSM6DS3 offers hardware flexibility to connect the pins with different mode connections to external sensors to expand functionalities such as adding a sensor hub, auxiliary SPI, etc. 

Up to 8 kbyte of FIFO with dynamic allocation of significant data (i.e. external sensors, time stamp, etc.) allows overall power saving of the system. 

Like the entire portfolio of MEMS sensor modules, the LSM6DS3 leverages ~~on~~ the robust and mature in-house manufacturing processes already used for the production of micromachined accelerometers and gyroscopes. The various sensing elements are manufactured using specialized micromachining processes, while the IC interfaces are developed using CMOS technology that allows the design of a dedicated circuit which is trimmed to better match the characteristics of the sensing element. 

The LSM6DS3 is available in a small plastic land grid array (LGA) package of 2.5 x 3.0 x 0.83 mm to address ultra-compact solutions. 

15/99 

DocID026899 Rev 7 

**Embedded low-power features** 

**LSM6DS3** 

### **2 Embedded low-power features** 

The LSM6DS3 has been designed to be fully compliant with Android, featuring the following on-chip functions: 

- 8 kbyte data buffering 

   - 100% efficiency with flexible configurations and partitioning 

   - possibility to store time stamp 

- Event-detection interrupts (fully configurable): 

   - free-fall 

   - wakeup 

   - 6D orientation 

   - tap and double-tap sensing 

   - activity / inactivity recognition 

- Specific IP blocks with negligible power consumption and high-performance: 

   - pedometer functions: step detector and step counters 

   - tilt (Android compliant, refer to _Section 2.1: Tilt detection_ for additional info 

   - significant motion (Android compliant) 

- Sensor hub 

   - up to 6 total sensors: 2 internal (accelerometer and gyroscope) and 4 external sensors 

- Data rate synchronization with external trigger for reduced sensor access and enhanced fusion 

#### **2.1 Tilt detection** 

The tilt function helps to detect activity change and has been implemented in hardware using only the accelerometer to achieve both the targets of ultra-low power consumption and robustness during the short duration of dynamic accelerations. 

It is based on a trigger of an event each time the device's tilt changes by an angle greater than 35 degrees from the start position. 

The tilt function can be used with different scenarios, for example: 

- a) Trigger when phone is in a front pants pocket and the user goes from sitting to standing or standing to sitting; 

- b) Doesn’t trigger when phone is in a front pants pocket and the user is walking, running or going upstairs. 

16/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Pin description** 

### **3 Pin description** 

###### **Figure 1. Pin connections** 



<!-- Start of picture text -->
Z<br>Y<br>X (TOP VIEW)<br>DIRECTION OF THE<br>DETECTABLE<br>ACCELERATIONS<br>12 14<br>NC 11 1 SDO/SA0<br>BOTTOM<br>OCS SDx<br>VIEW<br>INT2 SCx<br>+Ω<br>Z VDD 8 4 INT1<br>+Ω Y +Ω (TOP VIEW) 7 5<br>X DIRECTIONS OF THE<br>X<br>DETECTABLE<br>ANGULAR RATES<br>CS SCL SDA<br>GND GND VDDIO<br><!-- End of picture text -->

LSM6DS3 offers the flexibility to connect the pins in order to have three different mode connections and functionalities. In detail: 

- **Mode 1** : I<sup>2</sup> C slave interface or SPI (3- and 4-wire) serial interface is available; 

- **Mode 2** : I<sup>2</sup> C slave interface or SPI (3- and 4-wire) serial interface and I<sup>2</sup> C interface master for external sensors connections are available; 

- **Mode 3** : I<sup>2</sup> C slave interface and auxiliary SPI (3-wire) serial interface for external sensor connection (i.e. EIS application) are available. 

In the following table each mode is described for the pin connection and function. 

17/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Pin description** 

###### **Table 2. Pin description** 

|**Pin#**|**Name**|**Mode 1 function**|**Mode 2 function**|**Mode 3 function**|
|---|---|---|---|---|
|1|SDO/SA0|SPI 4-wire interface serial<br>data output (SDO)<br>I<sup>2</sup>C least significant bit of the<br>device address (SA0)|SPI 4-wire interface serial data<br>output (SDO)<br>I<sup>2</sup>C least significant bit of the<br>device address (SA0)|I<sup>2</sup>C least significant bit of the<br>device address (SA0)|
|2|SDx|Connect to VDDIO or GND|I<sup>2</sup>C serial data master (MSDA)|Auxiliary SPI 3-wire interface<br>serial data input (SDI)<br>and serial data output (SDO)|
|3|SCx|Connect to VDDIO or GND|I<sup>2</sup>C serial clock master (MSCL)|Auxiliary SPI 3-wire interface<br>serial port clock (SPC)|
|4|INT1||Programmable interrupt 1||
|5|VDDIO<sup>(1)</sup>||Power supply for I/O pins||
|6|GND||0 V supply||
|7|GND||0 V supply||
|8|VDD<sup>(2)</sup>||Power supply||
|9|INT2|Programmable interrupt 2<br>(INT2)/ Data enable (DEN)|Programmable interrupt 2<br>(INT2)/ Data enable (DEN)/<br>I<sup>2</sup>C master external<br>synchronization signal (MDRDY)|Programmable interrupt 2<br>(INT2)/ Data enable (DEN)|
|10|OCS|Leave unconnected|Leave unconnected|Auxiliary SPI 3-wire interface<br>enable|
|11|NC||Leave unconnected||
|12|CS|I<sup>2</sup>C/SPI mode selection<br>(1: SPI idle mode / I<sup>2</sup>C<br>communication enabled; 0:<br>SPI communication mode /<br>I<sup>2</sup>C disabled)|I<sup>2</sup>C/SPI mode selection<br>(1: SPI idle mode / I<sup>2</sup>C<br>communication enabled;<br>0: SPI communication mode /<br>I<sup>2</sup>C disabled)|Leave unconnected|
|13|SCL|I<sup>2</sup>C serial clock (SCL)<br>SPI serial port clock (SPC)|I<sup>2</sup>C serial clock (SCL)<br>SPI serial port clock (SPC)|I<sup>2</sup>C serial clock (SCL)|
|14|SDA|I<sup>2</sup>C serial data (SDA)<br>SPI serial data input (SDI)<br>3-wire interface serial data<br>output (SDO)|I<sup>2</sup>C serial data (SDA)<br>SPI serial data input (SDI)<br>3-wire interface serial data<br>output (SDO)|I<sup>2</sup>C serial data (SDA)|



1. Recommended 100 nF filter capacitor. 

2. Recommended 100 nF capacitor. 

18/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Module specifications** 

### **4 Module specifications** 

#### **4.1 Mechanical characteristics** 

@ Vdd = 1.8 V, T = 25 °C unless otherwise noted. 

**Table 3. Mechanical characteristics** 

|**Symbol**|**Parameter**|**Test conditions**|**Min.**|**Typ.**<sup>**(1)**</sup>|**Max.**|**Unit**|
|---|---|---|---|---|---|---|
|||||±2|||
||Linear acceleration measurement|||±4|||
|LA_FS|range|||±8||_g_|
|||||±16|||
|||||±125|||
|||||±245|||
|G_FS|Angular rate<br>measurement range|||±500||dps|
|||||±1000|||
|||||±2000|||
|||FS = ±2||0.061|||
|||FS = ±4||0.122|||
|LA_So|Linear acceleration sensitivity|FS = ±8||0.244||m_g_/LSB|
|||FS = ±16||0.488|||
|||FS = ±125||4.375|||
|||FS = ±245||8.75|||
|G_So|Angular rate sensitivity|FS = ±500||17.50||mdps/LSB|
|||FS = ±1000||35|||
|||FS = ±2000||70|||
|LA_SoDr|Linear acceleration sensitivity<br>change vs. temperature<sup>(2)</sup>|from -40° to +85°<br>delta from T=25°||±1||%|
|G_SoDr|Angular rate sensitivity change<br>vs. temperature<sup>(2)</sup>|from -40° to +85°<br>delta from T=25°||±1.5||%|
|LA_TyOff|<sup>Linear acceleration typical zero-</sup><sup>_g_</sup><br>level offset accuracy<sup>(3)</sup>|||±40||m_g_|
|G_TyOff|Angular rate typical zero-rate<br>level<sup>(3)</sup>|||±10||dps|
|LA_OffDr|<sup>Linear acceleration zero-</sup><sup>_g_ level</sup><br>change vs. temperature<sup>(2)</sup>|||±0.5||m_g/_°C|
|G_OffDr|Angular rate typical zero-rate<br>level change vs. temperature<sup>(2)</sup>|||±0.05||dps/°C|
|Rn|Rate noise density|||7||mdps/Hz|
|An|Acceleration noise density|FS= ±2_g_<br>ODR = 104 Hz||90||μ_g_/Hz|



19/99 

DocID026899 Rev 7 

**Module specifications** 

**LSM6DS3** 

**Table 3. Mechanical characteristics  (continued)** 

|**Symbol**|**Parameter**|**Test conditions**|**Min.**|**Typ.**<sup>**(1)**</sup>|**Max.**|**Unit**|
|---|---|---|---|---|---|---|
|LA_ODR|Linear acceleration output data<br>rate|||13<br>26<br>52<br>104<br>208<br>416<br>833<br>1666<br>3332<br>6664||Hz|
|||||13<br>26<br>52|||
|G_ODR|Angular rate output data rate|||104<br>208<br>416<br>833<br>1666|||
|Top|Operating temperature range||-40||+85|°C|



1. Typical specifications are not guaranteed. 

2. Measurements are performed in a uniform temperature setup. 

3. Values after soldering. 

20/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Module specifications** 

#### **4.2 Electrical characteristics** 

@ Vdd = 1.8 V, T = 25 °C unless otherwise noted. 

**Table 4. Electrical characteristics** 

|**Symbol**|**Parameter**|**Test conditions**|**Min.**|**Typ.**<sup>**(1)**</sup>|**Max.**|**Unit**|
|---|---|---|---|---|---|---|
|Vdd|Supply voltage||1.71|1.8|3.6|V|
|Vdd_IO|Power supply for I/O||1.62||Vdd +<br>0.1|V|
|IddHP|Gyroscope and accelerometer<br>in high-performance mode|up to ODR = 1.6 kHz||1.25||mA|
|IddNM|Gyroscope and accelerometer<br>in normal mode|ODR = 208 Hz||0.9||mA|
|IddLP|Gyroscope and accelerometer<br>in low-power mode|ODR = 13 Hz||0.42||mA|
|LA_IddHP|Accelerometer current<br>consumption in high-<br>performance mode|up to ODR = 1.6 kHz||240||μA|
|LA_IddNM|<sup>Accelerometer current</sup><br>consumption in normal mode|ODR = 104 Hz||70||μA|
|LA_IddLM|Accelerometer current<br>consumption in low-power<br>mode|ODR = 13 Hz||24||μA|
|IddPD|Gyroscope and accelerometer<br>in power down|||6||μA|
|Top|Operating temperature range||-40||+85|°C|



1. Typical specifications are not guaranteed. 

For details related to the LSM6DS3 operating modes, refer to _5.2: Gyroscope power modes_ and _5.3: Accelerometer power modes_ . 

21/99 

DocID026899 Rev 7 

**Module specifications** 

**LSM6DS3** 

#### **4.3 Temperature sensor characteristics** 

@ Vdd = 1.8 V, T = 25 °C unless otherwise noted. 

**Table 5. Temperature sensor characteristics** 

|**Symbol**|**Parameter**|**Test condition**|**Min.**|**Typ.**<sup>**(1)**</sup>|**Max.**|**Unit**|
|---|---|---|---|---|---|---|
|TODR|Temperature refresh rate|||52||Hz|
|Toff|Temperature offset<sup>(2)</sup>||-15||+15|°C|
|TSen|Temperature sensitivity|||16||LSB/°C|
|TST|Temperature stabilization<br>time<sup>(3)</sup>||||500|μs|
|T_ADC_res|Temperature ADC resolution|||12||bit|
|Top|Operating temperature range||-40||+85|°C|



1. Typical specifications are not guaranteed. 

2. The output of the temperature sensor is 0 LSB (typ.) at 25 °C. 

3. Time from power ON bit to valid data based on characterization data. 

22/99 

DocID026899 Rev 7 



<!-- Start of picture text -->
cs \ /:<br>' tsucs) tsp) tives) :<br>4 t ! H '<br>tsusi) H thsi H :<br><—<br>> «<——>. ' | '<br>1 tyso) 1 thiso) | tuis(so)<br>‘i u<br><!-- End of picture text -->



<!-- Start of picture text -->
REPEATED<br>ST ART<br>, f- tay START<br>' 1 teyeoa) thsoay | \ i fy \<br>H { tg<br>' ot to ' ! !<br>thisy — twsa) tw(scLH)<br><!-- End of picture text -->

**LSM6DS3** 

**Module specifications** 

#### **4.5 Absolute maximum ratings** 

Stresses above those listed as “Absolute maximum ratings” may cause permanent damage to the device. This is a stress rating only and functional operation of the device under these conditions is not implied. Exposure to maximum rating conditions for extended periods may affect device reliability. 

**Table 8. Absolute maximum ratings** 

|**Symbol**|**Ratings**|**Maximum value**|**Unit**|
|---|---|---|---|
|Vdd|Supply voltage|-0.3 to 4.8|V|
|TSTG|Storage temperature range|-40 to +125|°C|
|Sg|Acceleration_g_for 0.1 ms|10,000|_g_|
|ESD|Electrostatic discharge protection (HBM)|2|kV|
|Vin|Input voltage on any control pin<br>(including CS, SCL/SPC, SDA/SDI/SDO, SDO/SA0)|0.3 to Vdd_IO +0.3|V|



_Note: Supply voltage on any pin should never exceed 4.8 V._ 

This device is sensitive to mechanical shock, improper handling can cause permanent damage to the part. 



This device is sensitive to electrostatic discharge (ESD), improper handling can cause permanent damage to the part. 

25/99 

DocID026899 Rev 7 

**Module specifications** 

**LSM6DS3** 

#### **4.6 Terminology** 

##### **4.6.1 Sensitivity** 

Linear acceleration sensitivity can be determined, for example, by applying 1 _g_ acceleration to the device. Because the sensor can measure DC accelerations, this can be done easily by pointing the selected axis towards the ground, noting the output value, rotating the sensor 180 degrees (pointing towards the sky) and noting the output value again. By doing so, ±1 _g_ acceleration is applied to the sensor. Subtracting the larger output value from the smaller one, and dividing the result by 2, leads to the actual sensitivity of the sensor. This value changes very little over temperature and over time. The sensitivity tolerance describes the range of sensitivities of a large number of sensors. 

An angular rate gyroscope is device that produces a positive-going digital output for counterclockwise rotation around the axis considered. Sensitivity describes the gain of the sensor and can be determined by applying a defined angular velocity to it. This value changes very little over temperature and time. 

##### **4.6.2 Zero-** **_g_ and zero-rate level** 

Linear acceleration zero- _g_ level offset (TyOff) describes the deviation of an actual output signal from the ideal output signal if no acceleration is present. A sensor in a steady state on a horizontal surface will measure 0 _g_ on both the X-axis and Y-axis, whereas the Z-axis will measure 1 _g_ . Ideally, the output is in the middle of the dynamic range of the sensor (content of OUT registers 00h, data expressed as 2’s complement number). A deviation from the ideal value in this case is called zero- _g_ offset. 

Offset is to some extent a result of stress to MEMS sensor and therefore the offset can slightly change after mounting the sensor onto a printed circuit board or exposing it to extensive mechanical stress. Offset changes little over temperature, see “Linear acceleration zero- _g_ level change vs. temperature” in _Table 3_ . The zero- _g_ level tolerance (TyOff) describes the standard deviation of the range of zero- _g_ levels of a group of sensors. 

Zero-rate level describes the actual output signal if there is no angular rate present. The zero-rate level of precise MEMS sensors is, to some extent, a result of stress to the sensor and therefore the zero-rate level can slightly change after mounting the sensor onto a printed circuit board or after exposing it to extensive mechanical stress. This value changes very little over temperature and time. 

26/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Functionality** 

### **5 Functionality** 

#### **5.1 Operating modes** 

The LSM6DS3 has three operating modes available: 

- only accelerometer active and gyroscope in power-down 

- only gyroscope active and accelerometer in power-down 

- both accelerometer and gyroscope sensors active with independent ODR 

The accelerometer is activated from power down by writing ODR_XL[3:0] in _CTRL1_XL (10h)_ while the gyroscope is activated from power-down by writing ODR_G[3:0] in _CTRL2_G (11h)_ . For combo mode the ODRs are totally independent. 

#### **5.2 Gyroscope power modes** 

In the LSM6DS3, the gyroscope can be configured in four different operating modes: powerdown, low-power, normal mode and high-performance mode. The operating mode selected depends on the value of the G_HM_MODE bit in _CTRL7_G (16h)_ . If G_HM_MODE is set to ‘0’, high-performance mode is valid for all ODRs (from 13 Hz up to 1.6 kHz). 

To enable the low-power and normal mode, the G_HM_MODE bit has to be set to ‘1’. Lowpower mode is available for lower ODR (13, 26, 52 Hz) while normal mode is available for ODRs equal to 104 and 208 Hz. 

#### **5.3 Accelerometer power modes** 

In the LSM6DS3, the accelerometer can be configured in four different operating modes: power-down, low-power, normal mode and high-performance mode. The operating mode selected depends on the value of the XL_HM_MODE bit in _CTRL6_C (15h)_ . If XL_HM_MODE is set to ‘0’, high-performance mode is valid for all ODRs (from 13 Hz up to 6.66 kHz). 

To enable the low-power and normal mode, the XL_HM_MODE bit has to be set to ‘1’. Lowpower mode is available for lower ODRs (13, 26, 52 Hz) while normal mode is available for ODRs equal to 104 and 208 Hz. 

#### **5.4 FIFO** 

The presence of a FIFO allows consistent power saving for the system since the host processor does not need continuously poll data from the sensor, but it can wake up only when needed and burst the significant data out from the FIFO. 

LSM6DS3 embeds 8 kbytes data FIFO to store the following data: 

- gyroscope 

- accelerometer 

- external sensors 

- step counter and time stamp 

- temperature 

27/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Functionality** 

Writing data in the FIFO can be configured to be triggered by the: 

- accelerometer/gyroscope data-ready signal; in which case the ODR must be lower than or equal to both the accelerometer and gyroscope ODRs; 

- sensor hub data-ready signal; 

- step detection signal. 

In addition, each data can be stored at a decimated data rate compared to FIFO ODR and it is configurable by the user, setting the registers _FIFO_CTRL3 (08h)_ and _FIFO_CTRL4 (09h)_ . The available decimation factors are 2, 3, 4, 8, 16, 32. 

Programmable FIFO threshold can be set in _FIFO_CTRL1 (06h)_ and _FIFO_CTRL2 (07h)_ using the FTH [11:0] bits. 

To monitor the FIFO status, dedicated registers ( _FIFO_STATUS1 (3Ah)_ , _FIFO_STATUS2 (3Bh)_ , _FIFO_STATUS3 (3Ch)_ , _FIFO_STATUS4 (3Dh)_ ) can be read to detect FIFO overrun events, FIFO full status, FIFO empty status, FIFO threshold status and the number of unread samples stored in the FIFO. To generate dedicated interrupts on the INT1 and INT2 pads of these status events, the configuration can be set in _INT1_CTRL (0Dh)_ and _INT2_CTRL (0Eh)_ . 

FIFO buffer can be configured according to five different modes: 

- Bypass mode 

- FIFO mode 

- Continuous mode 

- Continuous-to-FIFO mode 

- Bypass-to-continuous mode 

Each mode is selected by the FIFO_MODE_[2:0] in _FIFO_CTRL5 (0Ah)_ register. To guarantee the correct acquisition of data during the switching into and out of FIFO mode, the first sample acquired must be discarded. 

##### **5.4.1** 

##### **Bypass mode** 

In Bypass mode ( _FIFO_CTRL5 (0Ah)_ (FIFO_MODE_[2:0] = 000), the FIFO is not operational and it remains empty. 

Bypass mode is also used to reset the FIFO when in FIFO mode. 

##### **5.4.2** 

##### **FIFO mode** 

In FIFO mode ( _FIFO_CTRL5 (0Ah)_ (FIFO_MODE_[2:0] = 001) data from the output channels are stored in the FIFO until it is full. 

To reset FIFO content, Bypass mode should be selected by writing _FIFO_CTRL5 (0Ah)_ (FIFO_MODE_[2:0]) to '000' After this reset command, it is possible to restart FIFO mode by writing _FIFO_CTRL5 (0Ah)_ (FIFO_MODE_[2:0]) to '001'. 

FIFO buffer memorizes up to 4096 samples of 16 bits each but the depth of the FIFO can be resized by setting the FTH [11:0] bits in _FIFO_CTRL1 (06h)_ and _FIFO_CTRL2 (07h)_ . If the STOP_ON_FTH bit in _CTRL4_C (13h)_ is set to '1', FIFO depth is limited up to FTH [11:0] bits in _FIFO_CTRL1 (06h)_ and _FIFO_CTRL2 (07h)_ . 

28/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Functionality** 

##### **5.4.3 Continuous mode** 

Continuous mode ( _FIFO_CTRL5 (0Ah)_ (FIFO_MODE_[2:0] = 110) provides a continuous FIFO update: as new data arrives, the older data is discarded. 

A FIFO threshold flag _FIFO_STATUS2 (3Bh)_ (FTH) is asserted when the number of unread samples in FIFO is greater than or equal to _FIFO_CTRL1 (06h)_ and _FIFO_CTRL2 (07h)_ (FTH [11:0]). 

It is possible to route _FIFO_STATUS2 (3Bh)_ (FTH) to the INT1 pin by writing in register _INT1_CTRL (0Dh)_ (INT1_FTH) = ‘1’ or to the INT2 pin by writing in register _INT2_CTRL (0Eh)_ (INT2_FTH) = ‘1’. 

A full-flag interrupt can be enabled, _INT1_CTRL (0Dh)_ (INT_ FULL_FLAG) = '1', in order to indicate FIFO saturation and eventually read its content all at once. 

If an overrun occurs, at least one of the oldest samples in FIFO has been overwritten and the OVER_RUN flag in _FIFO_STATUS2 (3Bh)_ is asserted. 

In order to empty the FIFO before it is full, it is also possible to pull from FIFO the number of unread samples available in _FIFO_STATUS1 (3Ah)_ and _FIFO_STATUS2 (3Bh)_ (DIFF_FIFO[11:0]). 

##### **5.4.4 Continuous-to-FIFO mode** 

In Continuous-to-FIFO mode ( _FIFO_CTRL5 (0Ah)_ (FIFO_MODE_[2:0] = 011), FIFO behavior changes according to the trigger event detected in one of the following interrupt registers _FUNC_SRC (53h)_ , _TAP_SRC (1Ch)_ , _WAKE_UP_SRC (1Bh)_ and _D6D_SRC (1Dh)_ . 

When the selected trigger bit is equal to '1', FIFO operates in FIFO mode. 

When the selected trigger bit is equal to '0', FIFO operates in Continuous mode. 

##### **5.4.5 Bypass-to-Continuous mode** 

In Bypass-to-Continuous mode ( _FIFO_CTRL5 (0Ah)_ (FIFO_MODE_[2:0] = '100'), data measurement storage inside FIFO operates in Continuous mode when selected triggers in one of the following interrupt registers _FUNC_SRC (53h)_ , _TAP_SRC (1Ch)_ , _WAKE_UP_SRC (1Bh)_ and _D6D_SRC (1Dh)_ are equal to '1', otherwise FIFO content is reset (Bypass mode). 

##### **5.4.6 FIFO reading procedure** 

The data stored in FIFO are accessible from dedicated registers ( _FIFO_DATA_OUT_L (3Eh)_ and _FIFO_DATA_OUT_H (3Fh)_ ) and each FIFO sample is composed of 16 bits. 

All FIFO status registers ( _FIFO_STATUS1 (3Ah)_ , _FIFO_STATUS2 (3Bh)_ , _FIFO_STATUS3 (3Ch)_ , _FIFO_STATUS4 (3Dh)_ ) can be read at the start of a reading operation, minimizing the intervention of the application processor. 

Saving data in the FIFO buffer is organized in four FIFO data sets consisting of 6 bytes each: 

The 1<sup>st</sup> FIFO data set is reserved for gyroscope data; 

The 2<sup>nd</sup> FIFO data set is reserved for accelerometer data; 

29/99 

DocID026899 Rev 7 



<!-- Start of picture text -->
Analog<br>Anti-aliasing Digital<br>LP Filter LP Filter<br>LPF1<br>FALHWAy ave Fy compositerite it<br>BW_XL[1:0] ODR_XLJ[3:0]<br><!-- End of picture text -->



<!-- Start of picture text -->
HP Filter<br>10<br>L 01 SLOPE_FDS<br>11<br>SLOPE_FDS ORFUNC_EN LPF2_XL_ENHP_SLOPE_XL_EN=1=0<br>SLOPE 00 XL<br>FILTER Activity / Output<br>— Inactivi Reg<br>Digital HPCF_XL[1:0]<br>LP Filter S/D Tap<br>LPF2<br>ie LPF2_XL_EN =1<br>HP_SLOPE_XL_EN=1 co<br>SLOPE_FDS ORFUNC_EN<br>LPF2_XL_EN =X<br>HP_SLOPE_XL_EN=0<br>Free-fall<br>6D / 4D<br>functions LOW_PASS_ON6D<br>©<br>/<br><!-- End of picture text -->



<!-- Start of picture text -->
Digital<br>HP Filter<br>AnalogAnti-aliasing i.<br>L P Filter L P Filter a<br>= Es HP_G_EN<br>ODR_G[3:0]<br><!-- End of picture text -->

<mark>ky</mark> 

**Digital interfaces** 

**LSM6DS3** 

### **6 Digital interfaces** 

The registers embedded inside the LSM6DS3 may be accessed through both the I<sup>2</sup> C and SPI serial interfaces. The latter may be SW configured to operate either in 3-wire or 4-wire interface mode. 

The serial interfaces are mapped onto the same pins. To select/exploit the I<sup>2</sup> C interface, the CS line must be tied high (i.e connected to Vdd_IO). 

**Table 9. Serial interface pin description** 

|**Pin name**|**Pin description**|
|---|---|
|CS|SPI enable<br>I<sup>2</sup>C/SPI mode selection (1: SPI idle mode / I<sup>2</sup>C communication enabled;<br>0: SPI communication mode / I<sup>2</sup>C disabled)|
|SCL/SPC|I<sup>2</sup>C Serial Clock (SCL)<br>SPI Serial Port Clock (SPC)|
|SDA/SDI/SDO|I<sup>2</sup>C Serial Data (SDA)<br>SPI Serial Data Input (SDI)<br>3-wire Interface Serial Data Output (SDO)|
|SDO/SA0|SPI Serial Data Output (SDO)<br>I<sup>2</sup>C less significant bit of the device address|



#### **6.1 I**<sup>**2**</sup> **C serial interface** 

The LSM6DS3 I<sup>2</sup> C is a bus slave. The I<sup>2</sup> C is employed to write the data to the registers, whose content can also be read back. 

The relevant I<sup>2</sup> C terminology is provided in the table below. 

**Table 10.  I**<sup>**2**</sup> **C terminology** 

|**Term**|**Description**|
|---|---|
|Transmitter|The device which sends data to the bus|
|Receiver|The device which receives data from the bus|
|Master|The device which initiates a transfer, generates clock signals and terminates a<br>transfer|
|Slave|The device addressed by the master|



There are two signals associated with the I<sup>2</sup> C bus: the serial clock line (SCL) and the Serial DAta line (SDA). The latter is a bidirectional line used for sending and receiving the data to/from the interface. Both the lines must be connected to Vdd_IO through external pull-up resistors. When the bus is free, both the lines are high. 

The I<sup>2</sup> C interface is implemeted with fast mode (400 kHz) I<sup>2</sup> C standards as well as with the standard mode. 

In order to disable the I<sup>2</sup> C block, (I2C_disable) = 1 must be written in _CTRL4_C (13h)_ . 

32/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Digital interfaces** 

##### **6.1.1 I**<sup>**2**</sup> **C operation** 

The transaction on the bus is started through a START (ST) signal. A START condition is defined as a HIGH to LOW transition on the data line while the SCL line is held HIGH. After this has been transmitted by the master, the bus is considered busy. The next byte of data transmitted after the start condition contains the address of the slave in the first 7 bits and the eighth bit tells whether the master is receiving data from the slave or transmitting data to the slave. When an address is sent, each device in the system compares the first seven bits after a start condition with its address. If they match, the device considers itself addressed by the master. 

The Slave ADdress (SAD) associated to the LSM6DS3 is 110101xb. The SDO/SA0 pin can be used to modify the less significant bit of the device address. If the SDO/SA0 pin is connected to the supply voltage, LSb is ‘1’ (address 1101011b); else if the SDO/SA0 pin is connected to ground, the LSb value is ‘0’ (address 1101010b). This solution permits to connect and address two different inertial modules to the same I<sup>2</sup> C bus. 

Data transfer with acknowledge is mandatory. The transmitter must release the SDA line during the acknowledge pulse. The receiver must then pull the data line LOW so that it remains stable low during the HIGH period of the acknowledge clock pulse. A receiver which has been addressed is obliged to generate an acknowledge after each byte of data received. 

The I<sup>2</sup> C embedded inside the LSM6DS3 behaves like a slave device and the following protocol must be adhered to. After the start condition (ST) a slave address is sent, once a slave acknowledge (SAK) has been returned, an 8-bit sub-address (SUB) is transmitted. The increment of the address is configured by the _CTRL3_C (12h)_ (IF_INC). 

The slave address is completed with a Read/Write bit. If the bit is ‘1’ (Read), a repeated START (SR) condition must be issued after the two sub-address bytes; if the bit is ‘0’ (Write) the master will transmit to the slave with direction unchanged. _Table 11_ explains how the SAD+Read/Write bit pattern is composed, listing all the possible configurations. 

**Table 11. SAD+Read/Write patterns** 

|**Command**|**SAD[6:1]**|**SAD[0] = SA0**|**R/W**|**SAD+R/W**|
|---|---|---|---|---|
|Read|110101|0|1|11010101 (D5h)|
|Write|110101|0|0|11010100 (D4h)|
|Read|110101|1|1|11010111 (D7h)|
|Write|110101|1|0|11010110 (D6h)|



**Table 12. Transfer when master is writing one byte to slave** 

|Master|ST<br>SAD|+ W||SUB||DA|TA|SP|
|---|---|---|---|---|---|---|---|---|
|Slave|||SAK||SAK||SAK||
|Master|**Table 13. Transf**<br>ST<br>SAD + W|**er whe**|**n maste**<br>SUB|**r is wri**|**ting mult**<br>DATA|**iple by**|**tes to slave**<br>DATA|SP|
|Slave||SAK||SAK||SAK|SAK||



33/99 

DocID026899 Rev 7 

**Digital interfaces** 

**LSM6DS3** 

**Table 14. Transfer when master is receiving (reading) one byte of data from slave** 

|Master|ST|SAD + W||SUB||SR|SAD + R|||NMAK|SP|
|---|---|---|---|---|---|---|---|---|---|---|---|
|Slave|||SAK||SAK|||SAK|DATA|||



**Table 15. Transfer when master is receiving (reading) multiple bytes of data from slave** 

|Master|ST|SAD+W||SUB||SR|SAD+R|||MAK||MAK||NMAK|SP|
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
|Slave|||SAK||SAK|||SAK|DATA||DAT<br>A||DATA|||



Data are transmitted in byte format (DATA). Each data transfer contains 8 bits. The number of bytes transferred per transfer is unlimited. Data is transferred with the Most Significant bit (MSb) first. If a receiver can’t receive another complete byte of data until it has performed some other function, it can hold the clock line, SCL LOW to force the transmitter into a wait state. Data transfer only continues when the receiver is ready for another byte and releases the data line. If a slave receiver doesn’t acknowledge the slave address (i.e. it is not able to receive because it is performing some real-time function) the data line must be left HIGH by the slave. The master can then abort the transfer. A LOW to HIGH transition on the SDA line while the SCL line is HIGH is defined as a STOP condition. Each data transfer must be terminated by the generation of a STOP (SP) condition. 

In the presented communication format MAK is Master acknowledge and NMAK is No Master Acknowledge. 

#### **6.2 SPI bus interface** 

The LSM6DS3 SPI is a bus slave. The SPI allows writing and reading the registers of the device. 

The serial interface communicates to the application using 4 wires: **CS** , **SPC** , **SDI** and **SDO** . 

**Figure 7. Read and write protocol** 



<!-- Start of picture text -->
��<br>���<br>���<br>�� ������������������������<br>��� ������������������<br>���<br>������������������������<br><!-- End of picture text -->

**CS** is the serial port enable and it is controlled by the SPI master. It goes low at the start of the transmission and goes back high at the end. **SPC** is the serial port clock and it is controlled by the SPI master. It is stopped high when **CS** is high (no transmission). **SDI** and **SDO** are, respectively, the serial port data input and output. Those lines are driven at the falling edge of **SPC** and should be captured at the rising edge of **SPC** . 

34/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Digital interfaces** 

Both the read register and write register commands are completed in 16 clock pulses or in multiples of 8 in case of multiple read/write bytes. Bit duration is the time between two falling edges of **SPC** . The first bit (bit 0) starts at the first falling edge of **SPC** after the falling edge of **CS** while the last bit (bit 15, bit 23, ...) starts at the last falling edge of SPC just before the rising edge of **CS** . 

**_bit 0_** : RW bit. When 0, the data DI(7:0) is written into the device. When 1, the data DO(7:0) from the device is read. In latter case, the chip will drive **SDO** at the start of bit 8. 

**_bit 1-7_** : address AD(6:0). This is the address field of the indexed register. 

**_bit 8-15_** : data DI(7:0) (write mode). This is the data that is written into the device (MSb first). 

**_bit 8-15_** : data DO(7:0) (read mode). This is the data that is read from the device (MSb first). 

In multiple read/write commands further blocks of 8 clock periods will be added. When the _CTRL3_C (12h)_ (IF_INC) bit is ‘0’, the address used to read/write data remains the same for every block. When the _CTRL3_C (12h)_ (IF_INC) bit is ‘1’, the address used to read/write data is increased at every block. 

The function and the behavior of **SDI** and **SDO** remain unchanged. 

##### **6.2.1 SPI read** 

**Figure 8. SPI read protocol** 



<!-- Start of picture text -->
��<br>���<br>���<br>��<br>��� ������������������<br>���<br>������������������������<br><!-- End of picture text -->

The SPI Read command is performed with 16 clock pulses. A multiple byte read command is performed by adding blocks of 8 clock pulses to the previous one. 

**_bit 0_** : READ bit. The value is 1. 

**_bit 1-7_** : address AD(6:0). This is the address field of the indexed register. 

**_bit 8-15_** : data DO(7:0) (read mode). This is the data that will be read from the device (MSb first). 

**_bit 16-..._** : data DO(...-8). Further data in multiple byte reads. 

35/99 

DocID026899 Rev 7 

Cs \ / <)oaAUn ~~A~~ 7VAUAUAUAUAUAUAUAUAUAUAUAUAUAUAUAUAUAUAUAV AU Ai=EEE sp) — ~~OOOOOOOOCOOOOOOCROOOOOOCE~~ = RW AD6 ADS AD4 AD3 AD2 AD1 ADO sp0 $$$ ~~. goon~~ DO7 DO6 DOS DO4 DO3 DO2 DO1 DOD DO1H01H01D01D01D01009 Dos 



<!-- Start of picture text -->
CS \ a<br>sPC V /V/V VV VV VIVID VI I<br>SDI XXX XXX XIE O> F XOF 0 OE> F X >><br>R W DI7 DI6 DIS DI4 DIZ DI2 DIT DIO<br>AD6 ADS AD4 AD3 AD2 AD1 ADO<br><!-- End of picture text -->



<!-- Start of picture text -->
CS \ /<br>< o UAVAVAUAUAVAUAUAUAUAUAUAUAUAUAUAUAUAUAUAUAUAUAUEEE<br>O OOOOOOCOOROROOROOOC E=<br>so) OOOO<br>aw DI7 DI6 DIS Di4 DIZ DI2 DI1 DIO DI15DI14DI13D112D111 DI1ODIE._DI8<br>AD6 ADS AD4 AD3 AD2 AD1 ADO<br><!-- End of picture text -->



<!-- Start of picture text -->
CS \ /<br>AV A VAUAUAUAUAUAUAUAUAUAUAUAUAEEE<br>Sn CAV<br>O OOOOOOOOOROO—=<br>splio (IO<br>R W DO7 DO6 DOS DO4 DO3 DO2 DO1 DOO<br>AD6 AD5 AD4 AD3 AD2 AD1 ADO<br><!-- End of picture text -->



<!-- Start of picture text -->
n n<br>LI<br>“S $boisa P NC<br>S SCX s | ny<br>GND orVDDIO CL] VIEW [] INT2 > Vdd<br>INT1 VDD<br>[| C1<br><<br>S<br>iS)5 Qao 29 ~TGND _ 100 nF<br>oo | Vdd _ lO =<br>100 nF 12C configuration<br>GND Rpu= 10kOhm<br>SCL<br>SDA<br>Pull-up to be added<br><!-- End of picture text -->



<!-- Start of picture text -->
12C configuration<br>Rpu= 10kOhm<br>SCL<br>SDA<br>Pull-up to be added<br><!-- End of picture text -->



<!-- Start of picture text -->
n n<br>y<br>."<br>L<br>an SDO/SAO Ne<br>ison TOP, Ey nc<br>~ Co EW h e<br>~ = MSCL MDRDY/INT2 ~ y "<br>INT1 [3] VDD<br>[| C1<br>S 9 9 ~T _ 100 nF<br>Q uD GND<br>fe)<br>co | Jad _ 10 Vdd_lO<br>100 nF 12C configuration<br>GND Rpu= 10kOhm<br>SCL<br>SDA<br>Pull-up to be added<br><!-- End of picture text -->



<!-- Start of picture text -->
12C configuration<br>Rpu= 10kOhm<br>SCL<br>SDA<br>Pull-up to be added<br><!-- End of picture text -->



<!-- Start of picture text -->
n oO z<br>Sy 5<br>LU<br>sao. NC<br>spyspo |Y VIEWTOP [I ~<q ocs<br>spc | Cy} it. vad<br>INT1 [4] VDD<br>[ C1<br>S 9 9 T1 1 00 nF<br>2 uD GND<br>fe)<br>C2 | Vad _ lO Vdd_lO<br>100 nF 12C configuration<br>GND Rpu= 10kOhm<br>SCL<br>SDA<br>Pull-up to be added<br><!-- End of picture text -->



<!-- Start of picture text -->
12C configuration<br>Rpu= 10kOhm<br>SCL<br>SDA<br>Pull-up to be added<br><!-- End of picture text -->

**LSM6DS3** 

**Register mapping** 

### **8 Register mapping** 

The table given below provides a list of the 8/16 bit registers embedded in the device and the corresponding addresses. 

**Table 16. Registers address map** 

|**Name**|**Type**|**Regist**|**er address**|**Default**|**Comment**|
|---|---|---|---|---|---|
|||**Hex**|**Binary**|||
|RESERVED|r/w|00|00000000|00000000|Reserved|
|FUNC_CFG_ACCESS|r/w|01|00000001|00000000|Embedded<br>functions<br>configuration<br>register|
|RESERVED|r/w|02|00000010|-|Reserved|
|RESERVED|r/w|03|00000011|-|Reserved|
|SENSOR_SYNC_TIME_<br>FRAME|r/w|04|00000100|00000000|Sensor sync<br>configuration<br>register|
|RESERVED|r/w|05|00000101|-|Reserved|
|FIFO_CTRL1|r/w|06|00000110|00000000||
|FIFO_CTRL2|r/w|07|00000111|00000000|FIFO|
|FIFO_CTRL3|r/w|08|00001000|00000000|configuration<br>|
|FIFO_CTRL4|r/w|09|00001001|00000000|registers|
|FIFO_CTRL5|r/w|0A|00001010|00000000||
|ORIENT_CFG_G|r/w|0B|00001011|00000000||
|RESERVED|r/w|0C|00001100|-|Reserved|
|INT1_CTRL|r/w|0D|00001101|00000000|INT1 pin control|
|INT2_CTRL|r/w|0E|00001110|00000000|INT2 pin control|
|WHO_AM_I|r|0F|00001111|01101001|Who I am ID|
|CTRL1_XL|r/w|10|00010000|00000000||
|CTRL2_G|r/w|11|00010001|00000000||
|CTRL3_C|r/w|12|00010010|00000100||
|CTRL4_C|r/w|13|00010011|00000000|Alt|
|CTRL5_C|r/w|14|00010100|00000000|cceeromeer<br>and gyroscope|
|CTRL6_C|r/w|15|00010101|00000000|<br>control<br>|
|CTRL7_G|r/w|16|00010110|00000000|registers|
|CTRL8_XL|r/w|17|0001 0111|00000000||
|CTRL9_XL|r/w|18|00011000|00111000||
|CTRL10_C|r/w|19|00011001|00111000||



41/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register mapping** 

**Table 16. Registers address map (continued)** 

|||**Regist**|**er address**|||
|---|---|---|---|---|---|
|**Name**|**Type**|||**Default**|**Comment**|
|||**Hex**|**Binary**|||
|MASTER_CONFIG|r/w|1A|00011010|00000000|I<sup>2</sup>C master<br>configuration<br>register|
|WAKE_UP_SRC|r|1B|00011011|output||
|TAP_SRC|r|1C|00011100|output|Interrupts<br>registers|
|D6D_SRC|r|1D|00011101|output||
|STATUS_REG|r|1E|00011110|output|Status data<br>register|
|RESERVED|r|1F|00011111|-|Reserved|
|OUT_TEMP_L|r|20|00100000|output|Temperature<br>|
|OUT_TEMP_H|r|21|00100001|output|output data<br>register|
|OUTX_L_G|r|22|00100010|output||
|OUTX_H_G|r|23|00100011|output||
|OUTY_L_G|r|24|00100100|output|Gyroscope|
|OUTY_H_G|r|25|00100101|output|output register|
|OUTZ_L_G|r|26|00100110|output||
|OUTZ_H_G|r|27|00100111|output||
|OUTX_L_XL|r|28|00101000|output||
|OUTX_H_XL|r|29|00101001|output||
|OUTY_L_XL|r|2A|00101010|output|Accelerometer|
|OUTY_H_XL|r|2B|00101011|output|output register|
|OUTZ_L_XL|r|2C|00101100|output||
|OUTZ_H_XL|r|2D|00101101|output||



42/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register mapping** 

**Table 16. Registers address map (continued)** 

|**Name**|**Type**|**Registe**|**r address**|**Default**|**Comment**|
|---|---|---|---|---|---|
|||**Hex**|**Binary**|||
|SENSORHUB1_REG|r|2E|00101110|output||
|SENSORHUB2_REG|r|2F|00101111|output||
|SENSORHUB3_REG|r|30|00110000|output||
|SENSORHUB4_REG|r|31|00110001|output||
|SENSORHUB5_REG|r|32|00110010|output||
|SENSORHUB6_REG|r|33|00110011|output|Sensor hub|
|SENSORHUB7_REG|r|34|00110100|output|output registers|
|SENSORHUB8_REG|r|35|00110101|output||
|SENSORHUB9_REG|r|36|00110110|output||
|SENSORHUB10_REG|r|37|00110111|output||
|SENSORHUB11_REG|r|38|00111000|output||
|SENSORHUB12_REG|r|39|00111001|output||
|FIFO_STATUS1|r|3A|00111010|output||
|FIFO_STATUS2|r|3B|00111011|output|FIFO status|
|FIFO_STATUS3|r|3C|00111100|output|registers|
|FIFO_STATUS4|r|3D|00111101|output||
|FIFO_DATA_OUT_L|r|3E|00111110|output|FIFO data|
|FIFO_DATA_OUT_H|r|3F|00111111|output|output registers|
|TIMESTAMP0_REG|r|40|01000000|output||
|TIMESTAMP1_REG|r|41|01000001|output|Timestamp<br>output registers|
|TIMESTAMP2_REG|r/w|42|01000010|output||
|RESERVED||43-48||--|Reserved|
|STEP_TIMESTAMP_L|r|49|0100 1001|output|Step counter<br>|
|STEP_TIMESTAMP_H|r|4A|0100 1010|output|timestamp<br>registers|
|STEP_COUNTER_L|r|4B|01001011|output|Step counter|
|STEP_COUNTER_H|r|4C|01001100|output|<br>output registers|
|SENSORHUB13_REG|r|4D|01001101|output||
|SENSORHUB14_REG|r|4E|01001110|output||
|SENSORHUB15_REG|r|4F|01001111|output|Sensor hub|
|SENSORHUB16_REG|r|50|01010000|output|output registers|
|SENSORHUB17_REG|r|51|01010001|output||
|SENSORHUB18_REG|r|52|01010010|output||
|FUNC_SRC|r|53|01010011|output|Interrupt<br>register|



43/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register mapping** 

**Table 16. Registers address map (continued)** 

|||**Registe**|**r address**|||
|---|---|---|---|---|---|
|**Name**|**Type**|||**Default**|**Comment**|
|||**Hex**|**Binary**|||
|RESERVED||54-57||--|Reserved|
|TAP_CFG|r/w|58|01011000|00000000||
|TAP_THS_6D|r/w|59|01011001|00000000||
|INT_DUR2|r/w|5A|01011010|00000000||
|WAKE_UP_THS|r/w|5B|01011011|00000000|Interrupt|
|WAKE_UP_DUR|r/w|5C|01011100|00000000|registers|
|FREE_FALL|r/w|5D|01011101|00000000||
|MD1_CFG|r/w|5E|01011110|00000000||
|MD2_CFG|r/w|5F|01011111|00000000||
|RESERVED||60-65||-|Reserved|
|OUT_MAG_RAW_X_L|r|66|0110 0110|output||
|OUT_MAG_RAW_X_H|r|67|0110 0111|output|Etl|
|OUT_MAG_RAW_Y_L|r|68|0110 1000|output|xerna<br>magnetometer|
|OUT_MAG_RAW_Y_H|r|69|0110 1001|output|raw data output<br>reisters|
|OUT_MAG_RAW_Z_L|r|6A|0110 1010|output|g|
|OUT_MAG_RAW_X_H|r|6B|0110 1011|output||



Registers marked as _Reserved_ must not be changed. Writing to those registers may cause permanent damage to the device. 

The content of the registers that are loaded at boot should not be changed. They contain the factory calibration values. Their content is automatically restored when the device is powered up. 

44/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

### **9 Register description** 

The device contains a set of registers which are used to control its behavior and to retrieve linear acceleration, angular rate and temperature data. The register addresses, made up of 7 bits, are used to identify them and to write the data through the serial interface. 

#### **9.1 FUNC_CFG_ACCESS (01h)** 

Enable embedded functions register (r/w). 

###### **Table 17. FUNC_CFG_ACCESS register** 

|FUNC_CFG_EN|0<sup>(1)</sup>|0<sup>(1)</sup>|0<sup>(1)</sup>|0<sup>(1)</sup>|0<sup>(1)</sup>|0<sup>(1)</sup><br>0<sup>(1)</sup>|
|---|---|---|---|---|---|---|



1. This bit must be set to ‘0’ for the correct operation of the device. 

###### **Table 18. FUNC_CFG_ACCESS register description** 

Enable access to the embedded functions configuration registers<sup>(1)</sup> from address 02h to 32h. Default value: 0. FUNC_CFG_EN (0: disable access to embedded functions configuration registers; 1: enable access to embedded functions configuration registers) 

1. The embedded functions configuration registers details are available in _10: Embedded functions register mapping_ and _11: Embedded functions registers description_ . 

#### **9.2 SENSOR_SYNC_TIME_FRAME (04h)** 

Sensor synchronization time frame register (r/w). 

**Table 19. SENSOR_SYNC_TIME_FRAME register** 

|TPH_7|TPH_6|TPH_5<br>TPH_4|TPH_3|TPH_2<br>TPH_1<br>TPH_0|
|---|---|---|---|---|
|TPH_ [7:0]|**Table 20.**<br>Senso<br>Unsign<br>Defaul|**SENSOR_SYNC_TIM**<br>r synchronization time fra<br>ed 8-bit.<br>t value: 0000 0000|**E_FRAME**<br>me with the|**register description**<br>step of 500 ms and full range of 5 s.|



#### **9.3 FIFO_CTRL1 (06h)** 

FIFO control register (r/w). 

**Table 21. FIFO_CTRL1 register** 

|FTH_7|FTH_6<br>FTH_5<br>FTH_4<br>FTH_3<br>FTH_2<br>FTH_1|FTH_0|
|---|---|---|
||**Table 22. FIFO_CTRL1 register description**<br>FIFO threshold level setting<sup>(1)</sup>. Default value: 0000 0000.||
|FTH_[7:0]|Watermark flag rises when the number of bytes written to FIFO after the n<br>greater than or equal to the threshold level.|ext write is|
||Minimum resolution for the FIFO is 1 LSB = 2 bytes (1 word) in FIFO||



1. For a complete watermark threshold configuration, consider FTH_[11:8] in _FIFO_CTRL2 (07h)_ . 

45/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.4 FIFO_CTRL2 (07h)** 

FIFO control register (r/w). 

###### **Table 23. FIFO_CTRL2 register** 

|TIMER_PEDO<br>_FIFO_EN|TIMER_PEDO<br>_FIFO_DRDY|0<sup>(1)</sup>|0<sup>(1)</sup>|FTH_11|FTH10|FTH_9|FTH_8|
|---|---|---|---|---|---|---|---|



1. This bit must be set to ‘0’ for the correct operation of the device. 

###### **Table 24. FIFO_CTRL2 register description** 

|TIMER_PEDO<br>_FIFO_EN|Enable pedometer step counter and time stamp as 4<sup>th</sup>FIFO data set. Default: 0<br>(0: disable step counter and time stamp data as 4<sup>th</sup>FIFO data set;<br>1: enable step counter and time stamp data as 4<sup>th</sup>FIFO data set)|
|---|---|
|TIMER_PEDO<br>_FIFO_DRDY|FIFO write mode<sup>(1)</sup>. Default: 0<br>(0: enable write in FIFO based on XL/Gyro data-ready;<br>1: enable write in FIFO at every step detected by step counter.)|
|FTH_[11:8]|FIFO threshold level setting<sup>(2)</sup>. Default value: 0000<br>Watermark flag rises when the number of bytes written to FIFO after the next<br>write is greater than or equal to the threshold level.<br>Minimum resolution for the FIFO is 1LSB = 2 bytes (1 word) in FIFO|



1. This bit is effective if the DATA_VALID_SEL_FIFO bit of the MASTER_CONFIG (1Ah) register is set to 0. 

2. For a complete watermark threshold configuration, consider FTH_[11:8] in _FIFO_CTRL1 (06h)_ 

#### **9.5 FIFO_CTRL3 (08h)** 

FIFO control register (r/w). 

**Table 25. FIFO_CTRL3 register** 

|0<sup>(1)</sup>|0<sup>(1)</sup>|DEC_FIFO|DEC_FIFO|DEC_FIFO|DEC_FIFO|DEC_FIFO|DEC_FIFO|
|---|---|---|---|---|---|---|---|
|||_GYRO2|_GYRO1|_GYRO0|_XL2|_XL1|_XL0|



1. This bit must be set to ‘0’ for the correct operation of the device. 

###### **Table 26. FIFO_CTRL3 register description** 

|DEC_FIFO_GYRO [2:0]|Gyro FIFO (first data set) decimation setting. Default: 000<br>For the configuration setting, refer to_Table 27_.|
|---|---|
|DEC_FIFO_XL [2:0]|Accelerometer FIFO (second data set) decimation setting. Default: 000<br>For the configuration setting, refer to_Table 28_.|



46/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

**Table 27. Gyro FIFO decimation setting** 

|**DEC_FIFO_GYRO [2:0]**|**Configuration**|
|---|---|
|000|Gyro sensor not in FIFO|
|001|No decimation|
|010|Decimation with factor 2|
|011|Decimation with factor 3|
|100|Decimation with factor 4|
|101|Decimation with factor 8|
|110|Decimation with factor 16|
|111|Decimation with factor 32|



**Table 28. Accelerometer FIFO decimation setting** 

|**DEC_FIFO_XL [2:0]**|**Configuration**|
|---|---|
|000|Accelerometer sensor not in FIFO|
|001|No decimation|
|010|Decimation with factor 2|
|011|Decimation with factor 3|
|100|Decimation with factor 4|
|101|Decimation with factor 8|
|110|Decimation with factor 16|
|111|Decimation with factor 32|



#### **9.6 FIFO_CTRL4 (09h)** 

FIFO control register (r/w). 

**Table 29. FIFO_CTRL4 register** 

|0<sup>(1)</sup><br>ONLY_HIGH<br>_DATA|DEC_DS4<br>_FIFO2<br>DEC_DS4<br>_FIFO1<br>DEC_DS4<br>_FIFO0<br>DEC_DS3<br>_FIFO2<br>DEC_DS3<br>_FIFO1|DEC_DS3<br>_FIFO0|
|---|---|---|
|1.<br>This bit must be set to<br>ONLY_HIGH_DATA|**Table 30. FIFO_CTRL4 register description**<br>‘0’ for the correct operation of the device.<br>8-bit data storage in FIFO. Default: 0<br>(0: disable MSByte only memorization in FIFO for XL and Gy<br>1: enable MSByte only memorization in FIFO for XL and Gyr|ro;<br>o in FIFO)|
|DEC_DS4_FIFO[2:0]|Fourth FIFO data set decimation setting. Default: 000<br>For the configuration setting, refer to_Table 31_.||
|DEC_DS3_FIFO[2:0]|Third FIFO data set decimation setting. Default: 000<br>For the configuration setting, refer to_Table 32_.||



47/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

**Table 31. Fourth FIFO data set decimation setting** 

|**DEC_DS4_FIFO[2:0]**|**Configuration**|
|---|---|
|000|Fourth FIFO data set not in FIFO|
|001|No decimation|
|010|Decimation with factor 2|
|011|Decimation with factor 3|
|100|Decimation with factor 4|
|101|Decimation with factor 8|
|110|Decimation with factor 16|
|111|Decimation with factor 32|



**Table 32. Third FIFO data set decimation setting** 

|**DEC_DS3_FIFO[2:0]**|**Configuration**|
|---|---|
|000|Third FIFO data set not in FIFO|
|001|No decimation|
|010|Decimation with factor 2|
|011|Decimation with factor 3|
|100|Decimation with factor 4|
|101|Decimation with factor 8|
|110|Decimation with factor 16|
|111|Decimation with factor 32|



#### **9.7 FIFO_CTRL5 (0Ah)** 

FIFO control register (r/w). 

**Table 33. FIFO_CTRL5 register** 

|0<sup>(1)</sup>|ODR_<br>FIFO_3|ODR_<br>FIFO_2|ODR_<br>FIFO_1|ODR_<br>FIFO_0|FIFO_<br>MODE_2|FIFO_<br>MODE_1|FIFO_<br>MODE_0|
|---|---|---|---|---|---|---|---|
|1.<br>This bi|t must be set t|o ‘0’ for the c|orrect operati|on of the devi|ce.|||



###### **Table 34. FIFO_CTRL5 register description** 

|ODR_FIFO_[3:0]|FIFO ODR selection, setting FIFO_MODE also. Default: 0000<br>For the configuration setting, refer to_Table 35_|
|---|---|
|FIFO_MODE_[2:0]|FIFO mode selection bits, setting ODR_FIFO also. Default value: 000<br>For the configuration setting refer to_Table 36_|



48/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

**Table 35. FIFO ODR selection** 

|**ODR_FIFO_[3:0]**|**Configuration**<sup>**(1)**</sup>|
|---|---|
|0000|FIFO disabled|
|0001|FIFO ODR is set to 13 Hz|
|0010|FIFO ODR is set to 26 Hz|
|0011|FIFO ODR is set to 52 Hz|
|0100|FIFO ODR is set to 104 Hz|
|0101|FIFO ODR is set to 208 Hz|
|0110|FIFO ODR is set to 416 Hz|
|0111|FIFO ODR is set to 833 Hz|
|1000|FIFO ODR is set to 1.66 kHz|
|1001|FIFO ODR is set to 3.33 kHz|
|1010|FIFO ODR is set to 6.66 kHz|



1. If the device is working at an ODR slower than the one selected, FIFO ODR is limited to that ODR value. Moreover, these bits are effective if both the DATA_VALID_SEL FIFO bit of MASTER_CONFIG (1Ah) and the TIMER_PEDO_FIFO_DRDY bit of FIFO_CTRL2 (07h) are set to 0. 

**Table 36. FIFO mode selection** 

|**FIFO_MODE_[2:0]**|**Configuration mode**|
|---|---|
|000|Bypass mode. FIFO disabled.|
|001|FIFO mode. Stops collecting data when FIFO is full.|
|010|Reserved|
|011|Continuous mode until trigger is deasserted, then FIFO mode.|
|100|Bypass mode until trigger is deasserted, then Continuous mode.|
|101|Reserved|
|110|Continuous mode. If the FIFO is full, the new sample overwrites the older one.|
|111|Reserved|



#### **9.8 ORIENT_CFG_G (0Bh)** 

Angular rate sensor sign and orientation register (r/w). 

###### **Table 37. ORIENT_CFG_G register** 

|0<sup>(1)</sup>|0<sup>(1)</sup>|SignX_G|SignY_G|SignZ_G|Orient_2|Orient_1|Orient_0|
|---|---|---|---|---|---|---|---|
|1.<br>This bit mu|st be set to ‘0|’ for the correc|t operation of|the device.||||



49/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

**Table 38. ORIENT_CFG_G register description** 

|SignX_G|Pitch axis (X) angular rate sign. Default value: 0<br>(0: positive sign; 1: negative sign)|
|---|---|
|SignY_G|Roll axis (Y) angular rate sign. Default value: 0<br>(0: positive sign; 1: negative sign)|
|SignZ_G|Yaw axis (Z) angular rate sign. Default value: 0<br>(0: positive sign; 1: negative sign)|
|Orient [2:0]|Directional user-orientation selection. Default value: 000<br>For the configuration setting, refer to_Table 39_.|



**Table 39. Settings for orientation of axes** 

|**Orient [2:0]**|**000**|**001**|**010**|**011**|**100**|**101**|
|---|---|---|---|---|---|---|
|Pitch|X|X|Y|Y|Z|Z|
|Roll|Y|Z|X|Z|X|Y|
|Yaw|Z|Y|Z|X|Y|X|



#### **9.9 INT1_CTRL (0Dh)** 

INT1 pad control register (r/w). 

Each bit in this register enables a signal to be carried through INT1. The pad’s output will supply the OR combination of the selected signals. 

**Table 40. INT1_CTRL register** 

|INT1_<br>STEP_<br>DETECTOR<br>INT1<br>_M|**Table 41. INT1_CTRL register description**<br>_SIGN<br>OT<br>INT1_FULL<br>_FLAG<br>INT1_<br>FIFO_OVR<br>INT1_<br>FTH<br>INT1_<br>BOOT<br>INT1_<br>DRDY_G|INT1_<br>DRDY_XL|
|---|---|---|
|INT1_ STEP_<br>DETECTOR|Pedometer step recognition interrupt enable on INT1 pad. Default<br>(0: disabled; 1: enabled)|value: 0|
|INT1_SIGN_MOT|Significant motion interrupt enable on INT1 pad. Default value: 0<br>(0: disabled; 1: enabled)||
|INT1_FULL_FLAG|FIFO full flag interrupt enable on INT1 pad. Default value: 0<br>(0: disabled; 1: enabled)||
|INT1_FIFO_OVR|FIFO overrun interrupt on INT1 pad. Default value: 0<br>(0: disabled; 1: enabled)||
|INT1_FTH|FIFO threshold interrupt on INT1 pad. Default value: 0<br>(0: disabled; 1: enabled)||
|INT1_ BOOT|Boot status available on INT1 pad. Default value: 0<br>(0: disabled; 1: enabled)||
|INT1_DRDY_G|Gyroscope Data Ready on INT1 pad. Default value: 0<br>(0: disabled; 1: enabled)||
|INT1_DRDY_XL|Accelerometer Data Ready on INT1 pad. Default value: 0<br>(0: disabled; 1: enabled)||



50/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.10 INT2_CTRL (0Eh)** 

INT2 pad control register (r/w). 

Each bit in this register enables a signal to be carried through INT2. The pad’s output will supply the OR combination of the selected signals. 

**Table 42. INT2_CTRL register** 

|INT2_STEP<br>_DELTA<br>INT2_S<br>COUNT|**Table 43. INT2_CTRL register description**<br>TEP_<br>_OV<br>INT2_<br>FULL_FLAG<br>INT2_<br>FIFO_OVR<br>INT2_<br>FTH<br>INT2_<br>DRDY<br>_TEMP<br>|INT2_<br>DRDY_G<br>|INT2_<br>DRDY_XL<br>|
|---|---|---|---|
|INT2_STEP_DELTA|Pedometer step recognition interrupt on delta time<sup>(</sup><br>Default value: 0<br>(0: disabled; 1: enabled)|<sup>)</sup>enable on|INT2 pad.|
|INT2_STEP_COUNT<br>_OV|Step counter overflow interrupt enable on INT2 pad<br>(0: disabled; 1: enabled)|. Default val|ue: 0|
|INT2_ FULL_FLAG|FIFO full flag interrupt enable on INT2 pad. Default<br>(0: disabled; 1: enabled)|value: 0||
|INT2_FIFO_OVR|FIFO overrun interrupt on INT2 pad. Default value:<br>(0: disabled; 1: enabled)|0||
|INT2_FTH|FIFO threshold interrupt on INT2 pad. Default value<br>(0: disabled; 1: enabled)|: 0||
|INT2_DRDY_TEMP|Temperature Data Ready in INT2 pad. Default valu<br>(0: disabled; 1: enabled)|e: 0||
|INT2_DRDY_G|Gyroscope Data Ready on INT2 pad. Default value<br>(0: disabled; 1: enabled)|: 0||
|INT2_DRDY_XL|Accelerometer Data Ready on INT2 pad. Default va<br>(0: disabled; 1: enabled)|lue: 0||



1. Delta time value is defined in register STEP_COUNT_DELTA (15h). 

#### **9.11 WHO_AM_I (0Fh)** 

Who_AM_I register (r). This register is a read-only register. Its value is fixed at 69h. 

**Table 44. WHO_AM_I register** 

|0<br>1<br>1<br>0<br>1<br>0<br>0<br>1|
|---|



#### **9.12 CTRL1_XL (10h)** 

Linear acceleration sensor control register 1 (r/w). 

###### **Table 45. CTRL1_XL register** 

|ODR_XL3|ODR_XL2|ODR_XL1|ODR_XL0|FS_XL1|FS_XL0|BW_XL1|BW_XL0|
|---|---|---|---|---|---|---|---|



51/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

**Table 46. CTRL1_XL register description** 

|ODR_XL [3:0]|Output data rate and power mode selection.Default value: 0000 (see_Table 47_).|
|---|---|
|FS_XL [1:0]|Accelerometer full-scale selection. Default value: 00.<br>(00: ±2_g_; 01: ±16_g_; 10: ±4_g_; 11: ±8_g_)|
|BW_XL [1:0]|Anti-aliasing filter bandwidth selection. Default value: 00<br>(00: 400 Hz; 01: 200 Hz; 10: 100 Hz; 11: 50 Hz)|



**Table 47. Accelerometer ODR register setting** 

|**ODR_**<br>**XL3**|**ODR_**<br>**XL2**|**ODR_**<br>**XL1**|**ODR_**<br>**XL0**|**ODR selection [Hz] when**<br>**XL_HM_MODE = 1**|**ODR selection [Hz] when**<br>**XL_HM_MODE = 0**|
|---|---|---|---|---|---|
|0|0|0|0|Power-down|Power-down|
|0|0|0|1|13 Hz (low power)|13 Hz (high performance)|
|0|0|1|0|26 Hz (low power)|26 Hz (high performance)|
|0|0|1|1|52 Hz (low power)|52 Hz (high performance)|
|0|1|0|0|104 Hz (normal mode)|104 Hz (high performance)|
|0|1|0|1|208 Hz (normal mode)|208 Hz (high performance)|
|0|1|1|0|416 Hz (high performance)|416 Hz (high performance)|
|0|1|1|1|833 Hz (high performance)|833 Hz (high performance)|
|1|0|0|0|1.66 kHz (high performance)|1.66 kHz (high performance)|
|1|0|0|1|3.33 kHz (high performance)|3.33 kHz (high performance)|
|1|0|1|0|6.66 kHz (high performance)|6.66 kHz (high performance)|



**Table 48. BW and ODR (high-performance mode)** 

|**ODR**<sup>**(1)**</sup>|**Analog filter BW (**|**XL_HM_MODE = 0)**|
|---|---|---|
||**XL_BW_SCAL_ODR = 0**|**XL_BW_SCAL_ODR = 1**|
|6.66 - 3.33 kHz|Filter not used||
|1.66 kHz|400 Hz||
|833 Hz|400 Hz|Bandwidth is determined by<br>|
|416 Hz|200 Hz|setting BW_XL[1:0] in<br>_CTRL1_XL (10h)_|
|208 Hz|100 Hz||
|104 - 13 Hz|50 Hz||



1. Filter not used when accelerometer is in normal and low-power modes. 

52/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.13 CTRL2_G (11h)** 

Angular rate sensor control register 2 (r/w). 

###### **Table 49. CTRL2_G register** 

|ODR_G3|ODR_G2|ODR_G1|ODR_G0|FS_G1|FS_G0|FS_125|0<sup>(1)</sup>|
|---|---|---|---|---|---|---|---|



1. This bit must be set to ‘0’ for the correct operation of the device. 

###### **Table 50. CTRL2_G register description** 

|ODR_G [3:0]|Gyroscope output data rate selection.Default value: 0000<br>(Refer to_Table 49_)|
|---|---|
|FS_G [1:0]|Gyroscope full-scale selection. Default value: 00<br>(00: 245 dps; 01: 500 dps; 10: 1000 dps; 11: 2000 dps)|
|FS_125|Gyroscope full-scale at 125 dps. Default value: 0<br>(0: disabled; 1: enabled)|



**Table 51. Gyroscope ODR configuration setting** 

|**ODR_G3**|**ODR_G2**|**ODR_G1**|**ODR_G0**|**ODR [Hz] when**<br>**G_HM_MODE = 1**|**ODR [Hz] when**<br>**G_HM_MODE = 0**|
|---|---|---|---|---|---|
|0|0|0|0|Power down|Power down|
|0|0|0|1|13 Hz (low power)|13 Hz (high performance)|
|0|0|1|0|26 Hz (low power)|26 Hz (high performance)|
|0|0|1|1|52 Hz (low power)|52 Hz (high performance)|
|0|1|0|0|104 Hz (normal mode)|104 Hz (high performance)|
|0|1|0|1|208 Hz (normal mode)|208 Hz (high performance)|
|0|1|1|0|416 Hz (high performance)|416 Hz (high performance)|
|0|1|1|1|833 Hz (high performance)|833 Hz (high performance)|
|1|0|0|0|1.66 kHz (high performance)|1.66 kHz (high performance)|



53/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.14 CTRL3_C (12h)** 

Control register 3 (r/w). 

||**Table 52. CTRL3_C register**|
|---|---|
|BOOT|BDU<br>H_LACTIVE<br>PP_OD<br>SIM<br>IF_INC<br>BLE<br>SW_RESET<br>**Table 53. CTRL3_C register description**|
|BOOT|Reboot memory content. Default value: 0<br>(0: normal mode; 1: reboot memory content<sup>(1)</sup>)|
|BDU|Block Data Update. Default value: 0<br>(0: continuous update; 1: output registers not updated until MSB and LSB have<br>been read)|
|H_LACTIVE|Interrupt activation level. Default value: 0<br>(0: interrupt output pads active high; 1: interrupt output pads active low)|
|PP_OD|Push-pull/open-drain selection on INT1 and INT2 pads. Default value: 0<br>(0: push-pull mode; 1: open-drain mode)|
|SIM|SPI Serial Interface Mode selection. Default value: 0<br>(0: 4-wire interface; 1: 3-wire interface).|
|IF_INC|Register address automatically incremented during a multiple byte access with a<br>serial interface (I<sup>2</sup>C or SPI). Default value: 1<br>(0: disabled; 1: enabled)|
|BLE|Big/Little Endian Data selection. Default value 0<br>(0: data LSB @ lower address; 1: data MSB @ lower address)|
|SW_RESET|<br>Software reset. Default value: 0<br>(0: normal mode; 1: reset device)<br>This bit is cleared by hardware after next flash boot.|



1. Boot request is executed as soon as internal oscillator is turned on. It is possible to set bit while in powerdown mode, in this case it will be served at the next normal mode or sleep mode. 

54/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.15 CTRL4_C (13h)** 

Control register 4 (r/w). 

**Table 54. CTRL4_C register** 

|XL_BW_<br>SCAL_ODR<br>SLE|EP_G<br>INT2_on_<br>INT1<br>FIFO_<br>TEMP_EN<br>DRDY_<br>MASK<br>I2C_disable<br>MODE3_<br>EN|STOP_ON<br>_FTH|
|---|---|---|
|XL_BW_<br>SCAL_ODR|**Table 55. CTRL4_C register description**<br>Accelerometer bandwidth selection. Default value: 0<br>(0<sup>(1)</sup>: bandwidth determined by ODR selection, refer to_Table 48_;<br>1<sup>(2)</sup>: bandwidth determined by setting BW_XL[1:0] in_CTRL1_XL (10_|_h)_register.)|
|SLEEP_G|Gyroscope sleep mode enable. Default value: 0<br>(0: disabled; 1: enabled)||
|INT2_on_INT1|All interrupt signals available on INT1 pad enable. Default value: 0<br>(0: interrupt signals divided between INT1 and INT2 pads;<br>1: all interrupt signals in logic or on INT1 pad)||
|FIFO_TEMP_EN|Enable temperature data as 4<sup>th</sup>FIFO data set<sup>(3)</sup>. Default: 0<br>(0: disable temperature data as 4<sup>th</sup>FIFO data set;<br>1: enable temperature data as 4<sup>th</sup>FIFO data set)||
|DRDY_MASK|Configuration 1<sup>(4)</sup>data available enable bit. Default value: 0<br>(0: DA timer disabled; 1: DA timer enabled)||
|I2C_disable|Disable I<sup>2</sup>C interface. Default value: 0<br>(0: both I<sup>2</sup>C and SPI enabled; 1: I<sup>2</sup>C disabled, SPI only)||
|MODE3_EN|Enable auxiliary SPI interface (Mode 3, refer to_Table 2_). Default valu<br>(0: auxiliary SPI disabled; 1: auxiliary SPI enabled<sup>(5)</sup>)|e: 0|
|STOP_ON_FTH|Enable FIFO threshold level use. Default value: 0.<br>(0: FIFO depth is not limited; 1: FIFO depth is limited to threshold lev|el)|
|1.<br>Filter used in high|-performance mode only with ODR less than 3.33 kHz.||



2. Filter used in high-performance mode only. 

3. This bit is effective if the TIMER_PEDO_FIFO_EN bit of FIFO_CTRL2 register is set to 0. 

4. In configuration 1, switching to combo mode, data are collected in FIFO only when both accelerometer and gyroscope are set. Switching to accelerometer only, data are collected in FIFO after filter setting. 

5. Conditioned pads are: SDx, SCx, OCS 

#### **9.16 CTRL5_C (14h)** 

Control register 5 (r/w). 

###### **Table 56. CTRL5_C register** 

|ROUNDING2 ROUNDING1 ROUNDING0<br>0<sup>(1)</sup>|ST1_G|ST0_G|ST1_XL|ST0_XL|
|---|---|---|---|---|



1. This bit must be set to ‘0’ for the correct operation of the device 

55/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

**Table 57. CTRL5_C register description** 

|ROUNDING[2:0]|Circular burst-mode (rounding) read from output registers. Default: 000<br>(000: no rounding; Others: refer to_Table 58_)|
|---|---|
|ST_G [1:0]|Angular rate sensor self-test enable. Default value: 00<br>(00: Self-test disabled; Other: refer to_Table 59_)|
|ST_XL [1:0]|Linear acceleration sensor self-test enable. Default value: 00<br>(00: Self-test disabled; Other: refer to_Table 60_)|



**Table 58. Output registers rounding pattern** 

|**ROUNDING[2:0]**|**Rounding pattern**|
|---|---|
|000|No rounding|
|001|Accelerometer only|
|010|Gyroscope only|
|011|Gyroscope + accelerometer|
|100|Registers from_SENSORHUB1_REG (2Eh)_to_SENSORHUB6_REG (33h)_only|
|101|Accelerometer + registers from_SENSORHUB1_REG (2Eh)_to<br>_SENSORHUB6_REG (33h)_|
|110|Gyroscope + accelerometer + registers from_SENSORHUB1_REG (2Eh)_to<br>_SENSORHUB6_REG (33h)_and registers from_SENSORHUB7_REG (34h)_to<br>_SENSORHUB12_REG(39h)_|
|111|Gyroscope + accelerometer + registers from_SENSORHUB1_REG (2Eh)_to<br>_SENSORHUB6_REG (33h)_|



**Table 59. Angular rate sensor self-test mode selection** 

|**ST1_G**|**ST0_G**|**Self-test mode**|
|---|---|---|
|0|0|Normal mode|
|0|1|Positive sign self-test|
|1|0|Not allowed|
|1|1|Negative sign self-test|



**Table 60. Linear acceleration sensor self-test mode selection** 

|**ST1_XL**|**ST0_XL**|**Self-test mode**|
|---|---|---|
|0|0|Normal mode|
|0|1|Positive sign self-test|
|1|0|Negative sign self-test|
|1|1|Not allowed|



56/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.17 CTRL6_C (15h)** 

Angular rate sensor control register 6 (r/w). 

###### **Table 61. CTRL6_C register** 



<!-- Start of picture text -->
TRIG_EN LVLen LVL2_EN XL_HM_MODE 0 (1) 0 (1) 0 (1) 0 (1)<br>1. This bit must be set to ‘0’ for the correct operation of the device.<br><!-- End of picture text -->

###### **Table 62. CTRL6_C register description** 

|TRIG_EN|Gyroscope data edge-sensitive trigger enable. Default value: 0<br>(0: external trigger disabled; 1: external trigger enabled)|
|---|---|
|LVLen|Gyroscope data level-sensitive trigger enable. Default value: 0<br>(0: level-sensitive trigger disabled; 1: level sensitive trigger enabled)|
|LVL2_EN|Gyroscope level-sensitive latched enable. Default value: 0<br>(0: level-sensitive latched disabled; 1: level sensitive latched enabled)|
|XL_HM_MODE|High-performance operating mode disable for accelerometer<sup>(1)</sup>. Default value: 0<br>(0: high-performance operating mode enabled;<br>1: high-performance operating mode disabled)|



1. Normal and low-power mode depends on the ODR setting, for details refer to _Table 47_ . 

#### **9.18 CTRL7_G (16h)** 

Angular rate sensor control register 7 (r/w). 

###### **Table 63. CTRL7_G register** 

|G_HM_MO|DE<br>HP_G_<br>EN<br>HPCF_G1<br>HPCF_G0<br>HP_G_R<br>ST<br>ROUNDING<br>STATUS<br>0<sup>(1)</sup>|0<sup>(1)</sup>|
|---|---|---|
|1.<br>This bit mu|_<br>st be set to ‘0’ for the correct operation of the device.<br>**Table 64. CTRL7_G register description**||
|G_HM_MODE|High-performance operating mode disable for gyroscope<sup>(1)</sup>. Default: 0<br>(0: high-performance operating mode enabled;<br>1: high-performance operating mode disabled)||
|HP_G_EN|Gyroscope high-pass filter enable. Default value: 0<br>(0: HPF disabled; 1: HPF enabled)||
|HP_G_RST|Gyro digital HP filter reset. Default: 0<br>(0: gyro digital HP filter reset OFF; 1: gyro digital HP filter reset ON)||
|ROUNDING_<br>STATUS|Source register rounding function enable on_STATUS_REG (1Eh)_,_FUNC__<br>_(53h)_and_WAKE_UP_SRC (1Bh)_registers. Default value: 0<br>(0: disabled; 1: enabled)|_SRC_|
|HPCF_G[1:0]|Gyroscope high-pass filter cutoff frequency selection. Default value: 00.<br>Refer to_Table 65_.||



1. Normal and low-power mode depends on the ODR setting, for details refer to _Table 51_ . 

57/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

**Table 65. Gyroscope high-pass filter mode configuration** 

|**HPCF_G1**|**HPCF_G0**|**High-pass filter cutoff frequency**|
|---|---|---|
|0|0|0.0081 Hz|
|0|1|0.0324 Hz|
|1|0|2.07 Hz|
|1|1|16.32 Hz|



#### **9.19 CTRL8_XL (17h)** 

Linear acceleration sensor control register 8 (r/w). 

**Table 66. CTRL8_XL register** 

|LPF2_XL_|HPCF_|HPCF_|0<sup>(1)</sup>|0<sup>(1)</sup>|HP_SLOPE_X|0<sup>(1)</sup>|LOW_PASS|
|---|---|---|---|---|---|---|---|
|EN|XL1|XL0|||L_EN||_ON_6D|



1. This bit must be set to ‘0’ for the correct operation of the device. 

**Table 67. CTRL8_XL register description** 

|LPF2_XL_EN|Accelerometer low-pass filter LPF2 selection. Refer to_Figure 5_.|
|---|---|
|HPCF_XL[1:0]|Accelerometer slope filter and high-pass filter configuration and cutoff<br>setting. Refer to_Table 68_. It is also used to select the cutoff frequency of the<br>LPF2 filter, as shown in _Table 69_. This low-pass filter can also be used in the<br>6D/4D functionality by setting the LOW_PASS_ON_6D bit of _CTRL8_XL_<br>_(17h)_ to 1.|
|HP_SLOPE_XL_EN|Accelerometer slope filter / high-pass filter selection. Refer to_Figure 5_.|
|LOW_PASS_ON_6D|Low-pass filter on 6D function selection. Refer to_Figure 5_.|



**Table 68. Accelerometer slope and high-pass filter selection and cutoff frequency** 

|**HPCF_XL[1:0]**|**Applied filter**|**HP filter cutoff frequency [Hz]**|
|---|---|---|
|00|Slope|ODR_XL/4|
|01|High-pass|ODR_XL/100|
|10|High-pass|ODR_XL/9|
|11|High-pass|ODR_XL/400|



**Table 69. Accelerometer LPF2 cutoff frequency** 

|**HPCF_XL[1:0]**|**LPF2 digital filter cutoff frequency [Hz]**|
|---|---|
|00|ODR_XL/50|
|01|ODR_XL/100|
|10|ODR_XL/9|
|11|ODR_XL/400|



58/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.20 CTRL9_XL (18h)** 

Linear acceleration sensor control register 9 (r/w). 

###### **Table 70. CTRL9_XL register** 

|0<sup>(1)</sup>|0<sup>(1</sup><br>Zen_XL<br>Yen_XL<br>Xen_XL<br>SOFT_EN|0<sup>(1)</sup><br>0<sup>(1)</sup>|
|---|---|---|
|1.<br>This bit mu|st be set to ‘0’ for the correct operation of the device.<br>**Table 71. CTRL9_XL register description**||
|Zen_XL|Accelerometer Z-axis output enable. Default value: 1<br>(0: Z-axis output disabled; 1: Z-axis output enabled)||
|Yen_XL|Accelerometer Y-axis output enable. Default value: 1<br>(0: Y-axis output disabled; 1: Y-axis output enabled)||
|Xen_XL|Accelerometer X-axis output enable. Default value: 1<br>(0: X-axis output disabled; 1: X-axis output enabled)||
|SOFT_EN|Enable soft-iron correction algorithm for magnetometer<sup>(1)</sup>.<br>(0: soft-iron correction algorithm disabled;<br>1: soft-iron correction algorithm disabled)|Default value: 0|



1. This bit is effective if the IRON_EN bit of MASTER_CONFIG (1Ah) is set to 1. 

#### **9.21 CTRL10_C (19h)** 

Control register 10 (r/w). 

###### **Table 72. CTRL10_C register** 

|0<sup>(1)</sup><br>0<sup>(1)</sup>|Zen_G|Yen_G<br>Xen_G<br>FUNC_EN<br>PEDO_RST<br>STEP|SIGN_<br>MOTIONEN|
|---|---|---|---|
|.<br>This bit must be<br>Zen_G|set to ‘0’ for the c<br>**Table**<br>Gyroscope yaw<br>(0: Z-axis outp|orrect operation of the device.<br>_<br>**73. CTRL10_C register description**<br>axis (Z) output enable. Default value: 1<br>ut disabled; 1: Z-axis output enabled)|_|
|Yen_G|Gyroscope roll<br>(0: Y-axis outp|axis (Y) output enable. Default value: 1<br>ut disabled; 1: Y axis output enabled)||
|Xen_G|Gyroscope pitc<br>(0: X-axis outp|h axis (X) output enable. Default value: 1<br>ut disabled; 1: X-axis output enabled)||
|FUNC_EN|Enable embed<br>and ironing) an<br>value: 0<br>(0: disable func<br>1: enable funct|ded functionalities (pedometer, tilt, significant motion<br>d accelerometer HP and LPF2 filters (refer to_Figure_<br>tionalities of embedded functions and acceleromete<br>ionalities of embedded functions and accelerometer|, sensor hub<br>_5_). Default<br>r filters;<br>filters)|
|PEDO_RST_<br>STEP|Reset pedome<br>(0: disabled; 1:|ter step counter. Default value: 0<br>enabled)||
|SIGN_MOTION<br>_EN|Enable signific<br>(0: disabled; 1:|ant motion function. Default value: 0<br>enabled)||



1. This bit must be set to ‘0’ for the correct operation of the device. 

59/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.22 MASTER_CONFIG (1Ah)** 

Master configuration register (r/w). 

**Table 74. MASTER_CONFIG register** 

|DRDY_ON<br>_INT1|DATA_VALID<br>_SEL_FIFO|0<sup>(1)</sup>|START_<br>CONFIG|PULL_UP<br>_EN|PASS_<br>THROUGH<br>_MODE|IRON_EN|MASTER_<br>ON|
|---|---|---|---|---|---|---|---|



1. This bit must be set to ‘0’ for the correct operation of the device. 

**Table 75. MASTER_CONFIG register description** 

|DRDY_ON_<br>INT1|Manage the Master DRDY signal on INT1 pad. Default: 0<br>(0: disable Master DRDY on INT1; 1: enable Master DRDY on INT1)|
|---|---|
|DATA_VALID_<br>SEL_FIFO|Selection of FIFO data-valid signal. Default value: 0<br>(0: data-valid signal used to write data in FIFO is the XL/Gyro data-ready or step<br>detection<sup>(1)</sup>;<br>1: data-valid signal used to write data in FIFO is the sensor hub data-ready)|
|START_<br>CONFIG|Sensor Hub trigger signal selection. Default value: 0<br>(0: Sensor hub signal is the XL/Gyro data-ready;<br>1: Sensor hub signal external from INT2 pad.)|
|PULL_UP_EN|Auxiliary I<sup>2</sup>C pull-up. Default value: 0<br>(0: internal pull-up on auxiliary I<sup>2</sup>C line disabled;<br>1: internal pull-up on auxiliary I<sup>2</sup>C line enabled)|
|PASS_THROUGH<br>_MODE|I<sup>2</sup>C interface pass-through. Default value: 0<br>(0: through disabled; 1: through enabled)|
|IRON_EN|Enable hard-iron correction algorithm for magnetometer. Default value: 0<br>(0:hard-iron correction algorithm disabled;<br>1: hard-iron correction algorithm enabled)|
|MASTER_ON|Sensor hub I<sup>2</sup>C master enable. Default: 0<br>(0: master I<sup>2</sup>C of sensor hub disabled; 1: master I<sup>2</sup>C of sensor hub enabled)|



1. If the TIMER_PEDO_FIFO_DRDY bit in FIFO_CTRL2(07h) is set to 0, the trigger for writing data in FIFO is XL/Gyro data-ready, otherwise it's the step detection. 

#### **9.23 WAKE_UP_SRC (1Bh)** 

Wake up interrupt source register (r). 

**Table 76. WAKE_UP_SRC register** 

|0<sup>(1)</sup>|0<sup>(1)</sup>|FF_IA|SLEEP_<br>STATE_IA|WU_IA|X_WU|Y_WU|Z_WU|
|---|---|---|---|---|---|---|---|



1. This bit must be set to ‘0’ for the correct operation of the device. 

60/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

**Table 77. WAKE_UP_SRC register description** 

|FF_IA|Free-fall event detection status. Default: 0<br>(0: free-fall event not detected; 1: free-fall event detected)|
|---|---|
|SLEEP_<br>STATE_IA|Sleep event status. Default value: 0<br>(0: sleep event not detected; 1: sleep event detected)|
|WU_IA|Wakeup event detection status. Default value: 0<br>(0: wakeup event not detected; 1: wakeup event detected.)|
|X_WU|Wakeup event detection status on X-axis. Default value: 0<br>(0: wakeup event on X-axis not detected; 1: wakeup event on X-axis detected)|
|Y_WU|Wakeup event detection status on Y-axis. Default value: 0<br>(0: wakeup event on Y-axis not detected; 1: wakeup event on Y-axis detected)|
|Z_WU|Wakeup event detection status on Z-axis. Default value: 0<br>(0: wakeup event on Z-axis not detected; 1: wakeup event on Z-axis detected)|



#### **9.24 TAP_SRC (1Ch)** 

Tap source register (r). 

**Table 78. TAP_SRC register** 

|0<sup>(1)</sup>|TAP_IA|SINGLE_<br>TAP|DOUBLE_<br>TAP|TAP_SIGN|X_TAP|Y_TAP|Z_TAP|
|---|---|---|---|---|---|---|---|



1. This bit must be set to ‘0’ for the correct operation of the device. 

**Table 79. TAP_SRC register description** 

|TAP_IA|Tap event detection status. Default: 0<br>(0: tap event not detected; 1: tap event detected)|
|---|---|
|SINGLE_TAP|Single-tap event status. Default value: 0<br>(0: single tap event not detected; 1: single tap event detected)|
|DOUBLE_TAP|Double-tap event detection status. Default value: 0<br>(0: double-tap event not detected; 1: double-tap event detected.)|
|TAP_SIGN|Sign of acceleration detected by tap event. Default: 0<br>(0: positive sign of acceleration detected by tap event;<br>1: negative sign of acceleration detected by tap event)|
|X_TAP|Tap event detection status on X-axis. Default value: 0<br>(0: tap event on X-axis not detected; 1: tap event on X-axis detected)|
|Y_TAP|Tap event detection status on Y-axis. Default value: 0<br>(0: tap event on Y-axis not detected; 1: tap event on Y-axis detected)|
|Z_TAP|Tap event detection status on Z-axis. Default value: 0<br>(0: tap event on Z-axis not detected; 1: tap event on Z-axis detected)|



61/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.25 D6D_SRC (1Dh)** 

Portrait, landscape, face-up and face-down source register (r) 

###### **Table 80. D6D_SRC register** 

|0<sup>(1)</sup>|D6DIA<br>ZH<br>ZL<br>YH<br>YL|XH|XL|
|---|---|---|---|
|1.<br>This b|it must be set to ‘0’ for the correct operation of the device.<br>_<br><br><br><br><br>**Table 81. D6D_SRC register description**|||
|D6D_<br>IA|Interrupt active for change position portrait, landscape, face-up, face<br>(0: change position not detected; 1: change position detected)|-down. Def|ault value: 0|
|ZH|Z-axis high event (over threshold). Default value: 0<br>(0: event not detected; 1: event (over threshold) detected)|||
|ZL|Z-axis low event (under threshold). Default value: 0<br>(0: event not detected; 1: event (under threshold) detected)|||
|YH|Y-axis high event (over threshold). Default value: 0<br>(0: event not detected; 1: event (over-threshold) detected)|||
|YL|Y-axis low event (under threshold). Default value: 0<br>(0: event not detected; 1: event (under threshold) detected)|||
|X_H|X-axis high event (over threshold). Default value: 0<br>(0: event not detected; 1: event (over threshold) detected)|||
|X_L|X-axis low event (under threshold). Default value: 0<br>(0: event not detected; 1: event (under threshold) detected)|||



#### **9.26 STATUS_REG (1Eh)** 

###### **Table 82. STATUS_REG register** 

|-|-<br>-<br>-<br>EV_BOOT<br>TDA|GDA|XLDA|
|---|---|---|---|
|EV_BOOT|**Table 83. STATUS_REG register description**<br>Boot running flag signal. Default value: 0<br>(0: no boot running; 1: boot running)|||
|TDA|Temperature new data available. Default: 0<br>(0: no set of data is available at temperature sensor output;<br>1: a new set of data is available at temperature sensor output)|||
|GDA|Gyroscope new data available. Default value: 0<br>(0: no set of data available at gyroscope output;<br>1: a new set of data is available at gyroscope output)|||
|XLDA|Accelerometer new data available. Default value: 0<br>(0: no set of data available at accelerometer output;<br>1: a new set of data is available at accelerometer output)|||



62/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.27 OUT_TEMP_L (20h), OUT_TEMP(21h)** 

Temperature data output register (r). L and H registers together express a 16-bit word in two’s complement (r). 

###### **Table 84. OUT_TEMP_L register** 

|Temp7|Temp6|Temp5<br>**Table**|Temp4<br>**85. OUT_**|Temp3<br>**TEMP_H re**|Temp2<br>**gister**|Temp1|Temp0|
|---|---|---|---|---|---|---|---|
|Temp15|Temp14|Temp13|Temp12|Temp11|Temp10|Temp9|Temp8|
|Temp[15:0]|Temperatu<br>The value|**Table 86.**<br>re sensor out<br>is expressed|**OUT_TEMP**<br>put data<br>as two’s co|**register d**<br>mplement sig|**escription**<br>n extended on|the MSB.||



#### **9.28 OUTX_L_G (22h)** 

Angular rate sensor pitch axis (X) angular rate output register (r). The value is expressed as a 16-bit word in two’s complement. (r) 

**Table 87. OUTX_L_G register** 

|D7|D6|D5|D4<br>D3|D2|D1|D0|
|---|---|---|---|---|---|---|
|D[7:0]|Pitch axis|**Table 88. O**<br>(X) angular rat|**UTX_L_G register de**<br>e value (LSbyte)|**scription**|||



#### **9.29 OUTX_H_G (23h)** 

Angular rate sensor pitch axis (X) angular rate output register (r). The value is expressed as a 16-bit word in two’s complement. (r) 

**Table 89. OUTX_H_G register** 

|D15|D14|D13|D12|D11|D10|D9|D8|
|---|---|---|---|---|---|---|---|
|D[15:8]|Pitch axis|**Table 90. O**<br>(X) angular ra|**UTX_H_G**<br>te value (MS|**register d**<br>byte)|**escription**|||



#### **9.30 OUTY_L_G (24h)** 

Angular rate sensor roll axis (Y) angular rate output register (r). The value is expressed as a 16-bit word in two’s complement. (r). 

###### **Table 91. OUTY_L_G register** 

|D7|D6|D5|D4|D3|D2|D1|D0|
|---|---|---|---|---|---|---|---|
|D[7:0]|Roll axis (|**Table 92. O**<br>Y) angular rate|**UTY_L_G**<br>value (LSby|**register de**<br>te)|**scription**|||



63/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.31 OUTY_H_G (25h)** 

Angular rate sensor roll axis (Y) angular rate output register (r). The value is expressed as a 16-bit word in two’s complement. (r). 

###### **Table 93. OUTY_H_G register** 

|D15|D14|D13|D12|D11|D10|D9|D8|
|---|---|---|---|---|---|---|---|
|||**Table 94.**|**OUTY_H_G**|**register d**|**escription**|||
|D[15:8]|Roll axis (|Y) angular rat|e value (MSb|yte)||||



#### **9.32 OUTZ_L_G (26h)** 

Angular rate sensor yaw axis (Z) angular rate output register (r). The value is expressed as a 16-bit word in two’s complement. (r). 

###### **Table 95. OUTZ_L_G register** 

|D7|D6|D5<br>D4<br>D3|D2|D1|D0|
|---|---|---|---|---|---|
|D[7:0]|Yaw axis|**Table 96. OUTZ_L_G register de**<br>(Z) angular rate value (LSbyte)|**scription**|||



#### **9.33 OUTZ_H_G (27h)** 

Angular rate sensor Yaw axis (Z) angular rate output register (r). The value is expressed as a 16-bit word in two’s complement. 

###### **Table 97. OUTZ_H_G register** 

|D15<br>D14|D13|D12|D11|D10|D9|D8|
|---|---|---|---|---|---|---|



###### **Table 98. OUTZ_H_G register description** 

D[15:8] Yaw axis (Z) angular rate value (MSbyte) 

#### **9.34** 

#### **OUTX_L_XL (28h)** 

Linear acceleration sensor X-axis output register (r). The value is expressed as a 16-bit word in two’s complement. 

**Table 99. OUTX_L_XL register** 

|D7|D6|D5|D4<br>D3|D2|D1|D0|
|---|---|---|---|---|---|---|
|||**Table 100.**|**OUTX_L_XL register**|**description**|||
|D[7:0]|X-axis li|near accelerati|on value (LSbyte)||||



64/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.35 OUTX_H_XL (29h)** 

Linear acceleration sensor X-axis output register (r). The value is expressed as a 16-bit word in two’s complement. 

###### **Table 101. OUTX_H_XL register** 

|D15|D14|D13|D12|D11|D10|D9|D8|
|---|---|---|---|---|---|---|---|
|||**Table 102. O**|**UTX_H_XL**|**register d**|**escription**|||
|D[15:8]|X-axis li|near acceleratio|n value (MSb|yte)||||



#### **9.36 OUTY_L_XL (2Ah)** 

Linear acceleration sensor Y-axis output register (r). The value is expressed as a 16-bit word in two’s complement. 

**Table 103. OUTY_L_XL register** 

|D7|D6|D5|D4|D3|D2|D1|D0|
|---|---|---|---|---|---|---|---|
|||**Table 104. O**|**UTY_L_XL**|**register**|**description**|||
|D[7:0]|Y-axis lin|ear acceleration|value (LSby|te)||||



#### **9.37 OUTY_H_XL (2Bh)** 

Linear acceleration sensor Y-axis output register (r). The value is expressed as a 16-bit word in two’s complement. 

###### **Table 105. OUTY_H_G register** 

|D15|D14|D13|D12<br>D11|D10|D9|D8|
|---|---|---|---|---|---|---|
|D[15:8]|Y-axis lin|**Table 106. O**<br>ear acceleration|**UTY_H_G register d**<br>value (MSbyte)|**escription**|||



#### **9.38 OUTZ_L_XL (2Ch)** 

Linear acceleration sensor Z-axis output register (r). The value is expressed as a 16-bit word in two’s complement. 

**Table 107. OUTZ_L_XL register** 

|D7|D6|D5|D4<br>D3|D2|D1|D0|
|---|---|---|---|---|---|---|
|||**Table 108.**|**OUTZ_L_XL register d**|**escription**|||
|D[7:0]|Z-axis li|near acceleratio|n value (LSbyte)||||



65/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.39 OUTZ_H_XL (2Dh)** 

Linear acceleration sensor Z-axis output register (r). The value is expressed as a 16-bit word in two’s complement. 

###### **Table 109. OUTZ_H_XL register** 

|D15|D14|D13|D12|D11|D10|D9|D8|
|---|---|---|---|---|---|---|---|
|||**Table 110.**|**OUTZ_H_X**|**L register d**|**escription**|||
|D[15:8]|Z-axis li|near acceleratio|n value (MS|byte)||||



#### **9.40 SENSORHUB1_REG (2Eh)** 

First byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

###### **Table 111. SENSORHUB1_REG register** 

SHub1_7 SHub1_6 SHub1_5 SHub1_4 SHub1_3 SHub1_2 SHub1_1 SHub1_0 **Table 112. SENSORHUB1_REG register description** SHub1_[7:0] First byte associated to external sensors 

#### **9.41 SENSORHUB2_REG (2Fh)** 

Second byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operations configurations (for external sensors from x = 0 to x = 3). 

###### **Table 113. SENSORHUB2_REG register** 

SHub2_7 SHub2_6 SHub2_5 SHub2_4 SHub2_3 SHub2_2 SHub2_1 SHub2_0 **Table 114. SENSORHUB2_REG register description** SHub2_[7:0] Second byte associated to external sensors 

#### **9.42 SENSORHUB3_REG (30h)** 

Third byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operations configurations (for external sensors from x = 0 to x = 3). 

###### **Table 115. SENSORHUB3_REG register** 

|SHub3_7|SHub3_6|SHub3_5|SHub3_4|SHub3_3|SHub3_2<br>SHub3_1|SHub3_0|
|---|---|---|---|---|---|---|
|SHub3_[7:0]|**Tabl**<br>Third byt|**e 116. SEN**<br>e associated|**SORHUB3**<br>to external s|**_REG regi**<br>ensors|**ster description**||



66/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.43 SENSORHUB4_REG (31h)** 

Fourth byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

**Table 117. SENSORHUB4_REG register** 

SHub4_7 SHub4_6 SHub4_5 SHub4_4 SHub4_3 SHub4_2 SHub4_1 SHub4_0 **Table 118. SENSORHUB4_REG register description** SHub4_[7:0] Fourth byte associated to external sensors 

#### **9.44 SENSORHUB5_REG (32h)** 

Fifth byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

###### **Table 119. SENSORHUB5_REG register** 

|SHub5_7|SHub5_6|SHub5_5|SHub5_4|SHub5_3<br>SHub5_2<br>SHub5_1|SHub5_0|
|---|---|---|---|---|---|
||**Tabl**|**e 120. SEN**|**SORHUB5**|**_REG register description**||
|SHub5_[7:0]|Fifth byte|associated t|o external se|nsors||



#### **9.45 SENSORHUB6_REG (33h)** 

Sixth byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

###### **Table 121. SENSORHUB6_REG register** 

SHub6_7 SHub6_6 SHub6_5 SHub6_4 SHub6_3 SHub6_2 SHub6_1 SHub6_0 **Table 122. SENSORHUB6_REG register description** SHub6_[7:0] Sixth byte associated to external sensors 

#### **9.46 SENSORHUB7_REG (34h)** 

Seventh byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

###### **Table 123. SENSORHUB7_REG register** 

|SHub7_7|SHub7_6|SHub7_5<br>SHub7_4|SHub7_3|SHub7_2<br>SHub7_1|SHub7_0|
|---|---|---|---|---|---|
|SHub7_[7:0]|**Tabl**<br>Seventh|**e 124. SENSORHUB7**<br>byte associated to externa|**_REG regi**<br>l sensors|**ster description**||



67/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.47 SENSORHUB8_REG(35h)** 

Eighth byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

**Table 125. SENSORHUB8_REG register** 

|SHub8_7|SHub8_6|SHub8_5|SHub8_4|SHub8_3|SHub8_2<br>SHub8_1|SHub8_0|
|---|---|---|---|---|---|---|
|SHub8_[7:0]|**Tabl**<br>Eighth by|**e 126. SEN**<br>te associated|**SORHUB8**<br>to external|**_REG regi**<br>sensors|**ster description**||



#### **9.48 SENSORHUB9_REG (36h)** 

Ninth byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

###### **Table 127. SENSORHUB9_REG register** 

|SHub9_7|SHub9_6|SHub9_5|SHub9_4|SHub9_3<br>SHub9_2<br>SHub9_1|SHub9_0|
|---|---|---|---|---|---|
||**Tabl**|**e 128. SEN**|**SORHUB9**|**_REG register description**||
|SHub9_[7:0]|Ninth byte|associated|to external s|ensors||



#### **9.49 SENSORHUB10_REG (37h)** 

Tenth byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

###### **Table 129. SENSORHUB10_REG register** 

SHub10_7 SHub10_6 SHub10_5 SHub10_4 SHub10_3 SHub10_2 SHub10_1 SHub10_0 **Table 130. SENSORHUB10_REG register description** SHub10_[7:0] Tenth byte associated to external sensors 

#### **9.50 SENSORHUB11_REG (38h)** 

Eleventh byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

###### **Table 131. SENSORHUB11_REG register** 

SHub11_7 SHub11_6 SHub11_5 SHub11_4 SHub11_3 SHub11_2 SHub11_1 SHub11_0 

###### **Table 132. SENSORHUB11_REG register description** 

|SHub11_[7:0]|Eleventh byte associated to external sensors|
|---|---|



68/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.51 SENSORHUB12_REG(39h)** 

Twelfth byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

**Table 133. SENSORHUB12_REG register** 

|SHub12_|7<br>SHub12_6<br>SHub12_5<br>SHub12_4|SHub12_3|SHub12_2|SHub12_1|SHub12_0|
|---|---|---|---|---|---|



###### **Table 134. SENSORHUB12_REG register description** 

|SHub12[7:0]|Twelfth byte associated to external sensors|
|---|---|



#### **9.52 FIFO_STATUS1 (3Ah)** 

FIFO status control register (r). For a proper reading of the register, it is recommended to set the BDU bit in _CTRL3_C (12h)_ to 1. 

**Table 135. FIFO_STATUS1 register** 

|DIFF_<br>FIFO_7|DIFF_<br>FIFO_6|DIFF_<br>FIFO_5|DIFF_<br>FIFO_4|DIFF_<br>FIFO_3|DIFF_<br>FIFO_2|DIFF_<br>FIFO_1|DIFF_<br>FIFO_0|
|---|---|---|---|---|---|---|---|
|DIFF_FIFO|**T**<br>_[7:0]<br>Nu|**able 136. F**<br>mber of unre|**IFO_STATU**<br>ad words (16|**S1 registe**<br>-bit axes) sto|**r descriptio**<br>red in FIFO<sup>(1</sup>|**n**<br><sup>)</sup>.||



1. For a complete number of unread samples, consider DIFF_FIFO [11:8] in _FIFO_STATUS2 (3Bh)_ 

#### **9.53 FIFO_STATUS2 (3Bh)** 

FIFO status control register (r). For a proper reading of the register, it is recommended to set the BDU bit in _CTRL3_C (12h)_ to 1. 

**Table 137. FIFO_STATUS2 register** 

||FIFO|FIFO|FIFO|DIFF|DIFF|DIFF|DIFF|
|---|---|---|---|---|---|---|---|
||_|_|_|_|_|_|_|
||OVER_RUN|FULL|EMPTY|FIFO_11|FIFO_10|FIFO_9|FIFO_8|



**Table 138. FIFO_STATUS2 register description** 

|FTH|FIFO watermark status. Default value: 0<br>(0: FIFO filling is lower than watermark level<sup>(1)</sup>;<br>1: FIFO filling is equal to or higher than the watermark level)|
|---|---|
|FIFO_OVER_RUN|FIFO overrun status. Default value: 0<br>(0: FIFO is not completely filled; 1: FIFO is completely filled)|
|FIFO_FULL|FIFO full status. Default value: 0<br>(0: FIFO is not full; 1: FIFO will be full at the next ODR)|
|FIFO_EMPTY|FIFO empty bit. Default value: 0<br>(0: FIFO contains data; 1: FIFO is empty)|
|DIFF_FIFO_[7:0]|Number of unread words (16-bit axes) stored in FIFO<sup>(2)</sup>.|



1. FIFO watermark level is set in FTH_[11:0] in _FIFO_CTRL1 (06h)_ and _FIFO_CTRL2 (07h)_ 

2. For a complete number of unread samples, consider DIFF_FIFO [11:8] in _FIFO_STATUS1 (3Ah)_ 

69/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.54 FIFO_STATUS3 (3Ch)** 

FIFO status control register (r). For a proper reading of the register, it is recommended to set the BDU bit in _CTRL3_C (12h)_ to 1. 

**Table 139. FIFO_STATUS3 register** 

FIFO_ FIFO_ FIFO_ FIFO_ FIFO_ FIFO_ FIFO_ FIFO_ PATTERN PATTERN PATTERN PATTERN PATTERN PATTERN PATTERN PATTERN _7 _6 _5 _4 _3 _2 _1 _0 

###### **Table 140. FIFO_STATUS3 register description** 

FIFO_ Word of recursive pattern read at the next reading. PATTERN_[7:0] 

#### **9.55 FIFO_STATUS4 (3Dh)** 

FIFO status control register (r). For a proper reading of the register, it is recommended to set the BDU bit in _CTRL3_C (12h)_ to 1. 

**Table 141. FIFO_STATUS4 register** 

|0<sup>(1)</sup>|0<sup>(1)</sup>|0<sup>(1)</sup>|0<sup>(1)</sup>|0<sup>(1)</sup>|0<sup>(1)</sup>|FIFO_<br>PATTERN_9|FIFO_<br>PATTERN_8|
|---|---|---|---|---|---|---|---|



1. This bit must be set to ‘0’ for the correct operation of the device. 

###### **Table 142. FIFO_STATUS4 register description** 

FIFO_ Word of recursive pattern read at the next reading. PATTERN_[9:8] 

#### **9.56 FIFO_DATA_OUT_L (3Eh)** 

FIFO data output register (r). For a proper reading of the register, it is recommended to set the BDU bit in _CTRL3_C (12h)_ to 1. 

**Table 143. FIFO_DATA_OUT_L register** 

|DATA_<br>OUT_<br>FIFO_L_7|DATA_<br>OUT_<br>FIFO_L_6<br>DATA_<br>OUT_<br>FIFO_L_5|DATA_<br>OUT_<br>FIFO_L_4|DATA_<br>OUT_<br>FIFO_L_3|DATA_<br>OUT_<br>FIFO_L_2|DATA_<br>OUT_<br>FIFO_L_1|DATA_<br>OUT_<br>FIFO_L_0|
|---|---|---|---|---|---|---|
|DATA_OUT|**Table 144. FIFO**<br>_FIFO_L_[7:0]<br>FIFO|**_DATA_O**<br>data output|**UT_L regis**<br>(first byte)|**ter descrip**|**tion**||



70/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.57 FIFO_DATA_OUT_H (3Fh)** 

FIFO data output register (r). For a proper reading of the register, it is recommended to set the BDU bit in _CTRL3_C (12h)_ to 1. 

###### **Table 145. FIFO_DATA_OUT_H register** 

|DATA_|DATA_|DATA_|DATA_|DATA_|DATA_|DATA_|DATA_|
|---|---|---|---|---|---|---|---|
|OUT_|OUT_|OUT_|OUT_|OUT_|OUT_|OUT_|OUT_|
|FIFO_H_7|FIFO_H_6|FIFO_H_5|FIFO_H_4|FIFO_H_3|FIFO_H_2|FIFO_H_1|FIFO_H_0|



###### **Table 146. FIFO_DATA_OUT_H register description** 

DATA_OUT_FIFO_H_[7:0] FIFO data output (second byte) 

#### **9.58 TIMESTAMP0_REG (40h)** 

Time stamp first byte data output register (r). The value is expressed as a 24-bit word and the bit resolution is defined by setting the value in _WAKE_UP_DUR (5Ch)_ . 

###### **Table 147. TIMESTAMP0_REG register** 

|TIMESTA<br>MP0_7|TIMESTA<br>MP0_6|TIMESTA<br>MP0_5|TIMESTA<br>MP0_4|TIMESTA<br>MP0_3|TIMESTA<br>MP0_2|TIMESTA<br>MP0_1|TIMESTA<br>MP0_0|
|---|---|---|---|---|---|---|---|
|TIMESTAM|**Tab**<br>P0_[7:0]|**le 148. TIM**<br>TIMESTAM|**ESTAMP0_**<br>P first byte d|**REG regis**<br>ata output|**ter descrip**|**tion**||



#### **9.59 TIMESTAMP1_REG (41h)** 

Time stamp second byte data output register (r). The value is expressed as a 24-bit word and the bit resolution is defined by setting value in _WAKE_UP_DUR (5Ch)_ . 

###### **Table 149. TIMESTAMP1_REG register** 

|TIMESTA<br>MP1_7|TIMESTA<br>MP1_6|TIMESTA<br>MP1_5|TIMESTA<br>MP1_4|TIMESTA<br>MP1_3|TIMESTA<br>MP1_2|TIMESTA<br>MP1_1|TIMESTA<br>MP1_0|
|---|---|---|---|---|---|---|---|
|TIMESTAM|**Tab**<br>P1_[7:0]|**le 150. TIM**<br>TIMESTAMP|**ESTAMP1_**<br>second byt|**REG regist**<br>e data output|**er descrip**|**tion**||



#### **9.60 TIMESTAMP2_REG (42h)** 

Time stamp third byte data output register (r/w). The value is expressed as a 24-bit word and the bit resolution is defined by setting the value in _WAKE_UP_DUR (5Ch)_ . To reset the timer, the AAh value has to be stored in this register. 

**Table 151. TIMESTAMP2_REG register** 

TIMESTA TIMESTA TIMESTA TIMESTA TIMESTA TIMESTA TIMESTA TIMESTA MP2_7 MP2_6 MP2_5 MP2_4 MP2_3 MP2_2 MP2_1 MP2_0 **Table 152. TIMESTAMP2_REG register description** TIMESTAMP2_[7:0] TIMESTAMP third byte data output 

71/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.61 STEP_TIMESTAMP_L (49h)** 

Step counter timestamp information register (r). When a step is detected, the value of TIMESTAMP_REG1 register is copied in STEP_TIMESTAMP_L. 

**Table 153. STEP_TIMESTAMP_L register** 

STEP_ STEP_ STEP_ STEP_ STEP_ STEP_ STEP_ STEP_ TIMESTA TIMESTA TIMESTA TIMESTA TIMESTA TIMESTA TIMESTA TIMESTA MP_L_7 MP_L_6 MP_L_5 MP_L_4 MP_L_3 MP_L_2 MP_L_1 MP_L_0 **Table 154. STEP_TIMESTAMP_L register description** STEP_TIMESTAMP_L[7:0] Timestamp of last step detected. 

#### **9.62 STEP_TIMESTAMP_H (4Ah)** 

Step counter timestamp information register (r). When a step is detected, the value of TIMESTAMP_REG2 register is copied in STEP_TIMESTAMP_H. 

**Table 155. STEP_TIMESTAMP_H register** 

STEP_ STEP_ STEP_ STEP_ STEP_ STEP_ STEP_ STEP_ TIMESTA TIMESTA TIMESTA TIMESTA TIMESTA TIMESTA TIMESTA TIMESTA MP_H_7 MP_H_6 MP_H_5 MP_H_4 MP_H_3 MP_H_2 MP_H_1 MP_H_0 

**Table 156. STEP_TIMESTAMP_H register description** 

STEP_TIMESTAMP_H[7:0] Timestamp of last step detected. 

#### **9.63 STEP_COUNTER_L (4Bh)** 

Step counter output register (r). 

**Table 157. STEP_COUNTER_L register** 

STEP_CO STEP_CO STEP_CO STEP_CO STEP_CO STEP_CO STEP_CO STEP_CO UNTER_L UNTER_L UNTER_L UNTER_L UNTER_L UNTER_L UNTER_L UNTER_L _7 _6 _5 _4 _3 _2 _1 _0 

**Table 158. STEP_COUNTER_L register description** 

STEP_COUNTER_L_[7:0] Step counter output (LSbyte) 

#### **9.64 STEP_COUNTER_H (4Ch)** 

Step counter output register (r). 

###### **Table 159. STEP_COUNTER_H register** 

STEP_CO STEP_CO STEP_CO STEP_CO STEP_CO STEP_CO STEP_CO STEP_CO UNTER_H UNTER_H UNTER_H UNTER_H UNTER_H UNTER_H UNTER_H UNTER_H _7 _6 _5 _4 _3 _2 _1 _0 

**Table 160. STEP_COUNTER_H register description** 

STEP_COUNTER_H_[7:0] Step counter output (MSbyte) 

72/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.65 SENSORHUB13_REG (4Dh)** 

Thirteenth byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

###### **Table 161. SENSORHUB13_REG register** 

|SHub13_7<br>SH|ub13_6<br>SHub13_5<br>SHub13_4<br>SHub13_3<br>SHub13_2<br>SHub13_1|SHub13_0|
|---|---|---|
||**Table 162. SENSORHUB13_REG register description**||
|SHub13_[7:0]|Thirteenth byte associated to external sensors||



#### **9.66 SENSORHUB14_REG (4Eh)** 

Fourteenth byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

###### **Table 163. SENSORHUB14_REG register** 

|SHub14_7<br>SH|ub14_6<br>SHub14_5<br>SHub14_4<br>SHub14_3<br>SHub14_2<br>SHub14_1|SHub14_0|
|---|---|---|
||**Table 164. SENSORHUB14_REG register description**||
|SHub14_[7:0]|Fourteenth byte associated to external sensors||



#### **9.67** 

#### **SENSORHUB15_REG (4Fh)** 

Fifteenth byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

###### **Table 165. SENSORHUB15_REG register** 

SHub15_7 SHub15_6 SHub15_5 SHub15_4 SHub15_3 SHub15_2 SHub15_1 SHub15_0 

###### **Table 166. SENSORHUB15_REG register description** 

SHub15_[7:0] Fifteenth byte associated to external sensors 

#### **9.68 SENSORHUB16_REG (50h)** 

Sixteenth byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

###### **Table 167. SENSORHUB16_REG register** 

SHub16_7 SHub16_6 SHub16_5 SHub16_4 SHub16_3 SHub16_2 SHub16_1 SHub16_0 

###### **Table 168. SENSORHUB16_REG register description** 

|SHub16_|[7:0]<br>Sixteenth byte associated to external sensors|
|---|---|



73/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.69 SENSORHUB17_REG (51h)** 

Seventeenth byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

###### **Table 169. SENSORHUB17_REG register** 

|SHub17_7<br>SHub17_6<br>SHub17|_5<br>SHub17|_4<br>SHub17_3|SHub17_2|SHub17_1|SHub17_0|
|---|---|---|---|---|---|



###### **Table 170. SENSORHUB17_REG register description** 

SHub17_[7:0] Seventeenth byte associated to external sensors 

#### **9.70 SENSORHUB18_REG (52h)** 

Eighteenth byte associated to external sensors. The content of the register is consistent with the SLAVEx_CONFIG number of read operation configurations (for external sensors from x = 0 to x = 3). 

**Table 171. SENSORHUB18_REG register** 

|SHub18_|7<br>SHub18_6<br>SHub18_5<br>SHub18_4|SHub18_3|SHub18_2<br>SHub18_1|SHub18_0|
|---|---|---|---|---|



###### **Table 172. SENSORHUB18_REG register description** 

SHub18_[7:0] Eighteenth byte associated to external sensors 

#### **9.71 FUNC_SRC (53h)** 

Significant motion, tilt, step detector, hard/soft-iron and sensor hub interrupt source register (r). 

**Table 173. FUNC_SRC register** 

|STEP_<br>COUNT<br>_DELTA<br>_IA<br>S<br>MOT|IGN_<br>ION_IA<br>TILT_IA|STEP_<br>DETECTED<br>STEP_<br>OVERFLOW<br>0<sup>(1)</sup><br>SI_END_<br>OP|SENSOR<br>HUB_<br>END_OP|
|---|---|---|---|
|1.<br>This bit must|be set to ‘0’ for the corre<br>**Table 174.**|ct operation of the device.<br>**FUNC_SRC register description**||
|STEP_COUNT<br>_DELTA_IA|Pedometer step reco<br>(0: no step recogniz<br>delta time)|gnition on delta time status. Default value: 0<br>ed during delta time; 1: at least one step recognize|d during|
|SIGN_<br>MOTION_IA|Significant motion ev<br>(0: significant motion|ent detection status. Default value: 0<br>event not detected; 1: significant motion event de|tected)|
|TILT_IA|Tilt event detection s<br>(0: tilt event not dete|tatus. Default value: 0<br>cted; 1: tilt event detected)||
|STEP_<br>DETECTED|Step detector event<br>(0: step detector eve|detection status. Default value: 0<br>nt not detected; 1: step detector event detected)||
|STEP_<br>OVERFLOW|Step counter overflo<br>(0: step counter valu|w status. Default value: 0<br>e < 2<sup>16</sup>; 1: step counter value reached 2<sup>16</sup>)||
|SI_END_OP|Hard/soft-iron calcul<br>(0: Hard/soft-iron cal|ation status. Default value: 0<br>culation not concluded; 1: Hard/soft-iron calculatio|n concluded)|
|SENSORHUB<br>_END_OP|Sensor hub commun<br>(0: sensor hub comm<br>1: sensor hub comm|ication status. Default value: 0<br>unication not concluded;<br>unication concluded)||



74/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.72 TAP_CFG (58h)** 

Time stamp, pedometer, tilt, filtering, and tap recognition functions configuration register (r/w). 

**Table 175. TAP_CFG register** 

|TIMER_<br>EN|PEDO_EN|TILT_EN|SLOPE<br>_FDS|TAP_X_EN|TAP_Y_EN|TAP_Z_EN|LIR|
|---|---|---|---|---|---|---|---|



###### **Table 176. TAP_CFG register description** 

|TIMER_EN|Time stamp count enable, output data are collected in_TIMESTAMP0_REG (40h)_,<br>_TIMESTAMP1_REG (41h)_,_TIMESTAMP2_REG (42h)_register. Default: 0<br>(0: time stamp count disabled; 1: time stamp count enabled)|
|---|---|
|PEDO_EN|Pedometer algorithm enable. Default value: 0<br>(0: pedometer algorithm disabled; 1: pedometer algorithm enabled)|
|TILT_EN|Tilt calculation enable. Default value: 0<br>(0: tilt calculation disabled; 1: tilt calculation enabled.)|
|SLOPE_FDS|<sup>Enable accelerometer HP and LPF2 filters (refer to</sup><sup>_Figure 5_). Default value: 0</sup><br>(0: disable; 1: enable)|
|TAP_X_EN|Enable X direction in tap recognition. Default value: 0<br>(0: X direction disabled; 1:X direction enabled)|
|TAP_Y_EN|Enable Y direction in tap recognition. Default value: 0<br>(0: Y direction disabled; 1:Y direction enabled)|
|TAP_Z_EN|Enable Z direction in tap recognition. Default value: 0<br>(0: Z direction disabled; 1:Z direction enabled)|
|LIR|Latched Interrupt. Default value: 0<br>(0: interrupt request not latched; 1: interrupt request latched)|



#### **9.73 TAP_THS_6D (59h)** 

Portrait/landscape position and tap function threshold register (r/w). 

**Table 177. TAP_THS_6D register** 

|D4D_EN<br>SIXD|_THS<br>1<br>SIXD_THS<br>0<br>TAP_THS<br>4<br>TAP_THS<br>3<br>TAP_THS<br>2|TAP_THS<br>1|TAP_THS<br>0|
|---|---|---|---|
|D4D_EN|**Table 178. TAP_THS_6D register description**<br>4D orientation detection enable. Z-axis position detection<br>Default value: 0<br>(0: enabled; 1: disabled)|<br>is disabled.||
|SIXD_THS[1:0]|Threshold for D6D function. Default value: 00<br>For details, refer to_Table 179_.|||
|TAP_THS[4:0]|Threshold for tap recognition. Default value: 00000|||



75/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

###### **Table 179. Threshold for D4D/D6D function** 

||**SIXD_THS[1:0]**||**Threshold value**|
|---|---|---|---|
|00||80 degrees||
|01||70 degrees||
|10||60 degrees||
|11||50 degrees||



#### **9.74 INT_DUR2 (5Ah)** 

Tap recognition function setting register (r/w). 

||**Tab**|**le 180. INT_DUR2 register**|
|---|---|---|
|DUR3|DUR2<br>DUR1|DUR0<br>QUIET1<br>QUIET0<br>SHOCK1<br>SHOCK0|
||**Table 181.**|**INT_DUR2 register description**|
|DUR[3:0]|Duration of maximu<br>When double tap rec<br>between two consec<br>default value of thes<br>DUR[3:0] bits are se|m time gap for double tap recognition. Default: 0000<br>ognition is enabled, this register expresses the maximum time<br>utive detected taps to determine a double tap event. The<br>e bits is 0000b which corresponds to 16*ODR_XL time. If the<br>t to a different value, 1LSB corresponds to 32*ODR_XL time.|
|QUIET[1:0]|Expected quiet time<br>Quiet time is the tim<br>overthreshold event.<br>2*ODR_XL time. If th<br>corresponds to 4*OD|after a tap detection. Default value: 00<br>e after the first detected tap in which there must not be any<br>The default value of these bits is 00b which corresponds to<br>e QUIET[1:0] bits are set to a different value, 1LSB<br>R_XL time.|
|SHOCK[1:0]|Maximum duration o<br>Maximum duration is<br>recognized as a tap<br>to 4*ODR_XL time. I<br>corresponds to 8*OD|f overthreshold event. Default value: 00<br>the maximum time of an overthreshold signal detection to be<br>event. The default value of these bits is 00b which corresponds<br>f the SHOCK[1:0] bits are set to a different value, 1LSB<br>R_XL time.|



#### **9.75 WAKE_UP_THS (5Bh)** 

Single and double-tap function threshold register (r/w). 

###### **Table 182. WAKE_UP_THS register** 

|SINGLE_<br>DOUBLE<br>_TAP|INACTIVITY WK_THS5|WK_THS4|WK_THS3|WK_THS2|WK_THS1|WK_THS0|
|---|---|---|---|---|---|---|



76/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

**Table 183. WAKE_UP_THS register description** 

|SINGLE_DOUBLE_TAP|Single/double-tap event enable. Default: 0<br>(0: only single-tap event enabled;<br>1: both single and double-tap events enabled)|
|---|---|
|INACTIVITY|Inactivity event enable. Default value: 0<br>(0: sleep disabled; 1: sleep enabled)|
|WK_THS[5:0]|Threshold for wakeup. Default value: 000000|



#### **9.76 WAKE_UP_DUR (5Ch)** 

Free-fall, wakeup, time stamp and sleep mode functions duration setting register (r/w). 

**Table 184. WAKE_UP_DUR register** 

|FF_DUR5<br>WAK<br>DUR|E_<br>1<br>WAKE_<br>DUR0<br>TIMER_<br>HR<br>SLEEP_<br>DUR3|SLEEP_<br>DUR2<br>SLEEP_<br>DUR1|SLEEP_<br>DUR0|
|---|---|---|---|
|FF_DUR5|**Table 185. WAKE_UP_DUR registe**<br>Free fall duration event. Default: 0<br>For the complete configuration of the free<br>_FREE_FALL (5Dh)_configuration.|**r description**<br>-fall duration, refer to FF_|DUR[4:0] in|
|WAKE_DUR[1:0]|Wake up duration event. Default: 00<br>1LSB = 1 ODR_time<br>|||
|TIMER_HR|Time stamp register resolution setting<sup>(1)</sup>.<br>(0: 1LSB = 6.4 ms; 1: 1LSB = 25 μs)|Default value: 0||
|SLEEP_DUR[3:0]|Duration to go in sleep mode. Default valu<br>1 LSB = 512 ODR|e: 0000||



1. Configuration of this bit affects _TIMESTAMP0_REG (40h)_ , _TIMESTAMP1_REG (41h)_ , _TIMESTAMP2_REG (42h)_ , _STEP_TIMESTAMP_L (49h)_ , _STEP_TIMESTAMP_H (4Ah)_ , and _STEP_COUNT_DELTA (15h)_ registers. 

#### **9.77 FREE_FALL (5Dh)** 

Free-fall function duration setting register (r/w). 

###### **Table 186. FREE_FALL register** 

|FF_DUR4<br>FF_DUR3<br>FF_DUR2<br>FF_DUR1<br>FF_DUR0<br>FF_THS2<br>FF_THS1<br>FF_THS0|
|---|
|**Table 187. FREE_FALL register description**<br>FF_DUR[4:0]<br>Free-fall duration event. Default: 0<br>For the complete configuration of the free fall duration, refer to FF_DUR5 in<br>_WAKE_UP_DUR (5Ch)_configuration|
|FF_THS[2:0]<br>Free fall threshold setting. Default: 000<br>For details refer to_Table 188_.|



77/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

**Table 188. Threshold for free-fall function** 

||**FF_THS[2:0]**||**Threshold value**|
|---|---|---|---|
|000||156 m_g_||
|001||219 m_g_||
|010||250 m_g_||
|011||312 m_g_||
|100||344 m_g_||
|101||406 m_g_||
|110||469 m_g_||
|111||500 m_g_||



#### **9.78 MD1_CFG (5Eh)** 

Functions routing on INT1 register (r/w). 

**Table 189. MD1_CFG register** 

|INT1_<br>INACT_<br>STATE<br>IN<br>SIN<br>|T1_<br>GLE_<br>TAP<br>INT1_WU<br>INT1_FF<br>INT1_<br>DOUBLE_<br>TAP<br>INT1_6D<br>INT1_TILT<br>**Table 190. MD1_CFG register description**|INT1_<br>TIMER|
|---|---|---|
|INT1_INACT_<br>STATE|Routing on INT1 of inactivity mode. Default: 0<br>(0: routing on INT1 of inactivity disabled; 1: routing on INT1 of inactivity|enabled)|
|INT1_SINGLE_<br>TAP|Single-tap recognition routing on INT1. Default: 0<br>(0: routing of single-tap event on INT1 disabled;<br>1: routing of single-tap event on INT1 enabled)||
|INT1_WU|Routing of wakeup event on INT1. Default value: 0<br>(0: routing of wakeup event on INT1 disabled;<br>1: routing of wakeup event on INT1 enabled)||
|INT1_FF|Routing of free-fall event on INT1. Default value: 0<br>(0: routing of free-fall event on INT1 disabled;<br>1: routing of free-fall event on INT1 enabled)||
|INT1_DOUBLE<br>_TAP|Routing of tap event on INT1. Default value: 0<br>(0: routing of double-tap event on INT1 disabled;<br>1: routing of double-tap event on INT1 enabled)||
|INT1_6D|Routing of 6D event on INT1. Default value: 0<br>(0: routing of 6D event on INT1 disabled; 1: routing of 6D event on INT|1 enabled)|
|INT1_TILT|Routing of tilt event on INT1. Default value: 0<br>(0: routing of tilt event on INT1 disabled; 1: routing of tilt event on INT1|enabled)|
|INT1_TIMER|Routing of end counter event of timer on INT1. Default value: 0<br>(0: routing of end counter event of timer on INT1 disabled;<br>1: routing of end counter event of timer event on INT1 enabled)||



78/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Register description** 

#### **9.79 MD2_CFG (5Fh)** 

Functions routing on INT2 register (r/w). 

###### **Table 191. MD2_CFG register** 

|INT2_<br>INACT_<br>STATE<br>I<br>SIN<br>|NT2_<br>GLE_<br>TAP<br>INT2_WU|INT2_FF|INT2_<br>DOUBLE_<br>TAP<br>INT2_6D<br>INT2_TILT|INT2_<br>IRON|
|---|---|---|---|---|
||**Table 192.**|**MD2_CF**|**G register description**||
|INT2_INACT_<br>STATE|Routing on INT2 of i<br>(0: routing on INT2|nactivity m<br>of inactivity|ode. Default: 0<br>disabled; 1: routing on INT2 of inactivity|enabled)|
|INT2_SINGLE_<br>TAP|Single-tap recognitio<br>(0: routing of single-<br>1: routing of single-t|n routing o<br>tap event o<br>ap event on|n INT2. Default: 0<br>n INT2 disabled;<br>INT2 enabled)||
|INT2_WU|Routing of wakeup e<br>(0: routing of wakeu<br>1: routing of wake-u|vent on IN<br>p event on<br>p event on|T2. Default value: 0<br>INT2 disabled;<br>INT2 enabled)||
|INT2_FF|Routing of free-fall e<br>(0: routing of free-fa<br>1: routing of free-fall|vent on IN<br>ll event on I<br>event on I|T2. Default value: 0<br>NT2 disabled;<br>NT2 enabled)||
|INT2_DOUBLE<br>_TAP|Routing of tap event<br>(0: routing of double<br>1: routing of double-|on INT2. D<br>-tap event<br>tap event o|efault value: 0<br>on INT2 disabled;<br>n INT2 enabled)||
|INT2_6D|Routing of 6D event<br>(0: routing of 6D eve|on INT2. D<br>nt on INT2|efault value: 0<br>disabled; 1: routing of 6D event on INT2|enabled)|
|INT2_TILT|Routing of tilt event<br>(0: routing of tilt eve|on INT2. D<br>nt on INT2|efault value: 0<br>disabled; 1: routing of tilt event on INT2|enabled)|
|INT2_IRON|Routing of soft-iron/<br>(0: routing of soft-iro<br>1: routing of soft-iro|hard-iron al<br>n/hard-iron<br>n/hard-iron|gorithm end event on INT2. Default valu<br>algorithm end event on INT2 disabled;<br>algorithm end event on INT2 enabled)|e: 0|



#### **9.80 OUT_MAG_RAW_X_L (66h)** 

External magnetometer raw data (r). 

###### **Table 193. OUT_MAG_RAW_X_L register** 

|D7|D6|D5|D4||D3|D2<br>D1|D0|
|---|---|---|---|---|---|---|---|
||**Table**|**194. OUT**|**_MAG_RA**|**W**|**_X_L regis**|**ter description**||
|D[7:0]|X-axis exte|rnal magnet|ometer valu|e|(LSbyte)|||



#### **9.81 OUT_MAG_RAW_X_H (67h)** 

External magnetometer raw data (r). 

###### **Table 195. OUT_MAG_RAW_X_H register** 

|D15<br>D14|D13|D12|D11|D10|D9|D8|
|---|---|---|---|---|---|---|



DocID026899 Rev 7 79/99 

**LSM6DS3** 

**Register description** 

###### **Table 196. OUT_MAG_RAW_X_H register description** 

D[15:8] X-axis external magnetometer value (MSbyte) 

#### **9.82 OUT_MAG_RAW_Y_L (68h)** 

External magnetometer raw data (r). 

###### **Table 197. OUT_MAG_RAW_Y_L register** 

|D7|D6|D5|D4<br>D3|D2|D1|D0|
|---|---|---|---|---|---|---|
||**Tabl**|**e 198. OUT**|**_MAG_RAW_Y_L regis**|**ter descri**|**ption**||
|D[7:0]|Y-axis exte|rnal magnet|ometer value (LSbyte)||||



#### **9.83 OUT_MAG_RAW_Y_H (69h)** 

External magnetometer raw data (r). 

###### **Table 199. OUT_MAG_RAW_Y_H register** 

|D15|D14|D13|D12|D11|D10|D9|D8|
|---|---|---|---|---|---|---|---|
||**Tabl**|**e 200. OUT**|**_MAG_RAW**|**_Y_H regi**|**ster descri**|**ption**||
|D[15:8]|Y-axis exte|rnal magnet|ometer value|(MSbyte)||||



#### **9.84 OUT_MAG_RAW_Z_L (6Ah)** 

External magnetometer raw data (r). 

###### **Table 201. OUT_MAG_RAW_Z_L register** 

|D7|D6|D5|D4|D3|D2|D1|D0|
|---|---|---|---|---|---|---|---|
||**Tabl**|**e 202. OUT_**|**MAG_RAW**|**_Z_L regis**|**ter descri**|**ption**||
|D[7:0]|Z-axis ext|ernal magneto|meter value|(LSbyte)||||



#### **9.85 OUT_MAG_RAW_Z_H (6Bh)** 

External magnetometer raw data (r). 

###### **Table 203. OUT_MAG_RAW_Z_H register** 

|D15|D14|D13|D12|D11|D10<br>D9|D8|
|---|---|---|---|---|---|---|
|D[15:8]|**Tabl**<br>Z-axis ext|**e 204. OUT**<br>ernal magnet|**_MAG_RAW**<br>ometer value|**_Z_H regis**<br>(MSbyte)|**ter description**||



80/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Embedded functions register mapping** 

### **10 Embedded functions register mapping** 

The table given below provides a list of the registers for the embedded functions available in the device and the corresponding addresses. Embedded functions registers are accessible when FUNC_CFG_EN is set to ‘1’ in _FUNC_CFG_ACCESS (01h)_ . 

**Table 205. Registers address map - embedded functions** 

|**Name**|**Type**|**Registe**|**r address**|**Default**|**Comment**|
|---|---|---|---|---|---|
|||**Hex**|**Binary**|||
|SLV0_ADD|r/w|02|00000010|00000000||
|SLV0_SUBADD|r/w|03|00000011|00000000||
|SLAVE0_CONFIG|r/w|04|00000100|00000000||
|SLV1_ADD|r/w|05|00000101|00000000||
|SLV1_SUBADD|r/w|06|00000110|00000000||
|SLAVE1_CONFIG|r/w|07|00000111|00000000||
|SLV2_ADD|r/w|08|00001000|00000000||
|SLV2_SUBADD|r/w|09|00001001|00000000||
|SLAVE2_CONFIG|r/w|0A|00001010|00000000||
|SLV3_ADD|r/w|0B|00001011|00000000||
|SLV3_SUBADD|r/w|0C|00001100|00000000||
|SLAVE3_CONFIG|r/w|0D|00001101|00000000||
|DATAWRITE_SRC_<br>MODE_SUB_SLV0|r/w|0E|00001110|00000000||
|PEDO_THS_REG|r/w|0F|00001111|00000000||
|RESERVED|r/w|10-12|||Reserved|
|SM_THS|r/w|13|00010011|00000110||
|PEDO_DEB_REG|r/w|14|00010100|00000000||
|STEP_COUNT_DELTA|r/w|15|0001 0101|00000000||
|MAG_SI_XX|r/w|24|00100100|00001000||
|MAG_SI_XY|r/w|25|00100101|00000000||
|MAG_SI_XZ|r/w|26|00100110|00000000||
|MAG_SI_YX|r/w|27|00100111|00000000||
|MAG_SI_YY|r/w|28|00101000|00001000||
|MAG_SI_YZ|r/w|29|00101001|00000000||
|MAG_SI_ZX|r/w|2A|00101010|00000000||
|MAG_SI_ZY|r/w|2B|00101011|00000000||
|MAG_SI_ZZ|r/w|2C|00101100|00001000||



81/99 

DocID026899 Rev 7 

**Embedded functions register mapping** 

**LSM6DS3** 

**Table 205. Registers address map - embedded functions (continued)** 

|**Name**|**Type**|**Registe**<br>**Hex**|**r address**<br>**Binary**|**Default**|**Comment**|
|---|---|---|---|---|---|
|MAG_OFFX_L|r/w|2D|00101101|00000000||
|MAG_OFFX_H|r/w|2E|00101110|00000000||
|MAG_OFFY_L|r/w|2F|00101111|00000000||
|MAG_OFFY_H|r/w|30|00110000|00000000||
|MAG_OFFZ_L|r/w|31|00110001|00000000||
|MAG_OFFZ_H|r/w|32|00110010|00000000||



Registers marked as _Reserved_ must not be changed. Writing to those registers may cause permanent damage to the device. 

The content of the registers that are loaded at boot should not be changed. They contain the factory calibration values. Their content is automatically restored when the device is powered up. 

82/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Embedded functions registers description** 

### **11 Embedded functions registers description** 

#### **11.1 SLV0_ADD (02h)** 

I<sup>2</sup> C slave address of the first external sensor (Sensor1) register (r/w). 

**Table 206. SLV0_ADD register** 

|Slave0_<br>add6|Slave0_<br>add5<br>Slave0_<br>add4|Slave0_<br>add3|Slave0_<br>add2|Slave0_<br>add1|Slave0_<br>add0|rw_0|
|---|---|---|---|---|---|---|
||**Table 207.**|**SLV0_AD**|**D register d**|**escription**|||
|Slave0_add|[6:0]<br>I<sup>2</sup>C slave address<br>Default value: 000|of Sensor1 t<br>0000|hat can be re|ad by sensor|hub.||
|rw_0|Read/write operati<br>(0: write operation|on on Senso<br>; 1: read ope|r1. Default v<br>ration)|alue: 0|||



#### **11.2 SLV0_SUBADD (03h)** 

Address of register on the first external sensor (Sensor1) register (r/w). 

###### **Table 208. SLV0_SUBADD register** 

|Slave0_<br>reg7<br>Slave0_<br>reg6|Slave0_<br>reg5|Slave0_<br>reg4|Slave0_<br>reg3|Slave0_<br>reg2|Slave0_<br>reg1|Slave0_<br>reg0|
|---|---|---|---|---|---|---|
|**Ta**|**ble 209. S**|**LV0_SUBA**|**DD registe**|**r descriptio**|**n**||
|Slave0_reg[7:0]<br>Addre<br>value|ss of registe<br>in_SLV0_AD_|r on Sensor1<br>_D (02h)_. Defa|that has to b<br>ult value: 00|e read/write a<br>000000|ccording to t|he rw_0 bit|



#### **11.3 SLAVE0_CONFIG (04h)** 

First external sensor (Sensor1) configuration and sensor hub settings register (r/w). 

**Table 210. SLAVE0_CONFIG register** 

|Slave0_<br>rate1|Slave0_<br>rate0|Aux_sens<br>_on1|Aux_sens<br>_on0|Src_mode|Slave0_<br>numop2|Slave0_<br>numop1|Slave0_<br>numop0|
|---|---|---|---|---|---|---|---|



83/99 

DocID026899 Rev 7 

**Embedded functions registers description** 

**LSM6DS3** 

###### **Table 211. SLAVE0_CONFIG register description** 



<!-- Start of picture text -->
Decimation of read operation on Sensor1 starting from the sensor hub trigger.<br>Default value: 00<br>(00: no decimation<br>Slave0_rate[1:0]<br>01: update every 2 samples<br>10: update every 4 samples<br>11: update every 8 samples)<br>Number of external sensors to be read by sensor hub. Default value: 00<br>(00: one sensor<br>Aux_sens_on[1:0] 01: two sensors<br>10: three sensors<br>11: four sensors)<br>Source mode conditioned read (1) . Default value: 0<br>Src_mode<br>(0: source mode read disabled; 1: source mode read enabled)<br>Slave0_numop[2:0] Number of read operations on Sensor1.<br><!-- End of picture text -->

1. Read conditioned by the content of the register at address specified in _DATAWRITE_SRC_MODE_SUB_SLV0 (0Eh)_ register. If the content is non-zero the operation continues with the reading of the address specified in teh _SLV0_SUBADD (03h)_ register, else the operation is interrupted. 

#### **11.4 SLV1_ADD (05h)** 

I<sup>2</sup> C slave address of the second external sensor (Sensor2) register (r/w). 

###### **Table 212. SLV1_ADD register** 



<!-- Start of picture text -->
Slave1_ Slave1_ Slave1_ Slave1_ Slave1_ Slave1_ Slave1_<br>r_1<br>add6 add5 add4 add3 add2 add1 add0<br>Table 213. SLV1_ADD register description<br>Slave1_add[6:0] I2C slave address of Sensor2 that can be read by sensor hub.<br>Default value: 0000000<br>Read operation on Sensor2 enable. Default value: 0<br>r_1<br>(0: read operation disabled; 1: read operation enabled)<br><!-- End of picture text -->

#### **11.5 SLV1_SUBADD (06h)** 

Address of register on the second external sensor (Sensor2) register (r/w). 

**Table 214. SLV1_SUBADD register** 



<!-- Start of picture text -->
Slave1_ Slave1_ Slave1_ Slave1_ Slave1_ Slave1_ Slave1_ Slave1_<br>reg7 reg6 reg5 reg4 reg3 reg2 reg1 reg0<br>Table 215. SLV1_SUBADD register description<br>Address of register on Sensor2 that has to be read according to the r_1 bit value<br>Slave1_reg[7:0]<br>in  SLV1_ADD (05h) . Default value: 00000000<br><!-- End of picture text -->

84/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Embedded functions registers description** 

#### **11.6 SLAVE1_CONFIG (07h)** 

Second external sensor (Sensor2) configuration register (r/w). 

###### **Table 216. SLAVE1_CONFIG register** 

|Slave1_|Slave1_|0<sup>(1)</sup>|0<sup>(1)</sup>|0<sup>(1)</sup>|Slave1_|Slave1_|Slave1_|
|---|---|---|---|---|---|---|---|
|rate1|rate0||||numop2|numop1|numop0|



1. This bit must be set to ‘0’ for the correct operation of the device. 

###### **Table 217. SLAVE1_CONFIG register description** 

|Slave1_rate[1:0]|Decimation of read operation on Sensor2 starting from the sensor hub trigger.<br>Default value: 00<br>(00: no decimation<br>01: update every 2 samples|
|---|---|
||10: update every 4 samples|
||11: update every 8 samples)|
|Slave1_numop[2:0]|Number of read operations on Sensor2.|



#### **11.7 SLV2_ADD (08h)** 

I<sup>2</sup> C slave address of the third external sensor (Sensor3) register (r/w). 

**Table 218. SLV2_ADD register** 

|Slave2_<br>add6<br>Slave2_<br>add5|Slave2_<br>add4|Slave2_<br>add3|Slave2_<br>add2|Slave2_<br>add1|Slave2_<br>add0|r_2|
|---|---|---|---|---|---|---|
|Slave2_add[6:0] <sup>I2C sl</sup><br>Defau|**Table 219.**<br><sup>ave address o</sup><br>lt value: 0000|**SLV2_AD**<br><sup>f Sensor3 t</sup><br>000|**D register d**<br><sup>hat can be re</sup>|**escription**<br><sup>ad by sensor</sup>|<sup>hub.</sup>||
|r_2<br>Read<br>(0: re|operation on<br>ad operation d|Sensor3 en<br>isabled; 1: r|able. Default<br>ead operatio|value: 0<br>n enabled)|||



#### **11.8 SLV2_SUBADD (09h)** 

Address of register on the third external sensor (Sensor3) register (r/w). 

**Table 220. SLV2_SUBADD register** 

|Slave2_<br>reg7|Slave2_<br>reg6<br>Slave2_<br>reg5<br>**Table 221. S**|Slave2_<br>reg4<br>**LV2_SUBA**|Slave2_<br>reg3<br>**DD registe**|Slave2_<br>reg2<br>**r descriptio**|Slave2_<br>reg1<br>**n**|Slave2_<br>reg0|
|---|---|---|---|---|---|---|
|Slave2_reg[|7:0]<br>Address of registe<br>in_SLV2_ADD (08h_|r on Sensor3<br>_)_. Default va|that has to b<br>lue: 000000|e read accor<br>00|ding to the r|_2 bit value|



85/99 

DocID026899 Rev 7 

**Embedded functions registers description** 

**LSM6DS3** 

#### **11.9 SLAVE2_CONFIG (0Ah)** 

Third external sensor (Sensor3) configuration register (r/w). 

###### **Table 222. SLAVE2_CONFIG register** 

|Slave2_|Slave2_|0<sup>(1)</sup>|0<sup>(1)</sup>|0<sup>(1)</sup>|Slave2_|Slave2_|Slave2_|
|---|---|---|---|---|---|---|---|
|rate1|rate0||||numop2|numop1|numop0|



1. This bit must be set to ‘0’ for the correct operation of the device. 

###### **Table 223. SLAVE2_CONFIG register description** 

|Slave2_rate[1:0]|Decimation of read operation on Sensor3 starting from the sensor hub trigger.<br>Default value: 00<br>(00: no decimation<br>01: update every 2 samples|
|---|---|
||10: update every 4 samples|
||11: update every 8 samples)|
|Slave2_numop[2:0]|Number of read operations on Sensor3.|



#### **11.10 SLV3_ADD (0Bh)** 

I<sup>2</sup> C slave address of the fourth external sensor (Sensor4) register (r/w). 

**Table 224. SLV3_ADD register** 

|Slave3_<br>add6|Slave3_<br>add5<br>Slave3_<br>add4|Slave3_<br>add3|Slave3_<br>add2|Slave3_<br>add1|Slave3_<br>add0|r_3|
|---|---|---|---|---|---|---|
|Slave3_add|**Table 225.**<br>[6:0]<br>I<sup>2</sup>C slave address<br>Default value: 000|**SLV3_AD**<br>of Sensor4 t<br>0000|**D register d**<br>hat can be re|**escription**<br>ad by the sen|sor hub.||
|r_3|Read operation on<br>(0: read operation|Sensor4 en<br>disabled; 1:|able. Default<br>read operatio|value: 0<br>n enabled)|||



#### **11.11 SLV3_SUBADD (0Ch)** 

Address of register on the fourth external sensor (Sensor4) register (r/w). 

**Table 226. SLV3_SUBADD register** 

|Slave3_|Slave3_|Slave3_|Slave3_|Slave3_|Slave3_|Slave3_|Slave3_|
|---|---|---|---|---|---|---|---|
|reg7|reg6|reg5|reg4|reg3|reg2|reg1|reg0|
||**T**|**able 227. S**|**LV3_SUBA**|**DD registe**|**r descriptio**|**n**||
|Slave3_reg[|7:0]<br>Addre<br>in_SL_|ss of register<br>_V3_ADD (0B_|on Sensor4<br>_h)_. Default va|that has to b<br>lue: 000000|e read accor<br>00|ding to the r|_3 bit value|



86/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Embedded functions registers description** 

#### **11.12 SLAVE3_CONFIG (0Dh)** 

Fourth external sensor (Sensor4) configuration register (r/w). 

###### **Table 228. SLAVE3_CONFIG register** 

|Slave3_|Slave3_|0<sup>(1)</sup>|0<sup>(1)</sup>|0<sup>(1)</sup>|Slave3_|Slave3_|Slave3_|
|---|---|---|---|---|---|---|---|
|rate1|rate0||||numop2|numop1|numop0|



1. This bit must be set to ‘0’ for the correct operation of the device. 

###### **Table 229. SLAVE3_CONFIG register description** 

|Slave3_rate[1:0]|Decimation of read operation on Sensor4 starting from the sensor hub trigger.<br>Default value: 00<br>(00: no decimation<br>01: update every 2 samples|
|---|---|
||10: update every 4 samples|
||11: update every 8 samples)|
|Slave3_numop[2:0]|Number of read operations on Sensor4.|



#### **11.13 DATAWRITE_SRC_MODE_SUB_SLV0 (0Eh)** 

Data to be written into the slave device register (r/w). 

**Table 230. DATAWRITE_SRC_MODE_SUB_SLV0 register** 

|Slave_|Slave_|Slave_|Slave_|Slave_|Slave_|Slave_|Slave_|
|---|---|---|---|---|---|---|---|
|dataw7|dataw6|dataw5|dataw4|dataw3|dataw2|dataw1|dataw0|



###### **Table 231. DATAWRITE_SRC_MODE_SUB_SLV0 register description** 

||Data to be written into the slave device according to the rw_0 bit in_SLV0_ADD_|
|---|---|
|Slave_dataw[7:0]|_(02h)_register or address to be read in source mode.|
||Default value: 00000000|



87/99 

DocID026899 Rev 7 

**Embedded functions registers description** 

**LSM6DS3** 

#### **11.14 PEDO_THS_REG (0Fh)** 

**Table 232. PEDO_THS_REG register default values** 

|PEDO_4G|-<br>-<br>THS_<br>MIN4|THS_<br>MIN3|THS_<br>MIN2|THS_<br>MIN1|THS_<br>MIN0|
|---|---|---|---|---|---|
|PEDO_ 4G|**Table 233. PEDO_THS_R**<br>This bit sets the internal full sca<br>saturation is avoided (e.g. FAS<br>0: internal full scale = 2_g_.<br>1: internal full scale 4_g_(device<br>internal full scale is 2_g_)|**EG regist**<br>le used in p<br>T walk).<br>full_scale @|**er descripti**<br>edometer fun<br>CTRL1_XL|**on**<br>ctions. Using<br>must be ≥ 4_g_|this bit,<br>, otherwise|
|THS_ MIN[4:0]|Configurable minimum threshol<br>@PEDO_4G=1|d. 1LSB = 1|6 m_g_@PED|O_4G=0, 1L|SB = 32 m_g_|



Procedure to modify the pedometer minimum threshold: 

- Write reg FUNC_CFG_ACCESS (01h) = 80h (Enables access to the embedded functions registers) 

- Set min threshold in bits [4:0] of reg 0Fh (1LSB = 16 m _g_ @ FS = 2 _g_ ) 

- Write reg FUNC_CFG_ACCESS (01h) = 00h (Disables access to the embedded functions registers) 

_Note: All modifications of the content of the embedded functions registers have to be performed with the device in power-down mode._ 

#### **11.15 SM_THS (13h)** 

Significant motion configuration register (r/w). 

**Table 234. SM_THS register** 

|SM_THS_<br>7<br>SM_THS_<br>6<br>SM_THS_<br>5|SM_THS_<br>4|SM_THS_<br>3|SM_THS_<br>2|SM_THS_<br>1|SM_THS_<br>0|
|---|---|---|---|---|---|
|**Table 235**|**. SM_THS**|**register de**|**scription**|||
|SM_THS[7:0]<br>Significant moti|on threshold.|Default val|ue: 00000110|||



88/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Embedded functions registers description** 

#### **11.16 PEDO_DEB_REG (14h)** 

**Table 236. PEDO_DEB_REG register default values** 

|DEB_<br>TIME4|DEB_<br>TIME3|DEB_<br>TIME2|DEB_<br>TIME1|DEB_<br>TIME0|DEB_<br>STEP2|DEB_<br>STEP1|DEB_<br>STEP0|
|---|---|---|---|---|---|---|---|
|0|0|0|0|0|1|1|0|



###### **Table 237. PEDO_DEB_REG register description** 

|DEB_ TIME[4:0]|If this time between steps is greater than DEB_TIME*80ms, the debouncer is<br>reactivated|
|---|---|
|DEB_ STEP[2:0]|Minimum number of steps to increment step counter (debouncer)|



The procedure to modify the pedometer debounce time is as follows: 

- Write reg FUNC_CFG_ACCESS (01h) = 80h (Enables access to the embedded functions) 

- Set debounce time in bits [3:7] of reg 14h (1LSB = 80 ms.This value must be > 0) 

- Write reg FUNC_CFG_ACCESS (01h) = 00h (Disables access to the embedded functions registers) 

_Note: All modifications of the content of the embedded functions registers have to be performed with the device in power-down mode._ 

89/99 

DocID026899 Rev 7 

**Embedded functions registers description** 

**LSM6DS3** 

#### **11.17 STEP_COUNT_DELTA (15h)** 

Time period register for step detection on delta time (r/w). 

**Table 238. STEP_COUNT_DELTA register** 

|SC_<br>DELTA_7|SC_<br>DELTA_6|SC_<br>DELTA_5|SC_<br>DELTA_4|SC_<br>DELTA_3|SC_<br>DELTA_2|SC_<br>DELTA_1|SC_<br>DELTA_0|
|---|---|---|---|---|---|---|---|
|SC_DELTA|**Table**<br>[7:0]<br>Ti|**239. STEP**<br>me period val|**_COUNT_**<br>ue<sup>(1)</sup>(1LSB|**DELTA regi**<br>= 1.6384 s)|**ster descri**|**ption**||



1. This value is effective if the TIMER_EN bit of the TAP_CFG register is set to 1 and the TIMER_HR bit of the WAKE_UP_DUR register is set to 0. 

#### **11.18 MAG_SI_XX (24h)** 

Soft-iron matrix correction register (r/w). 

**Table 240. MAG_SI_XX register** 

|MAG_SI_<br>XX_7<br>MAG_SI_<br>XX_6<br>MAG_SI_<br>XX_5<br>**Table 241.**|MAG_SI_<br>XX_4<br>**MAG_SI_X**|MAG_SI_<br>XX_3<br>**X register d**|MAG_SI_<br>XX_2<br>**escription**|MAG_SI_<br>XX_1|MAG_SI_<br>XX_0|
|---|---|---|---|---|---|
|MAG_SI_XX_[7:0]<br>Soft-iron correc|tion row1 col|1 coefficient<sup>(</sup>|<sup>1)</sup>. Default va|lue: 000010|00|



1. Value is expressed in sign-module format. 

#### **11.19 MAG_SI_XY (25h)** 

Soft-iron matrix correction register (r/w). 

**Table 242. MAG_SI_XY register** 

|MAG<br>XY|_SI_<br>_7|MAG_<br>XY_|SI_<br>6<br>MAG_SI_<br>XY_5|MAG_SI_<br>XY_4|MAG_SI_<br>XY_3|MAG_SI_<br>XY_2|MAG_SI_<br>XY_1|MAG_SI_<br>XY_0|
|---|---|---|---|---|---|---|---|---|
||||**Table 243.**|**MAG_SI_X**|**Y register**|**description**|||
|MAG_|SI_XY|_[7:0]|Soft-iron correct|ion row1 col2|coefficient<sup>(1</sup>|<sup>)</sup>. Default valu|e: 0000000|0|



1. Value is expressed in sign-module format. 

#### **11.20 MAG_SI_XZ (26h)** 

Soft-iron matrix correction register (r/w). 

###### **Table 244. MAG_SI_XZ register** 

|MAG_SI_|MAG_SI_<br>MAG_SI_|MAG_SI_<br>MAG_SI_|MAG_SI_|MAG_SI_|MAG_SI_|
|---|---|---|---|---|---|
|XZ_7|XZ_6<br>XZ_5|XZ_4<br>XZ_3|XZ_2|XZ_1|XZ_0|
||**Table 245.**|**MAG_SI_XZ register d**|**escription**|||
|MAG_SI_XZ|_[7:0]<br>Soft-iron corre|ction row1 col3 coefficient<sup>(</sup>|<sup>1)</sup>. Default va|lue: 00000|000|



1. Value is expressed in sign-module format. 

90/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Embedded functions registers description** 

#### **11.21 MAG_SI_YX (27h)** 

Soft-iron matrix correction register (r/w). 

**Table 246. MAG_SI_YX register** 

|MAG_SI_<br>YX_7<br>MAG_SI_<br>YX_6<br>MAG_SI_<br>YX_5|MAG_SI_<br>YX_4|MAG_SI_<br>YX_3|MAG_SI_<br>YX_2|MAG_SI_<br>YX_1|MAG_SI_<br>YX_0|
|---|---|---|---|---|---|
|**Table 247.**|**MAG_SI_Y**|**X register d**|**escription**|||
|MAG_SI_YX_[7:0]<br>Soft-iron correc|tion row2 col|1 coefficient<sup>(</sup>|<sup>1)</sup>. Default va|lue: 00000|000|



1. Value is expressed in sign-module format. 

#### **11.22 MAG_SI_YY (28h)** 

Soft-iron matrix correction register (r/w). 

**Table 248. MAG_SI_YY register** 

|MAG_<br>YY_|SI_<br>7<br>MAG_<br>YY_|SI_<br>6<br>MAG_SI_<br>YY_5|MAG_SI_<br>YY_4|MAG_SI_<br>YY_3|MAG_SI_<br>YY_2|MAG_SI_<br>YY_1|MAG_SI_<br>YY_0|
|---|---|---|---|---|---|---|---|
|||**Table 249.**|**MAG_SI_Y**|**Y register**|**description**|||
|MAG_|SI_YY_[7:0]|Soft-iron correc|tion row2 col|2 coefficient|<sup>(1)</sup>. Default va|lue: 000010|00|



1. Value is expressed in sign-module format. 

#### **11.23 MAG_SI_YZ (29h)** 

Soft-iron matrix correction register (r/w). 

**Table 250. MAG_SI_YZ register** 

|MAG_SI_<br>YZ_7<br>MAG_SI_<br>YZ_6<br>MAG_SI_<br>YZ_5|MAG_SI_<br>YZ_4<br>MAG_SI_<br>YZ_3<br>MAG_SI_<br>YZ_2|MAG_SI_<br>YZ_1|MAG_SI_<br>YZ_0|
|---|---|---|---|
|**Table 251.**|**MAG_SI_YZ register description**|||
|MAG_SI_YZ_[7:0]<br>Soft-iron corre|ction row2 col3 coefficient<sup>(1)</sup>. Default v|alue: 00000|000|



1. Value is expressed in sign-module format. 

#### **11.24 MAG_SI_ZX (2Ah)** 

Soft-iron matrix correction register (r/w). 

###### **Table 252. MAG_SI_ZX register** 

|MAG_SI_<br>ZX_7<br>MAG_<br>ZX_|SI_<br>6<br>MAG_SI_<br>ZX_5|MAG_SI_<br>ZX_4|MAG_SI_<br>ZX_3|MAG_SI_<br>ZX_2|MAG_SI_<br>ZX_1|MAG_SI_<br>ZX_0|
|---|---|---|---|---|---|---|
||**Table 253.**|**MAG_SI_Z**|**X register**|**description**|||
|MAG_SI_ZX_[7:0]|Soft-iron correct|ion row3 col1|coefficient<sup>(</sup>|<sup>1)</sup>. Default val|ue: 0000000|0|



1. Value is expressed in sign-module format. 

91/99 

DocID026899 Rev 7 

**Embedded functions registers description** 

**LSM6DS3** 

#### **11.25 MAG_SI_ZY (2Bh)** 

Soft-iron matrix correction register (r/w). 

**Table 254. MAG_SI_ZY register** 

MAG_SI_ MAG_SI_ MAG_SI_ MAG_SI_ MAG_SI_ MAG_SI_ MAG_SI_ MAG_SI_ ZY_7 ZY_6 ZY_5 ZY_4 ZY_3 ZY_2 ZY_1 ZY_0 **Table 255. MAG_SI_ZY register description** MAG_SI_ZY_[7:0] Soft-iron correction row3 col2 coefficient<sup>(1)</sup> . Default value: 00000000 

1. Value is expressed in sign-module format. 

#### **11.26 MAG_SI_ZZ (2Ch)** 

Soft-iron matrix correction register (r/w). 

**Table 256. MAG_SI_ZZ register** 

|MAG_SI_<br>ZZ_7|MAG_SI_<br>ZZ_6<br>MAG_SI_<br>ZZ_5|MAG_SI_<br>ZZ_4|MAG_SI_<br>ZZ_3|MAG_SI_<br>ZZ_2|MAG_SI_<br>ZZ_1|MAG_SI_<br>ZZ_0|
|---|---|---|---|---|---|---|
||**Table 257.**|**MAG_SI_Z**|**Z register**|**description**|||
|MAG_SI_ZZ|_[7:0]<br>Soft-iron correct|ion row3 col|3 coefficient<sup>(</sup>|<sup>1)</sup>. Default val|ue: 0000100|0|



1. Value is expressed in sign-module format. 

#### **11.27 MAG_OFFX_L (2Dh)** 

Offset for X-axis hard-iron compensation register (r/w). The value is expressed as a 16-bit word in two’s complement. 

###### **Table 258. MAG_OFFX_L register** 

MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF X_L_7 X_L_6 X_L_5 X_L_4 X_L_3 X_L_2 X_L_1 X_L_0 **Table 259. MAG_OFFX_L register description** MAG_OFFX_L_[7:0] Offset for X-axis hard-iron compensation. Default value: 00000000 

#### **11.28 MAG_OFFX_H (2Eh)** 

Offset for X-axis hard-iron compensation register (r/w).The value is expressed as a 16-bit word in two’s complement. 

**Table 260. MAG_OFFX_H register** 

MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF X_H_7 X_H_6 X_H_5 X_H_4 X_H_3 X_H_2 X_H_1 X_H_0 **Table 261. MAG_OFFX_L register description** MAG_OFFX_H_[7:0] Offset for X-axis hard-iron compensation. Default value: 00000000 

92/99 

DocID026899 Rev 7 

**LSM6DS3** 

**Embedded functions registers description** 

#### **11.29 MAG_OFFY_L (2Fh)** 

Offset for Y-axis hard-iron compensation register (r/w). The value is expressed as a 16-bit word in two’s complement. 

**Table 262. MAG_OFFY_L register** 

MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF Y_L_7 Y_L_6 Y_L_5 Y_L_4 Y_L_3 Y_L_2 Y_L_1 Y_L_0 **Table 263. MAG_OFFY_L register description** MAG_OFFY_L_[7:0] Offset for Y-axis hard-iron compensation. Default value: 00000000 

#### **11.30 MAG_OFFY_H (30h)** 

Offset for Y-axis hard-iron compensation register (r/w). The value is expressed as a 16-bit word in two’s complement. 

**Table 264. MAG_OFFY_H register** 

MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF Y_H_7 Y_H_6 Y_H_5 Y_H_4 Y_H_3 Y_H_2 Y_H_1 Y_H_0 

###### **Table 265. MAG_OFFY_L register description** 

MAG_OFFY_H_[7:0] Offset for Y-axis hard-iron compensation. Default value: 00000000 

#### **11.31 MAG_OFFZ_L (31h)** 

Offset for Z-axis hard-iron compensation register (r/w). The value is expressed as a 16-bit word in two’s complement. 

###### **Table 266. MAG_OFFZ_L register** 

MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF Z_L_7 Z_L_6 Z_L_5 Z_L_4 Z_L_3 Z_L_2 Z_L_1 Z_L_0 

###### **Table 267. MAG_OFFZ_L register description** 

MAG_OFFZ_L_[7:0] Offset for Z-axis hard-iron compensation. Default value: 00000000 

#### **11.32 MAG_OFFZ_H (32h)** 

Offset for Z-axis hard-iron compensation register (r/w). The value is expressed as a 16-bit word in two’s complement. 

**Table 268. MAG_OFFZ_H register** 

MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF MAG_OFF Z_H_7 Z_H_6 Z_H_5 Z_H_4 Z_H_3 Z_H_2 Z_H_1 Z_H_0 **Table 269. MAG_OFFX_L register description** MAG_OFFZ_H_[7:0] Offset for Z-axis hard-iron compensation. Default value: 00000000 

93/99 

DocID026899 Rev 7 

**Soldering information** 

**LSM6DS3** 

### **12 Soldering information** 

The LGA package is compliant with the ECOPACK<sup>®</sup> , RoHS and "Green" standard. It is qualified for soldering heat resistance according to JEDEC J-STD-020. 

Leave "Pin 1 Indicator" unconnected during soldering. 

Land pattern and soldering recommendations are available at www.st.com/mems. 

94/99 

DocID026899 Rev 7 



<!-- Start of picture text -->
Pin 1 indica t or<br>Dimensions are in millimeter unless otherwise specified<br>General tolerance is +/-0.1mm unless otherwise specified<br>OUTER DIMENSIONS<br>Length (L]<br>Width<br>Height [H]<br>8562052_6<br><!-- End of picture text -->



<!-- Start of picture text -->
T — P2 Po EI<br>0.3020.05 @1.50 sco 2.0029.05{! 4,900.10) 1.75+0.10<br>i |<br>!} yy ; a Bt i-<br>| VY 4 _<br>| DI<br>} © 1.50 MIN<br>! yy i‘ ed<br>Ko<br>Ao<br>SECTION Y-Y ~~<br>22> ° ° =]<br>> ° °<br>> a8<br>=r &oe °<br>[|| AoBo || 32 . 8030 «0.0+ -. 0.0 5 | (!)(ll) CumulativeMeasuredto centreline of from toleranc centreline pock e t.of 10 of sprocket sprocket hole<br>[ Ko | 1.00 +/-0.10 (ll) Measuredholes is +0.20. from centreline of sprocket<br>6.50 +«-0.05 hole to centreline of pocket.<br>[| PrW || 1206.0 0 +40.30n 0.10 FormingMequiedA format: tengite Press 170 speter form 1 - 2203,00! 17-B ALL(IV) Other DIMENSIONS materialIN available. MILLEMETRES UNLESS OTHERWISE STATED.<br><!-- End of picture text -->



<!-- Start of picture text -->
User Direction of Feed<br><!-- End of picture text -->



<!-- Start of picture text -->
40mm min,<br>Access hole at<br>siot location °T<br>r<br>|<br>|<br>naar ,<br>A b aS en anne<br>Full radius ~ Tape slot G measured at hub<br>in core for<br>ne tane start<br>2.5mm min width<br><!-- End of picture text -->

<mark>ky</mark> 

**LSM6DS3** 

**Revision history** 

### **14 Revision history** 

**Table 271. Document revision history** 

|**Date**|**Revision**|**Changes**|
|---|---|---|
|03-Nov-2014|1|Initial release|
|18-Dec-2014|2|Updated_Section 2: Embedded low-power features_and subsection<br>Updated_Section 5.4: FIFO_and subsections<br>Added_Section 5.4.7: Filter block diagrams_<br>Updated IddLP in_Table 4_ and TODR in_Table 5: Temperature sensor characteristics_<br>Updated_Table 16: Registers address map_<br>Revised registers in_Section 9: Register description_<br>Updated_Table 205: Registers address map - embedded functions_<br>Revised registers in_Section 11: Embedded functions registers description_<br>Textual update in_Figure 16: LGA-14 2.5x3x0.86 mm 14L package outline and_<br>_mechanical data_|
|05-Mar-2015|3|Document status promoted from preliminary to production data<br>Updated bit 0 in_Section 9.79: MD2_CFG (5Fh)_|
|23-Apr-2015|4|Updated Vdd_IO (max) in_Table 4: Electrical characteristics_<br>Added D4D_EN bit to_Section 9.73: TAP_THS_6D (59h)_<br>Updated_Table 181: INT-DUR2 register description_|
|06-May-2015|5|Updated direction of rotation of Y-axis in_Figure 1: Pin connections_<br>Updated_Table 68: Accelerometer slope and high-pass filter selection and cutoff_<br>_frequency_|
|16-Jul-2015|6|Updated chamfer of pin 1 indicator in_Figure 1, Figure 13, Figure 14, Figure 15_<br>Added footnote_2_to_Table 3: Mechanical characteristics_<br>Updated recommendation to set BDU bit to 1 (_CTRL3_C (12h)_) in_Section 9.52:_<br>_FIFO_STATUS1 (3Ah)_through_Section 9.57: FIFO_DATA_OUT_H (3Fh)_<br>Updated_Figure 16: LGA-14 2.5x3x0.86 mm 14L package outline and mechanical data_|
|09-Oct-2015|7|Updated_Figure 5: Accelerometer composite filter_and_Figure 16: LGA-14 2.5x3x0.86_<br>_mm 14L package outline and mechanical data_<br>Updated description of HPCF_XL bits in_Table 67: CTRL8_XL register description_<br>Added_Table 69: Accelerometer LPF2 cutoff frequency_<br>Added_PEDO_THS_REG (0Fh)_and_PEDO_DEB_REG (14h)_<br>Added_Section 13.2: LGA-14 packing information_|



98/99 

DocID026899 Rev 7 

**LSM6DS3** 

###### **IMPORTANT NOTICE – PLEASE READ CAREFULLY** 

STMicroelectronics NV and its subsidiaries (“ST”) reserve the right to make changes, corrections, enhancements, modifications, and improvements to ST products and/or to this document at any time without notice. Purchasers should obtain the latest relevant information on ST products before placing orders. ST products are sold pursuant to ST’s terms and conditions of sale in place at the time of order acknowledgement. 

Purchasers are solely responsible for the choice, selection, and use of ST products and ST assumes no liability for application assistance or the design of Purchasers’ products. 

No license, express or implied, to any intellectual property right is granted by ST herein. 

Resale of ST products with provisions different from the information set forth herein shall void any warranty granted by ST for such product. ST and the ST logo are trademarks of ST. All other product or service names are the property of their respective owners. Information in this document supersedes and replaces information previously supplied in any prior versions of this document. 

© 2015 STMicroelectronics – All rights reserved 

99/99 

DocID026899 Rev 7 


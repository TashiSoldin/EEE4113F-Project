# EEE4113F-Project

This ReadMe overviews and explains the software developed for group 17's bird weight measurement EEE4113F project. 

The system that this software operates was designed specifically to measure the weight of Kalaharian drongos, while providing a clean and effective user interface via a webserver. 

The following files make up this repository:

- The Full_Sketch.ino file provides the final code that was uploaded and run on the ESP32, including the data processing algorithm, temperature and load cell reading, and webserver and communication handling. As a .ino script, it should be run through the ArduinoIDE with the necessary libraries installed. Please Note that the development board used for this project was an ESP32-TTGO, so pinouts etc. defined in the code may require editing if another board is used. 

- The '/Data Processing' folder contains algorithms used for the testing of the data processing algorithm, before a final version was employed in the Full_Sketch.ino file. This folder includes:
  * KalmanFilter.py (contains an implementation of a kalman filter to be applied to collected weight measurement data)
  * TailoredAlgorithm.py (contains an implementation of a tailered Python algorithm to extract weight data modally)
  * Testing.IPYNB (This is a jupyter notebook file that tests the other Python files in this folder)

- LICENSE defines the license for the project.

<img width="753" alt="image" src="https://github.com/TashiSoldin/EEE4113F-Project/assets/75614710/03595e1b-d2c4-4455-b6fd-44221c70834b">

Copyright ©, 2023

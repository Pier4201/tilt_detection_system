//Includes the header file for this class
#include "MPU6050.h"

MPU6050::MPU6050(int8_t addr, bool run_update_thread) {
	int status;

	MPU6050_addr = addr;
	dt = 0.009; //Loop time (recalculated with each loop)
	_first_run = 1; //Variable for whether to set gyro angle to acceleration angle in compFilter
	calc_yaw = false;

	f_dev = open("/dev/i2c-1", O_RDWR); //Open the I2C device file
	if (f_dev < 0) { //Catch errors
		std::cerr << "ERR (MPU6050.cpp:MPU6050()): Failed to open /dev/i2c-1. Please check that I2C is enabled using raspi-config\n"; //Print error message
                exit(EXIT_FAILURE);
	}

	status = ioctl(f_dev, I2C_SLAVE, MPU6050_addr); //Set the I2C bus to use the correct address
	if (status < 0) {
		std::cout << "ERR (MPU6050.cpp:MPU6050()): Could not get I2C bus with " << addr << " address. Please confirm that this address is correct\n"; //Print error message
	}

	i2c_smbus_write_byte_data(f_dev, 0x6b, 0b00000000); //Signals MPU6050 to exit sleep mode
	i2c_smbus_write_byte_data(f_dev, 0x1a, 0b00000011); //Sets the low pass filter to 44Hz
	i2c_smbus_write_byte_data(f_dev, 0x19, 0b00000100); //Sets the sample rate divider to 200Hz - see Register Map
	i2c_smbus_write_byte_data(f_dev, 0x1b, GYRO_CONFIG); //Gyro settings configuration - see Register Map (see MPU6050.h for the GYRO_CONFIG parameters)
	i2c_smbus_write_byte_data(f_dev, 0x1c, ACCEL_CONFIG); //Accel settings configuration - see Register Map (see MPU6050.h for the ACCEL_CONFIG parameters)

	//Sets offset to zero
	i2c_smbus_write_byte_data(f_dev, 0x06, 0b00000000);
	i2c_smbus_write_byte_data(f_dev, 0x07, 0b00000000);
	i2c_smbus_write_byte_data(f_dev, 0x08, 0b00000000);
	i2c_smbus_write_byte_data(f_dev, 0x09, 0b00000000);
	i2c_smbus_write_byte_data(f_dev, 0x0A, 0b00000000);
	i2c_smbus_write_byte_data(f_dev, 0x0B, 0b00000000);
	i2c_smbus_write_byte_data(f_dev, 0x00, 0b10000001);
	i2c_smbus_write_byte_data(f_dev, 0x01, 0b00000001);
	i2c_smbus_write_byte_data(f_dev, 0x02, 0b10000001);

	//Starts a separate thread for the update routine to run while the program continues
	if (run_update_thread){
		std::thread(&MPU6050::_update, this).detach();
	}
}

MPU6050::MPU6050(int8_t addr) : MPU6050(addr, true){}

void MPU6050::getGyroRaw(float *roll, float *pitch, float *yaw) {
	int16_t X = i2c_smbus_read_byte_data(f_dev, 0x43) << 8 | i2c_smbus_read_byte_data(f_dev, 0x44); //Read X registers
	int16_t Y = i2c_smbus_read_byte_data(f_dev, 0x45) << 8 | i2c_smbus_read_byte_data(f_dev, 0x46); //Read Y registers
	int16_t Z = i2c_smbus_read_byte_data(f_dev, 0x47) << 8 | i2c_smbus_read_byte_data(f_dev, 0x48); //Read Z registers
	*roll = (float)X; //Roll on X axis
	*pitch = (float)Y; //Pitch on Y axis
	*yaw = (float)Z; //Yaw on Z axis
}

void MPU6050::getGyro(float *roll, float *pitch, float *yaw) {
	getGyroRaw(roll, pitch, yaw); //Store raw values into variables
	*roll = round((*roll - gyroOffsetX) * 1000.0 / GYRO_SENS) / 1000.0; //Remove the offset and divide by the gyroscope sensetivity (use 1000 and round() to round the value to three decimal places)
	*pitch = round((*pitch - gyroOffsetY) * 1000.0 / GYRO_SENS) / 1000.0;
	*yaw = round((*yaw - gyroOffsetZ) * 1000.0 / GYRO_SENS) / 1000.0;
}

void MPU6050::getAccelRaw(float *x, float *y, float *z) {
	int16_t X = i2c_smbus_read_byte_data(f_dev, 0x3b) << 8 | i2c_smbus_read_byte_data(f_dev, 0x3c); //Read X registers
	int16_t Y = i2c_smbus_read_byte_data(f_dev, 0x3d) << 8 | i2c_smbus_read_byte_data(f_dev, 0x3e); //Read Y registers
	int16_t Z = i2c_smbus_read_byte_data(f_dev, 0x3f) << 8 | i2c_smbus_read_byte_data(f_dev, 0x40); //Read Z registers
	*x = (float)X;
	*y = (float)Y;
	*z = (float)Z;
}

void MPU6050::getAccel(float *x, float *y, float *z) {
	getAccelRaw(x, y, z); //Store raw values into variables
	*x = round((*x - accelOffsetX) * 1000.0 / ACCEL_SENS) / 1000.0; //Removes the calculated offset value and divides by 1000 (use 1000 and round() to round the value to three decimal places)
	*y = round((*y - accelOffsetY) * 1000.0 / ACCEL_SENS) / 1000.0;
	*z = round((*z - accelOffsetZ) * 1000.0 / ACCEL_SENS) / 1000.0;
}

void MPU6050::getOffsets(float *ax_off, float *ay_off, float *az_off, float *gr_off, float *gp_off, float *gy_off) {
    std::cout << "Starting sensor calibration...\n\n";

    //float accel_off[3] = {0, 0, 0}; 
    //float gyro_off[3] = {0, 0, 0};
    const int samples = 10000;

    *gr_off = 0; *gp_off = 0; *gy_off = 0;
    *ax_off = 0; *ay_off = 0; *az_off = 0;

    for (int i = 0; i < samples; i++) { 
        float tempAx, tempAy, tempAz, tempGr, tempGp, tempGy;
        getGyroRaw(&tempGr, &tempGp, &tempGy);
        getAccelRaw(&tempAx, &tempAy, &tempAz);

        *gr_off += tempGr;
        *gp_off += tempGp;
        *gy_off += tempGy;
        *ax_off += tempAx;
        *ay_off += tempAy;
        *az_off += tempAz;
    }

    // Average of the calculated offset values to improve accountability
    *gr_off /= samples;
    *gp_off /= samples;
    *gy_off /= samples;
    *ax_off /= samples;
    *ay_off /= samples;
    *az_off = (*az_off / samples) - ACCEL_SENS; // compensating gravity

    // Saves accel and gyro offset values
    gyroOffsetX = *gr_off;
    gyroOffsetY = *gp_off;
    gyroOffsetZ = *gy_off;
	accelOffsetX = *ax_off;
    accelOffsetY = *ay_off;
    accelOffsetZ = *az_off;


    std::cout << "OFFSET CALCULATED:\n";
	std::cout << "Accelerometer X, Y, Z: " << accelOffsetX << ", " << accelOffsetY << ", " << accelOffsetZ << "\n";
    std::cout << "Gyroscope X, Y, Z: " << gyroOffsetX << ", " << gyroOffsetY << ", " << gyroOffsetZ << "\n\n";
    
}


int MPU6050::getAngle(int axis, float *result) {
	if (axis >= 0 && axis <= 2) { //Check that the axis is in the valid range
		*result = _angle[axis]; //Get the result
		return 0;
	}
	else {
		std::cout << "ERR (MPU6050.cpp:getAngle()): 'axis' must be between 0 and 2 (for roll, pitch or yaw)\n"; //Print error message
		*result = 0; //Set result to zero
		return 1;
	}
}

void MPU6050::_update() { //Main update function - runs continuously
	clock_gettime(CLOCK_REALTIME, &start); //Read current time into start variable

	while (1) { //Loop forever
		getGyro(&gr, &gp, &gy); //Get the data from the sensors
		getAccel(&ax, &ay, &az);

		//X (roll) axis
		_accel_angle[0] = atan2(ay, az) * RAD_T_DEG; //(tolto -90)Calculate the angle with z and y convert to degrees and subtract 90 degrees to rotate
		_gyro_angle[0] = _angle[0] + gr*dt; //Use roll axis (X axis)

		//Y (pitch) axis
		_accel_angle[1] = atan2(-ax, sqrt(ay * ay + az * az)) * RAD_T_DEG; //(tolto -90 me aggiornato formula) Calculate the angle with z and x convert to degrees and subtract 90 degrees to rotate
		_gyro_angle[1] = _angle[1] + gp*dt; //Use pitch axis (Y axis)

		//Z (yaw) axis
		if (calc_yaw) {
			_gyro_angle[2] = _angle[2] + gy*dt; //Use yaw axis (Z axis)
		}


		if (_first_run) { //Set the gyroscope angle reference point if this is the first function run
			for (int i = 0; i <= 1; i++) {
				_gyro_angle[i] = _accel_angle[i]; //Start off with angle from accelerometer (absolute angle since gyroscope is relative)
			_angle[i] = _accel_angle[i]; //Set the angle to the accelerometer angle
			}
			_gyro_angle[2] = 0; //Set the yaw axis to zero (because the angle cannot be calculated with the accelerometer when vertical)
			_angle[2] = 0; //Set the yaw axis to zero (because the angle cannot be calculated with the accelerometer when vertical)
			_first_run = 0;
		}

		/*float asum = abs(ax) + abs(ay) + abs(az); //Calculate the sum of the accelerations
		float gsum = abs(gr) + abs(gp) + abs(gy); //Calculate the sum of the gyro readings
		*/

		for (int i = 0; i <= 1; i++) { //Loop through roll and pitch axes
			/*if (abs(_gyro_angle[i] - _accel_angle[i]) > 5) { //Correct for very large drift (or incorrect measurment of gyroscope by longer loop time)
				_gyro_angle[i] = _accel_angle[i];
			}

			//Create result from either complementary filter or directly from gyroscope or accelerometer depending on conditions
			if (asum > 0.1 && asum < 3 && gsum > 0.3) { //Check that th movement is not very high (therefore providing inacurate angles)
				_angle[i] = (1 - TAU)*(_gyro_angle[i]) + (TAU)*(_accel_angle[i]); //Calculate the angle using a complementary filter
			}
			else if (gsum > 0.3) { //Use the gyroscope angle if the acceleration is high
				_angle[i] = _gyro_angle[i];
			}
			else  { //Use accelerometer angle if not much movement
				_angle[i] = _accel_angle[i];
			}*/
			_angle[i] = (1 - TAU) * _gyro_angle[i] + TAU * _accel_angle[i];
		}

		//The yaw axis will not work with the accelerometer angle, so only use gyroscope angle
		if (calc_yaw) { //Only calculate the angle when we want it to prevent large drift
			_angle[2] = _gyro_angle[2];
		}
		else {
			_angle[2] = 0;
			_gyro_angle[2] = 0;
		}

		clock_gettime(CLOCK_REALTIME, &end); //Save time to end clock
		dt = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9; //Calculate new dt
		clock_gettime(CLOCK_REALTIME, &start); //Save time to start clock
	}
}







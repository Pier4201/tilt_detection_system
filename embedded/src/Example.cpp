#include "MPU6050.h"
#include "DataSender.h"
#include <string>
#include <signal.h>

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {

    keep_running = 0;  // Set flag to 0 to end cycle
}

MPU6050 device(0x68);

int main() {
	float ax, ay, az, gr, gp, gy, gyroOffsetX, gyroOffsetY, gyroOffsetZ, roll, pitch, yaw; //Variables to store accelerometer gyroscope and angles values
	bool allarme;
	float lastRoll, lastPitch, lastYaw;
	unsigned int time, counter=0;
	std::string infrastruttura;
	sleep(1); //we wait 1 sec for the MPU6050 to stabilize
    

	//Call to dinamically measure drifting values
	std::cout << "Measuring offsets.\nKeep the device still and horizontal, this process might take some minutes.\n";
	device.getOffsets(&ax, &ay, &az, &gr, &gp, &gy);


	//Reads current yaw angle.
	device.calc_yaw = true;
        //inclination value
	float inclination_x, inclination_y;
	
	for (int i = 0; i < 40; i++) {

	 // Accelerometer readings
	 device.getAccel(&ax, &ay, &az);
	 if(i>=1){
	 std::cout << "Accelerstion in g on axys (X,Y,Z): " << ax << ", " << ay << ", " << az << "\n";
	    }
     device.getGyro(&gr, &gp, &gy);
	 // gyroscope readings 
	 if(i>=1){
	 std::cout << "Angular velocity around axys:\nX: " << gr << "°/s, Y: " << gp << "°/s, Z: " << gy << "°/s\n";
	    }

	 //  accelerometer only incline (for debug purposes only, they wont be sent to server) 
	 inclination_x = atan2(ay, az) * 180 / M_PI;
	 inclination_y = atan2(-ax, sqrt(ay * ay + az * az)) * 180 / M_PI;
	 if(i>=1){
	 std::cout << "[DEBUG] Inclinazione solo accelerometro --> X: " << inclination_x 
			   << "°, Y: " << inclination_y << "°\n";
	    }

    // Filtered angles (Roll, Pitch, Yaw)
	
	 device.getAngle(0, &roll); // Roll
	 device.getAngle(1, &pitch); // Pitch
	 device.getAngle(2, &yaw); // Yaw

	 if(i>=1){
	 //Effective incline: 
	 std::cout << "Incline on axys X: " << roll << "°, Y: " << pitch << "°, Z: " << yaw << "°\n\n\n";
	    }
	 usleep(750000); // 0.75 secs
	}
    
	std::cout << "Insert the name of infrastructure we are monitoring: ";
    std::cin >> infrastruttura;
    /*getAccel measures acceleration in g 
        X axys will be positive if tilted right, negative if tilted left.
        Y axys will be positive if tilted onwards, negative if tilted backwards.
        Z will always measure 1 cause it in line with our gravity.*/
		device.getAccel(&ax, &ay, &az);
		std::cout << "Accelerometer values (in g): X: " << ax << ", Y: " << ay << ", Z: " << az << "\n";
   
		/*getGyro measures the angular velocity at whitch our sensor rotates.
			X axys indicates onwards and backwards rotation.
			Y axys indicates left and right rotation.
			Z axys indicates if the device is rotating on itself. */
		device.getGyro(&gr, &gp, &gy);
		std::cout << "Gyroscope values (in °/s):\nX: " << gr << ", Y: " << gp << ", Z: " << gy << "\n";
   
		device.getAngle(0, &roll); // Roll
		device.getAngle(1, &pitch); // Pitch
		device.getAngle(2, &yaw); // Yaw
		if(abs(roll) > 1.5 || abs(pitch) > 1.5){
			allarme = true;
		} else {
			allarme = false;
		}
	   
		std::cout << "Final angles:\nX: " << roll << ", Y: " << pitch << ", Z: " << yaw << "\n";
		sendDataToServer(infrastruttura, ax, ay, az, gr, gp, gy, roll, pitch, yaw, allarme);

    signal(SIGINT, handle_sigint); //handles SIGINT command to manually exit loop
    
    while(keep_running){
	    
	    device.getAccel(&ax, &ay, &az);
	    std::cout << "Accelerometer values (in g): X: " << ax << ", Y: " << ay << ", Z: " << az << "\n";

	    
	    device.getGyro(&gr, &gp, &gy);
	    std::cout << "Gyroscope values (angular velocity):\nX: " << gr << ", Y: " << gp << ", Z: " << gy << "\n";

	    device.getAngle(0, &roll); // Roll
	    device.getAngle(1, &pitch); // Pitch
	    device.getAngle(2, &yaw); // Yaw
        if(abs(roll) > 1.5 || abs(pitch) > 1.5){
		     allarme = true;
	    } else {
		     allarme = false;
	    }
	if((lastRoll!=0 && lastPitch!=0 && lastYaw!=0) && (abs(lastRoll-roll)>2 || abs(lastPitch-pitch)>2 || abs(lastYaw-yaw)>2)){
		lastRoll=lastPitch=lastYaw=0;	
		break; //break loop and sends last data if there's a sudden incline change
	}
	if(allarme==true && counter%15000==0 && counter!=450000){ //if alarm is true sends data more frequently
		sendDataToServer(infrastruttura,ax,ay,az,gr,gp,gy,roll,pitch,yaw,allarme);
	}
	lastRoll=roll;
	lastPitch=pitch;
	lastYaw=yaw;
	counter++;
	if(counter==450000){ //sends data to server every 450000 cycles
		sendDataToServer(infrastruttura, ax, ay, az, gr, gp, gy, roll, pitch, yaw, allarme);
		counter=0;
	}
	usleep(750000);
		
	std::cout << "Final angles:\nX: " << roll << ", Y: " << pitch << ", Z: " << yaw << "\n";
	std::cout <<"\nPRESS CTRL+C TO MANUALLY END PROCESS.\n";	
   }
   sendDataToServer(infrastruttura, ax, ay, az, gr, gp, gy, roll, pitch, yaw, allarme);
   std::cout <<"Monitoring interrupted, last data sent to server.\n";
   return 0;
}


#include "MPU6050.h"
#include "DataSender.h"
#include <string>
#include <signal.h>

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {

    keep_running = 0;  // Imposto il flag a 0 per terminare il ciclo
}

MPU6050 device(0x68);

int main() {
	float ax, ay, az, gr, gp, gy, gyroOffsetX, gyroOffsetY, gyroOffsetZ, roll, pitch, yaw; //Variabili per conservare i dati dell'acceleromentro, del giroscopio e degli angoli
	bool allarme;
	float lastRoll, lastPitch, lastYaw;
	unsigned int time, counter=0;
	std::string infrastruttura;
	sleep(1); //Tempo per aspettare che l'MPU6050 si stabilizzi
    

	//Calcola gli  offsets. Se voglio saltare il passaggio per vedere se la macchina funziona, posso commentare questa parte.
	std::cout << "Calcolando gli offsets.\nMantenere il dispositivo fermo e in orizzontale, l'operazione potrebbe richiedere qualche minuto.\n";
	device.getOffsets(&ax, &ay, &az, &gr, &gp, &gy);


	//Leggi l'angolo di imbardata attuale.
	device.calc_yaw = true;
        //Variabile per inclinazione
	float inclination_x, inclination_y;
	
	for (int i = 0; i < 40; i++) {

	 // Lettura accelerometro per debug
	 device.getAccel(&ax, &ay, &az);
	 if(i>=1){
	 std::cout << "Accelerazione in g lungo gli assi (X,Y,Z): " << ax << ", " << ay << ", " << az << "\n";
	    }
     device.getGyro(&gr, &gp, &gy);
	 // Angoli filtrati dal filtro complementare (quelli corretti da usare) 
	 if(i>=1){
	 std::cout << "Velocità angolari intorno agli assi:\nX: " << gr << "°/s, Y: " << gp << "°/s, Z: " << gy << "°/s\n";
	    }

	 //  inclinazioni solo accelerometro (debug, NON filtrate) 
	 inclination_x = atan2(ay, az) * 180 / M_PI;
	 inclination_y = atan2(-ax, sqrt(ay * ay + az * az)) * 180 / M_PI;
	 if(i>=1){
	 std::cout << "[DEBUG] Inclinazione solo accelerometro --> X: " << inclination_x 
			   << "°, Y: " << inclination_y << "°\n";
	    }

    // Lettura angoli filtrati (Roll, Pitch, Yaw)
	
	 device.getAngle(0, &roll); // Roll
	 device.getAngle(1, &pitch); // Pitch
	 device.getAngle(2, &yaw); // Yaw

	 if(i>=1){
	 //Inclinazione reale: 
	 std::cout << "Inclinazione sugli assi X: " << roll << "°, Y: " << pitch << "°, Z: " << yaw << "°\n\n\n";
	    }
	 usleep(750000); // 0.75 secondi
	}
    
	std::cout << "Inserire il nome dell'infrastruttura su cui è avvenuta la rilevazione: ";
    std::cin >> infrastruttura;
    /*getAccel e quindi l'accelerometro  misura l'inclinazione rispetto alla gravità. 
        Per l'asse x è positivo se inclinato a destra, negativo se inclinato a sinistra.
        Per y è positivo se inclinato in avanti, negativo se inclinato indietro.
        Per z è abbastanza costante poichè  misura la gravità.*/
		device.getAccel(&ax, &ay, &az);
		std::cout << "Valori dell'accelerometro: X: " << ax << ", Y: " << ay << ", Z: " << az << "\n";
   
		/*getGyro e quindi il giroscopio misura la velocità angolare ovvero la velocità con cui ruota il sensore.
			L'asse delle x indica se il dispositivo ruota in avanti o all'indietro.
			L'asse delle y indica se il dispositivo ruota a destra o a sinistra.
			L'asse z indica se il dispositivo gira su se stesso. */
		device.getGyro(&gr, &gp, &gy);
		std::cout << "Valori del giroscopio(velocità angolare):\nX: " << gr << ", Y: " << gp << ", Z: " << gy << "\n";
   
		device.getAngle(0, &roll); // Roll
		device.getAngle(1, &pitch); // Pitch
		device.getAngle(2, &yaw); // Yaw
		if(abs(roll) > 1.5 || abs(pitch) > 1.5){
			allarme = true;
		} else {
			allarme = false;
		}
	   
		std::cout << "Angoli finali:\nX: " << roll << ", Y: " << pitch << ", Z: " << yaw << "\n";
		sendDataToServer(infrastruttura, ax, ay, az, gr, gp, gy, roll, pitch, yaw, allarme);

    signal(SIGINT, handle_sigint); //gestisco il comando SIGINT di uscita con la funzione handle_signint
    
    while(keep_running){
	    /*getAccel e quindi l'accelerometro  misura l'inclinazione rispetto alla gravità. 
           Per l'asse x è positivo se inclinato a destra, negativo se inclinato a sinistra.
           Per y è positivo se inclinato in avanti, negativo se inclinato indietro.
           Per z è abbastanza costante poichè  misura la gravità.*/
	    device.getAccel(&ax, &ay, &az);
	    std::cout << "Valori dell'accelerometro: X: " << ax << ", Y: " << ay << ", Z: " << az << "\n";

	    /*getGyro e quindi il giroscopio misura la velocità angolare ovvero la velocità con cui ruota il sensore.
           L'asse delle x indica se il dispositivo ruota in avanti o all'indietro.
           L'asse delle y indica se il dispositivo ruota a destra o a sinistra.
           L'asse z indica se il dispositivo gira su se stesso. */
	    device.getGyro(&gr, &gp, &gy);
	    std::cout << "Valori del giroscopio(velocità angolare):\nX: " << gr << ", Y: " << gp << ", Z: " << gy << "\n";

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
			break; //se c'è un cambio repentino nei dati li invia al server e termina il loop
		}
	    lastRoll=roll;
		lastPitch=pitch;
		lastYaw=yaw;
		counter++;
		if(counter==450000){ //ogni 450000 cicli manda la scansione di routine al server
			sendDataToServer(infrastruttura, ax, ay, az, gr, gp, gy, roll, pitch, yaw, allarme);
			counter=0;
		}
		usleep(750000);
		
	    std::cout << "Angoli finali:\nX: " << roll << ", Y: " << pitch << ", Z: " << yaw << "\n";
		std::cout <<"\nPREMERE CTRL+C SE SI VUOLE INTERROMPERE IL MONITORAGGIO.\n";	
    }
	sendDataToServer(infrastruttura, ax, ay, az, gr, gp, gy, roll, pitch, yaw, allarme);
	std::cout <<"Monitoraggio interrotto correttamente.\n";
	return 0;
}


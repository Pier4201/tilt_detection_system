#include "DataSender.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <string>
#include <curl/curl.h>

// Funzione per ottenere data e ora in UTC di quando avvengono le misurazioni 
std::string getTimestamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::gmtime(&t);
    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buffer);
}

// Funzione per ricevere e scrivere la risposta del server in caso di successo o errore
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize); //ogni volta che il server manda una parte di risposta viene aggiunta all'output
    return totalSize;
}

// Funzione per inviare i dati al server tramite HTTP POST
void sendDataToServer(std::string infrastruttura, float accel_x, float accel_y, float accel_z,
    float gyro_x, float gyro_y, float gyro_z,
    float roll, float pitch, float yaw, bool allarme) {
CURL *curl;
CURLcode res;

//Il JSON è un oggetto che raccoglie tutti i dati che si vogliono inviare al server
std::stringstream json;

json << "{"
<< "\"ID\":\"" << infrastruttura << "\","
<< "\"timestamp\":\"" << getTimestamp() << "\","
<< "\"accel_x\":" << accel_x << ","
<< "\"accel_y\":" << accel_y << ","
<< "\"accel_z\":" << accel_z << ","
<< "\"gyro_x\":" << gyro_x << ","
<< "\"gyro_y\":" << gyro_y << ","
<< "\"gyro_z\":" << gyro_z << ","
<< "\"roll\":" << roll << ","
<< "\"pitch\":" << pitch << ","
<< "\"yaw\":" << yaw << ","
<< "\"allarme\":" << (allarme ? "true" : "false")
<< "}";

std::string jsonData = json.str();
//inizializza CURLs
curl = curl_easy_init(); 
if(curl) {
std::string responseString;
/*La funzione curl_easy_setopt(curl, CURLoption, value); 
dove il secondo valore indica che cosa deve fare la funzione
CURLOPT_URL per impostare l'url, CURL_POST per indicare che voglio fare una POST,
POSTFIELDS E POSTFIELDSIZE per passare il corpo della POST rispettivamente i dati da passare e la loro grandezza
HTTPHEADER dopo la curl_list_append per specificare che l'header è di tipo JSON
WRITEFUNCTION e WRITEDATA per catturare la risposta*/ 
// Echo server per test
curl_easy_setopt(curl, CURLOPT-URL, "https://192.168.20.122/data"); //richiesta https
//curl_easy_setopt(curl, CURLOPT_URL, "192.168.20.122:3001/data"); // richiesta http
curl_easy_setopt(curl, CURLOPT_POST, 1L); //si mette 1 per indicare di attivare la POST, 0 per distattivare
//curl invia tramite HTTP POST il json 
//Il payload della richiesta POST viene creato con la funzione successiva
curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str()); //Passa il json come dati da invitare tramite POST
curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, jsonData.size()); //specifica la lunghezza dei dati
//Dice al server che sta inviando un JSON
struct curl_slist *headers = NULL;
headers = curl_slist_append(headers, "Content-Type: application/json");
curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

//Disabilita verifica ssl
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Disabilita verifica del certificato
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // Disabilita verifica dell'host

// Oppure, per usare un certificato CA specifico:
// curl_easy_setopt(curl, CURLOPT_CAINFO, "/usr/local/share/ca-certificates/server-cert.crt");


// Per leggere la risposta del server
curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

// Questa è la funzione che invia dati tramite la richiesta POST, quelle di prima servivano ad inizializzarla
res = curl_easy_perform(curl);
//Questo if else serve a vedere se la richiesta POST è andata a buon fine, caso in cui ne stampa la risposta altrimenti da errore
if(res != CURLE_OK) {
//coverte il codice di errore in una stringa leggibile
std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
} else {
std::cout << "Dati inviati correttamente al server.\n";
std::cout << "Risposta del server:\n" << responseString << std::endl;
}

// Libera lo spazio in memoria usato da CURL per evitare gli sprechi di memoria
curl_slist_free_all(headers);
curl_easy_cleanup(curl);
}
}

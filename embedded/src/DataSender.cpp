#include "DataSender.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <string>
#include <curl/curl.h>

// function to get date and time in utc format
std::string getTimestamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::gmtime(&t);
    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buffer);
}

// function to write server response to check if it succeded or not (used during development for debugging purposes)
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize); //writes server response while it's being received
    return totalSize;
}

// function to send data to server in a JSON
void sendDataToServer(std::string infrastruttura, float accel_x, float accel_y, float accel_z,
    float gyro_x, float gyro_y, float gyro_z,
    float roll, float pitch, float yaw, bool allarme) {
CURL *curl;
CURLcode res;

//JSON structure below
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
//CURL initialization
curl = curl_easy_init(); 
if(curl) {
std::string responseString;
/*curl function has the following structure ---> curl_easy_setopt(curl, CURLoption, value); 
CURLoption specifies curl_easy_setopt function
CURLOPT_URL to set up url, CURL_POST to specify that we want to do a POST request,

HTTPHEADER in curl_list_append to specify that we got a json header
WRITEFUNCTION e WRITEDATA to get the response*/


// Insert ur server ip in the next function:
curl_easy_setopt(curl, CURLOPT-URL, "https://192.168.20.122/data"); // https request (depending on how we want to set up our server we can comment this)
//curl_easy_setopt(curl, CURLOPT_URL, "192.168.20.122:3001/data"); // http request  (or this line)
curl_easy_setopt(curl, CURLOPT_POST, 1L); 

//in the next 2 calls we create the request payload
curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str()); //POSTFIELDS E POSTFIELDSIZE to specify the type and size of data that will be in the POST req body
curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, jsonData.size()); 

struct curl_slist *headers = NULL;
headers = curl_slist_append(headers, "Content-Type: application/json"); //we specify that the contect type is a json file
curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

//Disable ssl certificates verification cause in my case i autosigned them u don't need that if yours is from a trusted CA
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // disable certificate verification
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // disable host verification

// u can use this one if u want to use a certificate saved in ru system: (make sure to change it to your directory)
// curl_easy_setopt(curl, CURLOPT_CAINFO, "/usr/local/share/ca-certificates/server-cert.crt");


// function to read server response
curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

// we send the post request with the following line
res = curl_easy_perform(curl);
//if else to check request outcome
if(res != CURLE_OK) {
//converts error code into a string that we can read
std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
} else {
std::cout << "Data correctly sent to server.\n";
std::cout << "Server response:\n" << responseString << std::endl;
}

// calls to free memory in space to avoid memory leaks
curl_slist_free_all(headers);
curl_easy_cleanup(curl);
}

// Saves data in local folder
std::ofstream file("data_log.json", std::ios::app); // to write new data at the end of file
if (file.is_open()) {
    file << "{"
         << "\"timestamp\":\"" << getTimestamp() << "\","
         << "\"infrastruttura\":\"" << infrastruttura << "\","
         << "\"accel_x\":" << accel_x << ","
         << "\"accel_y\":" << accel_y << ","
         << "\"accel_z\":" << accel_z << ","
         << "\"gyro_x\":" << gyro_x << ","
         << "\"gyro_y\":" << gyro_y << ","
         << "\"gyro_z\":" << gyro_z << ","
         << "\"roll\":" << roll << ","
         << "\"pitch\":" << pitch << ","
         << "\"yaw\":" << yaw << ","
         << "\"fallDetected\":" << (fallDetected ? "true" : "false")
         << "}" << std::endl;
    file.close();
} else {
    std::cerr << "Failed to open file in write mode" << std::endl;
}
}

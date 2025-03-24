#ifndef DATASENDER_H
#define DATASENDER_H

#include <string>

void sendDataToServer(std::string infrastruttura, float accel_x, float accel_y, float accel_z,
                      float gyro_x, float gyro_y, float gyro_z,
                      float roll, float pitch, float yaw, bool allarme);

#endif // DATASENDER_H

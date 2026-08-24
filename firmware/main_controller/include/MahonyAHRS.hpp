#pragma once

class MahonyAHRS {
public:
    MahonyAHRS();
    void update(float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz);
    void updateIMU(float gx, float gy, float gz, float ax, float ay, float az);
    void getEulerAngles(float& pitch, float& roll, float& yaw);

private:
    float twoKp;
    float twoKi;
    float q0, q1, q2, q3;
    float integralFBx, integralFBy, integralFBz;
};

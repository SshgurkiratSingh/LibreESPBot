#include <iostream>
#include "software/controller_app/src/core/Types.hpp"
int main() {
    std::cout << "Command Size: " << sizeof(VehicleCommandPacket) << std::endl;
    std::cout << "Telemetry Size: " << sizeof(VehicleTelemetryPacket) << std::endl;
    return 0;
}

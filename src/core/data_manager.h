#pragma once
#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "shaders/Soltrace.h"
#include "element.h"
#include <vector>

namespace OptixCSP {

    // Class to manage data on the host and device.
    class dataManager {
    public:
        // TODO: move this to private member for best practice 
        // Host copy of launch parameters.
        OptixCSP::LaunchParams launch_params_H;
        // Device pointer to launch parameters.
        OptixCSP::LaunchParams* launch_params_D;

        // device pointer to geometry data
        GeometryDataST* geometry_data_array_D;

        dataManager();
        ~dataManager();

        void cleanup();

        OptixCSP::LaunchParams* getDeviceLaunchParams() const;

        void allocateLaunchParams();

        void updateLaunchParams();

        // create geometry_data_array_D on the device
        // then launch_params_D.geometry_data_array = geometry_data_array_D gets a copy.
        void allocateGeometryDataArray(std::vector<GeometryDataST> geometry_data_array);

        // update geometry_data_array_D on the device
        // then launch_params_D.geometry_data_array = geometry_data_array_D gets a copy.
        void updateGeometryDataArray(std::vector<GeometryDataST> geometry_data_array_H);
    };
}
#endif  // DATAMANAGER_H

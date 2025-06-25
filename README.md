
# OptixCSP

**OptixCSP** is a modern GPU-accelerated ray tracing middle-ware library for modeling and simulating optical systems encountered in enginnering applications, such as **Concentrated Solar Power (CSP)**.  It leverages [NVIDIA OptiX](https://developer.nvidia.com/optix), a GPU-based ray tracing framework and can be used as a stand-alone library or integrated to CSP simulation workflows, such as [SolTrace](https://github.com/NREL/SolTrace), to significantly accelerate ray tracing tasks.

OptixCSP focuses on **accuracy, performance, and extensibility** for users and developers to:

* Rapidly simulate optical behavior in CSP systems: heliostats, receivers, sun modeling, optical properties, etc.
* Leverage GPU acceleration for complex scenes, large-scale simulations and dynamic updates such as an annual simulation.
* Integrate OptixCSP into existing CSP software as a high-performance backend.
* Build standalone ray tracing applications using OptixCSP core.

---

## Prerequisites

Before building `OptixCSP`, ensure you have the following:

1. **NVIDIA GPU and Drivers**
   - An NVIDIA GPU and a compatible NVIDIA driver.
   - Minimum CUDA Toolkit version: 12.0.

2. **NVIDIA OptiX SDK**
   - Download the NVIDIA OptiX SDK from [NVIDIA's website](https://developer.nvidia.com/designworks/optix/download).
   - Install the SDK to a directory. For example:
     ```
     C:/ProgramData/NVIDIA Corporation/OptiX SDK 8.1.0/
     ```
   - The source code for the OptiX SDK is located in the `SDK` directory, for example, `C:/ProgramData/NVIDIA Corporation/OptiX SDK 8.1.0/SDK`. 

3. **CMake**
   - Version 3.18 or higher.

4. **C++ Compiler**
   - A compiler that supports C++17 or later.

---

## Building `OptixCSP`

### Configure the Build

1. **Clone the Repository**:
   ```bash
   git clone git@github.com:uwsbel/optixCSP.git
   cd optixCSP
   ```

2. **Set the `OptiX_INSTALL_DIR`**:
   - Provide the path to the OptiX SDK during CMake configuration.
   - For example, if the SDK is installed in `C:/ProgramData/NVIDIA Corporation/OptiX SDK 8.1.0/`:
     ```bash
     cmake -DOptiX_INSTALL_DIR="C:/ProgramData/NVIDIA Corporation/OptiX SDK 8.1.0/" .
     ```

3. **(Optional) Set the `OptiX_INSTALL_DIR` as an Environment Variable**:
   - Alternatively, you can set `OptiX_INSTALL_DIR` as an environment variable:
     - On **Windows**:
       ```powershell
       $env:OptiX_INSTALL_DIR="C:/ProgramData/NVIDIA Corporation/OptiX SDK 8.1.0/"
       ```
     - On **Linux/macOS**:
       ```bash
       export OptiX_INSTALL_DIR="/path/to/OptiX SDK/"
       ```

4. **Generate Build Files**:  Run CMake to generate build files for your project

5. **Build the Project**:  Build the optixCSP project
   
---

## Running the Demos

Once built, demos can be found in the `bin` directory. Run them from the command line or within your IDE.

---

## Using `OptixCSP` as a library

You can link `OptixCSP` to a third-party software such as `SolTrace` see [here](https://github.com/NREL/SolTrace.git).

1. Follow the insturctions [here](#building-optixcsp) to build `OptixCSP`, note that the build process automatically outputs `OptixCSPConfig.cmake` in its build folder (e.g. `build/cmake/`)

2. Configurd CMake for dependencies and to find `OptixCSP`:
    - Find CudaToolkit and Optix
      ```cmake
      find_package(CUDAToolkit)
      find_package(OptiX)
    - Specify the location of `OptixCSPConfig.cmake`:
      ```cmake
      set(OptixCSP_DIR "/path/to/OptixCSP/build/cmake")
      find_package(OptixCSP REQUIRED)
      ```

2. Then link to `OptixCSP_core`:
    ```cmake
    target_link_libraries(MyApp
          PUBLIC  OptixCSP::OptixCSP_core
                  CUDA::cuda_driver)
    ```
---

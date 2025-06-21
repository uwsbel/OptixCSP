/*

 * SPDX-FileCopyrightText: Copyright (c) 2019 - 2024  NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <optix.h>
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace OptixCSP {
    // ------------------------------------------------------------
    // Simple error checker (no log buffer)
    inline void optixCheck(OptixResult   res,
        const char* func,
        const char* file,
        unsigned int  line)
    {
        if (res != OPTIX_SUCCESS)
        {
            std::ostringstream oss;
            oss << "OptiX call (" << func << ") failed with code "
                << static_cast<int>(res) << " at " << file << ":" << line;
            throw std::runtime_error(oss.str());
        }
    }

    inline void cudaCheck(cudaError_t error, const char* call, const char* file, unsigned int line)
    {
        if (error != cudaSuccess)
        {
            std::stringstream ss;
            ss << "CUDA call (" << call << " ) failed with error: '"
                << cudaGetErrorString(error) << "' (" << file << ":" << line << ")\n";
            throw std::runtime_error(ss.str().c_str());
        }
    }


    // ------------------------------------------------------------
    // Error checker WITH log buffer
    inline void optixCheckLog(OptixResult  res,
        const char* log,
        size_t       sizeof_log,
        size_t       log_size,
        const char* func,
        const char* file,
        unsigned int line)
    {
        if (res != OPTIX_SUCCESS)
        {
            std::ostringstream oss;
            oss << "OptiX call (" << func << ") failed with code "
                << static_cast<int>(res) << " at " << file << ":" << line
                << "\nLog (" << log_size << " bytes):\n"
                << std::string(log, log + log_size);
            throw std::runtime_error(oss.str());
        }
        else if (log_size > 1)
        {
            std::cerr << "OptiX log for " << func << ":\n"
                << std::string(log, log + log_size) << std::endl;
        }
    }


    inline void cudaSyncCheck(const char* file, unsigned int line)
    {
        cudaDeviceSynchronize();
        cudaError_t error = cudaGetLastError();
        if (error != cudaSuccess)
        {
            std::stringstream ss;
            ss << "CUDA error on synchronize with error '"
                << cudaGetErrorString(error) << "' (" << file << ":" << line << ")\n";
            throw std::runtime_error(ss.str().c_str());
        }
    }

#define OPTIX_CHECK( call ) optixCheck( (call), #call, __FILE__, __LINE__ )
#define OPTIX_CHECK_LOG( call )                                   \
        do {                                                          \
            char   LOG_[2048] = {};                                   \
            size_t LOG_SIZE_  = sizeof( LOG_ );                       \
            optixCheckLog( (call), LOG_, sizeof( LOG_ ),              \
                           LOG_SIZE_, #call,                          \
                           __FILE__, __LINE__ );                      \
        } while( false )

#define CUDA_CHECK( call ) cudaCheck( call, #call, __FILE__, __LINE__ )
#define CUDA_SYNC_CHECK() cudaSyncCheck( __FILE__, __LINE__ )
}
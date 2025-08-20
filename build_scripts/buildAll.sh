#!/bin/bash
module load cuda/12.3
#module load gcc/12.1

BASE_DIR="${HOME}/solar"

cd ${BASE_DIR}
git clone git@github.com:uwsbel/OptixCSP.git 
git clone -b jm-dev-branch git@github.com:uwsbel/SolTrace.git


# this is where I installed optix 8.0 from the script provided by nvidia
OPTIX_DIR="${BASE_DIR}/Optix8.0"

# OptixCSP related
OPTIXCSP_SRC="${BASE_DIR}/OptixCSP"
OPTIXCSP_BLD="${OPTIXCSP_SRC}/build"
OPTIXCSP_DIR="${OPTIXCSP_BLD}/cmake"

# soltrace related 
SOLTRACE_SRC="${BASE_DIR}/SolTrace"
SOLTRACE_BLD="${SOLTRACE_SRC}/build"
UNIT_TEST_DIR="${SOLTRACE_BLD}/google-tests/unit-tests"

# this has to match run-time gcc version
# gcc and gxx version
CC="/opt/cray/pe/gcc/12.1.0/bin/gcc"
CXX="/opt/cray/pe/gcc/12.1.0/bin/g++"

echo "=== Building OptixCSP ==="
if [ -d "${OPTIXCSP_BLD}" ]; then
  echo "Removing existing OptixCSP build directory: ${OPTIXCSP_BLD}"
  rm -rf "${OPTIXCSP_BLD}"
fi
mkdir -p "${OPTIXCSP_BLD}"

cmake -S "${OPTIXCSP_SRC}" -B "${OPTIXCSP_BLD}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DOptiX_INSTALL_DIR="${OPTIX_DIR}"

cmake --build "${OPTIXCSP_BLD}" -j


echo "=== Building Soltrace ==="
if [ -d "${SOLTRACE_BLD}" ]; then
  echo "Removing existing build directory: ${SOLTRACE_BLD}"
  rm -rf "${SOLTRACE_BLD}"
fi
mkdir -p "${SOLTRACE_BLD}"

cmake -S "${SOLTRACE_SRC}" -B "${SOLTRACE_BLD}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="${CC}" \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DOptiX_INSTALL_DIR="${OPTIX_DIR}" \
  -DOptixCSP_DIR="${OPTIXCSP_DIR}" \
  -DSOLTRACE_BUILD_CORETRACE=ON \
  -DSOLTRACE_BUILD_GUI=OFF \
  -DSOLTRACE_BUILD_OPTIX_SUPPORT=ON

cmake --build "${SOLTRACE_BLD}"

echo "=== Running unit tests ==="
cd ${UNIT_TEST_DIR}
./UnitTests

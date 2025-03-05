#pragma once
#include <optix.h>
#include <sutil/vec_math.h>
#include <vector>
#include <cuda/GeometryDataST.h>
#include <cuda/Soltrace.h>
#include <string>
#include <SoltraceState.h>


std::vector<GeometryData::Parallelogram> GenerateHeliostatsFromFile(std::string filename);

void createGeometry(SoltraceState& state, std::vector<GeometryData::Parallelogram>& helistat_list, std::vector<GeometryData::Parallelogram>& receiver_list);

void createGeometry_cyl_receiver(SoltraceState& state, 
								 std::vector<GeometryData::Parallelogram>& helistat_list,
								 std::vector<GeometryData::Cylinder_Y>& receiver_list);


void createGeometry_parabolic(SoltraceState& state, 
							  std::vector<GeometryData::Rectangle_Parabolic>& helistat_list, 
							  std::vector<GeometryData::Parallelogram>& receiver_list);

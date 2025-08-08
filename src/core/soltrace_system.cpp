#include "soltrace_system.h"
#include "geometry_manager.h"
#include "data_manager.h"
#include "pipeline_manager.h"
#include "soltrace_type.h"
#include "CspElement.h"
#include "timer.h"

#include "utils/util_record.hpp"
#include "utils/util_check.hpp"
#include "utils/math_util.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>

using namespace OptixCSP;

// TODO: optix related type should go into one header file
typedef Record<OptixCSP::HitGroupData> HitGroupRecord;

void SolTraceSystem::print_launch_params() {

	LaunchParams params = data_manager->launch_params_H;

    float3 sun_box_a = params.sun_v0 - params.sun_v1;
    float3 sun_box_b = params.sun_v1 - params.sun_v2;

    float sun_box_edge_a = sqrtf(sun_box_a.x * sun_box_a.x + sun_box_a.y * sun_box_a.y + sun_box_a.z * sun_box_a.z);
    float sun_box_edge_b = sqrtf(sun_box_b.x * sun_box_b.x + sun_box_b.y * sun_box_b.y + sun_box_b.z * sun_box_b.z);

    std::cout << "print launch params: " << std::endl;
    std::cout << "width              : " << params.width << std::endl;
    std::cout << "height             : " << params.height << std::endl;
    std::cout << "max_depth          : " << params.max_depth << std::endl;
    std::cout << "hit_point_buffer   : " << params.hit_point_buffer << std::endl;
	std::cout << "sun_dir_buffer     : " << params.sun_dir_buffer << std::endl;
    std::cout << "sun_vector         : " << params.sun_vector.x << " " <<params.sun_vector.y << " " <<params.sun_vector.z << std::endl;
    std::cout << "max_sun_angle      : " << params.max_sun_angle << std::endl;
    std::cout << "sun_v0             : " << params.sun_v0.x << " " <<params.sun_v0.y << " " <<params.sun_v0.z << std::endl;
    std::cout << "sun_v1             : " << params.sun_v1.x << " " <<params.sun_v1.y << " " <<params.sun_v1.z << std::endl;
    std::cout << "sun_v2             : " << params.sun_v2.x << " " <<params.sun_v2.y << " " <<params.sun_v2.z << std::endl;
    std::cout << "sun_v3             : " << params.sun_v3.x << " " <<params.sun_v3.y << " " <<params.sun_v3.z << std::endl;
    std::cout << "sun_box_edge_a     : " << sun_box_edge_a << std::endl;
    std::cout << "sun_box_edge_b     : " << sun_box_edge_b << std::endl;
}

SolTraceSystem::SolTraceSystem(int numSunPoints)
    : m_num_sunpoints(numSunPoints),
      m_verbose(false),
      m_mem_free_before(0),
      m_mem_free_after(0),
      m_sun_angle(0.0),
      m_timer_setup(),
      m_timer_trace(),
      geometry_manager(std::make_shared<GeometryManager>(m_state)),
      data_manager(std::make_shared<dataManager>()),
      pipeline_manager(std::make_shared<pipelineManager>(m_state))
{
    CUDA_CHECK(cudaFree(0));
    CUcontext cuCtx = 0;
    OPTIX_CHECK(optixInit());
    OptixDeviceContextOptions options = {};
    options.logCallbackFunction = [](unsigned int level, const char* tag, const char* message, void*) {
        std::cerr << "[" << std::setw(2) << level << "][" << std::setw(12) << tag << "]: " << message << "\n";
    };
    options.logCallbackLevel = 4;
    OPTIX_CHECK(optixDeviceContextCreate(cuCtx, &options, &m_state.context));
}

SolTraceSystem::~SolTraceSystem() {
    //cleanup();
}

void SolTraceSystem::initialize() {
	cudaMemGetInfo(&m_mem_free_before, nullptr);
    m_timer_setup.start();


	Vec3d sun_vec = m_sun_vector.normalized(); // normalize the sun vector

    // set up input related to sun
	data_manager->launch_params_H.sun_vector = OptixCSP::toFloat3(sun_vec);
    data_manager->launch_params_H.max_sun_angle = (float)(m_sun_angle);

    Timer AABB_timer;
    AABB_timer.start();
	geometry_manager->collect_geometry_info(m_element_list, data_manager->launch_params_H);
	AABB_timer.stop();
	std::cout << "Time to compute AABB: " << AABB_timer.get_time_sec() << " seconds" << std::endl;

	Timer geometry_timer;
	geometry_timer.start();
    geometry_manager->create_geometries(data_manager->launch_params_H);
    geometry_timer.stop();
	std::cout << "Time to create geometries: " << geometry_timer.get_time_sec() << " seconds" << std::endl;

    // Pipeline setup.
	Timer pipeline_timer;
	pipeline_timer.start();
    pipeline_manager->createPipeline();
    pipeline_timer.stop();
	std::cout << "Time to create pipeline: " << pipeline_timer.get_time_sec() << " seconds" << std::endl;
    

	Timer sbt_timer;
	sbt_timer.start();
    create_shader_binding_table();
    sbt_timer.stop();
	std::cout << "Time to create SBT: " << sbt_timer.get_time_sec() << " seconds" << std::endl;

    // Initialize launch params
    data_manager->launch_params_H.width = m_num_sunpoints;
    data_manager->launch_params_H.height = 1;
    data_manager->launch_params_H.max_depth = MAX_TRACE_DEPTH;


	// seed for sun ray randomization
    data_manager->launch_params_H.sun_dir_seed = 123456ULL;

    // Allocate memory for the hit point buffer, size is number of rays launched * depth
    const size_t hit_point_buffer_size = data_manager->launch_params_H.width * data_manager->launch_params_H.height * sizeof(float4) * data_manager->launch_params_H.max_depth;

    CUDA_CHECK(cudaMalloc(
        reinterpret_cast<void**>(&data_manager->launch_params_H.hit_point_buffer),
        hit_point_buffer_size
    ));
    CUDA_CHECK(cudaMemset(data_manager->launch_params_H.hit_point_buffer, 0, hit_point_buffer_size));


	// Luning TODO: Allocate memory for the direction cosine buffer, size is number of rays launched * depth
    const size_t sun_dir_size = data_manager->launch_params_H.width * data_manager->launch_params_H.height * sizeof(float3);

    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&data_manager->launch_params_H.sun_dir_buffer), sun_dir_size));
    CUDA_CHECK(cudaMemset(data_manager->launch_params_H.sun_dir_buffer, 0, sun_dir_size));

    // Create a CUDA stream for asynchronous operations.
    CUDA_CHECK(cudaStreamCreate(&m_state.stream));

    // Link the GAS handle.
    data_manager->launch_params_H.handle = m_state.gas_handle;
    data_manager->allocateGeometryDataArray(geometry_manager->get_geometry_data_array());

    print_launch_params();


    // copy launch params to device
    data_manager->allocateLaunchParams();
    data_manager->updateLaunchParams();

    m_timer_setup.stop();

}

void SolTraceSystem::run() {

    int width = data_manager->launch_params_H.width;
    int height = data_manager->launch_params_H.height;

    size_t m_mem_free_after;
    cudaMemGetInfo(&m_mem_free_after, nullptr);
    std::cout << "Memory used by launch: " << (m_mem_free_before - m_mem_free_after) / (1024.0 * 1024.0) << " MB\n";

    m_timer_trace.start();
    // Launch the simulation.
    OPTIX_CHECK(optixLaunch(
        m_state.pipeline,
        m_state.stream,  // Assume this stream is properly created.
        reinterpret_cast<CUdeviceptr>(data_manager->getDeviceLaunchParams()),
        sizeof(OptixCSP::LaunchParams),
		&m_state.sbt,    // Shader Binding Table.
        width,  // Launch dimensions
        height,
        1));
    CUDA_SYNC_CHECK();

	m_timer_trace.stop();


}

void SolTraceSystem::update() {


    const size_t hit_point_buffer_size = data_manager->launch_params_H.width * data_manager->launch_params_H.height * sizeof(float4) * data_manager->launch_params_H.max_depth;

    // update aabb and sun plane accordingly
	geometry_manager->update_geometry_info(m_element_list, data_manager->launch_params_H);

    // update data on the device    
	data_manager->updateGeometryDataArray(geometry_manager->get_geometry_data_array());
    CUDA_CHECK(cudaMemset(data_manager->launch_params_H.hit_point_buffer, 0, hit_point_buffer_size));
	data_manager->updateLaunchParams();
}

bool SolTraceSystem::read_st_input(const char* filename) {
    FILE* fp = fopen(filename, "r");
	if (!fp)
	{
    	printf("failed to open system input file\n");
	}

	if ( !read_system( fp ) )
	{
		printf("error in system input file.\n");
		return false;
	}
	
	fclose(fp);

    return true;
}

void SolTraceSystem::write_hp_output(const std::string& filename) {
    int output_size = data_manager->launch_params_H.width * data_manager->launch_params_H.height * data_manager->launch_params_H.max_depth;
    std::vector<float4> hp_output_buffer(output_size);
    CUDA_CHECK(cudaMemcpy(hp_output_buffer.data(), data_manager->launch_params_H.hit_point_buffer, output_size * sizeof(float4), cudaMemcpyDeviceToHost));

    std::ofstream outFile(filename);

    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open the file " << filename << " for writing." << std::endl;
        return;
    }

    // Write header
	// TODO, if statements to check if one needs to write dir_cos_buffer or not
    outFile << "number,stage,loc_x,loc_y,loc_z,cosx,cosy,cosz\n";

    int currentRay = 1;
    int stage = 0;

    for (const auto& element : hp_output_buffer) {

        // Inline check: if y, z, and w are all zero, treat as marker for new ray.
        if ((element.y == 0) && (element.z == 0) && (element.w == 0)) {
            if (stage > 0) {
                currentRay++;
                stage = 0;
            }
            continue;  // Skip printing this marker element.
        }

        // If we haven't reached max_trace stages for the current ray, print the element.
        if (stage < data_manager->launch_params_H.max_depth) {
            outFile << currentRay << ","
                << element.x << "," << element.y << ","
                << element.z << "," << element.w << "\n";
            stage++;
        }
        else {
            // If max_trace stages reached, move to next ray and reset stage counter.
            currentRay++;
            stage = 0;
            outFile << currentRay << ","
                << element.x << "," << element.y << ","
                << element.z << "," << element.w << "\n";
            stage++;
        }


    }

    outFile.close();
    std::cout << "Data successfully written to " << filename << std::endl;
}

// write json output file for post processing
// need sun vector, number of rays, sun box, sun angle
// receiver stats, dimension, type, location, rotation matrix. 
// writing timing?

void SolTraceSystem::write_simulation_json(const std::string& filename) {
	std::ofstream out(filename);
    
    // process sun stats
    float3 sun_box_a = data_manager->launch_params_H.sun_v0 - data_manager->launch_params_H.sun_v1;
    float3 sun_box_b = data_manager->launch_params_H.sun_v1 - data_manager->launch_params_H.sun_v2;

	float sun_box_edge_a = length(sun_box_a);
	float sun_box_edge_b = length(sun_box_b);

    out << "{\n";
    out << "  \"sun\": {\n";
    out << "    \"number_of_sunpoints\": " << m_num_sunpoints << ",\n";
    out << "    \"sun_vector\": ["
        << m_sun_vector[0] << ", " << m_sun_vector[1] << ", " << m_sun_vector[2] << "],\n";
    out << "    \"sun_box_edge_a\": " << sun_box_edge_a << ",\n";
    out << "    \"sun_box_edge_b\": " << sun_box_edge_b << "\n";
    out << "  },\n";


    std::shared_ptr<CspElement> receiver = m_element_list.back();

    out << "  \"receiver\": {\n";

	// use enum to get the type of the receiver
	std::string receiver_type = "unknown";

    switch (receiver->get_surface_type()) {
        case SurfaceType::FLAT:
            receiver_type = "flat";
            out << "    \"type\": \"" << receiver_type << "\",\n";
			out << "    \"dimensions\": [" << receiver->get_aperture()->get_width() << ", " << 
                                              receiver->get_aperture()->get_height() << "],\n";

            break;
        case SurfaceType::CYLINDER:
            receiver_type = "cylinder";
            out << "    \"type\": \"" << receiver_type << "\",\n";
			out << "    \"radius\": " << receiver->get_aperture()->get_width() / 2.0f << ",\n";
			out << "    \"height\": " << receiver->get_aperture()->get_height() << ",\n";

            break;
        case SurfaceType::PARABOLIC:
            //TODO
            // can we have parabolic receiver?
            receiver_type = "parabolic";
            break;
        default:
            receiver_type = "unknown";
			break;
    }

	Vec3d receiver_location = receiver->get_origin();
	Matrix33d rotation_matrix = receiver->get_rotation_matrix(); // get the rotation matrix
	Vec3d receiver_x_basis = rotation_matrix.get_x_basis();
	Vec3d receiver_y_basis = rotation_matrix.get_y_basis();
	Vec3d receiver_z_basis = rotation_matrix.get_z_basis();

    out << "    \"location\": ["
        << receiver_location[0] << ", " << receiver_location[1] << ", " << receiver_location[2] << "],\n";

    // print out rotation matrix basis
	out << "    \"rotation_matrix\": {\n";
    out << "      \"x_basis\": ["
		<< receiver_x_basis[0] << ", " << receiver_x_basis[1] << ", " << receiver_x_basis[2] << "],\n";
	out << "      \"y_basis\": ["
		<< receiver_y_basis[0] << ", " << receiver_y_basis[1] << ", " << receiver_y_basis[2] << "],\n";
	out << "      \"z_basis\": ["
		<< receiver_z_basis[0] << ", " << receiver_z_basis[1] << ", " << receiver_z_basis[2] << "]}\n";


    out << "  }\n";
    out << "}\n";

    out.close();

}




void SolTraceSystem::write_sun_output(const std::string& filename) {
    int output_size = data_manager->launch_params_H.width * data_manager->launch_params_H.height;

    std::vector<float3> sun_dir_buffer(output_size);
    CUDA_CHECK(cudaMemcpy(sun_dir_buffer.data(), data_manager->launch_params_H.sun_dir_buffer, output_size * sizeof(float3), cudaMemcpyDeviceToHost));


    std::ofstream outFile(filename);

    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open the file " << filename << " for writing." << std::endl;
        return;
    }

    // Write header
    // TODO, if statements to check if one needs to write dir_cos_buffer or not
    outFile << "number,cosx,cosy,cosz\n";

    int currentRay = 1;

    for (const auto& element : sun_dir_buffer) {
        outFile << currentRay << ","
                << element.x << "," 
                << element.y << ","
                << element.z << "\n";

        currentRay++;
    }

    outFile.close();
    std::cout << "Data successfully written to " << filename << std::endl;
}

void SolTraceSystem::clean_up() {


    CUDA_CHECK(cudaDeviceSynchronize());
    // destroy pipeline related resources
	pipeline_manager->cleanup();

    // destory CUDA stream
	if (m_state.stream) {
		CUDA_CHECK(cudaStreamDestroy(m_state.stream));
	}

    OPTIX_CHECK(optixDeviceContextDestroy(m_state.context));


    // Free OptiX shader binding table (SBT) memory
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(m_state.sbt.raygenRecord)));
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(m_state.sbt.missRecordBase)));
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(m_state.sbt.hitgroupRecordBase)));

    // Free OptiX GAS output buffer
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(m_state.d_gas_output_buffer)));

    // Free device-side launch parameter memory
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(data_manager->launch_params_H.hit_point_buffer)));
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(data_manager->launch_params_H.sun_dir_buffer)));

    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(data_manager->launch_params_D)));

    data_manager->cleanup();


}

// Create and configure the Shader Binding Table (SBT).
// The SBT is a crucial data structure in OptiX that links geometry and ray types
// with their corresponding programs (ray generation, miss, and hit group).
void SolTraceSystem::create_shader_binding_table(){

    // Ray generation program record
    {
        CUdeviceptr d_raygen_record;                   // Device pointer to hold the raygen SBT record.
        size_t      sizeof_raygen_record = sizeof(EmptyRecord);

        CUDA_CHECK(cudaMalloc(
            reinterpret_cast<void**>(&d_raygen_record),
            sizeof_raygen_record));

        EmptyRecord rg_sbt;  // host

        optixSbtRecordPackHeader(m_state.raygen_prog_group, &rg_sbt);

        // Copy the raygen SBT record from host to device.
        CUDA_CHECK(cudaMemcpy(
            reinterpret_cast<void*>(d_raygen_record),
            &rg_sbt,
            sizeof_raygen_record,
            cudaMemcpyHostToDevice
        ));

        // Assign the device pointer to the raygenRecord field in the SBT.
        m_state.sbt.raygenRecord = d_raygen_record;
    }

    // Miss program record
    {
        CUdeviceptr d_miss_record;
        size_t sizeof_miss_record = sizeof(EmptyRecord);

        CUDA_CHECK(cudaMalloc(
            reinterpret_cast<void**>(&d_miss_record),
            sizeof_miss_record * OptixCSP::RAY_TYPE_COUNT));

        EmptyRecord ms_sbt[OptixCSP::RAY_TYPE_COUNT];
        // Pack the program header into the first miss SBT record.
        optixSbtRecordPackHeader(m_state.radiance_miss_prog_group, &ms_sbt[0]);

        CUDA_CHECK(cudaMemcpy(
            reinterpret_cast<void*>(d_miss_record),
            ms_sbt,
            sizeof_miss_record * OptixCSP::RAY_TYPE_COUNT,
            cudaMemcpyHostToDevice
        ));

        // Configure the SBT miss program fields.
        m_state.sbt.missRecordBase = d_miss_record;                   // Base address of the miss records.
        m_state.sbt.missRecordCount = OptixCSP::RAY_TYPE_COUNT;        // Number of miss records.
        m_state.sbt.missRecordStrideInBytes = static_cast<uint32_t>(sizeof_miss_record);    // Stride between miss records.
    }

    // Hitgroup program record
    {
        // Total number of hitgroup records is the number of optical entity types
        const unsigned int count_records = OptixCSP::NUM_OPTICAL_ENTITY_TYPES;
        std::vector<HitGroupRecord> hitgroup_records_list(count_records);

        // now we need to populate hitgroup_records_list, basically match the 
		// OpticalEntityType with the corresponding m_program_group
        for (unsigned int i = 0; i < count_records; i++) {

			OptixCSP::OpticalEntityType my_type = static_cast<OptixCSP::OpticalEntityType>(i);
            // initialize program handle and data
            OptixProgramGroup program_group_handle = nullptr;
            SurfaceApertureMap map = {};

			switch (my_type) {
            case OptixCSP::OpticalEntityType::RECTANGLE_FLAT_MIRROR: 
				map = { SurfaceType::FLAT, ApertureType::RECTANGLE };
                program_group_handle = pipeline_manager->getMirrorProgram(map);
                hitgroup_records_list[i].data.material_data.mirror = {0.875425, 0, 0, 0};
                printf("RECTANGLE_FLAT_MIRROR, program group address: %p \n", program_group_handle);
				break;
            case OptixCSP::OpticalEntityType::RECTANGLE_PARABOLIC_MIRROR:
                map = { SurfaceType::PARABOLIC, ApertureType::RECTANGLE };
                program_group_handle = pipeline_manager->getMirrorProgram(map);
                hitgroup_records_list[i].data.material_data.mirror = { 0.875425, 0, 0, 0 };
                printf("RECTANGLE_PARABOLIC_MIRROR, program group address: %p \n", program_group_handle);

                break;
            case OptixCSP::OpticalEntityType::RECTANGLE_FLAT_RECEIVER:
                program_group_handle = pipeline_manager->getReceiverProgram(SurfaceType::FLAT);
				hitgroup_records_list[i].data.material_data.receiver = { 0.95, 0, 0, 0 };
                printf("RECTANGLE_FLAT_RECEIVER, program group address: %p \n", program_group_handle);

                break;
            case OptixCSP::OpticalEntityType::CYLINDRICAL_RECEIVER:
                program_group_handle = pipeline_manager->getReceiverProgram(SurfaceType::CYLINDER);
                hitgroup_records_list[i].data.material_data.receiver = { 0.95, 0, 0, 0 };
                printf("CYLINDRICAL_RECEIVER, program group address: %p \n", program_group_handle);

                break;            
            default:
				std::cerr << "Unknown OpticalEntityType: " << my_type << std::endl;
			}

            OPTIX_CHECK(optixSbtRecordPackHeader(program_group_handle, &hitgroup_records_list[i].header));

        }

        // Allocate memory for hitgroup records on the device.
        CUdeviceptr hitgroup_records_D;
        size_t      sizeof_hitgroup_record = sizeof(HitGroupRecord);
        CUDA_CHECK(cudaMalloc(
            reinterpret_cast<void**>(&hitgroup_records_D),
            sizeof_hitgroup_record * count_records
        ));

        // Copy hitgroup records from host to device.
        CUDA_CHECK(cudaMemcpy(
            reinterpret_cast<void*>(hitgroup_records_D),
            hitgroup_records_list.data(),
            sizeof_hitgroup_record * count_records,
            cudaMemcpyHostToDevice
        ));

        // Configure the SBT hitgroup fields.
        m_state.sbt.hitgroupRecordBase = hitgroup_records_D;             // Base address of hitgroup records.
        m_state.sbt.hitgroupRecordCount = count_records;                  // Total number of hitgroup records.
        m_state.sbt.hitgroupRecordStrideInBytes = static_cast<uint32_t>(sizeof_hitgroup_record);  // Stride size.
    }
}

void SolTraceSystem::add_element(std::shared_ptr<CspElement> e)
{

    // update the euler angles for the element
    e->update_euler_angles();
    m_element_list.push_back(e);
}

double SolTraceSystem::get_time_trace() {
    return m_timer_trace.get_time_sec();
} 

double SolTraceSystem::get_time_setup() {
	return m_timer_setup.get_time_sec();
}

void SolTraceSystem::set_sun_vector(Vec3d vect) {
    m_sun_vector = vect;
    Vec3d sun_v = m_sun_vector.normalized(); // Normalize the sun vector
	data_manager->launch_params_H.sun_vector = OptixCSP::toFloat3(sun_v);
}

std::vector<std::string> SolTraceSystem::split(const std::string& str, const std::string& delim, bool ret_empty, bool ret_delim) {
	
    std::vector<std::string> list;

	char cur_delim[2] = {0,0};
	std::string::size_type m_pos = 0;
	std::string token;
	
	while (m_pos < str.length())
	{
		std::string::size_type pos = str.find_first_of(delim, m_pos);
		if (pos == std::string::npos)
		{
			cur_delim[0] = 0;
			token.assign(str, m_pos, std::string::npos);
			m_pos = str.length();
		}
		else
		{
			cur_delim[0] = str[pos];
			std::string::size_type len = pos - m_pos;			
			token.assign(str, m_pos, len);
			m_pos = pos + 1;
		}
		
		if (token.empty() && !ret_empty)
			continue;

		list.push_back( token );
		
		if ( ret_delim && cur_delim[0] != 0 && m_pos < str.length() )
			list.push_back( std::string( cur_delim ) );
	}
	
	return list;
}

void SolTraceSystem::read_line(char* buf, int len, FILE* fp) {
	fgets(buf, len, fp);
	size_t nch = strlen(buf);
	if (nch > 0 && buf[nch-1] == '\n')
		buf[nch-1] = 0;
	if (nch-1 > 0 && buf[nch-2] == '\r')
		buf[nch-2] = 0;
}

bool SolTraceSystem::read_sun(FILE* fp) {
	
    if (!fp) return false;

	char buf[1024];
	int bi = 0, count = 0;
	char cshape = 'g';
	double Sigma, HalfWidth;
	bool PointSource;

	read_line( buf, 1023, fp );

	sscanf(buf, "SUN\tPTSRC\t%d\tSHAPE\t%c\tSIGMA\t%lg\tHALFWIDTH\t%lg",
		&bi, &cshape, &Sigma, &HalfWidth);
	PointSource = (bi!=0);
	cshape = tolower(cshape);

    // TODO: Update if supporting other sun shapes
    set_sun_angle(Sigma * 0.001);

	read_line( buf, 1023, fp );
	double X, Y, Z, Latitude, Day, Hour;
	bool UseLDHSpec;
	sscanf(buf, "XYZ\t%lg\t%lg\t%lg\tUSELDH\t%d\tLDH\t%lg\t%lg\t%lg",
		&X, &Y, &Z, &bi, &Latitude, &Day, &Hour);
	UseLDHSpec = (bi!=0);
	
    // TODO: Update if supporting LDHSpec
	// if ( UseLDHSpec )
	// {
	// 	st_sun_position(cxt, Latitude, Day, Hour, &X, &Y, &Z);
	// }

    Vec3d sun_vector(X, Y, Z);
	set_sun_vector(sun_vector);

	//printf("sun ps? %d cs: %c  %lg %lg %lg\n", PointSource?1:0, cshape, X, Y, Z);

	read_line( buf, 1023, fp );
	sscanf(buf, "USER SHAPE DATA\t%d", &count);
    // TODO: Update if supporting user shape data
	if (count > 0)
	{
		double *angle = new double[count];
		double *intensity = new double[count];

		for (int i=0;i<count;i++)
		{
			double x, y;
			read_line( buf, 1023, fp );
			sscanf(buf, "%lg\t%lg", &x, &y);
			angle[i] = x;
			intensity[i] = y;
		}

		//st_sun_userdata(cxt, count, angle, intensity );

		delete [] angle;
		delete [] intensity;	
	}

	return true;
}

bool SolTraceSystem::read_optic_surface(FILE* fp) {
	
    if (!fp) return false;
	char buf[1024];
	read_line(buf, 1023, fp);
	std::vector<std::string> parts  = split( std::string(buf), "\t", true, false );
	if (parts.size() < 15)
	{
        printf("too few tokens for optical surface: %zu\n", parts.size());
		printf("\t>> %s\n", buf);
		return false;
	}

	char ErrorDistribution = 'g';
	if (parts[1].length() > 0)
		ErrorDistribution = parts[1][0];

    /*
	int ApertureStopOrGratingType = atoi( parts[2].c_str() );
	int OpticalSurfaceNumber = atoi( parts[3].c_str() );
	int DiffractionOrder = atoi( parts[4].c_str() );
	double Reflectivity = atof( parts[5].c_str() );
	double Transmissivity = atof( parts[6].c_str() );
	double RMSSlope = atof( parts[7].c_str() );
	double RMSSpecularity = atof( parts[8].c_str() );
	double RefractionIndexReal = atof( parts[9].c_str() );
	double RefractionIndexImag = atof( parts[10].c_str() );
	double GratingCoeffs[4];
	GratingCoeffs[0] = atof( parts[11].c_str() );
	GratingCoeffs[1] = atof( parts[12].c_str() );
	GratingCoeffs[2] = atof( parts[13].c_str() );
	GratingCoeffs[3] = atof( parts[14].c_str() );
    */

	bool UseReflectivityTable = false;
	int refl_npoints = 0;
	double *refl_angles = 0;
	double *refls = 0;

	bool UseTransmissivityTable = false;
	int trans_npoints = 0;
	double* trans_angles = 0;
	double* transs = 0;

	if (parts.size() >= 17)
	{
		UseReflectivityTable = (atoi( parts[15].c_str() ) > 0);
		refl_npoints = atoi( parts[16].c_str() );
		if (parts.size() >= 19)
		{
			UseTransmissivityTable = (atoi(parts[17].c_str()) > 0);
			trans_npoints = atoi(parts[18].c_str());
		}
	}

	if (UseReflectivityTable)
	{
		refl_angles = new double[refl_npoints];
		refls = new double[refl_npoints];

		for (int i=0;i<refl_npoints;i++)
		{
			read_line(buf,1023,fp);
			sscanf(buf, "%lg %lg", &refl_angles[i], &refls[i]);
		}
	}
	if (UseTransmissivityTable)
	{
		trans_angles = new double[trans_npoints];
		transs = new double[trans_npoints];

		for (int i = 0; i < trans_npoints; i++)
		{
			read_line(buf, 1023, fp);
			sscanf(buf, "%lg %lg", &trans_angles[i], &transs[i]);
		}
	}

	// TODO: Update once optical surface params are implemented
	// st_optic( cxt, iopt, fb, ErrorDistribution,
	// 	OpticalSurfaceNumber, ApertureStopOrGratingType, DiffractionOrder,
	// 	RefractionIndexReal, RefractionIndexImag,
	// 	Reflectivity, Transmissivity,
	// 	GratingCoeffs, RMSSlope, RMSSpecularity,
	// 	UseReflectivityTable ? 1 : 0, refl_npoints,
	// 	refl_angles, refls,
	// 	UseTransmissivityTable? 1 : 0, trans_npoints,
	// 	trans_angles, transs
	// 	);

	if (refl_angles != 0) delete [] refl_angles;
	if (refls != 0) delete [] refls;
	if (trans_angles != 0) delete[] trans_angles;
	if (transs != 0) delete[] transs;
	return true;
}

bool SolTraceSystem::read_optic(FILE* fp) {
	if (!fp) return false;
	char buf[1024];
	read_line( buf, 1023, fp );

	if (strncmp( buf, "OPTICAL PAIR", 12) == 0)
	{
		read_optic_surface( fp );
		read_optic_surface( fp );
		return true;
	}
	else return false;
}

bool SolTraceSystem::read_element(FILE* fp) {
	
    //int ielm = ::st_add_element( cxt, istage );

    char buf[1024];
    read_line(buf, 1023, fp);

    std::vector<std::string> tok = split(buf, "\t", true, false);
    if (tok.size() < 29)
    {
        printf("too few tokens for element: %d\n", static_cast<int>(tok.size()));
        printf("\t>> %s\n", buf);
        return false;
    }

    if (tok[8][0] == 'c' && tok[17][0] == 'f')
    {
        //printf("Assuming cylindrical element cap. Skipping element. \n");
        return true;
    }

    // st_element_enabled( cxt, istage, ielm,  atoi( tok[0].c_str() ) ? 1 : 0 );
    auto elem = std::make_shared<CspElement>();
    Vec3d origin(atof(tok[1].c_str()),
                    atof(tok[2].c_str()),
                    atof(tok[3].c_str())); // origin of the element
    if (tok[8][0] == 'l' && tok[17][0] == 't')
    {
        // Cylindrical element, offset y coordinate by radius to center the cylinder
        origin[1] += 1 / atof(tok[18].c_str()); // tok[18] is 1 / radius
    }
    Vec3d aim_point(atof(tok[4].c_str()),
                       atof(tok[5].c_str()),
                       atof(tok[6].c_str())); // aim point of the element
    double zrot = atof(tok[7].c_str());       // z rotation of the element

    elem->set_origin(origin);
    elem->set_aim_point(aim_point);
    elem->set_zrot(zrot);

    // TODO: Add more aperature and surface types
    // Aperatures
    if (tok[8][0] == 'r')
    {
        double dim_x = atof(tok[9].c_str());
        double dim_y = atof(tok[10].c_str());
        auto aperture = std::make_shared<ApertureRectangle>(dim_x, dim_y);
        elem->set_aperture(aperture);
    }
    else if (tok[8][0] == 'l' && tok[17][0] == 't')
    {
        // In SolTrace STINPUT, this is the Single Axis Curvature Section Type
        // Used for cylindrical elements. TODO: Update if used elsewhere.
        double dim_x = 2 * (1 / atof(tok[18].c_str())); // tok[18] is 1 / radius
        double dim_y = atof(tok[11].c_str());           // Length of the cylinder
        auto aperture = std::make_shared<ApertureRectangle>(dim_x, dim_y);
        elem->set_aperture(aperture);
    }
    else
    {
        // Aperature type not implemented
        printf("Aperture type not implemented: %s\n", tok[8].c_str());
        return false;
    }

    // Surfaces
    if (tok[17][0] == 'p')
    {
        double curv_x = atof(tok[18].c_str());
        double curv_y = atof(tok[19].c_str());
        auto surface = std::make_shared<SurfaceParabolic>();
        surface->set_curvature(curv_x, curv_y);
        elem->set_surface(surface);
    }
    else if (tok[8][0] == 'l' && tok[17][0] == 't')
    {
        // In SolTrace STINPUT, this is the Cylindrical Type
        // Used for cylindrical elements.
        double radius = 1 / atof(tok[18].c_str()); // tok[18] is 1 / radius
        double half_height = atof(tok[11].c_str()) / 2;
        auto surface = std::make_shared<SurfaceCylinder>();
        surface->set_radius(radius);
        surface->set_half_height(half_height);
        elem->set_surface(surface);
    }
    else if (tok[17][0] == 'f')
    {
        auto surface = std::make_shared<SurfaceFlat>();
        elem->set_surface(surface);
    }
    else
    {
        // Surface type not implemented
        printf("Surface type not implemented: %s\n", tok[17].c_str());
        return false;
    }

    // st_element_optic( cxt, istage, ielm,  tok[27].c_str() );
    // st_element_interaction( cxt, istage, ielm,  atoi( tok[28].c_str()) );

    add_element(elem); // Add the element to the system

    return true;
}

bool SolTraceSystem::read_stage(FILE* fp) {
	
    if (!fp) return false;

	char buf[1024];
	read_line( buf, 1023, fp );

	int virt=0,multi=1,count=0,tr=0;
	double X, Y, Z, AX, AY, AZ, ZRot;


	sscanf(buf, "STAGE\tXYZ\t%lg\t%lg\t%lg\tAIM\t%lg\t%lg\t%lg\tZROT\t%lg\tVIRTUAL\t%d\tMULTIHIT\t%d\tELEMENTS\t%d\tTRACETHROUGH\t%d",
		&X, &Y, &Z,
		&AX, &AY, &AZ,
		&ZRot,
		&virt,
		&multi,
		&count,
		&tr );

	read_line( buf, 1023, fp ); // read name

	//printf("stage '%s': [%d] %lg %lg %lg   %lg %lg %lg   %lg   %d %d %d\n",
	//	buf, count, X, Y, Z, AX, AY, AZ, ZRot, virt, multi, tr );
	
	for (int i=0;i<count;i++)
		if (!read_element( fp )) 
		{ printf("error in element %d\n", i ); return false; }

	return true;
}

bool SolTraceSystem::read_system(FILE* fp) {
	
    if (!fp) return false;

	char buf[1024];

	char c = fgetc(fp);
	if ( c == '#' )
	{
		int vmaj = 0, vmin = 0, vmic = 0;
		read_line( buf, 1023, fp ); sscanf( buf, " SOLTRACE VERSION %d.%d.%d INPUT FILE", &vmaj, &vmin, &vmic);

		//unsigned int file_version = vmaj*10000 + vmin*100 + vmic;
		
		printf( "loading input file version %d.%d.%d\n", vmaj, vmin, vmic );
	}
	else
	{
		ungetc( c, fp );
		printf("input file must start with '#'\n");
		return false;
	}

	if ( !read_sun( fp ) ) return false;
	
	int count = 0;

	count = 0;
	read_line( buf, 1023, fp ); sscanf(buf, "OPTICS LIST COUNT\t%d", &count);
	
	for (int i=0;i<count;i++)
		if (!read_optic( fp )) return false;

	count = 0;
	read_line( buf, 1023, fp ); sscanf(buf, "STAGE LIST COUNT\t%d", &count);
	for (int i=0;i<count;i++)
		if (!read_stage( fp )) return false;

	return true;
}
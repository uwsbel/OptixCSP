#pragma once

namespace OptixCSP {

	enum class ApertureType {
		RECTANGLE,
		CIRCLE,
		TRIANGLE
	};


	// types for both scene building and pipeline assembly
	enum class SurfaceType {
		FLAT,
		PARABOLIC,
		MESH,
		CYLINDER
	};

	// mapping of the surface type combined with the aperture type
	// for lookup in the sbt mapping
	struct SurfaceApertureMap {
		SurfaceType surfaceType;
		ApertureType apertureType;

		// TODO: might not need this, since i always compare 
		// surface and aperture types separately .... 
		bool operator==(SurfaceApertureMap& map) {
			return (surfaceType == map.surfaceType) && (apertureType == map.apertureType);
		}

	};
}
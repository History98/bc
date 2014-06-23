#include "irrlicht.h"

#include "Buoy.hpp"
#include "RadarData.hpp"
#include "Angles.hpp"
#include "IniFile.hpp"

using namespace irr;

Buoy::Buoy(const std::string& name, const irr::core::vector3df& location, irr::f32 radarCrossSection, irr::scene::ISceneManager* smgr)
{

    //Load from individual buoy.ini file if it exists
    std::string buoyIniFilename = "Models/Buoy/";
    buoyIniFilename.append(name);
    buoyIniFilename.append("/buoy.ini");

    //get filename from ini file (or empty string if file doesn't exist)
    std::string buoyFileName = IniFile::iniFileToString(buoyIniFilename,"FileName");
    if (buoyFileName=="") {
        buoyFileName = "buoy.x"; //Default if not set
    }

    //get scale factor from ini file (or zero if not set - assume 1)
    f32 buoyScale = IniFile::iniFileTof32(buoyIniFilename,"Scalefactor");
    if (buoyScale==0.0) {
        buoyScale = 1.0; //Default if not set
    }

    std::string buoyFullPath = "Models/Buoy/"; //FIXME: Use proper path handling
    buoyFullPath.append(name);
    buoyFullPath.append("/");
    buoyFullPath.append(buoyFileName);

    //Load the mesh
    scene::IMesh* buoyMesh = smgr->getMesh(buoyFullPath.c_str());
	//add to scene node
	if (buoyMesh==0) {
        //Failed to load mesh - load with dummy and continue - ToDo: should also flag this up to user
        buoy = smgr->addCubeSceneNode(0.1);
    } else {
        buoy = smgr->addMeshSceneNode( buoyMesh, 0, -1, location );
    }

    //Set lighting to use diffuse and ambient, so lighting of untextured models works
	if(buoy->getMaterialCount()>0) {
        for(int mat=0;mat<buoy->getMaterialCount();mat++) {
            buoy->getMaterial(mat).ColorMaterial = video::ECM_DIFFUSE_AND_AMBIENT;
        }
    }

    buoy->setScale(core::vector3df(buoyScale,buoyScale,buoyScale));
    buoy->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true); //Normalise normals on scaled meshes, for correct lighting

    //store length and RCS information for radar etc
    length = buoy->getBoundingBox().getExtent().Z;
    height = buoy->getBoundingBox().getExtent().Y * 0.75; //Assume 3/4 of the mesh is above water

    rcs = radarCrossSection; //Value given to constructor by Buoys.
    if (rcs == 0.0) {
        rcs = 0.005*std::pow(length,3); //Default RCS if not set, base radar cross section on length^3 (following RCS table Ship_RCS_table.pdf)
    }

}

Buoy::~Buoy()
{
    //dtor
}

irr::core::vector3df Buoy::getPosition() const
{
    buoy->updateAbsolutePosition();//ToDo: This may be needed, but seems odd that it's required
    return buoy->getAbsolutePosition();
}

irr::f32 Buoy::getLength() const
{
    return length;
}

irr::f32 Buoy::getHeight() const
{
    return height;
}

irr::f32 Buoy::getRCS() const
{
    return rcs;
}

RadarData Buoy::getRadarData(irr::core::vector3df scannerPosition) const
//Get data relative to scannerPosition
//Similar code in OtherShip.cpp
{
    RadarData radarData;

    //Get information about this buoy, and return a RadarData struct containing info
    irr::core::vector3df contactPosition = getPosition();
    irr::core::vector3df relativePosition = contactPosition-scannerPosition;

    radarData.relX = relativePosition.X;
    radarData.relZ = relativePosition.Z;

    radarData.angle = relativePosition.getHorizontalAngle().Y;
    radarData.range = relativePosition.getLength();

    radarData.heading = 0.0;

    radarData.height=getHeight();
    radarData.solidHeight=0; //Assume buoy never blocks radar
    //radarData.radarHorizon=99999; //ToDo: Implement when ARPA is implemented
    radarData.length=getLength();
    radarData.rcs=getRCS();

    //Calculate angles and ranges to each end of the contact
    irr::f32 relAngle1 = Angles::normaliseAngle(irr::core::RADTODEG*std::atan2( radarData.relX + 0.5*radarData.length*std::sin(irr::core::DEGTORAD*radarData.heading), radarData.relZ + 0.5*radarData.length*std::cos(irr::core::DEGTORAD*radarData.heading) ));
    irr::f32 relAngle2 = Angles::normaliseAngle(irr::core::RADTODEG*std::atan2( radarData.relX - 0.5*radarData.length*std::sin(irr::core::DEGTORAD*radarData.heading), radarData.relZ - 0.5*radarData.length*std::cos(irr::core::DEGTORAD*radarData.heading) ));
    irr::f32 range1 = std::sqrt(std::pow(radarData.relX + 0.5*radarData.length*std::sin(irr::core::DEGTORAD*radarData.heading),2) + std::pow(radarData.relZ + 0.5*radarData.length*std::cos(irr::core::DEGTORAD*radarData.heading),2));
    irr::f32 range2 = std::sqrt(std::pow(radarData.relX - 0.5*radarData.length*std::sin(irr::core::DEGTORAD*radarData.heading),2) + std::pow(radarData.relZ - 0.5*radarData.length*std::cos(irr::core::DEGTORAD*radarData.heading),2));
    radarData.minRange=std::min(range1,range2);
    radarData.maxRange=std::max(range1,range2);
    radarData.minAngle=std::min(relAngle1,relAngle2);
    radarData.maxAngle=std::max(relAngle1,relAngle2);

    //Initial defaults: Will need changing with full implementation
    radarData.hidden=false;
    radarData.racon=""; //Racon code if set
    radarData.raconOffsetTime=0.0;
    radarData.SART=false;

    return radarData;
}

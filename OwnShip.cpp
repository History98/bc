/*   Bridge Command 5.0 Ship Simulator
     Copyright (C) 2014 James Packer

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License version 2 as
     published by the Free Software Foundation

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY Or FITNESS For A PARTICULAR PURPOSE.  See the
     GNU General Public License For more details.

     You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

//Extends from the general 'Ship' class
#include "OwnShip.hpp"

#include "Constants.hpp"
#include "SimulationModel.hpp"
#include "IniFile.hpp"

using namespace irr;

void OwnShip::load(const std::string& scenarioName, irr::scene::ISceneManager* smgr, SimulationModel* model, Terrain* terrain)
{
    //Store reference to terrain
    this->terrain = terrain;

    //construct scenario ownship.ini filename
    std::string scenarioOwnShipFilename = scenarioName;
    scenarioOwnShipFilename.append("/ownship.ini");

    //Load from ownShip.ini file
    std::string ownShipName = IniFile::iniFileToString(scenarioOwnShipFilename,"ShipName");
    //Get initial position and heading, and set these
    spd = IniFile::iniFileTof32(scenarioOwnShipFilename,"InitialSpeed")*KTS_TO_MPS;
    xPos = model->longToX(IniFile::iniFileTof32(scenarioOwnShipFilename,"InitialLong"));
    yPos = 0;
    zPos = model->latToZ(IniFile::iniFileTof32(scenarioOwnShipFilename,"InitialLat"));
    hdg = IniFile::iniFileTof32(scenarioOwnShipFilename,"InitialBearing");

    //Load from boat.ini file if it exists
    std::string shipIniFilename = "Models/OwnShip/";
    shipIniFilename.append(ownShipName);
    shipIniFilename.append("/boat.ini");

    //get the model file
    std::string ownShipFileName = IniFile::iniFileToString(shipIniFilename,"FileName");
    std::string ownShipFullPath = "Models/OwnShip/"; //FIXME: Use proper path handling
                ownShipFullPath.append(ownShipName);
                ownShipFullPath.append("/");
                ownShipFullPath.append(ownShipFileName);

    //Load dynamics settings
    shipMass = IniFile::iniFileTof32(shipIniFilename,"Mass");
    inertia = IniFile::iniFileTof32(shipIniFilename,"Inertia");
    maxEngineRevs = IniFile::iniFileTof32(shipIniFilename,"MaxRevs");
    dynamicsSpeedA = IniFile::iniFileTof32(shipIniFilename,"DynamicsSpeedA");
    dynamicsSpeedB = IniFile::iniFileTof32(shipIniFilename,"DynamicsSpeedB");
    dynamicsTurnDragA = IniFile::iniFileTof32(shipIniFilename,"DynamicsTurnDragA");
    dynamicsTurnDragB = IniFile::iniFileTof32(shipIniFilename,"DynamicsTurnDragB");
    rudderA = IniFile::iniFileTof32(shipIniFilename,"RudderA");
    rudderB = IniFile::iniFileTof32(shipIniFilename,"RudderB");
    rudderBAstern = IniFile::iniFileTof32(shipIniFilename,"RudderBAstern");
    maxForce = IniFile::iniFileTof32(shipIniFilename,"Max_propulsion_force");
    propellorSpacing = IniFile::iniFileTof32(shipIniFilename,"PropSpace");
    asternEfficiency = IniFile::iniFileTof32(shipIniFilename,"AsternEfficiency");// (Optional, default 1)
    if (asternEfficiency == 0)
        {asternEfficiency = 1;}
    propWalkAhead = IniFile::iniFileTof32(shipIniFilename,"PropWalkAhead");// (Optional, default 0)
    propWalkAstern = IniFile::iniFileTof32(shipIniFilename,"PropWalkAstern");// (Optional, default 0)

    //Todo: Missing:
    //Number of engines
    //CentrifugalDriftEffect
    //PropWalkDriftEffect
    //Buffet
    //Swell
    //Windage
    //WindageTurnEffect
    //Also:
    //DeviationMaximum
    //DeviationMaximumHeading
    //?Depth
    //?AngleCorrection

    //Start in engine control mode
    controlMode = MODE_ENGINE;

    //calculate max speed from dynamics parameters
    maxSpeedAhead  = ((-1 * dynamicsSpeedB) + sqrt((dynamicsSpeedB*dynamicsSpeedB)-4*dynamicsSpeedA*-2*maxForce))/(2*dynamicsSpeedA);
	maxSpeedAstern = ((-1 * dynamicsSpeedB) + sqrt((dynamicsSpeedB*dynamicsSpeedB)-4*dynamicsSpeedA*-2*maxForce*asternEfficiency))/(2*dynamicsSpeedA);

    //Fixme: These should be set from speed, and sliders set appropriately
    portEngine = requiredEngineProportion(spd);
    stbdEngine = requiredEngineProportion(spd);
    rudder = 0;

    //Scale
    f32 scaleFactor = IniFile::iniFileTof32(shipIniFilename,"ScaleFactor");
    f32 yCorrection = IniFile::iniFileTof32(shipIniFilename,"YCorrection");

    //camera offset (in unscaled and uncorrected ship coords)
    irr::u32 numberOfViews = IniFile::iniFileTof32(shipIniFilename,"Views");
    if (numberOfViews==0) {
        //ToDo: Tell user that view positions couldn't be loaded
        exit(EXIT_FAILURE);
    }
    for(int i=1;i<=numberOfViews;i++) {
        f32 camOffsetX = IniFile::iniFileTof32(shipIniFilename,IniFile::enumerate1("ViewX",i));
        f32 camOffsetY = IniFile::iniFileTof32(shipIniFilename,IniFile::enumerate1("ViewY",i));
        f32 camOffsetZ = IniFile::iniFileTof32(shipIniFilename,IniFile::enumerate1("ViewZ",i));
        views.push_back(core::vector3df(scaleFactor*camOffsetX,scaleFactor*(camOffsetY+yCorrection),scaleFactor*camOffsetZ));
    }

    //Load the model
    scene::IMesh* shipMesh = smgr->getMesh(ownShipFullPath.c_str());

    //Translate and scale mesh here
    core::matrix4 transformMatrix;
    transformMatrix.setScale(core::vector3df(scaleFactor,scaleFactor,scaleFactor));
    transformMatrix.setTranslation(core::vector3df(0,yCorrection*scaleFactor,0));
    smgr->getMeshManipulator()->transform(shipMesh,transformMatrix);

    //Make mesh scene node
    if (shipMesh==0) {
        //Failed to load mesh - load with dummy and continue - ToDo: should also flag this up to user
        ship = smgr->addCubeSceneNode(0.1);
    } else {
        ship = smgr->addMeshSceneNode(
                        shipMesh,
                        0,
                        -1,
                        core::vector3df(0,0,0));
    }

    ship->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, true); //Normalise normals on scaled meshes, for correct lighting

    //Set lighting to use diffuse and ambient, so lighting of untextured models works
	if(ship->getMaterialCount()>0) {
        for(int mat=0;mat<ship->getMaterialCount();mat++) {
            ship->getMaterial(mat).ColorMaterial = video::ECM_DIFFUSE_AND_AMBIENT;
        }
    }
}

void OwnShip::setRudder(irr::f32 rudder)
{
    controlMode = MODE_ENGINE; //Switch to engine and rudder mode
    //Set the rudder (-ve is port, +ve is stbd)
    this->rudder = rudder;
}

void OwnShip::setPortEngine(irr::f32 port)
{
    controlMode = MODE_ENGINE; //Switch to engine and rudder mode
    portEngine = port; //+-1
}

void OwnShip::setStbdEngine(irr::f32 stbd)
{
    controlMode = MODE_ENGINE; //Switch to engine and rudder mode
    stbdEngine = stbd; //+-1
}

irr::f32 OwnShip::getPortEngine() const
{
    return portEngine;
}

irr::f32 OwnShip::getStbdEngine() const
{
    return stbdEngine;
}

irr::f32 OwnShip::requiredEngineProportion(irr::f32 speed)
{
    irr::f32 proportion = 0;
    if (speed >= 0) {
        proportion=(dynamicsSpeedA*speed*speed + dynamicsSpeedB*speed)/(2*maxForce);
    } else {
        proportion=(-1*dynamicsSpeedA*speed*speed + dynamicsSpeedB*speed)/(2*maxForce*asternEfficiency);
    }
    return proportion;
}

void OwnShip::update(irr::f32 deltaTime, irr::f32 tideHeight)
{
    if (controlMode == MODE_ENGINE) {
        //Update spd and hdg with rudder and engine controls
        portThrust = portEngine * maxForce;
        stbdThrust = stbdEngine * maxForce;
        if (portThrust<0) {portThrust*=asternEfficiency;}
        if (stbdThrust<0) {stbdThrust*=asternEfficiency;}
        if (spd<0) { //Compensate for loss of sign when squaring
            drag = -1*dynamicsSpeedA*spd*spd + dynamicsSpeedB*spd;
		} else {
			drag =    dynamicsSpeedA*spd*spd + dynamicsSpeedB*spd;
		}
		acceleration = (portThrust+stbdThrust-drag)/shipMass;
        spd += acceleration*deltaTime;

        //Fixme:: Put turn proper dynamics here!
        hdg += rudder*deltaTime*0.1;
    } //End of engine mode

    //move, according to heading and speed
    xPos = xPos + sin(hdg*core::DEGTORAD)*spd*deltaTime;
    zPos = zPos + cos(hdg*core::DEGTORAD)*spd*deltaTime;
    yPos = tideHeight;

    //Set position & speed by calling own ship methods
    setPosition(core::vector3df(xPos,yPos,zPos));
    setRotation(core::vector3df(0, hdg, 0)); //Global vectors

}

irr::f32 OwnShip::getDepth()
{
    return -1*terrain->getHeight(xPos,zPos)+getPosition().Y;
}

std::vector<irr::core::vector3df> OwnShip::getCameraViews() const
{
    return views;
}



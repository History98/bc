#ifndef __SIMULATIONMODEL_HPP_INCLUDED__
#define __SIMULATIONMODEL_HPP_INCLUDED__

#include <vector>

#include "irrlicht.h"

#include "GUIMain.hpp"
#include "Buoy.hpp"
#include "OwnShip.hpp"

class SimulationModel //Start of the 'Model' part of MVC
{

public:

    SimulationModel(irr::IrrlichtDevice* dev, irr::video::IVideoDriver* drv, irr::scene::ISceneManager* scene, GUIMain* gui);

    void setSpeed(irr::f32 spd);
    void setHeading(irr::f32 hdg);
    const irr::f32 getSpeed();
    const irr::f32 getHeading();
    void updateModel();

private:
    irr::IrrlichtDevice* device;
    //irr::scene::IMeshSceneNode* ownShipNode;
    OwnShip ownShip;
    std::vector<Buoy> buoys;
    irr::video::IVideoDriver* driver;
    irr::scene::ISceneManager* smgr;
    irr::scene::ICameraSceneNode* camera;
    GUIMain* guiMain;

    //Ship movement
    irr::f32 heading;
    irr::f32 xPos;
    irr::f32 yPos;
    irr::f32 zPos;
    irr::f32 speed;
    irr::u32 currentTime;
    irr::u32 previousTime;
    irr::f32 deltaTime;

};
#endif

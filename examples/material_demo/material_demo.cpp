/* -*-c++-*- OpenSceneGraph Haptics Library - * Copyright (C) 2006 VRlab, Ume� University
*
* This application is open source and may be redistributed and/or modified   
* freely and without restriction, both in commericial and non commericial applications,
* as long as this copyright notice is maintained.
* 
* This application is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*!
  Author: Anders Backman, VRlab, Ume� University 2006-05-15

  Demonstrates different surface properties in osgHaptics.
  Uses a visitor (HapticMaterialVisitor that parses strings in node-descriptions
  to create an set properties on surfaces.
*/


#include <osgSensor/OsgSensorCallback.h>
#include <osgSensor/SensorMgr.h>

#include "HapticMaterialVisitor.h"

#include <osgHaptics/HapticDevice.h>
#include <osgHaptics/osgHaptics.h>
#include <osgHaptics/Shape.h>
#include <osgHaptics/VibrationForceOperator.h>
#include <osgHaptics/ForceEffect.h>
#include <osgHaptics/HapticRootNode.h>
#include <osgHaptics/HapticRenderPrepareVisitor.h>
#include <osgHaptics/SpringForceOperator.h>
#include <osgHaptics/TouchModel.h>
#include <osgHaptics/Shape.h>
#include <osgHaptics/Material.h>
#include <osgHaptics/BBoxVisitor.h>

#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/KeySwitchMatrixManipulator>


#include <osgViewer/Viewer>

#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>

#include <osg/Drawable>
#include <osg/PositionAttitudeTransform>
#include <osg/io_utils>
#include <osg/ShapeDrawable>
#include <osg/PolygonMode>
#include <osg/MatrixTransform>

#include <osg/Notify>

using namespace sensors;

/* Simple class for drawing the forcevector */
class VectorDrawable : public osg::Drawable{
public:
  VectorDrawable() : Drawable() {
    setUseDisplayList(false);
    setColor(1.0f,0.0f,0.0f);
    set(osg::Vec3(0,0,0), osg::Vec3(1,1,1));
  }

  VectorDrawable(const VectorDrawable& drawable, 
    const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY) : 
  osg::Drawable(drawable, copyop) {};

  META_Object(osg,VectorDrawable)


    void set(const osg::Vec3& start, const osg::Vec3& end) {
      m_start = start; m_end = end;
      dirtyBound();
    }

    void setColor(float r, float g, float b ) {  m_color.set(r,g,b); }

    void drawImplementation(osg::RenderInfo& state) const
    {
      glPushAttrib(GL_ALL_ATTRIB_BITS);
      glDisable(GL_LIGHTING);
      glColor3fv( m_color.ptr() );

      glBegin(GL_LINES);
      glVertex3fv(m_start.ptr());
      glVertex3fv(m_end.ptr());  
      glEnd();

      glPopAttrib();
    }

    osg::BoundingBox computeBound() const 
    {
      osg::BoundingBox bbox;
      bbox.expandBy(m_start);
      bbox.expandBy(m_end);
      return bbox;
    }
private:
  osg::Vec3 m_color;
  osg::Vec3 m_start, m_end;
};



int main( int argc, char **argv )
{

  // use an ArgumentParser object to manage the program arguments.
  osg::ArgumentParser arguments(&argc,argv);

  // set up the usage document, in case we need to print out how to use this program.
  arguments.getApplicationUsage()->setApplicationName(arguments.getApplicationName());
  arguments.getApplicationUsage()->setDescription(arguments.getApplicationName()+" is the standard OpenSceneGraph example which loads and visualises 3d models.");
  arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" [options] filename ...");
  arguments.getApplicationUsage()->addCommandLineOption("--image <filename>","Load an image and render it on a quad");
  arguments.getApplicationUsage()->addCommandLineOption("--dem <filename>","Load an image/DEM and render it on a HeightField");
  arguments.getApplicationUsage()->addCommandLineOption("-h or --help","Display command line paramters");
  arguments.getApplicationUsage()->addCommandLineOption("--help-env","Display environmental variables available");
  arguments.getApplicationUsage()->addCommandLineOption("--help-keys","Display keyboard & mouse bindings available");
  arguments.getApplicationUsage()->addCommandLineOption("--help-all","Display all command line, env vars and keyboard & mouse bindigs.");


  // construct the viewer.
  osgViewer::Viewer viewer(arguments);
	// Set threading model to SingleThreaded, otherwise we will get a hang in OpenHaptics.
	viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded); // CullDrawThreadPerContext also works

	osg::ref_ptr<osgGA::KeySwitchMatrixManipulator> keyswitchManipulator = new osgGA::KeySwitchMatrixManipulator;
	keyswitchManipulator->addMatrixManipulator( '1', "Trackball", new osgGA::TrackballManipulator() );
	viewer.setCameraManipulator( keyswitchManipulator.get() );


	// add the window size toggle handler
	viewer.addEventHandler(new osgViewer::WindowSizeHandler);

	// add the stats handler
	viewer.addEventHandler(new osgViewer::StatsHandler);

	// add the help handler
	viewer.addEventHandler(new osgViewer::HelpHandler(arguments.getApplicationUsage()));


  // get details on keyboard and mouse bindings used by the viewer.
  viewer.getUsage(*arguments.getApplicationUsage());

  // if user request help write it out to cout.
  bool helpAll = arguments.read("--help-all");
  unsigned int helpType = ((helpAll || arguments.read("-h") || arguments.read("--help"))? osg::ApplicationUsage::COMMAND_LINE_OPTION : 0 ) |
    ((helpAll ||  arguments.read("--help-env"))? osg::ApplicationUsage::ENVIRONMENTAL_VARIABLE : 0 ) |
    ((helpAll ||  arguments.read("--help-keys"))? osg::ApplicationUsage::KEYBOARD_MOUSE_BINDING : 0 );
  if (helpType)
  {
    arguments.getApplicationUsage()->write(std::cout, helpType);
    return 1;
  }

  // report any errors if they have occured when parsing the program aguments.
  if (arguments.errors())
  {
    arguments.writeErrorMessages(std::cout);
    return 1;
  }

  // read the scene from the list of file specified commandline args.
  osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFiles(arguments);

  // if no model has been successfully loaded report failure.
  if (!loadedModel.valid()) 
  {
    std::cout << arguments.getApplicationName() <<": No data loaded" << std::endl;
    return 1;
  }

  // optimize the scene graph, remove redundant nodes and state etc.
	// Lets not run the optimizer right now because due to a bug it
	// removes the static nodes even if they contain a description attached to it.
	// We need those to get the haptic material data.
  //osgUtil::Optimizer optimizer;
  //optimizer.optimize(loadedModel.get());

  // any option left unread are converted into errors to write out later.
  arguments.reportRemainingOptionsAsUnrecognized();

  // report any errors if they have occured when parsing the program aguments.
  if (arguments.errors())
  {
    arguments.writeErrorMessages(std::cout);
  }

  // Overall scene root
  osg::ref_ptr<osg::Group> root = new osg::Group;


  // Root of the visual scene
  osg::ref_ptr<osg::Group> visual_root = new osg::Group;

  root->addChild(visual_root.get());


  // pass the loaded scene graph to the viewer.
  viewer.setSceneData(root.get());

  // create the windows and run the threads.
  viewer.realize();

  try {
    // Create a haptic device
    osg::ref_ptr<osgHaptics::HapticDevice> haptic_device = new osgHaptics::HapticDevice();
		haptic_device->createContext();			
		haptic_device->makeCurrent(); // Make this device the current one


    // Root of the haptic scene
    osg::Camera *camera = viewer.getCamera();

    osg::ref_ptr<osgHaptics::HapticRootNode> haptic_root = new osgHaptics::HapticRootNode(camera);
    root->addChild(haptic_root.get());

    HapticMaterialVisitor vis(haptic_device.get());
    loadedModel->accept(vis);


    // add it to the visual node to be rendered visually
    visual_root->addChild(loadedModel.get());

    // Add it to the haptic root
    haptic_root->addChild(loadedModel.get());


    // Create a proxy sphere for rendering
    osg::ref_ptr<osg::Node> proxy_sphere = osgDB::readNodeFile("pen.ac"); // Load a pen geometry
    osg::ref_ptr<osg::MatrixTransform> proxy_transform = new osg::MatrixTransform;    
    proxy_transform->addChild(proxy_sphere.get());

    visual_root->addChild(proxy_transform.get());


    /*
    Get the bounding box of the loaded scene
    */
    osg::BoundingSphere bs = visual_root->getBound();
    float radius = bs.radius();

    /*
    There are many ways to specify a working volume for the haptics.
    One is to use the ViewFrustum.
    This requires use to set the VF to a limited volume so we dont get extensive scalings in any axis.
    Usually the far-field is set to something like 1000, which is not appropriate.

    Below is a way to restrain the near and far field.
    setWorkspaceMode(VIEW_MODE) will effectively use the viewfrustum as a haptic workspace.
    This will also make the haptic device follow the camera
    */
    //haptic_device->setWorkspaceMode(osgHaptics::HapticDevice::VIEW_MODE);

    //double hfov = viewer.getCamera()->getHorizontalFov();
    //double vfov = viewer.getCamera()->getVerticalFov();
    //viewer.getCamera(0)->getLens()->setPerspective( hfov, vfov, 1, 10 );

    int camera_no = 0;
    double left, right, bottom, top, nearclip, farclip;
    /*viewer.getCamera(camera_no)->getLens()->getParams(left, right, bottom, top, nearclip, farclip);
    viewer.getCamera(camera_no)->getLens()->setAutoAspect(false); 
    viewer.getCamera(camera_no)->getLens()->setFrustum(left, right, bottom, top, 1, 1.2*radius); 
    */
    //viewer.getCamera(0)->getLens()->setPerspective( hfov, vfov, 1, bs.radius()*3 );

	bool is_frustum = viewer.getCamera()->getProjectionMatrixAsFrustum(left, right, bottom, top, nearclip, farclip);
	assert( is_frustum );

    osg::BoundingBox bbox;
    osgHaptics::BBoxVisitor bvis;
    haptic_root->accept(bvis);
    bbox = bvis.getBoundingBox();
    //bbox.expandBy(osg::Vec3(4,3,1.5)*(-0.5));
    //bbox.expandBy(osg::Vec3(4,3,1.5)*(0.5));
    //bbox.expandBy(bs);


    //osg::Vec3 mmin(-0.5,-0.5, -0.5);
    //osg::Vec3 mmax(0.5,0.5, 0.5);

    /*
    Another way is to set the workspace of the Haptic working area to enclose the bounding box of the scene.
    Also, set the WorkSpace mode to use the Bounding box
    Notice that the haptic device will not follow the camera as it does by default. (VIEW_MODE)
    */
    haptic_device->setWorkspace(bbox._min, bbox._max);
    //haptic_device->setWorkspaceMode(osgHaptics::HapticDevice::BBOX_MODE);


    /* 
    A transformation using the TouchWorkSpace matrix will also scale all the force/position and orientation output
    from the haptic device.

    It will also affect the size, orientation and position of the haptic workspace.
    Practical if you want to rotate the workspace relative to the device.
    */
    osg::Matrixd tm;
    /*tm.makeRotate(osg::PI_4, osg::Vec3(1,0,0), 
    osg::PI_4*0.4, osg::Vec3(1,0,0), 
    osg::PI_4*1.4, osg::Vec3(1,0,0));*/
    //tm.makeScale(2,2,2);

    haptic_device->setTouchWorkspaceMatrix(tm);
    osg::ref_ptr<osgSensor::OsgSensor> sensor = new osgSensor::OsgSensor(haptic_device.get());
    osg::ref_ptr<osgSensor::OsgSensorCallback> sensor_callback = new osgSensor::OsgSensorCallback(sensor.get());

    proxy_transform->setUpdateCallback(sensor_callback.get());

    bool set_pos = false;


    // Add a custom drawable that draws the rendered force of the device
		VectorDrawable *force_drawable = 0L;
		if (0) {
			osg::Geode *geode = new osg::Geode;
			force_drawable = new VectorDrawable();
			geode->addDrawable(force_drawable);
			visual_root->addChild(geode);
		}

    /*
    Add pre and post draw callbacks to the camera so that we start and stop a haptic frame
    at a time when we have a valid OpenGL context.
    */
    osgHaptics::prepareHapticCamera(viewer.getCamera(), haptic_device.get(), root.get());



    while( !viewer.done() )
    {
      // wait for all cull and draw threads to complete.
      //viewer.sync();        

      // Update all registrated sensors (HapticDevice) is one.
      g_SensorMgr->update();

      // update the scene by traversing it with the the update visitor which will
      // call all node update callbacks and animations.
      //viewer.update();

			if (force_drawable) {
				osg::Vec3 start, end;

				start = haptic_device->getProxyPosition();
				osg::Vec3 force = haptic_device->getForce();
				float len = force.length();

				force.normalize();
				osg::Vec3 dir = force;

				// Update the force drawable with the current force of the device
				force_drawable->set(start, start+dir*len);
			}

      // fire off the cull and draw traversals of the scene.
      viewer.frame();        
    }

    // wait for all cull and draw threads to complete before exit.
    //viewer.sync();

    // Shutdown all registrated sensors
    osgSensor::SensorMgr::instance()->shutdown();

  } catch (std::exception& e) {
    osg::notify(osg::FATAL) << "Caught exception: " << e.what() << std::endl;
  }

  return 0;
}

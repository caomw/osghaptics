/* -*-c++-*- $Id: Version,v 1.2 2004/04/20 12:26:04 andersb Exp $ */
/**
* OsgHaptics - OpenSceneGraph Sensor Library
* Copyright (C) 2006 VRlab, Ume� University
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/

#ifndef __OsgSensorCallback_h__
#define __OsgSensorCallback_h__

#include <stdexcept>


#include <osgSensor/OsgSensor.h>
#include <osgSensor/export.h>

#include <osg/NodeVisitor>
#include <osg/NodeCallback>
#include <osg/Vec3>

namespace osgSensor {

  /// Connects a OsgSensor and a osg::Transform node

  /*!
  Updates the value of the Transform matrix with the value of the sensor
  */
  class  OSGSENSOR_EXPORT OsgSensorCallback : public osg::NodeCallback
  {
  public:
    /*!
    Constructor 
    \param sensor - A OsgSensor that contains the active matrix
    */
    OsgSensorCallback( OsgSensor *sensor );

    /// Callback operator. Updates the transformation with the matrix from the active sensor
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

    void update(osg::Node *node);
    
    /*!
      Enable/disables the callback
      If disabled, the Transform will not be updated with the current value of the sensor
    */
    void setEnable(bool flag) { m_enable = flag; }
    
    /*!
    Return true if the Callback is enabled
    */
    bool getEnable() const { return m_enable; }

  protected:

    osg::ref_ptr<OsgSensor> m_sensor;
    bool m_enable;
  private:
    int m_previousTraversalNumber;
  };


} // namespace sensors

#endif //__OsgSensor_h__


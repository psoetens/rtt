/***************************************************************************
 tag: Peter Soetens  Mon Jun 10 14:27:46 CEST 2002  ControlKernel.hpp 

                       ControlKernel.hpp -  description
                          -------------------
   begin                : Mon June 10 2002
   copyright            : (C) 2002 Peter Soetens
   email                : peter.soetens@mech.kuleuven.ac.be

***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/


#ifndef CONTROLKERNEL_HPP
#define CONTROLKERNEL_HPP

/**
 * @brief This is the main namespace of the control_kernel package. 
 *
 * It groups all classes that are meant for controlling 
 * a realtime (robot) system. It defines Kernels and Components.
 * The Kernel's main task is to provide a servoloop implementation,
 * but it can be extended in a generic way with extra functionality.
 *
 * The kernel_components package provides implementations of components
 * of this package.
 *
 * This package is documented in <a href="../../../orocos-control-kernel.html">
 * The Orocos ControlKernel Manual</a>.
 */
namespace ORO_ControlKernel {}

#endif

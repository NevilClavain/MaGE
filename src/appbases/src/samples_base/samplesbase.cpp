/* -*-LIC_BEGIN-*- */
/*
*
* MaGE rendering framework
* Emmanuel Chaumont Copyright (c) 2013-2025
*
* This file is part of MaGE.
*
*    MaGE is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    MaGE is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with MaGE.  If not, see <http://www.gnu.org/licenses/>.
*
*/
/* -*-LIC_END-*- */

#include "samplesbase.h"

#include "aspects.h"
#include "sysengine.h"
#include "d3d11system.h"
#include "timesystem.h"
#include "syncvariable.h"
#include "timemanager.h"
#include "resourcesystem.h"
#include "renderingqueuesystem.h"
#include "worldsystem.h"
#include "worldposition.h"
#include "shader.h"
#include "linemeshe.h"
#include "trianglemeshe.h"
#include "renderstate.h"

#include "dataprintsystem.h"

#include "logger_service.h"

#include "filesystem.h"
#include "logconf.h"

#include "entity.h"

#include "datacloud.h"

#include "animatorfunc.h"
#include "animators_helpers.h"


using namespace mage;
using namespace mage::core;
using namespace mage::rendering;


SamplesBase::SamplesBase()
{
	/////////// create common specific logger for events
	services::LoggerSharing::getInstance()->createLogger("Events");
}


mage::core::Entitygraph* SamplesBase::entitygraph()
{
	return &m_entitygraph;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


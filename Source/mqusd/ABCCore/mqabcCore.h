#pragma once

#define mqabcEnableHDF5

#define mqabcAttrVertexColor    "C"
#define mqabcAttrMaterialID     "MID"


#pragma warning(push)
#pragma warning(disable:4244 4245 4100)
#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#ifdef mqabcEnableHDF5
    #include <Alembic/AbcCoreHDF5/All.h>
#endif
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcMaterial/All.h>
#pragma warning(pop)

using namespace Alembic;
using abcV2 = Imath::V2f;
using abcV3 = Imath::V3f;
using abcV3i = Imath::V3i;
using abcV3d = Imath::V3d;
using abcV4 = Imath::V4f;
using abcC3 = Imath::C3f;
using abcC4 = Imath::C4f;
using abcM44 = Imath::M44f;
using abcM44d = Imath::M44d;
using abcBox = Imath::Box3f;
using abcBoxd = Imath::Box3d;
using abcChrono = Abc::chrono_t;

#include "mqusd.h"
#include "Foundation/mqusdSceneGraph.h"
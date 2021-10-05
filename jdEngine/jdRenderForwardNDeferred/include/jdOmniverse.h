/*****************************************************************************/
/**
 * @file    jdRenderFND.h
 * @author  Sergio Diaz (idv17c.sdiaz@uartesdigitales.edu.mx)
 * @date    06/DIC/2020
 * @brief   class for Render with forward and deferred shader metod
 *
 *
 *
 * @bug     No known bugs.
 */
 /*****************************************************************************/
#pragma once
#include <jdRenderFNDPrerequisites.h>
#include <jdModulo.h>
#include <jdString.h>
#include <jdModel.h>
#include <jdComponentLight.h>

#ifndef PCH_H
#define PCH_H

// TODO: add headers that you want to pre-compile here

#endif //PCH_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#ifdef _WIN32
#include <filesystem>
#include <conio.h>
#else
#include <experimental/filesystem>
#endif
#include <mutex>
#include <memory>
#include <map>
#include <condition_variable>

#include "OmniClient.h"
#include "OmniUsdLive.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/metrics.h"
#include <pxr/base/gf/matrix4f.h>
#include "pxr/base/gf/vec2f.h"
#include "pxr/usd/usdUtils/pipeline.h"
#include "pxr/usd/usdUtils/sparseValueWriter.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"
#include <pxr/usd/usdGeom/xform.h>
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdShade/tokens.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usd/modelAPI.h>

PXR_NAMESPACE_USING_DIRECTIVE

// Multiplatform array size
#define HW_ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))
// Private tokens for building up SdfPaths. We recommend
// constructing SdfPaths via tokens, as there is a performance
// cost to constructing them directly via strings (effectively,
// a table lookup per path element). Similarly, any API which
// takes a token as input should use a predefined token
// rather than one created on the fly from a string.
TF_DEFINE_PRIVATE_TOKENS(
	_tokens,
	(box)
	(model)
	(mesh)
	(obj)
	(Light)
	(Looks)
	(Root)
	(Shader)
	(st)

	// These tokens will be reworked or replaced by the official MDL schema for USD.
	// https://developer.nvidia.com/usd/MDLschema
	(Material)
	((_module, "module"))
	(name)
	(out)
	((shaderId, "mdlMaterial"))
	(mdl)

	// Tokens used for USD Preview Surface
	(diffuseColor)
	(normal)
	(file)
	(result)
	(varname)
	(rgb)
	(RAW)
	(sRGB)
	(surface)
	(PrimST)
	(UsdPreviewSurface)
	((UsdShaderId, "UsdPreviewSurface"))
	((PrimStShaderId, "UsdPrimvarReader_float2"))
	(UsdUVTexture)
);

namespace jdEngineSDK {
   
		class OmniversConnect
		{
		 public:
				/**
					* @brief default constructor
					*/
				OmniversConnect();

				/**
					* @brief default destructor
					*/
				~OmniversConnect();

				/**
					* @brief Function to check conection status
					* @param userData is a struct with userData
					* @param url is the url which is connected
					* @param status is status of conection
					* @note this function just bind, you don't have to do or sent anything
					*/
				static void
				OmniClientConnectionStatusCallbackImpl(void* userData, 
                                           const char* url, 
                                           OmniClientConnectionStatus status) noexcept;

				/**
					* @brief Function to notify fails
					* @param msg is the message
					* @param details have the details
					* @note this function just bind, you don't have to do or sent anything
					*/
				static void
				failNotify(const char* msg, const char* detail = nullptr, ...);

				/**
					* @brief Function to make log
					* @note this function just bind, you don't have to do or sent anything, and you can
					*       desactive it with the member gOmniverseLoggingEnabled
					*/
				static void
				logCallback(const char* threadName, 
                const char* component, 
                OmniClientLogLevel level, 
                const char* message) noexcept;

				/**
					* @brief Function to release omniverse
					* @note this function has to be called after did all with omniverse 
					*/
				static void
				shutdownOmniverse();
		
				/**
					* @brief Function to start omniverse
					* @return true if omniverse started successfully
					* @note this function has to be called before did anything with omniverse
					*/
				static bool
				startOmniverse();

				/**
					* @brief Function create a stage (.usd)
					* @param destinationPath is the path in the server to create the stage
					* @param stageName is the name which is going to created
					* @return the a string with the all url path created
					* @note this function set the new stage as the current stage
					*/
				static String
				createOmniverseStage(const String& destinationPath, const String& stageName);

				/**
					* @brief Function create a stage (.usd)
					* @param destinationPath is the path in the server to create the stage
					* @return the a string with the all url path created
					* @note this function set the new stage as the current stage
					*/
				static String
				createOmniverseStage(const String& destinationPath);
		
				/**
					* @brief Function to check if a url is valid (if exists)
					* @param maybeURL is the path/Url to check
					* @return true if the url exists
					*/
				static bool
				isValidOmniURL(const std::string& maybeURL);
		
				/**
					* @brief Function to print the username is used 
					* @param stageUrl is teh satage to check
					*/
				static void
				printConnectedUsername(const String& stageUrl);
		
				/**
					* @brief Function to create a box and save the data in a geomMesh and in the stage (usd) 
					* @param boxNumber is only for add the number to the mesh's name;
					* @return a geomMesh with the box data 
					*/
				static UsdGeomMesh
				createBox(int boxNumber = 0);

				/**
					* @brief Function to create a geomMesh with a model and save it in the current stage 
					* @param model is the model to get data
					* @return a geomMesh with the model data 
					* @note this is the first version to save the model, save each mesh like a geomMesh, but
					*       in the stage you can move each part (mesh) for separate.
					*/
				static UsdGeomMesh
				createFromModel(SPtr<Model> model);

    /**
					* @brief Function to create a geomMesh with a model and save it in the current stage 
					* @param model is the model to get data
					* @return a geomMesh with the model data 
					* @note this is the second version to save the model, save all the model's meshes in only
					*       one geomMesh but create subset for each mesh, with this you have only one mesh in
					*       the stage but you can set diferents materials for each subSet
					*/
				static UsdGeomMesh
				createFromModelV2(SPtr<Model> model);

    /**
					* @brief Function to create a geomMesh with a model and save it in the current stage 
					* @param model is the model to get data
					* @param m_parentPath is parentPart
					* @return a geomMesh with the model data 
					* @note this is the second version to save the model, save all the model's meshes in only
					*       one geomMesh but create subset for each mesh, with this you have only one mesh in
					*       the stage but you can set diferents materials for each subSet
					*/
				static UsdGeomMesh
				createFromModelInPath(SPtr<Model> model, SdfPath& m_parentPath);

				/**
					* @brief Function to get the first geom mesh found in current stage 
					* @return a model created with de geomMesh data
					*/
				static SPtr<Model> 
				getModelFromStage();


				/**
					* @brief Function to get teh geomMesh data an create a Model
					* @return a model created with de geomMesh data
					* @note this is the first version to get the model, only get the first found and only the
					*	      first mesh
					*/
				static SPtr<Model>
				geomMeshToModel(UsdGeomMesh gMesh);

				/**
					* @brief Function to get teh geomMesh data an create a Model
					* @return a model created with de geomMesh data
					* @note this is the second version to get the model, get all the data of the geomMesh and
					*       separe the data to meshes with the subsets data.
					*/
				static SPtr<Model>
				geomMeshToModelV2(UsdGeomMesh gMesh);

				/**
					* @brief Function to get teh geomMesh data an create a Model
					* @return a model created with de geomMesh data
					* @note this is the second version to get the model, get all the data of the geomMesh and
					*       separe the data to meshes with the subsets data.
					*/
				static SPtr<Model>
				getJDModelFromGeoMesh(UsdGeomMesh gMesh);

				/**
					* @brief Function Create a light source in the current scene (stage).
					*/
				static void 
				createLight();

				static UsdPrim
				createLightFromComponent(CLight*, SdfPath& m_parentPath);

				static UsdPrim
				crateDirectLight(CLight*, SdfPath& m_parentPath);
		
				/**
					* @brief Function to place a checkpoint in the stage history (You can see in the console or in create).
					* @param stageUrl is the url to place the check point (You need put all de path)
					* @param comment is the message you want to say the checkpoint
					*/
				static void 
				checkpointFile(const char* stageUrl, const char* comment);

				/**
					* @brief Function to Upload a material and its textures to the Omniverse Server
					* @param destinationPath is the path to save the material
					* @note the textures and the names material are define in the function, this is only to test
					*       not to create material
					*/
				static void 
				uploadMaterial(const std::string& destinationPath);

				/**
					* @brief Function to Bind a material to this geometry
					* @param meshIn is geomMesh to bind the material
					* @note bind the material created with uploadMaterial function
					*/
				static void 
				createMaterial(UsdGeomMesh meshIn);

				/**
					* @brief Function to open a stage an set it as the current stage
					* @param stagePath is stage's path
					* @return false if cant found the path
					* @note if cant found the path, the current stage will be null
					*/
				static bool
				openStage(const String& stagePath);

				/**
					* @brief Function to open a stage an set it as the current stage
					* @param stagePath is stage's path
					* @return false if cant found the path
					* @note if cant found the path, the current stage will be null
					*/
				static bool
				openStage(const String& folder, const String& stageName);

				/**
					* @brief Function to create a folder
					* @param emptyFolderPath is the path to create the folder with the folde name
					* @note Example of path "c::users/folderName"
					*/
				static void 
				createEmptyFolder(const std::string& emptyFolderPath);

				static void
				SetOp(UsdGeomXformable& xForm, 
          UsdGeomXformOp& op, 
          UsdGeomXformOp::Type opType, 
          const GfVec3d& value, 
          const UsdGeomXformOp::Precision precision) {
						if (!op)
						{
								op = xForm.AddXformOp(opType, precision);
								std::unique_lock<std::mutex> lk(m_logMutex);
								std::cout << " Adding " << UsdGeomXformOp::GetOpTypeToken(opType) << std::endl;
						}

						if (op.GetPrecision() == UsdGeomXformOp::Precision::PrecisionFloat)
								op.Set(GfVec3f(value));
						else
								op.Set(value);

						std::unique_lock<std::mutex> lk(m_logMutex);
						std::cout << " Setting " << UsdGeomXformOp::GetOpTypeToken(opType) << std::endl;
				}

				/**
					* @brief Function to create a folder
					* @param emptyFolderPath is the path to create the folder with the folde name
					* @note Example of path "c::users/folderName"
					*/
				static void
				getSceneGraph();

				/**
					* @brief Function to create a folder
					* @param emptyFolderPath is the path to create the folder with the folde name
					* @note Example of path "c::users/folderName"
					*/
				static void
				getSceneGraphV2();

				static void
				getObjects(UsdPrim& prim, SPtr<GameObject> parent);

				/**
					* @brief Function to save sceneGraph in the currentStage
					*/
				static void
				saveSceneGraph();
				
				static void
				saveObject(SPtr<GameObject> obj, SdfPath& m_parentPath, String& name);

				static void 
				resetCurrentStage();

				static void
				getTransformComponents(UsdGeomXformable& inXform, 
                           JDVector3& outPos, 
                           JDVector3& outRot, 
                           JDVector3& outScal);

				static void
				setTransformComponents(UsdGeomXformable& outXform, 
                           JDVector3& inPos, 
                           JDVector3& inRot, 
                           JDVector3& inScal);

				static void 
				inUpdateData();

				static void 
				outUpdateData();

				/**
	    * @brief current stage
	    * @note if is null none stage was open
					*/
				static UsdStageRefPtr m_stage;
				
				/**
					* @brief bool to active or desative the logs
					*/
				static bool m_omniverseLoggingEnabled;

				/**
					* @brief log mutex
					*/
    static std::mutex m_logMutex;

				/**
					* @brief bool to check if omnivers already startup
					*/
    static bool m_isStartUp;
		
				//JD_PLUGIN_EXPORT bool isValidURL;

				static SdfPath m_rootPrimPath;

				static String m_currentURL;

				static String m_currentFolder;
				
				static String m_currentStageName;

				static uint32 m_numObjects;
		};

}

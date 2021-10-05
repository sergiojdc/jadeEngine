#include "jdOmniverse.h"
#include <jdGraphicApi.h>
#include <jdSceneGraph.h>
#include <jdResourceManager.h>
#include <jdComponentTransform.h>
#include <jdQuaternion.h>
#include <jdDegree.h>
#include <jdComponentRenderModel.h>
#include <jdRender.h>

namespace jdEngineSDK {

  bool OmniversConnect::m_omniverseLoggingEnabled = false;
  bool OmniversConnect::m_isStartUp = false;
  UsdStageRefPtr OmniversConnect::m_stage = nullptr;
  std::mutex OmniversConnect::m_logMutex = std::mutex();
  SdfPath OmniversConnect::m_rootPrimPath = SdfPath();
  String OmniversConnect::m_currentURL = "";
  String OmniversConnect::m_currentFolder = "";
  String OmniversConnect::m_currentStageName = "";
  uint32 OmniversConnect::m_numObjects = 0;

		OmniversConnect::OmniversConnect()
		{
		}
  
		OmniversConnect::~OmniversConnect()
		{
		}
		
  void
		OmniversConnect::OmniClientConnectionStatusCallbackImpl(void* userData, 
                                                          const char* url, 
                                                          OmniClientConnectionStatus status) noexcept	{
				// Let's just print this regardless
    {
    	 std::unique_lock<std::mutex> lk(m_logMutex);
    	 std::cout << "Connection Status: " << omniClientGetConnectionStatusString(status) << " [" << url << "]" << std::endl;
    }
    if (status == eOmniClientConnectionStatus_ConnectError)
    {
    	 // We shouldn't just exit here - we should clean up a bit, but we're going to do it anyway
    	 std::cout << "[ERROR] Failed connection, exiting." << std::endl;
    	 exit(-1);
    }
		}
  
  void 
  OmniversConnect::failNotify(const char* msg, const char* detail, ...) {
    std::unique_lock<std::mutex> lk(m_logMutex);
  
    fprintf(stderr, "%s\n", msg);
    if (detail != nullptr)
    {
      fprintf(stderr, "%s\n", detail);
    }
  }
  
  void
  OmniversConnect::logCallback(const char* 
                               threadName, 
                               const char* component, 
                               OmniClientLogLevel level, 
                               const char* message) noexcept {
    std::unique_lock<std::mutex> lk(m_logMutex);
    if (m_omniverseLoggingEnabled)
    {
      puts(message);
    }
  }
  
  void 
  OmniversConnect::shutdownOmniverse() {
    // Calling this prior to shutdown ensures that all pending live updates complete.
    omniUsdLiveWaitForPendingUpdates();
  
    // The stage is a sophisticated object that needs to be destroyed properly.  
    // Since m_stage is a smart pointer we can just reset it
    m_stage.Reset();
  
    //omniClientTick(1000);
    omniClientShutdown();
    m_isStartUp = false;
  }
  
  bool 
  OmniversConnect::startOmniverse() {
    // Register a function to be called whenever the library wants to print something to a log
    omniClientSetLogCallback(logCallback);
  
    // The default log level is "Info", set it to "Debug" to see all messages
    omniClientSetLogLevel(eOmniClientLogLevel_Debug);
  
    // Initialize the library and pass it the version constant defined in OmniClient.h
    // This allows the library to verify it was built with a compatible version. It will
    // return false if there is a version mismatch.
    if (!omniClientInitialize(kOmniClientVersion))
    {
      m_isStartUp = false;
      return false;
    }
  
    omniClientRegisterConnectionStatusCallback(nullptr, OmniClientConnectionStatusCallbackImpl);
  
    // Enable live updates
    omniUsdLiveSetDefaultEnabled(true);
    m_isStartUp = true;
    return true;
  }
  
  String 
  OmniversConnect::createOmniverseStage(const String& destinationPath, 
                                        const String& stageName) {
    String realFolder;
    String realName;
    String realPath;
    uint32 index = 0;
    while ('\0' != destinationPath[index]) {
      realFolder.push_back(destinationPath[index]);
      ++index;
    }
    index = 0;
    while ('\0' != stageName[index]) {
      realName.push_back(stageName[index]);
      ++index;
    }
    if (String::npos == realName.find(".usd")) {
      realName += ".usd";
    }
    realPath = realFolder + "/" + realName;

  
    // Delete the old version of this file on Omniverse and wait for the operation to complete
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "Waiting for " << realPath << " to delete... " << std::endl;
    }
    omniClientWait(omniClientDelete(realPath.c_str(), nullptr, nullptr));
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "finished" << std::endl;
    }
  
    // Create this file in Omniverse cleanly
    m_stage = UsdStage::CreateNew(realPath);
    if (!m_stage) {
      failNotify("Failure to create model in Omniverse", realPath.c_str());
      return std::string();
    }
  
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "New stage created: " << realPath << std::endl;
    }
  
    // Always a good idea to declare your up-ness
    UsdGeomSetStageUpAxis(m_stage, UsdGeomTokens->y);
    m_currentURL = realPath;
    m_currentFolder = realFolder;
    m_currentStageName = realName;
    return realPath;
  }
  
  String 
  OmniversConnect::createOmniverseStage(const String& destinationPath) {
    
    std::string stageUrl = destinationPath;
    if (String::npos == destinationPath.find(".usd")) {
      stageUrl += ".usd";
    }

    // Delete the old version of this file on Omniverse and wait for the operation to complete
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "Waiting for " << stageUrl << " to delete... " << std::endl;
    }
    omniClientWait(omniClientDelete(stageUrl.c_str(), nullptr, nullptr));
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "finished" << std::endl;
    }

    // Create this file in Omniverse cleanly
    m_stage = UsdStage::CreateNew(stageUrl);
    if (!m_stage) {
      failNotify("Failure to create model in Omniverse", stageUrl.c_str());
      return std::string();
    }

    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "New stage created: " << stageUrl << std::endl;
    }

    // Always a good idea to declare your up-ness
    UsdGeomSetStageUpAxis(m_stage, UsdGeomTokens->y);
    m_currentURL = stageUrl;
    m_stage->Save();
    omniUsdLiveProcess();
    return stageUrl;
  }

  bool
  OmniversConnect::isValidOmniURL(const std::string& maybeURL) {
    bool isValidURL = false;
    OmniClientUrl* url = omniClientBreakUrl(maybeURL.c_str());
    if (url->host && url->path &&
        (String(url->scheme) == String("omniverse") ||
        String(url->scheme) == String("omni"))) {
      isValidURL = true;
    }
    omniClientFreeUrl(url);
    if (!isValidURL) {
      std::cout << "This is not an Omniverse Nucleus URL: " << maybeURL.c_str() << std::endl;
      std::cout << "Correct Omniverse URL format is: omniverse://server_name/Path/To/Example/Folder" << std::endl;
      std::cout << "Allowing program to continue because file paths may be provided in the form: C:/Path/To/Stage" << std::endl;
    }
    return isValidURL;
  }
  
  void 
  OmniversConnect::printConnectedUsername(const String& stageUrl) {
    // Get the username for the connection
    String userName("_none_");
    omniClientWait(omniClientGetServerInfo(stageUrl.c_str(), 
                                           &userName, 
                                           [](void* userData, 
                                           OmniClientResult result, 
                                           struct OmniClientServerInfo const* info) noexcept {
                                             std::string* userName = static_cast<std::string*>(userData);
                                             if (userData && userName && info && info->username)
                                             {
                                               userName->assign(info->username);
                                             }
                                           }));
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "Connected username: " << userName << std::endl;
    }
  }
  
  UsdGeomMesh 
  OmniversConnect::createBox(int boxNumber) {
    // Create a simple box in USD with normals and UV information
    static double h = 50.0;
    static int gBoxVertexIndices[] = { 0, 1, 2, 1, 3, 2, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11, 12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23 };
    static double gBoxNormals[][3] = { {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0} };
    static double gBoxPoints[][3] = { {h, -h, -h}, {-h, -h, -h}, {h, h, -h}, {-h, h, -h}, {h, h, h}, {-h, h, h}, {-h, -h, h}, {h, -h, h}, {h, -h, h}, {-h, -h, h}, {-h, -h, -h}, {h, -h, -h}, {h, h, h}, {h, -h, h}, {h, -h, -h}, {h, h, -h}, {-h, h, h}, {h, h, h}, {h, h, -h}, {-h, h, -h}, {-h, -h, h}, {-h, h, h}, {-h, h, -h}, {-h, -h, -h} };
    static float gBoxUV[][2] = { {0, 0}, {0, 1}, {1, 1}, {1, 0}, {0, 0}, {0, 1}, {1, 1}, {1, 0}, {0, 0}, {0, 1}, {1, 1}, {1, 0}, {0, 0}, {0, 1}, {1, 1}, {1, 0}, {0, 0}, {0, 1}, {1, 1}, {1, 0}, {0, 0}, {0, 1}, {1, 1}, {1, 0} };
  
    // Keep the model contained inside of "Root", only need to do this once per model
    //SdfPath rootPrimPath = SdfPath::AbsoluteRootPath().AppendChild(_tokens->Root);
    //UsdGeomXform::Define(m_stage, m_rootPrimPath);
  
    // Create the geometry inside of "Root"
    std::string boxName("box_");
    boxName.append(std::to_string(boxNumber));
    SdfPath boxPrimPath = m_rootPrimPath.AppendChild(TfToken(boxName));//_tokens->box);
    UsdGeomMesh Model = UsdGeomMesh::Define(m_stage, boxPrimPath);
  
    if (!Model)
      return Model;
  
    // Set orientation
    Model.CreateOrientationAttr(VtValue(UsdGeomTokens->rightHanded));
  
    // Add all of the vertices
    int num_vertices = HW_ARRAY_COUNT(gBoxPoints);
    VtArray<GfVec3f> points;
    points.resize(num_vertices);
    for (int i = 0; i < num_vertices; i++)
    {
      points[i] = GfVec3f(gBoxPoints[i][0], gBoxPoints[i][1], gBoxPoints[i][2]);
    }
    Model.CreatePointsAttr(VtValue(points));
  
    // Calculate indices for each triangle
    int num_indices = HW_ARRAY_COUNT(gBoxVertexIndices); // 2 Triangles per face * 3 Vertices per Triangle * 6 Faces
    VtArray<int> vecIndices;
    vecIndices.resize(num_indices);
    for (int i = 0; i < num_indices; i++)
    {
      vecIndices[i] = gBoxVertexIndices[i];
    }
    Model.CreateFaceVertexIndicesAttr(VtValue(vecIndices));
  
    // Add vertex normals
    int num_normals = HW_ARRAY_COUNT(gBoxNormals);
    VtArray<GfVec3f> meshNormals;
    meshNormals.resize(num_vertices);
    for (int i = 0; i < num_vertices; i++)
    {
      meshNormals[i] = GfVec3f((float)gBoxNormals[i][0], (float)gBoxNormals[i][1], (float)gBoxNormals[i][2]);
    }
    Model.CreateNormalsAttr(VtValue(meshNormals));
  
    // Add face vertex count
    VtArray<int> faceVertexCounts;
    faceVertexCounts.resize(12); // 2 Triangles per face * 6 faces
    std::fill(faceVertexCounts.begin(), faceVertexCounts.end(), 3);
    Model.CreateFaceVertexCountsAttr(VtValue(faceVertexCounts));
  
    // Set the color on the Model
    UsdPrim meshPrim = Model.GetPrim();
    UsdAttribute displayColorAttr = Model.CreateDisplayColorAttr();
    {
      VtVec3fArray valueArray;
      GfVec3f rgbFace(0.463f, 0.725f, 0.0f);
      valueArray.push_back(rgbFace);
      displayColorAttr.Set(valueArray);
    }
  
    // Set the UV (st) values for this Model
    UsdGeomPrimvar attr2 = Model.CreatePrimvar(_tokens->st, SdfValueTypeNames->TexCoord2fArray);
    {
      int uv_count = HW_ARRAY_COUNT(gBoxUV);
      VtVec2fArray valueArray;
      valueArray.resize(uv_count);
      for (int i = 0; i < uv_count; ++i)
      {
        valueArray[i].Set(gBoxUV[i]);
      }
  
      bool status = attr2.Set(valueArray);
    }
    attr2.SetInterpolation(UsdGeomTokens->vertex);
  
    // Commit the changes to the USD
    m_stage->Save();
    omniUsdLiveProcess();
  
    return Model;
  }

  UsdGeomMesh 
  OmniversConnect::createFromModel(SPtr<Model> model)
  {
    //This function save each model's meshes as a geomMesh, each of them have its own transform and material

    // Keep the model contained inside of "Root", only need to do this once per model
    //SdfPath rootPrimPath = SdfPath::AbsoluteRootPath().AppendChild(_tokens->Root);
    //UsdGeomXform::Define(m_stage, rootPrimPath);
    //Set the model token
    std::string boxName("model_");
    boxName.append(std::to_string(1));
    SdfPath gModelPrimPath = m_rootPrimPath.AppendChild(TfToken(boxName));//_tokens->box);
    //define the path in the stage like a geomMesh
    UsdGeomMesh gModel = UsdGeomMesh::Define(m_stage, gModelPrimPath);
    if (!gModel)
      return gModel;
    // Set orientation
    gModel.CreateOrientationAttr(VtValue(UsdGeomTokens->rightHanded));

    //get the mesh num
    uint32 numMeshe = model->m_meshes.size();
    for (uint32  m = 0; m < numMeshe; ++m) {
      //set the mesh token
      std::string MeshName("mesh_");
      MeshName.append(std::to_string(m));
      SdfPath gMeshPrimPath = gModelPrimPath.AppendChild(TfToken(MeshName));//_tokens->box);
      //define the mesh path as a geom mesh, the path is a child of model
      UsdGeomMesh gMesh = UsdGeomMesh::Define(m_stage, gMeshPrimPath);
      if (!gMesh)
        return gModel;
      // Set orientation
      gMesh.CreateOrientationAttr(VtValue(UsdGeomTokens->rightHanded));

      // Add all of the vertices
      int num_vertices = model->m_meshes[m]->m_numVertex;
      auto vertexData = model->m_meshes[m]->m_vertexData.get();
      VtArray<GfVec3f> points;
      points.resize(num_vertices);
      for (int i = 0; i < num_vertices; i++)
      {
        auto vec = vertexData[i].Pos;
        points[i] = GfVec3f(vec.x, vec.y, vec.z);
      }
      //set the vertex pouints to geomMesh
      gMesh.CreatePointsAttr(VtValue(points));

      // Calculate indices for each triangle
      int num_indices = model->m_meshes[m]->m_numIndex;
      auto indices = model->m_meshes[m]->m_index.get();
      VtArray<int> vecIndices;
      vecIndices.resize(num_indices);
      for (int i = 0; i < num_indices; i++)
      {
        vecIndices[i] = indices[i];
      }
      //set the index to geomMesh
      gMesh.CreateFaceVertexIndicesAttr(VtValue(vecIndices));

      // Add vertex normals
      int num_normals = model->m_meshes[m]->m_numVertex;
      VtArray<GfVec3f> meshNormals;
      meshNormals.resize(num_vertices);
      for (int i = 0; i < num_vertices; i++)
      {
        auto norm = vertexData[i].Norm;
        meshNormals[i] = GfVec3f(norm.x, norm.y, norm.z);
      }
      //set the vertex normals to geomMesh
      gMesh.CreateNormalsAttr(VtValue(meshNormals));

      // Add face vertex count
      VtArray<int> faceVertexCounts;
      faceVertexCounts.resize(num_indices / 3); // num_indices / 3 (3 index per face)
      std::fill(faceVertexCounts.begin(), faceVertexCounts.end(), 3);
      gMesh.CreateFaceVertexCountsAttr(VtValue(faceVertexCounts));

      // Set the color on the Model
      UsdPrim meshPrim = gMesh.GetPrim();
      UsdAttribute displayColorAttr = gMesh.CreateDisplayColorAttr();
      {
        VtVec3fArray valueArray;
        GfVec3f rgbFace(0.463f, 0.725f, 0.0f);
        valueArray.push_back(rgbFace);
        displayColorAttr.Set(valueArray);
      }

      // Set the UV (st) values for this Model
      UsdGeomPrimvar attr2 = gMesh.CreatePrimvar(_tokens->st, SdfValueTypeNames->TexCoord2fArray);
      {
        int uv_count = model->m_meshes[m]->m_numVertex;
        VtVec2fArray valueArray;
        valueArray.resize(uv_count);
        for (int i = 0; i < uv_count; ++i)
        {
          auto uv = vertexData[i].TexCoord;
          valueArray[i].Set(uv.Vector);
        }

        bool status = attr2.Set(valueArray);
      }
      attr2.SetInterpolation(UsdGeomTokens->vertex);
    }

    // Commit the changes to the USD
    m_stage->Save();
    omniUsdLiveProcess();

    return gModel;
  }

  UsdGeomMesh 
  OmniversConnect::createFromModelV2(SPtr<Model> model) {
    //This function save all model's meshes in one geomMesh and create subset of it for each mesh,
    //All have the same transform but each can has diferent material

    // Keep the model contained inside of "Root", only need to do this once per model
    //SdfPath rootPrimPath = SdfPath::AbsoluteRootPath().AppendChild(_tokens->Root);
    //UsdGeomXform::Define(m_stage, rootPrimPath);

    //set geomMesh token name
    std::string boxName("model_");
    boxName.append(std::to_string(1));
    SdfPath gModelPrimPath = m_rootPrimPath.AppendChild(TfToken(boxName));
    //define the geomMesh in the modelPath which is a child of root
    UsdGeomMesh gModel = UsdGeomMesh::Define(m_stage, gModelPrimPath);
    if (!gModel)
      return gModel;
    // Set orientation
    gModel.CreateOrientationAttr(VtValue(UsdGeomTokens->rightHanded));
    //Get number of meshes
    uint32 numMeshe = model->m_meshes.size();
    //declare vars to combine in one array
    int num_vertices = 0; //num of all vetex
    int num_indices = 0; //num of all index
    int currentMaxindex = 0; // Current Mx index, is use to organize the index of meshes
    int Maxindex = 0; // this use to comparet the index with the current max index, after currentMaxIndex equaled to this
    int currentVertexIndex = 0; // this is the current index in the vector of vertex info 
    int currentindexIndex = 0; // this is the current index in the vector of index
    //Get the vertex and index num of all meshes
    for (uint32 m = 0; m < numMeshe; ++m) {
      num_vertices += model->m_meshes[m]->m_numVertex;
      num_indices += model->m_meshes[m]->m_numIndex;
    }

    //create arrays for all the thiks.
    VtArray<GfVec3f> points;
    points.resize(num_vertices);

    VtArray<int> vecIndices;
    vecIndices.resize(num_indices);
    Vector<VtArray<int>> vecVecIndices;
    vecVecIndices.resize(numMeshe);

    int uv_count = num_vertices;
    VtVec2fArray valueArray;
    valueArray.resize(uv_count);


    int num_normals = num_vertices;
    VtArray<GfVec3f> meshNormals;
    meshNormals.resize(num_vertices);


    UsdGeomPrimvar attr2 = gModel.CreatePrimvar(_tokens->st, SdfValueTypeNames->TexCoord2fArray);
    for (uint32 m = 0; m < numMeshe; ++m) {
      //Add all the info in vectors
      //Get vertex position
      auto vertexData = model->m_meshes[m]->m_vertexData.get();
      auto currentVertexNum = model->m_meshes[m]->m_numVertex;
      auto currentIndexNum = model->m_meshes[m]->m_numIndex;
      for (uint32 i = 0; i < currentVertexNum; i++) {
        //Add vertex
        auto vec = vertexData[i].Pos;
        points[currentVertexIndex] = GfVec3f(vec.x, vec.y, vec.z);
        //add normal
        auto norm = vertexData[i].Norm;
        meshNormals[currentVertexIndex] = GfVec3f(norm.x, norm.y, norm.z);
        //ad uv
        auto uv = vertexData[i].TexCoord;
        valueArray[currentVertexIndex].Set(uv.Vector);
        //increment real vertex index
        ++currentVertexIndex;
      }

      // Calculate indices for each triangle
      auto indices = model->m_meshes[m]->m_index.get();
      //For now, i don use this vecVecIndices
      vecVecIndices[m].resize(currentIndexNum);
      for (uint32 i = 0; i < currentIndexNum; i++)
      {
        auto idx = indices[i] + currentMaxindex;
        if (idx > Maxindex) {
          Maxindex = idx + 1;
        }
        vecIndices[currentindexIndex] = vecVecIndices[m][i] = idx;
        //increment real index index
        ++currentindexIndex;
      }
      currentMaxindex = Maxindex;
    }
    //set the info
    gModel.CreatePointsAttr(VtValue(points));//Positions
    gModel.CreateFaceVertexIndicesAttr(VtValue(vecIndices));//index
    gModel.CreateNormalsAttr(VtValue(meshNormals));//normals
    // Add face vertex count
    VtArray<int> faceVertexCounts;
    faceVertexCounts.resize(num_indices / 3); // faces = num index / 3 (3 points per face)
    std::fill(faceVertexCounts.begin(), faceVertexCounts.end(), 3);
    gModel.CreateFaceVertexCountsAttr(VtValue(faceVertexCounts));

    // Set the color on the Model
    UsdPrim meshPrim = gModel.GetPrim();

    UsdAttribute displayColorAttr = gModel.CreateDisplayColorAttr();
    {
      VtVec3fArray valueArray;
      GfVec3f rgbFace(0.463f, 0.725f, 0.0f);
      valueArray.push_back(rgbFace);
      displayColorAttr.Set(valueArray);
    }


    bool status = attr2.Set(valueArray);

    attr2.SetInterpolation(UsdGeomTokens->vertex);
    uint32 last = 0;//Last face saved, change after to saved teh faces of current mesh
    //Create the subsets
    for (uint32 m = 0; m < numMeshe; ++m) {
      std::string meshName("mesh_");
      meshName.append(std::to_string(m));
      //gMesh.CreateGeomSubset(gModel, TfToken(meshName), UsdGeomTokens->face, vecVecIndices[m]);
      VtArray<int> faceVertexCounts;
      uint32 size = model->m_meshes[m]->m_numIndex / 3; // get the num of faces of mesh
      faceVertexCounts.resize(size);
      for (uint32 i = 0; i < size; i++)
      {
        faceVertexCounts[i] = i + last;//save the faces, go of 0 to n, and for the nex mesh go to n to m and etc.
      }
      last = size ;
      //create the subset
      UsdGeomSubset::CreateGeomSubset(gModel, TfToken(meshName), UsdGeomTokens->face, faceVertexCounts);
    }

    // Commit the changes to the USD
    m_stage->Save();
    omniUsdLiveProcess();

    return gModel;
  }

  UsdGeomMesh 
  OmniversConnect::createFromModelInPath(SPtr<Model> model, SdfPath& m_parentPath) {
    //This function save all model's meshes in one geomMesh and create subset of it for each mesh,
    //All have the same transform but each can has diferent material

    //define the geomMesh in the modelPath which is a child of root
    UsdGeomMesh gModel = UsdGeomMesh::Define(m_stage, m_parentPath);
    if (!gModel)
      return gModel;
    // Set orientation
    gModel.CreateOrientationAttr(VtValue(UsdGeomTokens->rightHanded));
    //Get number of meshes
    uint32 numMeshe = model->m_meshes.size();
    //declare vars to combine in one array
    int num_vertices = 0; //num of all vetex
    int num_indices = 0; //num of all index
    int currentMaxindex = 0; // Current Mx index, is use to organize the index of meshes
    int Maxindex = 0; // this use to comparet the index with the current max index, after currentMaxIndex equaled to this
    int currentVertexIndex = 0; // this is the current index in the vector of vertex info 
    int currentindexIndex = 0; // this is the current index in the vector of index
    //Get the vertex and index num of all meshes
    for (uint32 m = 0; m < numMeshe; ++m) {
      num_vertices += model->m_meshes[m]->m_numVertex;
      num_indices += model->m_meshes[m]->m_numIndex;
    }

    //create arrays for all the thiks.
    VtArray<GfVec3f> points;
    points.resize(num_vertices);

    VtArray<int> vecIndices;
    vecIndices.resize(num_indices);
    Vector<VtArray<int>> vecVecIndices;
    vecVecIndices.resize(numMeshe);

    int uv_count = num_vertices;
    VtVec2fArray valueArray;
    valueArray.resize(uv_count);


    int num_normals = num_vertices;
    VtArray<GfVec3f> meshNormals;
    meshNormals.resize(num_vertices);


    UsdGeomPrimvar attr2 = gModel.CreatePrimvar(_tokens->st, SdfValueTypeNames->TexCoord2fArray);
    for (uint32 m = 0; m < numMeshe; ++m) {
      //Add all the info in vectors
      //Get vertex position
      auto vertexData = model->m_meshes[m]->m_vertexData.get();
      auto currentVertexNum = model->m_meshes[m]->m_numVertex;
      auto currentIndexNum = model->m_meshes[m]->m_numIndex;
      for (uint32 i = 0; i < currentVertexNum; i++) {
        //Add vertex
        auto vec = vertexData[i].Pos;
        points[currentVertexIndex] = GfVec3f(vec.x, vec.y, vec.z);
        //add normal
        auto norm = vertexData[i].Norm;
        meshNormals[currentVertexIndex] = GfVec3f(norm.x, norm.y, norm.z);
        //ad uv
        auto uv = vertexData[i].TexCoord;
        valueArray[currentVertexIndex].Set(uv.Vector);
        //increment real vertex index
        ++currentVertexIndex;
      }

      // Calculate indices for each triangle
      auto indices = model->m_meshes[m]->m_index.get();
      //For now, i don use this vecVecIndices
      vecVecIndices[m].resize(currentIndexNum);
      for (uint32 i = 0; i < currentIndexNum; i++)
      {
        auto idx = indices[i] + currentMaxindex;
        if (idx > Maxindex) {
          Maxindex = idx;
        }
        vecIndices[currentindexIndex] = vecVecIndices[m][i] = idx;
        //increment real index index
        ++currentindexIndex;
      }
      currentMaxindex = Maxindex + 1;
    }
    //set the info
    gModel.CreatePointsAttr(VtValue(points));//Positions
    gModel.CreateFaceVertexIndicesAttr(VtValue(vecIndices));//index
    gModel.CreateNormalsAttr(VtValue(meshNormals));//normals
    // Add face vertex count
    VtArray<int> faceVertexCounts;
    faceVertexCounts.resize(num_indices / 3); // faces = num index / 3 (3 points per face)
    std::fill(faceVertexCounts.begin(), faceVertexCounts.end(), 3);
    gModel.CreateFaceVertexCountsAttr(VtValue(faceVertexCounts));

    // Set the color on the Model
    UsdPrim meshPrim = gModel.GetPrim();

    UsdAttribute displayColorAttr = gModel.CreateDisplayColorAttr();
    {
      VtVec3fArray valueArray;
      GfVec3f rgbFace(0.463f, 0.725f, 0.0f);
      valueArray.push_back(rgbFace);
      displayColorAttr.Set(valueArray);
    }


    bool status = attr2.Set(valueArray);

    attr2.SetInterpolation(UsdGeomTokens->vertex);
    uint32 last = 0;//Last face saved, change after to saved teh faces of current mesh
    //Create the subsets
    for (uint32 m = 0; m < numMeshe; ++m) {
      std::string meshName("mesh_");
      meshName.append(std::to_string(m));
      //gMesh.CreateGeomSubset(gModel, TfToken(meshName), UsdGeomTokens->face, vecVecIndices[m]);
      VtArray<int> faceVertexCounts;
      uint32 size = model->m_meshes[m]->m_numIndex / 3; // get the num of faces of mesh
      faceVertexCounts.resize(size);
      for (uint32 i = 0; i < size; i++)
      {
        faceVertexCounts[i] = i + last;//save the faces, go of 0 to n, and for the nex mesh go to n to m and etc.
      }
      last = size;
      //create the subset
      UsdGeomSubset::CreateGeomSubset(gModel, TfToken(meshName), UsdGeomTokens->face, faceVertexCounts);
    }

    // Commit the changes to the USD
    m_stage->Save();
    omniUsdLiveProcess();

    return gModel;
  }

  SPtr<Model>
  OmniversConnect::getModelFromStage() {
    if (!m_stage)
    {
      failNotify("No stage opened");

      return nullptr;
    }

    // Traverse the stage and return the first UsdGeomMesh we find
    auto range = m_stage->Traverse();
    for (const auto& node : range)
    {
      if (node.IsA<UsdGeomMesh>())
      {
        {
          std::unique_lock<std::mutex> lk(m_logMutex);
          std::cout << "Found UsdGeomMesh: " << node.GetName() << std::endl;
        }
        UsdGeomMesh Model(node);
        //geomMeshToModel(Model);
        geomMeshToModelV2(Model);
        return nullptr;
      }
    }

    // No UsdGeomMesh found in stage (what kind of stage is this anyway!?)
    std::cout << "ERROR: No UsdGeomMesh found in current stage" << std::endl;
    return nullptr;
  }

  SPtr<Model> OmniversConnect::geomMeshToModel(UsdGeomMesh gMesh)
  {
    //Get vertex position
    VtValue posVals;
    gMesh.GetPointsAttr().Get(&posVals);
    VtArray<GfVec3f> points = posVals.Get<VtArray<GfVec3f>>();
    
    VtValue norVals;
    gMesh.GetNormalsAttr().Get(&norVals);
    VtArray<GfVec3f> normals = norVals.Get<VtArray<GfVec3f>>();
    
    VtValue idxVals;
    gMesh.GetFaceVertexIndicesAttr().Get(&idxVals);
    VtArray<int> index = idxVals.Get<VtArray<int>>();

    VtValue uvVals;
    gMesh.GetPrimvar(_tokens->st).Get(&uvVals);
    VtVec2fArray valueArray = uvVals.Get<VtVec2fArray>();

    String gMname= gMesh.GetPath().GetName();

    Model* newModel = new Model;
    //newModel->setID(binfo.id);
    //newModel->m_name = name;

    Mesh* newMesh = new Mesh;

    newModel->setName(gMname);
    newMesh->setName(gMname);
    SPtr<Mesh> nMesh(newMesh);
    newModel->addMesh(nMesh);
    uint32 numVertex = points.size();
    uint32 numIndex = index.size();

    DefaultVertexData* meshVertex = new DefaultVertexData[numVertex];
    uint32* meshIndex = new uint32[numIndex];

    for (uint32 i = 0; i < numVertex; i++)
    {
      auto vec = points[i].data();
      meshVertex[i].Pos = { vec[0], vec[1], vec[2], 1.0f };

      auto norms = normals[i].data();
      meshVertex[i].Norm = { norms[0], norms[1], norms[2], 0.0f };

      JDVector4 c1 = meshVertex[i].Norm.cross(JDVector4(0.0f, 0.0f, 1.0f,0.0f));
      JDVector4 c2 = meshVertex[i].Norm.cross(JDVector4(0.0f, 1.0f, 0.0f,0.0f));
      JDVector4 tang;
      if (c1.magnitude() > c2.magnitude())
      {
        tang = c1;
      }
      else
      {
        tang = c2;
      }
      meshVertex[i].Tang = tang;

      auto uv = valueArray[i].data();
      meshVertex[i].TexCoord = { uv[0], uv[1] };

      for (uint32 b = 0; b < 4; ++b) {
        meshVertex[i].boneWeights[b] = 1;
      }
    }
    SkeletalMesh* skeletal = new SkeletalMesh;
    SPtr<DefaultVertexData> spVetexData(meshVertex);
    newMesh->m_skeletalSData.reset(skeletal);
    newMesh->setVertexData(spVetexData);

    newMesh->setVertexBuffer(g_graphicsApi().createVertexBuffer(numVertex, 
                                                                sizeof(DefaultVertexData), 
                                                                meshVertex));
    newMesh->m_bonesTransforms.clear();
    newMesh->m_bonesTransforms.resize(skeletal->m_numBones);

    //Index Info
    for (uint32 indx = 0; indx < numIndex; ++indx)
    {
      meshIndex[indx] = (uint32)index[indx];
    }
    SPtr<uint32> spIndex(meshIndex);
    newMesh->setIndex(spIndex);
    newMesh->setIndexNum(numIndex);
    newMesh->m_numVertex = numVertex;

    newMesh->setIndexBuffer(g_graphicsApi().createIndexBuffer(numIndex, meshIndex));

    newMesh->setAlbedoTexture(g_ResourceMan().DEFAULT_TEXTURE_WHITE);
    newMesh->setNormalTexture(g_ResourceMan().DEFAULT_TEXTURE_NORMAL);
    newMesh->setSpecularTexture(g_ResourceMan().DEFAULT_TEXTURE_ERROR);
    newMesh->setMetalnessTexture(g_ResourceMan().DEFAULT_TEXTURE_ERROR);
    newMesh->setRoughnessTexture(g_ResourceMan().DEFAULT_TEXTURE_ERROR);

    //Global inverse transform
    ModelNodes* rootNode = new ModelNodes();

    newModel->m_animationNodes.reset(rootNode);

    newModel->m_AnimationsList.push_back((char*)"None");
    rootNode->m_name ="root";
    rootNode->m_numChildren = 0;
    rootNode->m_numMeshes = 0;

    newModel->m_numAnimations = 0;
    SPtr<Model> modelCreate(newModel);
    g_ResourceMan().m_models.push_back(modelCreate);
    String name = gMname;
    char* cname = new char[name.size() + 1];
    strcpy(cname, name.c_str());
    g_ResourceMan().m_modelsNames.push_back(cname);


    SPtr<Resource> newResource(modelCreate);
    uint32 newId = g_ResourceMan().createHash(cname);
    newResource->setName(name);
    newResource.get()->setID(newId);
    g_ResourceMan().m_resources.insert(std::pair <uint32, SPtr<Resource>>(newId, newResource));
    return modelCreate;
    
  }

  SPtr<Model> 
  OmniversConnect::geomMeshToModelV2(UsdGeomMesh gMesh) {
    //Get vertex position
    VtValue posVals;
    gMesh.GetPointsAttr().Get(&posVals);
    VtArray<GfVec3f> points = posVals.Get<VtArray<GfVec3f>>();

    VtValue norVals;
    gMesh.GetNormalsAttr().Get(&norVals);
    VtArray<GfVec3f> normals = norVals.Get<VtArray<GfVec3f>>();

    VtValue idxVals;
    gMesh.GetFaceVertexIndicesAttr().Get(&idxVals);
    VtArray<int> index = idxVals.Get<VtArray<int>>();

    VtValue uvVals;
    gMesh.GetPrimvar(_tokens->st).Get(&uvVals);
    VtVec2fArray valueArray = uvVals.Get<VtVec2fArray>();
    String gMname = gMesh.GetPath().GetName();

    auto subSets = UsdGeomSubset::GetGeomSubsets(gMesh);
    uint32 numMeshse = subSets.size();

    Model* newModel = new Model;
    //newModel->setID(binfo.id);
    //newModel->m_name = name;
    newModel->setName(gMname);

    uint32 currentMaxIndex = 0;
    uint32 currentIndex = 0;
    for (uint32 i = 0; i < numMeshse; ++i) {
      auto faceIndexAttr = subSets[i].GetIndicesAttr();
      VtValue faceIndexVal;
      faceIndexAttr.Get(&faceIndexVal);
      auto faceIndex = faceIndexVal.Get<VtArray<int>>();
      uint32 numFaces = faceIndex.size();
      uint32 numIndex = numFaces * 3;

      Mesh* newMesh = new Mesh;
      String gMeshName = subSets[i].GetPath().GetName();
      newMesh->setName(gMeshName);
      SPtr<Mesh> nMesh(newMesh);
      newModel->addMesh(nMesh);

      uint32 maxIndex = 0;
      uint32 numVertex = 0;
      Vector<uint32> vecIndex;
      for (uint32 in = 0; in < numIndex; ++in) {
        auto idx = index[currentIndex] - currentMaxIndex;
        if (idx > maxIndex) {
          maxIndex = idx;
          numVertex = idx + 1;
        }
        vecIndex.push_back(idx);
        ++currentIndex;
      }

      DefaultVertexData* meshVertex = new DefaultVertexData[numVertex];
      for (uint32 v = 0; v < numVertex; v++) {
        auto vecIndex = currentMaxIndex + v;
        auto vec = points[vecIndex].data();
        meshVertex[v].Pos = { vec[0], vec[1], vec[2], 1.0f };

        auto norms = normals[vecIndex].data();
        meshVertex[v].Norm = { norms[0], norms[1], norms[2], 0.0f };

        JDVector4 c1 = meshVertex[v].Norm.cross(JDVector4(0.0f, 0.0f, 1.0f, 0.0f));
        JDVector4 c2 = meshVertex[v].Norm.cross(JDVector4(0.0f, 1.0f, 0.0f, 0.0f));
        JDVector4 tang;
        if (c1.magnitude() > c2.magnitude())
        {
          tang = c1;
        }
        else
        {
          tang = c2;
        }
        meshVertex[v].Tang = tang;

        auto uv = valueArray[vecIndex].data();
        meshVertex[v].TexCoord = { uv[0], uv[1] };

        for (uint32 b = 0; b < 4; ++b) {
          meshVertex[v].boneWeights[b] = 1;
        }
      }

      currentMaxIndex += maxIndex + 1;

      SkeletalMesh* skeletal = new SkeletalMesh;
      SPtr<DefaultVertexData> spVetexData(meshVertex);
      newMesh->m_skeletalSData.reset(skeletal);
      newMesh->setVertexData(spVetexData);

      newMesh->setVertexBuffer(g_graphicsApi().createVertexBuffer(numVertex,
                                                                  sizeof(DefaultVertexData),
                                                                  meshVertex));
      newMesh->m_bonesTransforms.clear();
      newMesh->m_bonesTransforms.resize(skeletal->m_numBones);

      //Index Info
      uint32* meshIndex = new uint32[numIndex];
      for (uint32 idx = 0; idx < numIndex; idx++) {
        meshIndex[idx] = vecIndex[idx];
      }
       
      //memccpy(meshIndex, vecIndex.data(), sizeof(uint32), vecIndex.size());
      SPtr<uint32> spIndex(meshIndex);
      newMesh->setIndex(spIndex);
      newMesh->setIndexNum(numIndex);
      newMesh->m_numVertex = numVertex;

      newMesh->setIndexBuffer(g_graphicsApi().createIndexBuffer(numIndex, meshIndex));

      newMesh->setAlbedoTexture(g_ResourceMan().DEFAULT_TEXTURE_WHITE);
      newMesh->setNormalTexture(g_ResourceMan().DEFAULT_TEXTURE_NORMAL);
      newMesh->setSpecularTexture(g_ResourceMan().DEFAULT_TEXTURE_ERROR);
      newMesh->setMetalnessTexture(g_ResourceMan().DEFAULT_TEXTURE_ERROR);
      newMesh->setRoughnessTexture(g_ResourceMan().DEFAULT_TEXTURE_ERROR);

    }

    //Global inverse transform
    ModelNodes* rootNode = new ModelNodes();

    newModel->m_animationNodes.reset(rootNode);

    newModel->m_AnimationsList.push_back((char*)"None");
    rootNode->m_name = "root";
    rootNode->m_numChildren = 0;
    rootNode->m_numMeshes = 0;

    newModel->m_numAnimations = 0;
    SPtr<Model> modelCreate(newModel);
    g_ResourceMan().m_models.push_back(modelCreate);
    String name = gMname;
    char* cname = new char[name.size() + 1];
    strcpy(cname, name.c_str());
    g_ResourceMan().m_modelsNames.push_back(cname);


    SPtr<Resource> newResource(modelCreate);
    uint32 newId = g_ResourceMan().createHash(cname);
    newResource->setName(name);
    newResource.get()->setID(newId);
    g_ResourceMan().m_resources.insert(std::pair <uint32, SPtr<Resource>>(newId, newResource));
    return modelCreate;
  }

  SPtr<Model> 
  OmniversConnect::getJDModelFromGeoMesh(UsdGeomMesh gMesh) {

    auto subSets = UsdGeomSubset::GetGeomSubsets(gMesh).size();
    if (subSets <=0) {
      return geomMeshToModel(gMesh);
    }

    return geomMeshToModelV2(gMesh);
  }

  void
  OmniversConnect::createLight() {
    // Construct /Root/Light path
    SdfPath lightPath = SdfPath::AbsoluteRootPath()
      .AppendChild(_tokens->Root)
      .AppendChild(_tokens->Light);
    UsdLuxDistantLight newLight = UsdLuxDistantLight::Define(m_stage, lightPath);

    // Set the attributes
    newLight.CreateAngleAttr(VtValue(0.53f));
    GfVec3f color(1.0f, 1.0f, 0.745f);
    newLight.CreateColorAttr(VtValue(color));
    newLight.CreateIntensityAttr(VtValue(5000.0f));

    // Commit the changes to the USD
    m_stage->Save();
    omniUsdLiveProcess();
  }

  UsdPrim
  OmniversConnect::createLightFromComponent(CLight* light, SdfPath& m_parentPath) {
    auto typeL = light->getTypeLight();
    UsdPrim prim;
    // Set the attributes
    switch (typeL) {
     case 0:
       prim = crateDirectLight(light, m_parentPath);
      break;
     default:
       prim;
      break;
    }

    g_Render().updateLight(light);
    // Commit the changes to the USD
    m_stage->Save();
    omniUsdLiveProcess();
    return prim;
  }

  UsdPrim
  OmniversConnect::crateDirectLight(CLight* light, SdfPath& m_parentPath) {
    UsdLuxDistantLight newLight = UsdLuxDistantLight::Define(m_stage, m_parentPath);
    //newLight.CreateAngleAttr(VtValue(0.53f));
    auto lColor = light->m_color;
    GfVec3f color(lColor.x, lColor.y, lColor.z);
    newLight.CreateColorAttr(VtValue(color));
    newLight.CreateIntensityAttr(VtValue(5000.0f));
    auto prim = newLight.GetPrim();
    return prim;
  }

  void 
  OmniversConnect::checkpointFile(const char* stageUrl, const char* comment) {
    bool bCheckpointsSupported = false;
    omniClientWait(omniClientGetServerInfo(stageUrl, &bCheckpointsSupported,
      [](void* UserData, OmniClientResult Result, OmniClientServerInfo const* Info) noexcept
    {
      if (Result == eOmniClientResult_Ok && Info && UserData)
      {
        bool* bCheckpointsSupported = static_cast<bool*>(UserData);
        *bCheckpointsSupported = Info->checkpointsEnabled;
      }
    }));

    if (bCheckpointsSupported)
    {
      const bool bForceCheckpoint = true;
      omniClientWait(omniClientCreateCheckpoint(stageUrl, comment, bForceCheckpoint, nullptr,
        [](void* userData, OmniClientResult result, char const* checkpointQuery) noexcept
      {}));
    }
  }

  void 
  OmniversConnect::uploadMaterial(const std::string& destinationPath) {
    std::string uriPath = destinationPath + "/Materials";

    // Delete the old version of this folder on Omniverse and wait for the operation to complete
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "Waiting for " << uriPath << " to delete... ";
    }
    omniClientWait(omniClientDelete(uriPath.c_str(), nullptr, nullptr));
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "finished" << std::endl;
    }

    // Upload the material folder (MDL and textures)
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "Waiting for the resources/Materials folder to upload to " << uriPath << " ... ";
    }
    omniClientWait(omniClientCopy("resources/Materials", uriPath.c_str(), nullptr, nullptr));
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "finished" << std::endl;
    }
  }

  void 
  OmniversConnect::createMaterial(UsdGeomMesh meshIn) {

    std::string materialName = "Fieldstone";

    // Create a material instance for this in USD
    TfToken materialNameToken(materialName);
    // Make path for "/Root/Looks/Fieldstone";
    SdfPath matPath = SdfPath::AbsoluteRootPath()
      .AppendChild(_tokens->Root)
      .AppendChild(_tokens->Looks)
      .AppendChild(materialNameToken);
    UsdShadeMaterial newMat = UsdShadeMaterial::Define(m_stage, matPath);

    // MDL Shader
    {
      // Create the MDL shader to bind to the material
      SdfAssetPath mdlShaderModule = SdfAssetPath("./Materials/Fieldstone.mdl");

      SdfPath shaderPath = matPath.AppendChild(materialNameToken);
      UsdShadeShader mdlShader = UsdShadeShader::Define(m_stage, shaderPath);
      mdlShader.CreateIdAttr(VtValue(_tokens->shaderId));

      // These attributes will be reworked or replaced in the official MDL schema for USD.
      // https://developer.nvidia.com/usd/MDLschema
      std::string newShaderName = mdlShaderModule.GetAssetPath();
      mdlShader.SetSourceAsset(SdfAssetPath(newShaderName), _tokens->mdl);
      mdlShader.GetPrim().CreateAttribute(TfToken("info:mdl:sourceAsset:subIdentifier"), SdfValueTypeNames->Token, false, SdfVariabilityUniform).Set(TfToken(materialName));

      // Set the linkage between material and MDL shader
      UsdShadeOutput mdlOutput = newMat.CreateSurfaceOutput(_tokens->mdl);
      mdlOutput.ConnectToSource(mdlShader, _tokens->out);
    }


    // USD Preview Surface Shaders
    {
      // Create the "USD Primvar reader for float2" shader
      SdfPath shaderPath = matPath.AppendChild(_tokens->PrimST);
      UsdShadeShader primStShader = UsdShadeShader::Define(m_stage, shaderPath);
      primStShader.CreateIdAttr(VtValue(_tokens->PrimStShaderId));
      primStShader.CreateOutput(_tokens->result, SdfValueTypeNames->Float2);
      primStShader.CreateInput(_tokens->varname, SdfValueTypeNames->Token).Set(_tokens->st);

      // Create the "Diffuse Color Tex" shader
      std::string diffuseColorShaderName = materialName + "DiffuseColorTex";
      std::string diffuseFilePath = "./Materials/Fieldstone/Fieldstone_BaseColor.png";
      shaderPath = matPath.AppendChild(TfToken(diffuseColorShaderName));
      UsdShadeShader diffuseColorShader = UsdShadeShader::Define(m_stage, shaderPath);
      diffuseColorShader.CreateIdAttr(VtValue(_tokens->UsdUVTexture));
      UsdShadeInput texInput = diffuseColorShader.CreateInput(_tokens->file, SdfValueTypeNames->Asset);
      texInput.Set(SdfAssetPath(diffuseFilePath));
      texInput.GetAttr().SetColorSpace(_tokens->sRGB);
      diffuseColorShader.CreateInput(_tokens->st, SdfValueTypeNames->Float2).ConnectToSource(primStShader.CreateOutput(_tokens->result, SdfValueTypeNames->Float2));
      UsdShadeOutput diffuseColorShaderOutput = diffuseColorShader.CreateOutput(_tokens->rgb, SdfValueTypeNames->Float3);

      // Create the "Normal Tex" shader
      std::string normalShaderName = materialName + "NormalTex";
      std::string normalFilePath = "./Materials/Fieldstone/Fieldstone_N.png";
      shaderPath = matPath.AppendChild(TfToken(normalShaderName));
      UsdShadeShader normalShader = UsdShadeShader::Define(m_stage, shaderPath);
      normalShader.CreateIdAttr(VtValue(_tokens->UsdUVTexture));
      UsdShadeInput normalTexInput = normalShader.CreateInput(_tokens->file, SdfValueTypeNames->Asset);
      normalTexInput.Set(SdfAssetPath(normalFilePath));
      normalTexInput.GetAttr().SetColorSpace(_tokens->RAW);
      normalShader.CreateInput(_tokens->st, SdfValueTypeNames->Float2).ConnectToSource(primStShader.CreateOutput(_tokens->result, SdfValueTypeNames->Float2));
      UsdShadeOutput normalShaderOutput = normalShader.CreateOutput(_tokens->rgb, SdfValueTypeNames->Float3);

      // Create the USD Preview Surface shader
      std::string usdPreviewSurfaceShaderName = materialName + "PreviewSurface";
      shaderPath = matPath.AppendChild(TfToken(usdPreviewSurfaceShaderName));
      UsdShadeShader usdPreviewSurfaceShader = UsdShadeShader::Define(m_stage, shaderPath);
      usdPreviewSurfaceShader.CreateIdAttr(VtValue(_tokens->UsdPreviewSurface));
      UsdShadeInput diffuseColorInput = usdPreviewSurfaceShader.CreateInput(_tokens->diffuseColor, SdfValueTypeNames->Color3f);
      diffuseColorInput.ConnectToSource(diffuseColorShaderOutput);
      UsdShadeInput normalInput = usdPreviewSurfaceShader.CreateInput(_tokens->normal, SdfValueTypeNames->Normal3f);
      normalInput.ConnectToSource(normalShaderOutput);

      // Set the linkage between material and USD Preview surface shader
      UsdShadeOutput usdPreviewSurfaceOutput = newMat.CreateSurfaceOutput();
      usdPreviewSurfaceOutput.ConnectToSource(usdPreviewSurfaceShader, _tokens->surface);
    }

    // Final step, associate the material with the face
    UsdShadeMaterialBindingAPI usdMaterialBinding(meshIn);
    usdMaterialBinding.Bind(newMat);

    // Commit the changes to the USD
    m_stage->Save();
    omniUsdLiveProcess();
  }

  bool 
  OmniversConnect::openStage(const String& stagePath) {
    String realPath;
    uint32 index = 0;
    while ('\0' != stagePath[index]){
      realPath.push_back(stagePath[index]);
      ++index;
    }
    // Open this file from Omniverse
    m_stage = UsdStage::Open(realPath);
    if (!m_stage)
    {
      failNotify("Failure to open stage in Omniverse:", realPath.c_str());

      m_currentURL = "";
      return false;
    }

    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "Existing stage opened: " << realPath << std::endl;
    }

    if (UsdGeomTokens->y != UsdGeomGetStageUpAxis(m_stage))
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "Stage is not Y-up so live xform edits will be incorrect.  Stage is " << UsdGeomGetStageUpAxis(m_stage) << "-up" << std::endl;
    }
    m_rootPrimPath = SdfPath::AbsoluteRootPath();
    m_currentURL = realPath;
    return true;
  }

  bool 
  OmniversConnect::openStage(const String& folder, const String& stageName) {

    String realFolder;
    String realName;
    String realPath;
    uint32 index = 0;
    while ('\0' != folder[index]) {
      realFolder.push_back(folder[index]);
      ++index;
    }
    index = 0;
    while ('\0' != stageName[index]) {
      realName.push_back(stageName[index]);
      ++index;
    }
    if (String::npos == realName.find(".usd")) {
      realName += ".usd";
    }
    realPath = realFolder + "/" + realName;
    if (true)
    {

    }
    // Open this file from Omniverse
    m_stage = UsdStage::Open(realPath);
    if (!m_stage)
    {
      failNotify("Failure to open stage in Omniverse:", realPath.c_str());

      m_currentURL = "";
      m_currentFolder = "";
      m_currentStageName = "";
      return false;
    }

    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "Existing stage opened: " << realPath << std::endl;
    }

    if (UsdGeomTokens->y != UsdGeomGetStageUpAxis(m_stage))
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "Stage is not Y-up so live xform edits will be incorrect.  Stage is " << UsdGeomGetStageUpAxis(m_stage) << "-up" << std::endl;
    }
    m_rootPrimPath = SdfPath::AbsoluteRootPath();
    m_currentURL = realPath;
    m_currentFolder = realFolder;
    m_currentStageName = realName;
    return true;
  }

  void 
  OmniversConnect::createEmptyFolder(const std::string& emptyFolderPath) {
    if (isValidOmniURL(emptyFolderPath))
    {
      std::cout << "Folder: " << emptyFolderPath << " Already exists "<<std::endl;
    }
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      //omniClientWait(omniClientDelete(emptyFolderPath.c_str(), nullptr, nullptr));
      std::cout << "Waiting to create a new folder: " << emptyFolderPath << " ... ";
    }

    OmniClientResult localResult;
    localResult = Count_eOmniClientResult;
    
    

    omniClientWait(omniClientCreateFolder(emptyFolderPath.c_str(), &localResult, [](void* userData, OmniClientResult result) noexcept
    {
      auto returnResult = static_cast<OmniClientResult*>(userData);
      *returnResult = result;
    }));

    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "finished [" << omniClientGetResultString(localResult) << "]" << std::endl;
    }
  }


  void 
  OmniversConnect::getSceneGraph() {
    if (!m_stage) {
      failNotify("No stage opened");
      return;
    }
    SceneGraph::instance().m_root->m_omniSync = true;
    SceneGraph::instance().m_root->m_omniPath = m_stage->GetPseudoRoot().GetPath().GetString();

    //auto rootPath = m_rootPrimPath.GetString();
    // Traverse the stage and return the first UsdGeomMesh we find
    auto range = m_stage->Traverse();
    for (const auto& node : range)
    {
      if (node.IsA<UsdGeomMesh>())
      {
        {
          std::unique_lock<std::mutex> lk(m_logMutex);
          std::cout << "Found UsdGeomMesh: " << node.GetName() << std::endl;
        }
        UsdGeomMesh Model(node);
        //geomMeshToModel(Model);
        auto newObject = SceneGraph::instance().createGameObject();
        newObject->m_omniSync = true;
        newObject->m_omniPath = node.GetPath().GetString();
        auto component =
          newObject->getComponent(COMPONENT_TYPE::TRANSFORM);
        if (nullptr != component) {
          CTransform* trans = reinterpret_cast<CTransform*>(component.get());
          UsdGeomXformable form = Model;
          JDVector3 rot;
          getTransformComponents(form, trans->position, rot, trans->scale);
          trans->rotation = Quaternion(Degree(rot.x), Degree(rot.y), Degree(rot.z));
          trans->Update(0.0f);
        }
        auto nModel = geomMeshToModel(Model);
        SPtr<CRenderModel> rm(new CRenderModel);
        rm->setModel(nModel);
        rm->setGameObject(newObject.get());
        newObject->addComponent(COMPONENT_TYPE::RENDERMODEL, rm);
        return;
      }
    }

    // No UsdGeomMesh found in stage (what kind of stage is this anyway!?)
    std::cout << "ERROR: No UsdGeomMesh found in current stage" << std::endl;
    return;
  }

  void 
  OmniversConnect::getSceneGraphV2() {
    if (!m_stage) {
      failNotify("No stage opened");
      return;
    }
    //SceneGraph::instance().resetSceneGraph();
    g_Render().clearScene();
    SdfPath rootPrimPath = SdfPath::AbsoluteRootPath();
    auto rootScene = SceneGraph::instance().m_root;
    rootScene->m_omniSync = true;
    rootScene->m_omniPath = rootPrimPath.GetString();
    m_rootPrimPath = rootPrimPath;

    SdfPath path = SdfPath(rootScene->m_omniPath);
    auto prim = m_stage->GetPrimAtPath(path);
    auto children = prim.GetAllChildren();
    for (auto child : children){
      getObjects(child, rootScene);
    }

    //for (auto child : children) {
    //
    //}

    //auto rootPath = m_rootPrimPath.GetString();
    // Traverse the stage and return the first UsdGeomMesh we find
    return;
  }

  void 
  OmniversConnect::getObjects(UsdPrim& prim, SPtr<GameObject> parent) {
    if (prim.IsA<UsdGeomSubset>()){
      return;
    }
    auto newObject = SceneGraph::instance().createGameObject(parent);
    newObject->m_omniSync = true;
    newObject->m_omniPath = prim.GetPath().GetString();
    newObject->setName(prim.GetName());

    if (prim.IsA<UsdGeomXform>()) {
      {
        std::unique_lock<std::mutex> lk(m_logMutex);
        std::cout << "Found UsdGeomMesh: " << prim.GetName() << std::endl;
      }
      auto component =
        newObject->getComponent(COMPONENT_TYPE::TRANSFORM);
      if (nullptr != component) {
        CTransform* trans = reinterpret_cast<CTransform*>(component.get());
        UsdGeomXformable form(prim);
        JDVector3 rot;
        getTransformComponents(form, trans->position, rot, trans->scale);
        trans->rotation = Quaternion(Degree(rot.x), Degree(rot.y), Degree(rot.z));
        trans->Update(0.0f);
      }
    }

    if (prim.IsA<UsdGeomMesh>())
    {
      {
        std::unique_lock<std::mutex> lk(m_logMutex);
        std::cout << "Found UsdGeomMesh: " << prim.GetName() << std::endl;
      }
      UsdGeomMesh Model(prim);
      //geomMeshToModel(Model);
     
      auto component =
        newObject->getComponent(COMPONENT_TYPE::TRANSFORM);
      if (nullptr != component) {
        CTransform* trans = reinterpret_cast<CTransform*>(component.get());
        UsdGeomXformable form = Model;
        JDVector3 rot;
        getTransformComponents(form, trans->position, rot, trans->scale);
        trans->rotation = Quaternion(Degree(rot.x), Degree(rot.y), Degree(rot.z));
        trans->Update(0.0f);
      }
      auto nModel = getJDModelFromGeoMesh(Model);
      SPtr<CRenderModel> rm(new CRenderModel);
      rm->setModel(nModel);
      rm->setGameObject(newObject.get());
      newObject->addComponent(COMPONENT_TYPE::RENDERMODEL, rm);
    }

    if (prim.IsA<UsdLuxDistantLight>())
    {
      {
        std::unique_lock<std::mutex> lk(m_logMutex);
        std::cout << "Found UsdGeomMesh: " << prim.GetName() << std::endl;
      }
      //geomMeshToModel(Model);

      auto component =
        newObject->getComponent(COMPONENT_TYPE::TRANSFORM);
      if (nullptr != component) {
        CTransform* trans = reinterpret_cast<CTransform*>(component.get());
        UsdGeomXformable form(prim);
        JDVector3 rot;
        getTransformComponents(form, trans->position, rot, trans->scale);
        trans->rotation = Quaternion(Degree(rot.x), Degree(rot.y), Degree(rot.z));
        trans->Update(0.0f);
      }
      CLight* light = new CLight();
      UsdLuxDistantLight usdLight(prim);
      auto col = usdLight.GetColorAttr();
      VtValue colV;
      col.Get(&colV);
      GfVec3f color = colV.Get<GfVec3f>();
      auto c = color.data();
      light->m_color = { c[0], c[1], c[2] };
      g_Render().addLightComponentToObject(light, newObject);
     
    }

    auto children = prim.GetAllChildren();
    for (auto child : children) {
      getObjects(child, newObject);
    }

  }



  void
  OmniversConnect::saveSceneGraph() {
    //resetCurrentStage();
    //resetCurrentStage();
    if (!m_stage) {
      failNotify("No stage opened");
      return;
    }

    m_stage->GetRootLayer()->ClearDefaultPrim();
    SdfPath rootPrimPath = SdfPath::AbsoluteRootPath();
    resetCurrentStage();
    //auto prim = m_stage->GetPrimAtPath(rootPrimPath);
    //auto children = prim.GetAllChildren();
    //for (auto chil : children)
    //{
    //  m_stage->RemovePrim(chil.GetPath());
    //}


    UsdGeomXform::Define(m_stage, m_rootPrimPath);
    auto rootScene = SceneGraph::instance().m_root;
    rootScene->m_omniSync = true;
    rootScene->m_omniPath = rootPrimPath.GetString();
    m_rootPrimPath = rootPrimPath;
    uint32 objNum = 0;
    omniUsdLiveWaitForPendingUpdates();
    for (auto son : rootScene->m_children) {
      std::string objName("obj_");
      objName.append(std::to_string(objNum++));
      saveObject(son, m_rootPrimPath, objName);
    }
    m_numObjects = 0;
    m_stage->Save();
    omniUsdLiveProcess();
  }

  void 
  OmniversConnect::resetCurrentStage() {
    if (!m_stage) {
      failNotify("No stage opened");
      return;
    }
    //SceneGraph::instance().resetSceneGraph();
    SdfPath rootPrimPath = SdfPath::AbsoluteRootPath();
    auto prim = m_stage->GetPrimAtPath(rootPrimPath);
    auto children = prim.GetAllChildren();
    for (auto chil : children)
    {
      m_stage->RemovePrim(chil.GetPath());
    }
    UsdGeomMesh ori(prim);
    //ori.CreateOrientationAttr(VtValue(UsdGeomTokens->leftHanded));
    m_stage->Save();
    omniUsdLiveProcess();
    return;
    
    std::string stageUrl = m_currentURL;

    // Delete the old version of this file on Omniverse and wait for the operation to complete
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "Waiting for " << stageUrl << " to delete... " << std::endl;
    }
    omniClientWait(omniClientDelete(stageUrl.c_str(), nullptr, nullptr));
    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "finished" << std::endl;
    }

    // Create this file in Omniverse cleanly
    m_stage = UsdStage::CreateNew(stageUrl);
    if (!m_stage) {
      failNotify("Failure to create model in Omniverse", stageUrl.c_str());
      m_isStartUp = false;
      return;
    }

    {
      std::unique_lock<std::mutex> lk(m_logMutex);
      std::cout << "New stage created: " << stageUrl << std::endl;
    }
    m_isStartUp = true;
    // Always a good idea to declare your up-ness
    UsdGeomSetStageUpAxis(m_stage, UsdGeomTokens->y);
  }

  void 
  OmniversConnect::saveObject(SPtr<GameObject> obj, SdfPath& m_parentPath, String& name) {

    // Create the geometry inside of "Root"
    //std::string objName("obj_");
    //objName.append(std::to_string(m_numObjects++));
    SdfPath objPrimPath = m_parentPath.AppendChild(TfToken(name));
    obj->m_omniSync = true;
    obj->m_omniPath = objPrimPath.GetString();

    auto component = obj->getComponent(COMPONENT_TYPE::TRANSFORM);
    CTransform* trans = reinterpret_cast<CTransform*>(component.get());
    if (nullptr != component) {
      auto form = UsdGeomXform::Define(m_stage, objPrimPath);
      UsdGeomXformable xform(form);
      setTransformComponents(xform, trans->position, trans->euler, trans->scale);
    }

    component = obj->getComponent(COMPONENT_TYPE::RENDERMODEL);
    if (nullptr != component) {
      auto rModel = reinterpret_cast<CRenderModel*>(component.get());
      if (nullptr != rModel->m_model) {
        auto usdMesh = createFromModelInPath(rModel->m_model, objPrimPath);
        UsdGeomXformable xform(usdMesh);
        setTransformComponents(xform, trans->position, trans->euler, trans->scale);
      }
    }

    component = obj->getComponent(COMPONENT_TYPE::LIGHT);
    if (nullptr != component) {
      auto light = reinterpret_cast<CLight*>(component.get());
      if (nullptr != light) {
        auto newlight = createLightFromComponent(light, objPrimPath);
        UsdGeomXformable xform(newlight);
        setTransformComponents(xform, trans->position, trans->euler, trans->scale);
      }
    }

    uint32 childNum = 0;
    for (auto son : obj->m_children) {
      std::string objName("obj_");
      objName.append(std::to_string(childNum++));
      saveObject(son, objPrimPath, objName);
    }
  }


  void
  OmniversConnect::getTransformComponents(UsdGeomXformable& inXform, 
                                          JDVector3& outPos, 
                                          JDVector3& outRot, 
                                          JDVector3& outScal) {
    // Get the xform ops stack
    bool resetXformStack = false;
    std::vector<UsdGeomXformOp> xFormOps = inXform.GetOrderedXformOps(&resetXformStack);

    // Define storage for the different xform ops that Omniverse Kit likes to use
    UsdGeomXformOp translateOp;
    UsdGeomXformOp rotateOp;
    UsdGeomXformOp scaleOp;
    GfVec3d position(0);
    GfVec3f rotZYX(0);
    GfVec3f scale(1);

    // Get the current xform op values
    for (size_t i = 0; i < xFormOps.size(); i++)
    {
      switch (xFormOps[i].GetOpType()) {
      case UsdGeomXformOp::TypeTranslate:
        translateOp = xFormOps[i];
        translateOp.Get(&position);
        break;
      case UsdGeomXformOp::TypeRotateZYX:
        rotateOp = xFormOps[i];
        rotateOp.Get(&rotZYX);
        break;
      case UsdGeomXformOp::TypeScale:
        scaleOp = xFormOps[i];
        scaleOp.Get(&scale);
        break;
      }
    }

    //position += GfVec3d(1000, 0, 1000);
    auto pos = position.data();
    outPos = { (float)pos[0], (float)pos[1], (float)pos[2] };
    auto rot = rotZYX.data();
    outRot = { (float)rot[0], (float)rot[1], (float)rot[2] };
    auto scal = scale.data();
    outScal = { (float)scal[0], (float)scal[1], (float)scal[2] };
    
  }

  void 
  OmniversConnect::setTransformComponents(UsdGeomXformable& outXform, 
                                          JDVector3& inPos, 
                                          JDVector3& inRot, 
                                          JDVector3& inScal) {
    // Get the xform ops stack
    bool resetXformStack = false;
    std::vector<UsdGeomXformOp> xFormOps = outXform.GetOrderedXformOps(&resetXformStack);

    // Define storage for the different xform ops that Omniverse Kit likes to use
    UsdGeomXformOp translateOp;
    UsdGeomXformOp rotateOp;
    UsdGeomXformOp scaleOp;

    // Get the current xform op values
    for (size_t i = 0; i < xFormOps.size(); i++)
    {
      switch (xFormOps[i].GetOpType()) {
      case UsdGeomXformOp::TypeTranslate:
        translateOp = xFormOps[i];
        break;
      case UsdGeomXformOp::TypeRotateZYX:
        rotateOp = xFormOps[i];
        break;
      case UsdGeomXformOp::TypeScale:
        scaleOp = xFormOps[i];
        break;
      }
    }

    GfVec3d position = { inPos.x, inPos.y, inPos.z };
    GfVec3f rotZYX = { inRot.x, inRot.y, inRot.z };
    GfVec3f scale = { inScal.x, inScal.y, inScal.z };

    SetOp(outXform, translateOp, UsdGeomXformOp::TypeTranslate, position, UsdGeomXformOp::Precision::PrecisionDouble);
    SetOp(outXform, rotateOp, UsdGeomXformOp::TypeRotateZYX, rotZYX, UsdGeomXformOp::Precision::PrecisionFloat);
    SetOp(outXform, scaleOp, UsdGeomXformOp::TypeScale, scale, UsdGeomXformOp::Precision::PrecisionFloat);

    // Make sure the xform op order is correct (translate, rotate, scale)
    std::vector<UsdGeomXformOp> xFormOpsReordered;
    xFormOpsReordered.push_back(translateOp);
    xFormOpsReordered.push_back(rotateOp);
    xFormOpsReordered.push_back(scaleOp);
    outXform.SetXformOpOrder(xFormOpsReordered);
  }

  void 
  OmniversConnect::inUpdateData() {
    if (!m_stage) {
      return;
    }
    // Process any updates that may have happened to the stage from another client
    omniUsdLiveWaitForPendingUpdates();
    //SdfPath path = SdfPath();
    //m_stage->GetObjectAtPath(path);
    for (auto obj : SceneGraph::instance().m_GObjects)
    {
      if (obj->m_omniSync)
      {
        SdfPath path = SdfPath(obj->m_omniPath);
        auto prim = m_stage->GetPrimAtPath(path);
        if (prim && prim.IsA<UsdGeomXformable>()) {
          auto component =
            obj->getComponent(COMPONENT_TYPE::TRANSFORM);
          if (nullptr != component) {
            CTransform* trans = reinterpret_cast<CTransform*>(component.get());
            UsdGeomXformable form(prim);
            JDVector3 rot = trans->rotation.getEuler();
            getTransformComponents(form, trans->position, rot, trans->scale); 
            trans->rotation = Quaternion(Degree(rot.x), Degree(rot.y), Degree(rot.z));
            trans->euler = trans->rotation.getEuler();
            trans->Update(0.0f);
          }
        }
        if (prim && prim.IsA<UsdGeomMesh>()) {
          UsdGeomMesh mesh(prim);
          auto component =
            obj->getComponent(COMPONENT_TYPE::TRANSFORM);
          if (nullptr != component) {
            CTransform* trans = reinterpret_cast<CTransform*>(component.get());
            UsdGeomXformable form = mesh;
            JDVector3 rot;
            getTransformComponents(form, trans->position, rot, trans->scale);
            trans->rotation = Quaternion(Degree(rot.x), Degree(rot.y), Degree(rot.z));
            trans->euler = trans->rotation.getEuler();
            trans->Update(0.0f);
          }
        }
        //Light Dir
        if (prim && prim.IsA<UsdLuxDistantLight>()) {
          auto component = obj->getComponent(COMPONENT_TYPE::TRANSFORM);
          if (nullptr != component) {
            CTransform* trans = reinterpret_cast<CTransform*>(component.get());
            UsdGeomXformable form(prim);
            JDVector3 rot;
            getTransformComponents(form, trans->position, rot, trans->scale);
            trans->rotation = Quaternion(Degree(rot.x), Degree(rot.y), Degree(rot.z));
            trans->euler = trans->rotation.getEuler();
            trans->Update(0.0f);

            component = obj->getComponent(COMPONENT_TYPE::LIGHT);
            if (nullptr != component) {
              CLight* light = reinterpret_cast<CLight*>(component.get());
              UsdLuxDistantLight usdLight(prim);
              auto col = usdLight.GetColorAttr();
              VtValue colV;
              col.Get(&colV);
              GfVec3f color = colV.Get<GfVec3f>();
              auto c= color.data();
              light->m_color = { c[0], c[1], c[2] };
              g_Render().updateLight(light);
            }

          }
        }
      }
    }
  }

  void 
  OmniversConnect::outUpdateData() {
    if (!m_stage) {
      return;
    }
    // Process any updates that may have happened to the stage from another client
    omniUsdLiveWaitForPendingUpdates();

    for (auto obj : SceneGraph::instance().m_GObjects) {
      if (obj->m_omniSync && obj != SceneGraph::instance().m_root) {
        SdfPath path = SdfPath(obj->m_omniPath);
        auto prim = m_stage->GetPrimAtPath(path);
        if (prim && prim.IsA<UsdGeomXformable>()) {
          auto component =
            obj->getComponent(COMPONENT_TYPE::TRANSFORM);
          if (nullptr != component) {
            CTransform* trans = reinterpret_cast<CTransform*>(component.get());
            UsdGeomXformable form(prim);
            JDVector3 rot = trans->rotation.getEuler();
            setTransformComponents(form, trans->position, rot, trans->scale);
          }
        }
        if (prim && prim.IsA<UsdGeomMesh>()) {
          UsdGeomMesh mesh(prim);
          auto component =
            obj->getComponent(COMPONENT_TYPE::TRANSFORM);
          if (nullptr != component) {
            CTransform* trans = reinterpret_cast<CTransform*>(component.get());
            UsdGeomXformable form = mesh;
            JDVector3 rot = trans->rotation.getEuler();
            setTransformComponents(form, trans->position, rot, trans->scale);
          }
        }

        //Light Dir
        if (prim && prim.IsA<UsdLuxDistantLight>()) {
          auto component = obj->getComponent(COMPONENT_TYPE::TRANSFORM);
          if (nullptr != component) {
            CTransform* trans = reinterpret_cast<CTransform*>(component.get());
            UsdGeomXformable form(prim);
            JDVector3 rot;
            getTransformComponents(form, trans->position, rot, trans->scale);
            trans->rotation = Quaternion(Degree(rot.x), Degree(rot.y), Degree(rot.z));
            trans->euler = trans->rotation.getEuler();
            trans->Update(0.0f);

            component = obj->getComponent(COMPONENT_TYPE::LIGHT);
            if (nullptr != component) {
              CLight* light = reinterpret_cast<CLight*>(component.get());
              UsdLuxDistantLight usdLight(prim);
              GfVec3f color(light->m_color.x, light->m_color.y, light->m_color.z);
              usdLight.CreateColorAttr(VtValue(color));
            }
          }
        }

      }
    }
    m_stage->Save();
    omniUsdLiveProcess();
  }
}
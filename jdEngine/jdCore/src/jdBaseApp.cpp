#include "jdBaseApp.h"
#include <jdTime.h>

#include "jdCompute.h"
#include <errhandlingapi.h>
#include <sstream>
#include <SFML/Graphics/Image.hpp>

namespace jdEngineSDK {
  
  BaseApp::BaseApp()
    : m_wndPosition(-1, -1),
      m_clientSize(640, 480),
      m_wndTitle("jdEngine App"),
    m_fullScreen(false) {
  }

  int32
  BaseApp::run() {
    createWindow();

    initSystems();

    g_Profiler().start();
    onCreate();
    g_Profiler().toc("onCreate Function");
    //Main loop
    //float time = 0.0f;
    bool shoudClose = false;
    Event wndEvent;
    sf::Clock clock;

    float deltaTime = g_time().getFrameDelta();
    while (m_window.isOpen()) {
      deltaTime = g_time().getFrameDelta();
      sf::Time elapsed = clock.restart();
      MSG message;
      while (PeekMessageW(&message, m_window.getSystemHandle(), 0, 0, PM_REMOVE))
      {
        TranslateMessage(&message);
        DispatchMessageW(&message);
        m_inputAPI->handleMessage(message);
      }
      while (m_window.pollEvent(wndEvent)) {
        if (Event::Closed == wndEvent.type) {
          m_window.close();
          shoudClose = true;
          break;
        }
        handleWindowEvent(wndEvent);
      }
      if (shoudClose) {
        break;
      }
      //handleWindownput(elapsed.asSeconds());
      handleWindownput(deltaTime);
      //g_Profiler().toc("Inputs");

      //update(elapsed.asSeconds());
      update(deltaTime);
      //g_Profiler().toc("Update fucntion");

      render();
      //g_Profiler().toc("Render funtion");
      g_time().update();
    }

    onDestroy();

    destroySystems();

    return 0;
  }

  void 
  BaseApp::onResize(int32 width, int32 height) {
    auto clientSize = m_window.getSize();
    m_clientSize.x = clientSize.x;
    m_clientSize.y = clientSize.y;

    g_graphicsApi().resizeSwapChain(width, height);
  }

  void BaseApp::createWindow() {
    //Agregar windowbase

    if (!m_window.isOpen()) {
      m_window.create(sf::VideoMode(m_clientSize.x, m_clientSize.y),
                       m_wndTitle.c_str(),
                       m_fullScreen ? sf::Style::Fullscreen : sf::Style::Default);

      sf::Image icon;
      icon.loadFromFile("Jade_Engine_logo.png");
      m_window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
      //m_window.setPosition(sf::Vector2i(m_wndPosition.x, m_wndPosition.y));
      if (-1 != m_wndPosition.x && -1 != m_wndPosition.y)
      {
        m_window.setPosition(sf::Vector2i(m_wndPosition.x, m_wndPosition.y));
      }

      m_windowHasFocus = m_window.hasFocus();
    }
  }

  void
  BaseApp::initSystems() {
    Time::startUp();
    WindowHandle handle = m_window.getSystemHandle();
    HINSTANCE m_inputHInstance;
    //input api
#ifdef _DEBUG
    m_inputHInstance = LoadLibraryExA("sysInput_Gainputd.dll",
    //m_inputHInstance = LoadLibraryExA("SysInput_OISd.dll",
                                      nullptr,
                                      LOAD_WITH_ALTERED_SEARCH_PATH);
#else
    //m_inputHInstance = LoadLibraryExA("sysInput_Gainput.dll",
    m_inputHInstance = LoadLibraryExA("sysInput_Gainputd.dll",
                                      nullptr,
                                      LOAD_WITH_ALTERED_SEARCH_PATH);
#endif
    if (!m_inputHInstance) {
      return;
    }

    using fnProtInput = InputAPI * (*)();
    fnProtInput createInputApi = reinterpret_cast<fnProtInput>(GetProcAddress(m_inputHInstance, "createInputApi"));
    if (!createInputApi) {
      return;
    }

    //start input api module
    m_inputAPI = createInputApi();
    m_inputAPI->init(m_clientSize.x, m_clientSize.x, handle);
    //Compute::startUp();

    HINSTANCE hin;
#ifdef _DEBUG
    hin = LoadLibraryExA("jdDXGraphicApid.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
    hin = LoadLibraryExA("jdDXGraphicApi.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
#endif


    if (!hin)
    {
      return;
    }

    auto createGraphicsApi = reinterpret_cast<graphApi>(GetProcAddress(hin, "createGraphicAPI"));
    if (!createGraphicsApi)
    {
      return;
    }
    GraphicApi::startUp();
    GraphicApi* gsApi = createGraphicsApi();
    g_graphicsApi().setObject(gsApi);

    g_graphicsApi().createDevice();
    g_graphicsApi().createSwapChain(handle, FORMAT::E::FORMAT_R8G8B8A8_UNORM, m_clientSize.x, m_clientSize.y);
    g_graphicsApi().createRenderTargetView(m_clientSize.x, m_clientSize.y);

    CameraManager::startUp();
    ResourceManager::startUp();
    SceneGraph::startUp();
    PhysicalManager::startUp();
    g_Physics().onCreate();
    Logger::startUp();
    Profiler::startUp();

    HINSTANCE ren;
#ifdef _DEBUG
    ren = LoadLibraryExA("jdRenderForwardNDeferredd.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    //ren = LoadLibraryExA("jdRenderShaderTestsd.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    //ren = LoadLibraryExA("jdRenderRayTracingd.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
    ren = LoadLibraryExA("jdRenderForwardNDeferred.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
#endif


    if (!ren)
    {
      auto error = GetLastError();
      std::ostringstream os;
      os << error;
      std::cout << os.str() << std::endl;
      return;
    }

    auto createRenderApi = reinterpret_cast<RenApi>(GetProcAddress(ren, "createRenderAPI"));
    if (!createRenderApi)
    {
      return;
    }
    RenderApi::startUp();
    RenderApi* rendApi = createRenderApi();
    g_Render().setObject(rendApi);

    g_Render().init(m_window.getSystemHandle(), m_clientSize);
    g_Render().m_windowHasFocus = &m_windowHasFocus;
    g_Render().m_simulate = &m_simulate;
    g_Render().m_simulatePause = &m_simulatePause;
    g_Render().m_inputAPI = m_inputAPI;
  }

  void 
  BaseApp::destroySystems() {
    Profiler::shutDown();
    Logger::shutDown();
    PhysicalManager::shutDown();
    RenderApi::shutDown();
    SceneGraph::instance().release();
    SceneGraph::shutDown();
    ResourceManager::instance().release();
    ResourceManager::shutDown();
    CameraManager::shutDown();
    g_graphicsApi().shutDown();
    delete m_inputAPI;
    Time::shutDown();
  }
  
  void 
  BaseApp::handleWindowEvent(Event& wndEvent) {
    //WindowHandle handle = m_window.getSystemHandle();

    //if (nullptr == ImGui::GetCurrentContext())
    //{
    //  return;
    //}

    switch (wndEvent.type)     
    {
     case wndEvent.Resized:
      onResize(wndEvent.size.width, wndEvent.size.height);
      break;
     case wndEvent.LostFocus:
      m_windowHasFocus = false;
      onLostFocus();
      break;
     case wndEvent.GainedFocus:
      m_windowHasFocus = true;
      onGainedFocus();
      break;
     case wndEvent.TextEntered:
      //io.AddInputCharacterUTF16(static_cast<uint16>(wndEvent.text.unicode));

      onTextEnterd(static_cast<UNICHAR>(wndEvent.text.unicode));
      break;
     case wndEvent.KeyPressed:
      //if (wndEvent.key.code >=0 )
      //{
      //  io.KeysDown[static_cast<int32>(wndEvent.key.code)] = true;
      //}

      onKeyPressed(wndEvent.key.code,
                   wndEvent.key.alt,
                   wndEvent.key.control,
                   wndEvent.key.shift,
                   wndEvent.key.system);
      break;
     case wndEvent.KeyReleased:
      //if (wndEvent.key.code >= 0)
      //{
      //  io.KeysDown[static_cast<int32>(wndEvent.key.code)] = false;
      //}

      onKeyReleased(wndEvent.key.code,
                   wndEvent.key.alt,
                   wndEvent.key.control,
                   wndEvent.key.shift,
                   wndEvent.key.system);
      break;
     case wndEvent.MouseWheelMoved:
      break;
     case wndEvent.MouseWheelScrolled:
      //io.MouseWheel += wndEvent.mouseWheelScroll.delta;

      onMouseWheelScrolled(wndEvent.mouseWheelScroll.wheel,
                           wndEvent.mouseWheelScroll.delta,
                           wndEvent.mouseWheelScroll.x,
                           wndEvent.mouseWheelScroll.y);
      break;
     case wndEvent.MouseButtonPressed:
      //if (!ImGui::IsAnyMouseDown && nullptr == ::GetCapture())
      //{
      //  ::SetCapture(handle);
      //}
      //io.MouseDown[static_cast<int32>(wndEvent.mouseButton.button)] = true;

      onMouseButtonPressed(wndEvent.mouseButton.button, 
                           wndEvent.mouseButton.x, 
                           wndEvent.mouseButton.y);
      break;
     case wndEvent.MouseButtonReleased:
      //if (!ImGui::IsAnyMouseDown && ::GetCapture() == handle)
      //{
      //  ::ReleaseCapture();
      //}
      //io.MouseDown[static_cast<int32>(wndEvent.mouseButton.button)] = false;

      onMouseButtonReleased(wndEvent.mouseButton.button, 
                            wndEvent.mouseButton.x, 
                            wndEvent.mouseButton.y);
      break;
     case wndEvent.MouseMoved:
      onMouseMoved(wndEvent.mouseMove.x, wndEvent.mouseMove.y);
      break;
     case wndEvent.MouseEntered:
      onMouseEntered();
      break;
     case wndEvent.MouseLeft:
      onMouseLeft();
      break;
     case wndEvent.JoystickButtonPressed:
      onJoystickButtonPressed(wndEvent.joystickButton.joystickId,
                              wndEvent.joystickButton.button);
      break;
     case wndEvent.JoystickButtonReleased:
      onJoystickButtonReleased(wndEvent.joystickButton.joystickId,
                               wndEvent.joystickButton.button);
      break;
     case wndEvent.JoystickMoved:
      
      onJoystickMoved(wndEvent.joystickMove.joystickId, 
                      wndEvent.joystickMove.axis, 
                      wndEvent.joystickMove.position);
      break;
     case wndEvent.JoystickConnected:
      onJoystickConnected(wndEvent.joystickConnect.joystickId);
      break;
     case wndEvent.JoystickDisconnected:
      onJoystickDisconnected(wndEvent.joystickConnect.joystickId);
      break;
     case wndEvent.TouchBegan:
      break;
     case wndEvent.TouchMoved:
      break;
     case wndEvent.TouchEnded:
      break;
     case wndEvent.SensorChanged:
      break;
     default:
      break;
    }
    
  }

  //void 
  //BaseApp::handleWindownput(const float& wndEvent) {
  //
  //}

  void
  BaseApp::update(const float& deltaTime) {
    m_inputAPI->update();
    onUpdate(deltaTime);
    if (m_simulate && !m_simulatePause) {
      g_Physics().simulated(deltaTime);
    }
  }

  void 
  BaseApp::render() {
    onRender();
    g_graphicsApi().Present();
  }
}

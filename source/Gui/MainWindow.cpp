#include "MainWindow.h"

#include <iostream>

#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#include "ImFileDialog.h"
#include "implot.h"
#include "Fonts/IconsFontAwesome5.h"

#include "EngineInterface/Serializer.h"
#include "EngineInterface/SimulationController.h"

#include "ModeController.h"
#include "SimulationView.h"
#include "StyleRepository.h"
#include "TemporalControlWindow.h"
#include "SpatialControlWindow.h"
#include "SimulationParametersWindow.h"
#include "StatisticsWindow.h"
#include "GpuSettingsDialog.h"
#include "Viewport.h"
#include "NewSimulationDialog.h"
#include "StartupController.h"
#include "AlienImGui.h"
#include "AboutDialog.h"
#include "MassOperationsDialog.h"
#include "LogWindow.h"
#include "SimpleLogger.h"
#include "UiController.h"
#include "AutosaveController.h"
#include "GettingStartedWindow.h"
#include "DisplaySettingsDialog.h"
#include "EditorController.h"
#include "SelectionWindow.h"
#include "PatternEditorWindow.h"
#include "WindowController.h"
#include "CreatorWindow.h"
#include "MultiplierWindow.h"
#include "PatternAnalysisDialog.h"
#include "MessageDialog.h"
#include "FpsController.h"
#include "NetworkController.h"
#include "BrowserWindow.h"
#include "LoginDialog.h"
#include "UploadSimulationDialog.h"
#include "CreateUserDialog.h"
#include "ActivateUserDialog.h"
#include "DelayedExecutionController.h"
#include "DeleteUserDialog.h"
#include "NetworkSettingsDialog.h"
#include "ResetPasswordDialog.h"
#include "NewPasswordDialog.h"
#include "ImageToPatternDialog.h"
#include "GenericFileDialogs.h"
#include "ShaderWindow.h"
#include "GenomeEditorWindow.h"
#include "RadiationSourcesWindow.h"
#include "OverlayMessageController.h"
#include "BalancerController.h"
#include "ExitDialog.h"

namespace
{
    void glfwErrorCallback(int error, const char* description)
    {
        throw std::runtime_error("Glfw error " + std::to_string(error) + ": " + description);
    }

    _SimulationView* simulationViewPtr;
    void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        if (width > 0 && height > 0) {
            simulationViewPtr->resize({width, height});
            glViewport(0, 0, width, height);
        }
    }
}

_MainWindow::_MainWindow(SimulationController const& simController, SimpleLogger const& logger)
{
    _logger = logger;
    _simController = simController;
    
    auto glfwVersion = initGlfw();

    WindowController::getInstance().init();

    auto windowData = WindowController::getInstance().getWindowData();
    glfwSetFramebufferSizeCallback(windowData.window, framebuffer_size_callback);
    glfwSwapInterval(1);  //enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

//     ImGui::StyleColorsDark();
//     ImGui::StyleColorsLight();

    StyleRepository::getInstance().init();

    // Setup Platform/Renderer back-ends
    ImGui_ImplGlfw_InitForOpenGL(windowData.window, true);
    ImGui_ImplOpenGL3_Init(glfwVersion);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    _viewport = std::make_shared<_Viewport>();
    _uiController = std::make_shared<_UiController>();
    _autosaveController = std::make_shared<_AutosaveController>(_simController, _viewport);

    _editorController =
        std::make_shared<_EditorController>(_simController, _viewport);
    _modeController = std::make_shared<_ModeController>(_editorController);
    _networkController = std::make_shared<_NetworkController>();
    _simulationView = std::make_shared<_SimulationView>(_simController, _modeController, _viewport, _editorController->getEditorModel());
    simulationViewPtr = _simulationView.get();
    _statisticsWindow = std::make_shared<_StatisticsWindow>(_simController);
    _temporalControlWindow = std::make_shared<_TemporalControlWindow>(_simController, _statisticsWindow);
    _spatialControlWindow = std::make_shared<_SpatialControlWindow>(_simController, _viewport);
    _radiationSourcesWindow = std::make_shared<_RadiationSourcesWindow>(_simController);
    _balancerController = std::make_shared<_BalancerController>(_simController);
    _simulationParametersWindow = std::make_shared<_SimulationParametersWindow>(_simController, _radiationSourcesWindow, _balancerController);
    _gpuSettingsDialog = std::make_shared<_GpuSettingsDialog>(_simController);
    _startupController = std::make_shared<_StartupController>(_simController, _temporalControlWindow, _viewport);
    _exitDialog = std::make_shared<_ExitDialog>(_onExit);
    _aboutDialog = std::make_shared<_AboutDialog>();
    _massOperationsDialog = std::make_shared<_MassOperationsDialog>(_simController);
    _logWindow = std::make_shared<_LogWindow>(_logger);
    _gettingStartedWindow = std::make_shared<_GettingStartedWindow>();
    _newSimulationDialog = std::make_shared<_NewSimulationDialog>(_simController, _temporalControlWindow, _viewport, _statisticsWindow);
    _displaySettingsDialog = std::make_shared<_DisplaySettingsDialog>();
    _patternAnalysisDialog = std::make_shared<_PatternAnalysisDialog>(_simController);
    _fpsController = std::make_shared<_FpsController>();
    _browserWindow = std::make_shared<_BrowserWindow>(_simController, _networkController, _statisticsWindow, _viewport, _temporalControlWindow);
    _activateUserDialog = std::make_shared<_ActivateUserDialog>(_browserWindow, _networkController);
    _createUserDialog = std::make_shared<_CreateUserDialog>(_activateUserDialog, _networkController);
    _newPasswordDialog = std::make_shared<_NewPasswordDialog>(_browserWindow, _networkController);
    _resetPasswordDialog = std::make_shared<_ResetPasswordDialog>(_newPasswordDialog, _networkController);
    _loginDialog = std::make_shared<_LoginDialog>(_browserWindow, _createUserDialog, _activateUserDialog, _resetPasswordDialog, _networkController);
    _uploadSimulationDialog = std::make_shared<_UploadSimulationDialog>(_browserWindow, _simController, _networkController, _viewport);
    _deleteUserDialog = std::make_shared<_DeleteUserDialog>(_browserWindow, _networkController);
    _networkSettingsDialog = std::make_shared<_NetworkSettingsDialog>(_browserWindow, _networkController);
    _imageToPatternDialog = std::make_shared<_ImageToPatternDialog>(_viewport, _simController);
    _shaderWindow = std::make_shared<_ShaderWindow>(_simulationView);

    //cyclic references
    _browserWindow->registerCyclicReferences(_loginDialog, _uploadSimulationDialog);
    _activateUserDialog->registerCyclicReferences(_createUserDialog);

    ifd::FileDialog::Instance().CreateTexture = [](uint8_t* data, int w, int h, char fmt) -> void* {
        GLuint tex;

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, (fmt == 0) ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        return reinterpret_cast<void*>(uintptr_t(tex));
    };
    ifd::FileDialog::Instance().DeleteTexture = [](void* tex) {
        GLuint texID = reinterpret_cast<uintptr_t>(tex);
        glDeleteTextures(1, &texID);
    };

    _window = windowData.window;
}

void _MainWindow::mainLoop()
{
    while (!glfwWindowShouldClose(_window) && !_onExit)
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

     //   ImGui::ShowDemoWindow(NULL);

        switch (_startupController->getState()) {
        case _StartupController::State::Unintialized:
            processUninitialized();
            break;
        case _StartupController::State::RequestLoading:
            processRequestLoading();
            break;
        case _StartupController::State::LoadingSimulation:
            processLoadingSimulation();
            break;
        case _StartupController::State::LoadingControls:
            processLoadingControls();
            break;
        case _StartupController::State::FinishedLoading:
            processFinishedLoading();
            break;
        default:
            THROW_NOT_IMPLEMENTED();
        }
    }
}

void _MainWindow::shutdown()
{
    WindowController::getInstance().shutdown();
    _autosaveController->shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(_window);
    glfwTerminate();

    _simulationView.reset();
}

char const* _MainWindow::initGlfw()
{
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize Glfw.");
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    return glsl_version;
}

void _MainWindow::processUninitialized()
{
    _startupController->process();

    // render mainData
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0, 0, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(_window);
}

void _MainWindow::processRequestLoading()
{
    _startupController->process();
    renderSimulation();
}

void _MainWindow::processLoadingSimulation()
{
    _startupController->process();
    renderSimulation();
}

void _MainWindow::processLoadingControls()
{
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, Const::SliderBarWidth);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);

    processMenubar();
    processDialogs();
    processWindows();
    processControllers();

    _uiController->process();
    _simulationView->processControls();
    _startupController->process();

    ImGui::PopStyleVar(2);

    renderSimulation();
}

void _MainWindow::processFinishedLoading()
{
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, Const::SliderBarWidth);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);

    processMenubar();
    processDialogs();
    processWindows();
    processControllers();
    _uiController->process();
    _simulationView->processControls();

    ImGui::PopStyleVar(2);

    renderSimulation();
}

void _MainWindow::renderSimulation()
{
    int display_w, display_h;
    glfwGetFramebufferSize(_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    if (_renderSimulation) {
        _simulationView->draw();
    } else {
        glClearColor(0, 0, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    ImGui::Render();

    _fpsController->processForceFps(WindowController::getInstance().getFps());

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(_window);
}

void _MainWindow::processMenubar()
{
    auto selectionWindow = _editorController->getSelectionWindow();
    auto patternEditorWindow = _editorController->getPatternEditorWindow();
    auto creatorWindow = _editorController->getCreatorWindow();
    auto multiplierWindow = _editorController->getMultiplierWindow();
    auto genomeEditorWindow = _editorController->getGenomeEditorWindow();

    if (ImGui::BeginMainMenuBar()) {
        if (AlienImGui::ShutdownButton()) {
            _exitDialog->open();
        }
        ImGui::Dummy(ImVec2(10.0f, 0.0f));
        if (AlienImGui::BeginMenuButton(" " ICON_FA_GAMEPAD "  Simulation ", _simulationMenuToggled, "Simulation")) {
            if (ImGui::MenuItem("New", "CTRL+N")) {
                _newSimulationDialog->open();
                _simulationMenuToggled = false;
            }
            if (ImGui::MenuItem("Open", "CTRL+O")) {
                onOpenSimulation();
                _simulationMenuToggled = false;
            }
            if (ImGui::MenuItem("Save", "CTRL+S")) {
                onSaveSimulation();
                _simulationMenuToggled = false;
            }
            ImGui::Separator();
            ImGui::BeginDisabled(_simController->isSimulationRunning());
            if (ImGui::MenuItem("Run", "SPACE")) {
                onRunSimulation();
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(!_simController->isSimulationRunning());
            if (ImGui::MenuItem("Pause", "SPACE")) {
                onPauseSimulation();
            }
            ImGui::EndDisabled();
            AlienImGui::EndMenuButton();
        }

        if (AlienImGui::BeginMenuButton(" " ICON_FA_GLOBE "  Network ", _networkMenuToggled, "Network", false)) {
            if (ImGui::MenuItem("Browser", "ALT+W", _browserWindow->isOn())) {
                _browserWindow->setOn(!_browserWindow->isOn());
            }
            ImGui::Separator();
            ImGui::BeginDisabled((bool)_networkController->getLoggedInUserName());
            if (ImGui::MenuItem("Login", "ALT+L")) {
                _loginDialog->open();
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(!_networkController->getLoggedInUserName());
            if (ImGui::MenuItem("Logout", "ALT+T")) {
                _networkController->logout();
                _browserWindow->onRefresh();
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(!_networkController->getLoggedInUserName());
            if (ImGui::MenuItem("Upload", "ALT+D")) {
                _uploadSimulationDialog->open();
            }
            ImGui::EndDisabled();

            ImGui::Separator();
            ImGui::BeginDisabled(!_networkController->getLoggedInUserName());
            if (ImGui::MenuItem("Delete", "ALT+J")) {
                _deleteUserDialog->open();
            }
            ImGui::EndDisabled();
            AlienImGui::EndMenuButton();
        }

        if (AlienImGui::BeginMenuButton(" " ICON_FA_WINDOW_RESTORE "  Windows ", _windowMenuToggled, "Windows")) {
            if (ImGui::MenuItem("Temporal control", "ALT+1", _temporalControlWindow->isOn())) {
                _temporalControlWindow->setOn(!_temporalControlWindow->isOn());
            }
            if (ImGui::MenuItem("Spatial control", "ALT+2", _spatialControlWindow->isOn())) {
                _spatialControlWindow->setOn(!_spatialControlWindow->isOn());
            }
            if (ImGui::MenuItem("Statistics", "ALT+3", _statisticsWindow->isOn())) {
                _statisticsWindow->setOn(!_statisticsWindow->isOn());
            }
            if (ImGui::MenuItem("Simulation parameters", "ALT+4", _simulationParametersWindow->isOn())) {
                _simulationParametersWindow->setOn(!_simulationParametersWindow->isOn());
            }
            if (ImGui::MenuItem("Radiation sources", "ALT+5", _radiationSourcesWindow->isOn())) {
                _radiationSourcesWindow->setOn(!_radiationSourcesWindow->isOn());
            }
            if (ImGui::MenuItem("Shader parameters", "ALT+6", _shaderWindow->isOn())) {
                _shaderWindow->setOn(!_shaderWindow->isOn());
            }
            if (ImGui::MenuItem("Log", "ALT+7", _logWindow->isOn())) {
                _logWindow->setOn(!_logWindow->isOn());
            }
            AlienImGui::EndMenuButton();
        }

        if (AlienImGui::BeginMenuButton(" " ICON_FA_PEN_ALT "  Editor ", _editorMenuToggled, "Editor")) {
            if (ImGui::MenuItem("Activate", "ALT+E", _modeController->getMode() == _ModeController::Mode::Editor)) {
                _modeController->setMode(
                    _modeController->getMode() == _ModeController::Mode::Editor ? _ModeController::Mode::Navigation
                                                                        : _ModeController::Mode::Editor);
            }
            ImGui::Separator();
            ImGui::BeginDisabled(_ModeController::Mode::Navigation == _modeController->getMode());
            if (ImGui::MenuItem("Selection", "ALT+S", selectionWindow->isOn())) {
                selectionWindow->setOn(!selectionWindow->isOn());
            }
            if (ImGui::MenuItem("Creator", "ALT+R", creatorWindow->isOn())) {
                creatorWindow->setOn(!creatorWindow->isOn());
            }
            if (ImGui::MenuItem("Pattern editor", "ALT+M", patternEditorWindow->isOn())) {
                patternEditorWindow->setOn(!patternEditorWindow->isOn());
            }
            if (ImGui::MenuItem("Genome editor", "ALT+B", genomeEditorWindow->isOn())) {
                genomeEditorWindow->setOn(!genomeEditorWindow->isOn());
            }
            if (ImGui::MenuItem("Multiplier", "ALT+A", multiplierWindow->isOn())) {
                multiplierWindow->setOn(!multiplierWindow->isOn());
            }
            ImGui::EndDisabled();
            ImGui::Separator();
            ImGui::BeginDisabled(_ModeController::Mode::Navigation == _modeController->getMode() || !_editorController->isObjectInspectionPossible());
            if (ImGui::MenuItem("Inspect objects", "ALT+N")) {
                _editorController->onInspectSelectedObjects();
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(_ModeController::Mode::Navigation == _modeController->getMode() || !_editorController->isGenomeInspectionPossible());
            if (ImGui::MenuItem("Inspect principal genome", "ALT+F")) {
                _editorController->onInspectSelectedGenomes();
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(_ModeController::Mode::Navigation == _modeController->getMode() || !_editorController->areInspectionWindowsActive());
            if (ImGui::MenuItem("Close inspections", "ESC")) {
                _editorController->onCloseAllInspectorWindows();
            }
            ImGui::EndDisabled();
            ImGui::Separator();
            ImGui::BeginDisabled(_ModeController::Mode::Navigation == _modeController->getMode() || !_editorController->isCopyingPossible());
            if (ImGui::MenuItem("Copy", "CTRL+C")) {
                _editorController->onCopy();
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(_ModeController::Mode::Navigation == _modeController->getMode() || !_editorController->isPastingPossible());
            if (ImGui::MenuItem("Paste", "CTRL+V")) {
                _editorController->onPaste();
            }
            ImGui::EndDisabled();
            AlienImGui::EndMenuButton();
        }

        if (AlienImGui::BeginMenuButton(" " ICON_FA_EYE "  View ", _viewMenuToggled, "View")) {
            if (ImGui::MenuItem("Information overlay", "ALT+O", _simulationView->isOverlayActive())) {
                _simulationView->setOverlayActive(!_simulationView->isOverlayActive());
            }
            if (ImGui::MenuItem("Render UI", "ALT+U", _uiController->isOn())) {
                _uiController->setOn(!_uiController->isOn());
            }
            if (ImGui::MenuItem("Render simulation", "ALT+I", _renderSimulation)) {
                _renderSimulation = !_renderSimulation;
            }
            AlienImGui::EndMenuButton();
        }

        if (AlienImGui::BeginMenuButton(" " ICON_FA_TOOLS "  Tools ", _toolsMenuToggled, "Tools")) {
            if (ImGui::MenuItem("Mass operations", "ALT+H")) {
                _massOperationsDialog->show();
                _toolsMenuToggled = false;
            }
            if (ImGui::MenuItem("Pattern analysis", "ALT+P")) {
                _patternAnalysisDialog->show();
                _toolsMenuToggled = false;
            }
            if (ImGui::MenuItem("Image converter", "ALT+G")) {
                _imageToPatternDialog->show();
                _toolsMenuToggled = false;
            }
            AlienImGui::EndMenuButton();
        }

        if (AlienImGui::BeginMenuButton(" " ICON_FA_COG "  Settings ", _settingsMenuToggled, "Settings", false)) {
            if (ImGui::MenuItem("Auto save", "", _autosaveController->isOn())) {
                _autosaveController->setOn(!_autosaveController->isOn());
            }
            if (ImGui::MenuItem("CUDA settings", "ALT+C")) {
                _gpuSettingsDialog->open();
            }
            if (ImGui::MenuItem("Display settings", "ALT+V")) {
                _displaySettingsDialog->open();
            }
            if (ImGui::MenuItem("Network settings", "ALT+K")) {
                _networkSettingsDialog->open();
            }
            AlienImGui::EndMenuButton();
        }

        if (AlienImGui::BeginMenuButton(" " ICON_FA_LIFE_RING "  Help ", _helpMenuToggled, "Help")) {
            if (ImGui::MenuItem("About", "")) {
                _aboutDialog->open();
                _helpMenuToggled = false;
            }
            if (ImGui::MenuItem("Getting started", "", _gettingStartedWindow->isOn())) {
                _gettingStartedWindow->setOn(!_gettingStartedWindow->isOn());
            }
            AlienImGui::EndMenuButton();
        }
        ImGui::EndMainMenuBar();
    }

    //hotkeys
    auto io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        if (io.KeyCtrl && ImGui::IsKeyPressed(GLFW_KEY_N)) {
            _newSimulationDialog->open();
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(GLFW_KEY_O)) {
            onOpenSimulation();
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(GLFW_KEY_S)) {
            onSaveSimulation();
        }
        if (ImGui::IsKeyPressed(GLFW_KEY_SPACE)) {
            if (_simController->isSimulationRunning()) {
                onPauseSimulation();
                printOverlayMessage("Pause");
            } else {
                onRunSimulation();
                printOverlayMessage("Run");
            }
            
        }

        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_W)) {
            _browserWindow->setOn(!_browserWindow->isOn());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_L) && !_networkController->getLoggedInUserName()) {
            _loginDialog->open();
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_T)) {
            _networkController->logout();
            _browserWindow->onRefresh();
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_D) && _networkController->getLoggedInUserName()) {
            _uploadSimulationDialog->open();
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_J) && _networkController->getLoggedInUserName()) {
            _deleteUserDialog->open();
        }

        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_1)) {
            _temporalControlWindow->setOn(!_temporalControlWindow->isOn());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_2)) {
            _spatialControlWindow->setOn(!_spatialControlWindow->isOn());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_3)) {
            _statisticsWindow->setOn(!_statisticsWindow->isOn());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_4)) {
            _simulationParametersWindow->setOn(!_simulationParametersWindow->isOn());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_5)) {
            _radiationSourcesWindow->setOn(!_radiationSourcesWindow->isOn());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_6)) {
            _shaderWindow->setOn(!_shaderWindow->isOn());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_7)) {
            _logWindow->setOn(!_logWindow->isOn());
        }

        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_E)) {
            _modeController->setMode(
                _modeController->getMode() == _ModeController::Mode::Editor ? _ModeController::Mode::Navigation : _ModeController::Mode::Editor);
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_S)) {
            selectionWindow->setOn(!selectionWindow->isOn());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_M)) {
            patternEditorWindow->setOn(!patternEditorWindow->isOn());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_B)) {
            genomeEditorWindow->setOn(!genomeEditorWindow->isOn());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_R)) {
            creatorWindow->setOn(!creatorWindow->isOn());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_A)) {
            multiplierWindow->setOn(!multiplierWindow->isOn());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_N) && _editorController->isObjectInspectionPossible()) {
            _editorController->onInspectSelectedObjects();
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_F) && _editorController->isGenomeInspectionPossible()) {
            _editorController->onInspectSelectedGenomes();
        }
        if (ImGui::IsKeyPressed(GLFW_KEY_ESCAPE)) {
            _editorController->onCloseAllInspectorWindows();
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(GLFW_KEY_C) && _editorController->isCopyingPossible()) {
            _editorController->onCopy();
            printOverlayMessage("Selection copied");
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(GLFW_KEY_V) && _editorController->isPastingPossible()) {
            _editorController->onPaste();
            printOverlayMessage("Selection pasted");
        }

        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_C)) {
            _gpuSettingsDialog->open();
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_V)) {
            _displaySettingsDialog->open();
        }
        if (ImGui::IsKeyPressed(GLFW_KEY_F7)) {
            auto& windowController = WindowController::getInstance();
            if (windowController.isDesktopMode()) {
                windowController.setWindowedMode();
            } else {
                windowController.setDesktopMode();
            }
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_K)) {
            _networkSettingsDialog->open();
        }

        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_O)) {
            _simulationView->setOverlayActive(!_simulationView->isOverlayActive());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_U)) {
            _uiController->setOn(!_uiController->isOn());
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_I)) {
            _renderSimulation = !_renderSimulation;
        }

        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_H)) {
            _massOperationsDialog->show();
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_P)) {
            _patternAnalysisDialog->show();
        }
        if (io.KeyAlt && ImGui::IsKeyPressed(GLFW_KEY_G)) {
            _imageToPatternDialog->show();
        }
    }
}

void _MainWindow::processDialogs()
{
    _newSimulationDialog->process();
    _aboutDialog->process();
    _massOperationsDialog->process();
    _gpuSettingsDialog->process();
    _displaySettingsDialog->process(); 
    _patternAnalysisDialog->process();
    _loginDialog->process();
    _createUserDialog->process();
    _activateUserDialog->process();
    _uploadSimulationDialog->process();
    _deleteUserDialog->process();
    _networkSettingsDialog->process();
    _resetPasswordDialog->process();
    _newPasswordDialog->process();
    _exitDialog->process();

    MessageDialog::getInstance().process();
    GenericFileDialogs::getInstance().process();
}

void _MainWindow::processWindows()
{
    _temporalControlWindow->process();
    _spatialControlWindow->process();
    _modeController->process();
    _statisticsWindow->process();
    _simulationParametersWindow->process();
    _logWindow->process();
    _browserWindow->process();
    _gettingStartedWindow->process();
    _shaderWindow->process();
    _radiationSourcesWindow->process();
}

void _MainWindow::processControllers()
{
    _autosaveController->process();
    _editorController->process();
    _balancerController->process();
    _networkController->process();
    OverlayMessageController::getInstance().process();
    DelayedExecutionController::getInstance().process();
}

void _MainWindow::onOpenSimulation()
{
    GenericFileDialogs::getInstance().showOpenFileDialog(
        "Open simulation", "Simulation file (*.sim){.sim},.*", _startingPath, [&](std::filesystem::path const& path) {
            auto firstFilename = ifd::FileDialog::Instance().GetResult();
            auto firstFilenameCopy = firstFilename;
            _startingPath = firstFilenameCopy.remove_filename().string();

            DeserializedSimulation deserializedData;
            if (Serializer::deserializeSimulationFromFiles(deserializedData, firstFilename.string())) {
                printOverlayMessage("Loading ...");
                delayedExecution([=, this] {
                    _simController->closeSimulation();
                    _statisticsWindow->reset();

                    _simController->newSimulation(
                        deserializedData.auxiliaryData.timestep,
                        deserializedData.auxiliaryData.generalSettings,
                        deserializedData.auxiliaryData.simulationParameters);
                    _simController->setClusteredSimulationData(deserializedData.mainData);
                    _viewport->setCenterInWorldPos(deserializedData.auxiliaryData.center);
                    _viewport->setZoomFactor(deserializedData.auxiliaryData.zoom);
                    _temporalControlWindow->onSnapshot();
                    printOverlayMessage(firstFilename.filename().string());
                });
            } else {
                printMessage("Open simulation", "The selected file could not be opened.");
            }
        });
}

void _MainWindow::onSaveSimulation()
{
    GenericFileDialogs::getInstance().showSaveFileDialog(
        "Save simulation", "Simulation file (*.sim){.sim},.*", _startingPath, [&](std::filesystem::path const& path) {
            auto firstFilename = ifd::FileDialog::Instance().GetResult();
            auto firstFilenameCopy = firstFilename;
            _startingPath = firstFilenameCopy.remove_filename().string();
            printOverlayMessage("Saving ...");
            delayedExecution([=, this] {
                DeserializedSimulation sim;
                sim.auxiliaryData.timestep = static_cast<uint32_t>(_simController->getCurrentTimestep());
                sim.auxiliaryData.zoom = _viewport->getZoomFactor();
                sim.auxiliaryData.center = _viewport->getCenterInWorldPos();
                sim.auxiliaryData.generalSettings = _simController->getGeneralSettings();
                sim.auxiliaryData.simulationParameters = _simController->getSimulationParameters();
                sim.mainData = _simController->getClusteredSimulationData();

                if (!Serializer::serializeSimulationToFiles(firstFilename.string(), sim)) {
                    MessageDialog::getInstance().show("Save simulation", "The simulation could not be saved to the specified file.");
                }
            });
        });
}

void _MainWindow::onRunSimulation()
{
    _simController->runSimulation();
}

void _MainWindow::onPauseSimulation()
{
    _simController->pauseSimulation();
}

void _MainWindow::reset()
{
    _statisticsWindow->reset();
}

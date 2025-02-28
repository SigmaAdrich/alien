#pragma once

#include "EngineInterface/Definitions.h"
#include "Definitions.h"

class _MainWindow
{
public:
    _MainWindow(SimulationController const& simController, SimpleLogger const& logger);
    void mainLoop();
    void shutdown();

private:
    char const* initGlfw();  //return glsl version

    void processUninitialized();
    void processRequestLoading();
    void processLoadingSimulation();
    void processLoadingControls();
    void processFinishedLoading();

    void renderSimulation();

    void processMenubar();
    void processDialogs();
    void processWindows();
    void processControllers();

    void onOpenSimulation();
    void onSaveSimulation();
    void onRunSimulation();
    void onPauseSimulation();

    void processExitDialog();
    void reset();

    GLFWwindow* _window;
    SimpleLogger _logger;

    Viewport _viewport;

    SimulationView _simulationView;
    TemporalControlWindow _temporalControlWindow;
    SpatialControlWindow _spatialControlWindow;
    SimulationParametersWindow _simulationParametersWindow;
    StatisticsWindow _statisticsWindow;
    LogWindow _logWindow;
    GettingStartedWindow _gettingStartedWindow;
    BrowserWindow _browserWindow;
    ShaderWindow _shaderWindow;
    RadiationSourcesWindow _radiationSourcesWindow;

    ExitDialog _exitDialog;
    GpuSettingsDialog _gpuSettingsDialog;
    MassOperationsDialog _massOperationsDialog;
    NewSimulationDialog _newSimulationDialog;
    DisplaySettingsDialog _displaySettingsDialog;
    PatternAnalysisDialog _patternAnalysisDialog;
    AboutDialog _aboutDialog;
    LoginDialog _loginDialog;
    CreateUserDialog _createUserDialog;
    UploadSimulationDialog _uploadSimulationDialog;
    ActivateUserDialog _activateUserDialog;
    DeleteUserDialog _deleteUserDialog;
    NetworkSettingsDialog _networkSettingsDialog;
    ResetPasswordDialog _resetPasswordDialog;
    NewPasswordDialog _newPasswordDialog;
    ImageToPatternDialog _imageToPatternDialog;

    ModeController _modeController;
    SimulationController _simController;
    StartupController _startupController;
    AutosaveController _autosaveController; 
    UiController _uiController; 
    EditorController _editorController; 
    FpsController _fpsController;
    NetworkController _networkController;
    BalancerController _balancerController;

    bool _onExit = false;
    bool _simulationMenuToggled = false;
    bool _networkMenuToggled = false;
    bool _windowMenuToggled = false;
    bool _settingsMenuToggled = false;
    bool _viewMenuToggled = false;
    bool _editorMenuToggled = false;
    bool _toolsMenuToggled = false;
    bool _helpMenuToggled = false;
    bool _renderSimulation = true;

    std::string _startingPath;
};
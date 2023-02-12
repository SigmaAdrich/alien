#include "NewSimulationDialog.h"

#include <imgui.h>

#include "EngineInterface/SimulationController.h"
#include "Viewport.h"
#include "StatisticsWindow.h"
#include "TemporalControlWindow.h"
#include "AlienImGui.h"
#include "GlobalSettings.h"

namespace
{
    auto const ContentTextInputWidth = 60.0f;
}

_NewSimulationDialog::_NewSimulationDialog(
    SimulationController const& simController,
    TemporalControlWindow const& temporalControlWindow,
    Viewport const& viewport,
    StatisticsWindow const& statisticsWindow)
    : _simController(simController)
    , _temporalControlWindow(temporalControlWindow)
    , _viewport(viewport)
    , _statisticsWindow(statisticsWindow)
{
    _adoptSimulationParameters = GlobalSettings::getInstance().getBoolState("dialogs.new simulation.adopt simulation parameters", true);
}

_NewSimulationDialog::~_NewSimulationDialog()
{
    GlobalSettings::getInstance().setBoolState("dialogs.new simulation.adopt simulation parameters", _adoptSimulationParameters);
}

void _NewSimulationDialog::process()
{
    if (!_on) {
        return;
    }
    ImGui::OpenPopup("New simulation");
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("New simulation", NULL, 0)) {

        AlienImGui::InputInt(AlienImGui::InputIntParameters().name("Width").textWidth(ContentTextInputWidth), _width);
        AlienImGui::InputInt(AlienImGui::InputIntParameters().name("Height").textWidth(ContentTextInputWidth), _height);
        AlienImGui::Checkbox(
            AlienImGui::CheckboxParameters().name("Adopt simulation parameters").textWidth(0), _adoptSimulationParameters);

        AlienImGui::Separator();
        if (AlienImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
            onNewSimulation();
            _on = false;
        }
        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();
        if (AlienImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            _on = false;
        }

        ImGui::EndPopup();
        _width = std::max(1, _width);
        _height = std::max(1, _height);
    }
}

void _NewSimulationDialog::show()
{
    _on = true;
    auto worldSize = _simController->getWorldSize();
    _width = worldSize.x;
    _height = worldSize.y;
}

void _NewSimulationDialog::onNewSimulation()
{
    _simController->closeSimulation();

    _statisticsWindow->reset();

    GeneralSettings generalSettings;
    generalSettings.worldSizeX = _width;
    generalSettings.worldSizeY = _height;
    SimulationParameters parameters;
    if (_adoptSimulationParameters) {
        parameters = _simController->getSimulationParameters();
    }
    _simController->newSimulation(0, generalSettings, parameters);
    _viewport->setCenterInWorldPos({toFloat(_width) / 2, toFloat(_height) / 2});
    _viewport->setZoomFactor(12.0f);
    _temporalControlWindow->onSnapshot();
}

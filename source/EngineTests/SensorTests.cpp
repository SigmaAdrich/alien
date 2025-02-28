#include <gtest/gtest.h>

#include "EngineInterface/DescriptionHelper.h"
#include "EngineInterface/Descriptions.h"
#include "EngineInterface/SimulationController.h"
#include "IntegrationTestFramework.h"

class SensorTests : public IntegrationTestFramework
{
public:
    static SimulationParameters getParameters()
    {
        SimulationParameters result;
        result.innerFriction = 0;
        result.baseValues.friction = 0;
        for (int i = 0; i < MAX_COLORS; ++i) {
            result.baseValues.radiationCellAgeStrength[i] = 0;
        }
        return result;
    }
    SensorTests()
        : IntegrationTestFramework(getParameters())
    {}

    ~SensorTests() = default;
};

TEST_F(SensorTests, scanNeighborhood_noActivity)
{
    DataDescription data;
    data.addCells(
        {CellDescription()
             .setId(1)
             .setPos({100.0f, 100.0f})
             .setMaxConnections(2)
             .setExecutionOrderNumber(0)
             .setInputExecutionOrderNumber(5)
             .setCellFunction(SensorDescription()),
         CellDescription()
             .setId(2)
             .setPos({101.0f, 100.0f})
             .setMaxConnections(1)
             .setExecutionOrderNumber(5)
             .setCellFunction(NerveDescription())
             .setActivity({0, 0, 0, 0, 0, 0, 0, 0})});
    data.addConnection(1, 2);

    _simController->setSimulationData(data);
    _simController->calcSingleTimestep();

    auto actualData = _simController->getSimulationData();
    auto actualAttackCell = getCell(actualData, 1);

    EXPECT_EQ(2, actualData.cells.size());
    EXPECT_TRUE(approxCompare(0.0f, actualAttackCell.activity.channels[0]));
}

TEST_F(SensorTests, scanNeighborhood_noOtherCell)
{
    DataDescription data;
    data.addCells(
        {CellDescription()
             .setId(1)
             .setPos({100.0f, 100.0f})
             .setMaxConnections(2)
             .setExecutionOrderNumber(0)
             .setInputExecutionOrderNumber(5)
             .setCellFunction(SensorDescription()),
         CellDescription()
             .setId(2)
             .setPos({101.0f, 100.0f})
             .setMaxConnections(1)
             .setExecutionOrderNumber(5)
             .setCellFunction(NerveDescription())
             .setActivity({1, 0, 0, 0, 0, 0, 0, 0})});
    data.addConnection(1, 2);

    _simController->setSimulationData(data);
    _simController->calcSingleTimestep();

    auto actualData = _simController->getSimulationData();
    auto actualAttackCell = getCell(actualData, 1);

    EXPECT_TRUE(approxCompare(0.0f, actualAttackCell.activity.channels[0]));
}

TEST_F(SensorTests, scanNeighborhood_densityTooLow)
{
    DataDescription data;
    data.addCells(
        {CellDescription()
             .setId(1)
             .setPos({100.0f, 100.0f})
             .setMaxConnections(2)
             .setExecutionOrderNumber(0)
             .setInputExecutionOrderNumber(5)
             .setCellFunction(SensorDescription()),
         CellDescription()
             .setId(2)
             .setPos({101.0f, 100.0f})
             .setMaxConnections(1)
             .setExecutionOrderNumber(5)
             .setCellFunction(NerveDescription())
             .setActivity({1, 0, 0, 0, 0, 0, 0, 0})});
    data.addConnection(1, 2);

    data.add(DescriptionHelper::createRect(DescriptionHelper::CreateRectParameters().center({10.0f, 100.0f}).width(10).height(10).cellDistance(2.5f)));

    _simController->setSimulationData(data);
    _simController->calcSingleTimestep();

    auto actualData = _simController->getSimulationData();
    auto actualAttackCell = getCell(actualData, 1);

    EXPECT_TRUE(approxCompare(0.0f, actualAttackCell.activity.channels[0]));
}

TEST_F(SensorTests, scanNeighborhood_wrongColor)
{
    DataDescription data;
    data.addCells(
        {CellDescription()
             .setId(1)
             .setPos({100.0f, 100.0f})
             .setMaxConnections(2)
             .setExecutionOrderNumber(0)
             .setInputExecutionOrderNumber(5)
             .setCellFunction(SensorDescription().setColor(1)),
         CellDescription()
             .setId(2)
             .setPos({101.0f, 100.0f})
             .setMaxConnections(1)
             .setExecutionOrderNumber(5)
             .setCellFunction(NerveDescription())
             .setActivity({1, 0, 0, 0, 0, 0, 0, 0})});
    data.addConnection(1, 2);

    data.add(DescriptionHelper::createRect(DescriptionHelper::CreateRectParameters().center({10.0f, 100.0f}).width(10).height(10).cellDistance(2.5f)));

    _simController->setSimulationData(data);
    _simController->calcSingleTimestep();

    auto actualData = _simController->getSimulationData();
    auto actualAttackCell = getCell(actualData, 1);

    EXPECT_TRUE(approxCompare(0.0f, actualAttackCell.activity.channels[0]));
}

TEST_F(SensorTests, scanNeighborhood_foundAtFront)
{
    DataDescription data;
    data.addCells(
        {CellDescription()
             .setId(1)
             .setPos({100.0f, 100.0f})
             .setMaxConnections(2)
             .setExecutionOrderNumber(0)
             .setInputExecutionOrderNumber(5)
             .setCellFunction(SensorDescription()),
         CellDescription()
             .setId(2)
             .setPos({101.0f, 100.0f})
             .setMaxConnections(1)
             .setExecutionOrderNumber(5)
             .setCellFunction(NerveDescription())
             .setActivity({1, 0, 0, 0, 0, 0, 0, 0})});
    data.addConnection(1, 2);

    data.add(DescriptionHelper::createRect(DescriptionHelper::CreateRectParameters().center({10.0f, 100.0f}).width(16).height(16).cellDistance(0.5f)));

    _simController->setSimulationData(data);
    _simController->calcSingleTimestep();

    auto actualData = _simController->getSimulationData();
    auto actualAttackCell = getCell(actualData, 1);

    EXPECT_TRUE(approxCompare(1.0f, actualAttackCell.activity.channels[0]));
    EXPECT_TRUE(actualAttackCell.activity.channels[1] > 0.3f);
    EXPECT_TRUE(actualAttackCell.activity.channels[2] > 80.0f / 256);
    EXPECT_TRUE(actualAttackCell.activity.channels[2] < 105.0f / 256);
    EXPECT_TRUE(actualAttackCell.activity.channels[3] > -15.0f / 365);
    EXPECT_TRUE(actualAttackCell.activity.channels[3] < 15.0f / 365);
}

TEST_F(SensorTests, scanNeighborhood_foundAtRightHandSide)
{
    DataDescription data;
    data.addCells(
        {CellDescription()
             .setId(1)
             .setPos({100.0f, 100.0f})
             .setMaxConnections(2)
             .setExecutionOrderNumber(0)
             .setInputExecutionOrderNumber(5)
             .setCellFunction(SensorDescription()),
         CellDescription()
             .setId(2)
             .setPos({101.0f, 100.0f})
             .setMaxConnections(1)
             .setExecutionOrderNumber(5)
             .setCellFunction(NerveDescription())
             .setActivity({1, 0, 0, 0, 0, 0, 0, 0})});
    data.addConnection(1, 2);

    data.add(DescriptionHelper::createRect(DescriptionHelper::CreateRectParameters().center({100.0f, 10.0f}).width(16).height(16).cellDistance(0.5f)));

    _simController->setSimulationData(data);
    _simController->calcSingleTimestep();

    auto actualData = _simController->getSimulationData();
    auto actualAttackCell = getCell(actualData, 1);

    EXPECT_TRUE(approxCompare(1.0f, actualAttackCell.activity.channels[0]));
    EXPECT_TRUE(actualAttackCell.activity.channels[1] > 0.3f);
    EXPECT_TRUE(actualAttackCell.activity.channels[2] > 80.0f / 256);
    EXPECT_TRUE(actualAttackCell.activity.channels[2] < 105.0f / 256);
    EXPECT_TRUE(actualAttackCell.activity.channels[3] > 70.0f / 365);
    EXPECT_TRUE(actualAttackCell.activity.channels[3] < 105.0f / 365);
}

TEST_F(SensorTests, scanNeighborhood_foundAtLeftHandSide)
{
    DataDescription data;
    data.addCells(
        {CellDescription()
             .setId(1)
             .setPos({100.0f, 100.0f})
             .setMaxConnections(2)
             .setExecutionOrderNumber(0)
             .setInputExecutionOrderNumber(5)
             .setCellFunction(SensorDescription()),
         CellDescription()
             .setId(2)
             .setPos({101.0f, 100.0f})
             .setMaxConnections(1)
             .setExecutionOrderNumber(5)
             .setCellFunction(NerveDescription())
             .setActivity({1, 0, 0, 0, 0, 0, 0, 0})});
    data.addConnection(1, 2);

    data.add(DescriptionHelper::createRect(DescriptionHelper::CreateRectParameters().center({100.0f, 190.0f}).width(16).height(16).cellDistance(0.5f)));

    _simController->setSimulationData(data);
    _simController->calcSingleTimestep();

    auto actualData = _simController->getSimulationData();
    auto actualAttackCell = getCell(actualData, 1);

    EXPECT_TRUE(approxCompare(1.0f, actualAttackCell.activity.channels[0]));
    EXPECT_TRUE(actualAttackCell.activity.channels[1] > 0.3f);
    EXPECT_TRUE(actualAttackCell.activity.channels[2] > 80.0f / 256);
    EXPECT_TRUE(actualAttackCell.activity.channels[2] < 105.0f / 256);
    EXPECT_TRUE(actualAttackCell.activity.channels[3] < -70.0f / 365);
    EXPECT_TRUE(actualAttackCell.activity.channels[3] > -105.0f / 365);
}

TEST_F(SensorTests, scanNeighborhood_foundAtBack)
{
    DataDescription data;
    data.addCells(
        {CellDescription()
             .setId(1)
             .setPos({100.0f, 100.0f})
             .setMaxConnections(2)
             .setExecutionOrderNumber(0)
             .setInputExecutionOrderNumber(5)
             .setCellFunction(SensorDescription()),
         CellDescription()
             .setId(2)
             .setPos({101.0f, 100.0f})
             .setMaxConnections(1)
             .setExecutionOrderNumber(5)
             .setCellFunction(NerveDescription())
             .setActivity({1, 0, 0, 0, 0, 0, 0, 0})});
    data.addConnection(1, 2);

    data.add(DescriptionHelper::createRect(DescriptionHelper::CreateRectParameters().center({190.0f, 100.0f}).width(16).height(16).cellDistance(0.5f)));

    _simController->setSimulationData(data);
    _simController->calcSingleTimestep();

    auto actualData = _simController->getSimulationData();
    auto actualAttackCell = getCell(actualData, 1);

    EXPECT_TRUE(approxCompare(1.0f, actualAttackCell.activity.channels[0]));
    EXPECT_TRUE(actualAttackCell.activity.channels[1] > 0.3f);
    EXPECT_TRUE(actualAttackCell.activity.channels[2] > 80.0f / 256);
    EXPECT_TRUE(actualAttackCell.activity.channels[2] < 105.0f / 256);
    EXPECT_TRUE(actualAttackCell.activity.channels[3] < -165.0f / 365 || actualAttackCell.activity.channels[3] > 165.0f / 365);
}


TEST_F(SensorTests, scanNeighborhood_twoMasses)
{
    DataDescription data;
    data.addCells(
        {CellDescription()
             .setId(1)
             .setPos({100.0f, 100.0f})
             .setMaxConnections(2)
             .setExecutionOrderNumber(0)
             .setInputExecutionOrderNumber(5)
             .setCellFunction(SensorDescription()),
         CellDescription()
             .setId(2)
             .setPos({101.0f, 100.0f})
             .setMaxConnections(1)
             .setExecutionOrderNumber(5)
             .setCellFunction(NerveDescription())
             .setActivity({1, 0, 0, 0, 0, 0, 0, 0})});
    data.addConnection(1, 2);

    data.add(DescriptionHelper::createRect(DescriptionHelper::CreateRectParameters().center({100.0f, 10.0f}).width(16).height(16).cellDistance(0.8f)));
    data.add(DescriptionHelper::createRect(DescriptionHelper::CreateRectParameters().center({100.0f, 200.0f}).width(16).height(16).cellDistance(0.5f)));

    _simController->setSimulationData(data);
    _simController->calcSingleTimestep();

    auto actualData = _simController->getSimulationData();
    auto actualAttackCell = getCell(actualData, 1);

    EXPECT_TRUE(approxCompare(1.0f, actualAttackCell.activity.channels[0]));
    EXPECT_TRUE(actualAttackCell.activity.channels[1] > 0.3f);
    EXPECT_TRUE(actualAttackCell.activity.channels[2] > 80.0f / 256);
    EXPECT_TRUE(actualAttackCell.activity.channels[2] < 105.0f / 256);
    EXPECT_TRUE(actualAttackCell.activity.channels[3] > 70.0f / 365);
    EXPECT_TRUE(actualAttackCell.activity.channels[3] < 105.0f / 365);
}

TEST_F(SensorTests, scanByAngle_found)
{
    DataDescription data;
    data.addCells(
        {CellDescription()
             .setId(1)
             .setPos({100.0f, 100.0f})
             .setMaxConnections(2)
             .setExecutionOrderNumber(0)
             .setInputExecutionOrderNumber(5)
             .setCellFunction(SensorDescription().setFixedAngle(-90.0f)),
         CellDescription()
             .setId(2)
             .setPos({101.0f, 100.0f})
             .setMaxConnections(1)
             .setExecutionOrderNumber(5)
             .setCellFunction(NerveDescription())
             .setActivity({1, 0, 0, 0, 0, 0, 0, 0})});
    data.addConnection(1, 2);

    data.add(DescriptionHelper::createRect(DescriptionHelper::CreateRectParameters().center({100.0f, 190.0f}).width(16).height(16).cellDistance(0.5f)));

    _simController->setSimulationData(data);
    _simController->calcSingleTimestep();

    auto actualData = _simController->getSimulationData();
    auto actualAttackCell = getCell(actualData, 1);

    EXPECT_TRUE(approxCompare(1.0f, actualAttackCell.activity.channels[0]));
    EXPECT_TRUE(actualAttackCell.activity.channels[1] > 0.3f);
    EXPECT_TRUE(actualAttackCell.activity.channels[2] > 80.0f / 256);
    EXPECT_TRUE(actualAttackCell.activity.channels[2] < 105.0f / 256);
}

TEST_F(SensorTests, scanByAngle_wrongAngle)
{
    DataDescription data;
    data.addCells(
        {CellDescription()
             .setId(1)
             .setPos({100.0f, 100.0f})
             .setMaxConnections(2)
             .setExecutionOrderNumber(0)
             .setInputExecutionOrderNumber(5)
             .setCellFunction(SensorDescription().setFixedAngle(90.0f)),
         CellDescription()
             .setId(2)
             .setPos({101.0f, 100.0f})
             .setMaxConnections(1)
             .setExecutionOrderNumber(5)
             .setCellFunction(NerveDescription())
             .setActivity({1, 0, 0, 0, 0, 0, 0, 0})});
    data.addConnection(1, 2);

    data.add(DescriptionHelper::createRect(DescriptionHelper::CreateRectParameters().center({100.0f, 190.0f}).width(16).height(16).cellDistance(0.5f)));

    _simController->setSimulationData(data);
    _simController->calcSingleTimestep();

    auto actualData = _simController->getSimulationData();
    auto actualAttackCell = getCell(actualData, 1);

    EXPECT_TRUE(approxCompare(0.0f, actualAttackCell.activity.channels[0]));
}

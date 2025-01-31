#pragma once

#include "EngineInterface/CellFunctionConstants.h"
#include "EngineInterface/GenomeConstants.h"
#include "Base.cuh"
#include "Cell.cuh"

struct GenomeHeader
{
    ConstructionShape shape;
    bool singleConstruction;
    bool separateConstruction;
    ConstructorAngleAlignment angleAlignment;
    float stiffness;
    float connectionDistance;
};

class GenomeDecoder
{
public:
    template <typename Func>
    __inline__ __device__ static void executeForEachNode(uint8_t* genome, int genomeSize, Func func);
    template <typename Func>
    __inline__ __device__ static void executeForEachNodeRecursively(uint8_t* genome, int genomeSize, Func func);
    template <typename Func>
    __inline__ __device__ static void executeForEachNodeUntilReadPosition(ConstructorFunction const& constructor, Func func);
    __inline__ __device__ static int getGenomeDepth(uint8_t* genome, int genomeSize);
    __inline__ __device__ static int getNumNodesRecursively(uint8_t* genome, int genomeSize);

    __inline__ __device__ static int getRandomGenomeNodeAddress(
        SimulationData& data,
        uint8_t* genome,
        int genomeSize,
        bool considerZeroSubGenomes,
        int* subGenomesSizeIndices = nullptr,
        int* numSubGenomesSizeIndices = nullptr,
        int randomRefIndex = 0);
    __inline__ __device__ static void setRandomCellFunctionData(
        SimulationData& data,
        uint8_t* genome,
        int nodeAddress,
        CellFunction const& cellFunction,
        bool makeSelfCopy,
        int subGenomeSize);

    __inline__ __device__ static int getNumNodes(uint8_t* genome, int genomeSize);
    __inline__ __device__ static int getNodeAddress(uint8_t* genome, int genomeSize, int nodeIndex);
    __inline__ __device__ static int findStartNodeAddress(uint8_t* genome, int genomeSize, int refIndex);
    __inline__ __device__ static int getNextCellFunctionDataSize(uint8_t* genome, int genomeSize, int nodeAddress, bool withSubgenomes = true);
    __inline__ __device__ static int getNextCellFunctionType(uint8_t* genome, int nodeAddress);
    __inline__ __device__ static bool isNextCellSelfCopy(uint8_t* genome, int nodeAddress);
    __inline__ __device__ static int getNextCellColor(uint8_t* genome, int nodeAddress);
    __inline__ __device__ static void setNextCellFunctionType(uint8_t* genome, int nodeAddress, CellFunction cellFunction);
    __inline__ __device__ static void setNextCellColor(uint8_t* genome, int nodeAddress, int color);
    __inline__ __device__ static void setNextAngle(uint8_t* genome, int nodeAddress, uint8_t angle);
    __inline__ __device__ static void setNextRequiredConnections(uint8_t* genome, int nodeAddress, uint8_t angle);
    __inline__ __device__ static void setNextConstructionAngle1(uint8_t* genome, int nodeAddress, uint8_t angle);
    __inline__ __device__ static void setNextConstructionAngle2(uint8_t* genome, int nodeAddress, uint8_t angle);
    __inline__ __device__ static void setNextConstructorSeparation(uint8_t* genome, int nodeAddress, bool separation);

    __inline__ __device__ static int
    getNextSubGenomeSize(uint8_t* genome, int genomeSize, int nodeAddress);  //prerequisites: (constructor or injector) and !makeSelfCopy
    __inline__ __device__ static int getCellFunctionDataSize(
        CellFunction cellFunction,
        bool makeSelfCopy,
        int genomeSize);  //genomeSize only relevant for cellFunction = constructor or injector
    __inline__ __device__ static bool hasSelfCopy(uint8_t* genome, int genomeSize);


    //automatic increment genomeReadPosition
    __inline__ __device__ static bool readBool(ConstructorFunction& constructor);
    __inline__ __device__ static uint8_t readByte(ConstructorFunction& constructor);
    __inline__ __device__ static int readOptionalByte(ConstructorFunction& constructor, int moduloValue);
    __inline__ __device__ static int readWord(ConstructorFunction& constructor);
    __inline__ __device__ static float readFloat(ConstructorFunction& constructor);   //return values from -1 to 1
    __inline__ __device__ static float readEnergy(ConstructorFunction& constructor);  //return values from 36 to 1060
    __inline__ __device__ static float readAngle(ConstructorFunction& constructor);

    __inline__ __device__ static bool isAtFirstNode(ConstructorFunction const& constructor);
    __inline__ __device__ static bool isAtLastNode(ConstructorFunction const& constructor);
    __inline__ __device__ static bool isFinished(ConstructorFunction const& constructor);
    __inline__ __device__ static bool isFinishedSingleConstruction(ConstructorFunction const& constructor);
    __inline__ __device__ static bool isSeparating(ConstructorFunction const& constructor);
    template <typename ConstructorOrInjector>
    __inline__ __device__ static bool containsSelfReplication(ConstructorOrInjector const& cellFunction);

    template <typename GenomeHolderSource, typename GenomeHolderTarget>
    __inline__ __device__ static void copyGenome(SimulationData& data, GenomeHolderSource& source, GenomeHolderTarget& target);
    __inline__ __device__ static GenomeHeader readGenomeHeader(ConstructorFunction const& constructor);
    __inline__ __device__ static int readWord(uint8_t* genome, int nodeAddress);
    __inline__ __device__ static void writeWord(uint8_t* genome, int address, int word);

    __inline__ __device__ static bool convertByteToBool(uint8_t b);
    __inline__ __device__ static uint8_t convertBoolToByte(bool value);
    __inline__ __device__ static int convertBytesToWord(uint8_t b1, uint8_t b2);
    __inline__ __device__ static void convertWordToBytes(int word, uint8_t& b1, uint8_t& b2);
    __inline__ __device__ static uint8_t convertAngleToByte(float angle);
    __inline__ __device__ static uint8_t convertOptionalByteToByte(int value);

    static auto constexpr MAX_SUBGENOME_RECURSION_DEPTH = 30;
};

/************************************************************************/
/* Implementation                                                       */
/************************************************************************/
__inline__ __device__ bool GenomeDecoder::readBool(ConstructorFunction& constructor)
{
    return convertByteToBool(readByte(constructor));
}

__inline__ __device__ uint8_t GenomeDecoder::readByte(ConstructorFunction& constructor)
{
    if (isFinished(constructor)) {
        return 0;
    }
    uint8_t result = constructor.genome[constructor.genomeReadPosition++];
    return result;
}

__inline__ __device__ int GenomeDecoder::readOptionalByte(ConstructorFunction& constructor, int moduloValue)
{
    auto result = static_cast<int>(readByte(constructor));
    result = result > 127 ? -1 : result % moduloValue;
    return result;
}

__inline__ __device__ int GenomeDecoder::readWord(ConstructorFunction& constructor)
{
    auto b1 = readByte(constructor);
    auto b2 = readByte(constructor);
    return convertBytesToWord(b1, b2);
}

__inline__ __device__ float GenomeDecoder::readFloat(ConstructorFunction& constructor)
{
    return static_cast<float>(static_cast<int8_t>(readByte(constructor))) / 128;
}

__inline__ __device__ float GenomeDecoder::readEnergy(ConstructorFunction& constructor)
{
    return static_cast<float>(static_cast<int8_t>(readByte(constructor))) / 128 * 512 + 548;
}

__inline__ __device__ float GenomeDecoder::readAngle(ConstructorFunction& constructor)
{
    return static_cast<float>(static_cast<int8_t>(readByte(constructor))) / 120 * 180;
}

__inline__ __device__ bool GenomeDecoder::isAtFirstNode(ConstructorFunction const& constructor)
{
    return constructor.genomeReadPosition <= Const::GenomeHeaderSize;
}

__inline__ __device__ bool GenomeDecoder::isAtLastNode(ConstructorFunction const& constructor)
{
    auto nextNodeBytes =
        Const::CellBasicBytes + getNextCellFunctionDataSize(constructor.genome, toInt(constructor.genomeSize), toInt(constructor.genomeReadPosition));
    return toInt(constructor.genomeReadPosition) + nextNodeBytes >= toInt(constructor.genomeSize);
}

__inline__ __device__ bool GenomeDecoder::isFinished(ConstructorFunction const& constructor)
{
    return constructor.genomeReadPosition >= constructor.genomeSize;
}

template <typename GenomeHolderSource, typename GenomeHolderTarget>
__inline__ __device__ void GenomeDecoder::copyGenome(SimulationData& data, GenomeHolderSource& source, GenomeHolderTarget& target)
{
    bool makeGenomeCopy = readBool(source);
    if (!makeGenomeCopy) {
        auto size = readWord(source);
        size = min(size, toInt(source.genomeSize) - toInt(source.genomeReadPosition));
        target.genomeSize = size;
        target.genome = data.objects.auxiliaryData.getAlignedSubArray(size);
        //#TODO can be optimized
        for (int i = 0; i < size; ++i) {
            target.genome[i] = readByte(source);
        }
    } else {
        auto size = source.genomeSize;
        target.genomeSize = size;
        target.genome = data.objects.auxiliaryData.getAlignedSubArray(size);
        //#TODO can be optimized
        for (int i = 0; i < size; ++i) {
            target.genome[i] = source.genome[i];
        }
    }
}

__inline__ __device__ bool GenomeDecoder::isFinishedSingleConstruction(ConstructorFunction const& constructor)
{
    auto genomeHeader = readGenomeHeader(constructor);
    return genomeHeader.singleConstruction
        && (constructor.genomeReadPosition >= constructor.genomeSize || (constructor.genomeReadPosition == 0 && constructor.genomeSize == Const::GenomeHeaderSize));
}

__inline__ __device__ bool GenomeDecoder::isSeparating(ConstructorFunction const& constructor)
{
    auto genomeHeader = readGenomeHeader(constructor);
    return genomeHeader.separateConstruction;
}

template <typename ConstructorOrInjector>
__inline__ __device__ bool GenomeDecoder::containsSelfReplication(ConstructorOrInjector const& cellFunction)
{
    for (int currentNodeAddress = Const::GenomeHeaderSize; currentNodeAddress < cellFunction.genomeSize;) {
        if (isNextCellSelfCopy(cellFunction.genome, currentNodeAddress)) {
            return true;
        }
        currentNodeAddress += Const::CellBasicBytes + getNextCellFunctionDataSize(cellFunction.genome, cellFunction.genomeSize, currentNodeAddress);
    }

    return false;
}

__inline__ __device__ GenomeHeader GenomeDecoder::readGenomeHeader(ConstructorFunction const& constructor)
{
    GenomeHeader result;
    if (constructor.genomeSize < Const::GenomeHeaderSize) {
        CUDA_THROW_NOT_IMPLEMENTED();
    }
    result.shape = constructor.genome[0] % ConstructionShape_Count;
    result.singleConstruction = GenomeDecoder::convertByteToBool(constructor.genome[1]);
    result.separateConstruction = GenomeDecoder::convertByteToBool(constructor.genome[2]);
    result.angleAlignment = constructor.genome[3] % ConstructorAngleAlignment_Count;
    result.stiffness = toFloat(constructor.genome[4]) / 255;
    result.connectionDistance = toFloat(constructor.genome[5]) / 255 + 0.5f;
    return result;
}

__inline__ __device__ int GenomeDecoder::readWord(uint8_t* genome, int address)
{
    return GenomeDecoder::convertBytesToWord(genome[address], genome[address + 1]);
}

__inline__ __device__ void GenomeDecoder::writeWord(uint8_t* genome, int address, int word)
{
    GenomeDecoder::convertWordToBytes(word, genome[address], genome[address + 1]);
}

__inline__ __device__ bool GenomeDecoder::convertByteToBool(uint8_t b)
{
    return static_cast<int8_t>(b) > 0;
}

__inline__ __device__ uint8_t GenomeDecoder::convertBoolToByte(bool value)
{
    return value ? 1 : 0;
}

__inline__ __device__ int GenomeDecoder::convertBytesToWord(uint8_t b1, uint8_t b2)
{
    return static_cast<int>(b1) | (static_cast<int>(b2 << 8));
}

__inline__ __device__ void GenomeDecoder::convertWordToBytes(int word, uint8_t& b1, uint8_t& b2)
{
    b1 = static_cast<uint8_t>(word & 0xff);
    b2 = static_cast<uint8_t>((word >> 8) & 0xff);
}

__inline__ __device__ uint8_t GenomeDecoder::convertAngleToByte(float angle)
{
    if (angle > 180.0f) {
        angle -= 360.0f;
    }
    if (angle < -180.0f) {
        angle += 360.0f;
    }
    return static_cast<uint8_t>(static_cast<int8_t>(angle / 180 * 120));
}

__inline__ __device__ uint8_t GenomeDecoder::convertOptionalByteToByte(int value)
{
    return static_cast<uint8_t>(value);
}

template <typename Func>
__inline__ __device__ void GenomeDecoder::executeForEachNode(uint8_t* genome, int genomeSize, Func func)
{
    for (int currentNodeAddress = Const::GenomeHeaderSize; currentNodeAddress < genomeSize;) {
        currentNodeAddress +=
            Const::CellBasicBytes + GenomeDecoder::getNextCellFunctionDataSize(genome, genomeSize, currentNodeAddress);

        func(currentNodeAddress);
    }
}

template <typename Func>
__inline__ __device__ void GenomeDecoder::executeForEachNodeRecursively(uint8_t* genome, int genomeSize, Func func)
{
    if (genomeSize < Const::GenomeHeaderSize) {
        CUDA_THROW_NOT_IMPLEMENTED();
    }
    int subGenomeEndAddresses[MAX_SUBGENOME_RECURSION_DEPTH];
    int depth = 0;
    for (auto nodeAddress = Const::GenomeHeaderSize; nodeAddress < genomeSize;) {
        auto cellFunction = GenomeDecoder::getNextCellFunctionType(genome, nodeAddress);
        func(depth, nodeAddress);

        bool goToNextSibling = true;
        if (cellFunction == CellFunction_Constructor || cellFunction == CellFunction_Injector) {
            auto cellFunctionFixedBytes = cellFunction == CellFunction_Constructor ? Const::ConstructorFixedBytes : Const::InjectorFixedBytes;
            auto makeSelfCopy = GenomeDecoder::convertByteToBool(genome[nodeAddress + Const::CellBasicBytes + cellFunctionFixedBytes]);
            if (!makeSelfCopy) {
                auto subGenomeSize = GenomeDecoder::getNextSubGenomeSize(genome, genomeSize, nodeAddress);
                nodeAddress += Const::CellBasicBytes + cellFunctionFixedBytes + 3;
                subGenomeEndAddresses[depth++] = nodeAddress + subGenomeSize;
                nodeAddress += Const::GenomeHeaderSize;
                goToNextSibling = false;
            }
        }
        if (goToNextSibling) {
            nodeAddress += Const::CellBasicBytes + GenomeDecoder::getNextCellFunctionDataSize(genome, genomeSize, nodeAddress);
        }
        for (int i = 0; i < MAX_SUBGENOME_RECURSION_DEPTH && depth > 0; ++i) {
            if (depth > 0) {
                if (subGenomeEndAddresses[depth - 1] == nodeAddress) {
                    --depth;
                } else {
                    break;
                }
            }
        }
    }
}

template <typename Func>
__inline__ __device__ void GenomeDecoder::executeForEachNodeUntilReadPosition(ConstructorFunction const& constructor, Func func)
{
    for (int currentNodeAddress = Const::GenomeHeaderSize; currentNodeAddress <= constructor.genomeReadPosition;) {
        currentNodeAddress +=
            Const::CellBasicBytes + GenomeDecoder::getNextCellFunctionDataSize(constructor.genome, constructor.genomeSize, currentNodeAddress);

        func(currentNodeAddress > constructor.genomeReadPosition);
    }
}

__inline__ __device__ int GenomeDecoder::getGenomeDepth(uint8_t* genome, int genomeSize)
{
    auto result = 0;
    executeForEachNodeRecursively(genome, genomeSize, [&result](int depth, int nodeAddress) { result = max(result, depth); });
    return result;
}

__inline__ __device__ int GenomeDecoder::getNumNodesRecursively(uint8_t* genome, int genomeSize)
{
    auto result = 0;
    executeForEachNodeRecursively(genome, genomeSize, [&result](int depth, int nodeAddress) { ++result; });
    return result;
}

__inline__ __device__ int GenomeDecoder::getRandomGenomeNodeAddress(
    SimulationData& data,
    uint8_t* genome,
    int genomeSize,
    bool considerZeroSubGenomes,
    int* subGenomesSizeIndices,
    int* numSubGenomesSizeIndices,
    int randomRefIndex)
{
    if (numSubGenomesSizeIndices) {
        *numSubGenomesSizeIndices = 0;
    }
    if (genomeSize < Const::GenomeHeaderSize) {
        CUDA_THROW_NOT_IMPLEMENTED();
    }
    if (genomeSize == Const::GenomeHeaderSize) {
        return Const::GenomeHeaderSize;
    }
    if (randomRefIndex == 0) {
        randomRefIndex = data.numberGen1.random(genomeSize - 1);
    }

    int result = 0;
    for (int depth = 0; depth < MAX_SUBGENOME_RECURSION_DEPTH; ++depth) {
        auto nodeAddress = findStartNodeAddress(genome, genomeSize, randomRefIndex);
        result += nodeAddress;
        auto cellFunction = getNextCellFunctionType(genome, nodeAddress);

        if (cellFunction == CellFunction_Constructor || cellFunction == CellFunction_Injector) {
            auto cellFunctionFixedBytes = cellFunction == CellFunction_Constructor ? Const::ConstructorFixedBytes : Const::InjectorFixedBytes;
            auto makeSelfCopy = GenomeDecoder::convertByteToBool(genome[nodeAddress + Const::CellBasicBytes + cellFunctionFixedBytes]);
            if (makeSelfCopy) {
                break;
            } else {
                if (nodeAddress + Const::CellBasicBytes + cellFunctionFixedBytes > randomRefIndex) {
                    break;
                }
                if (numSubGenomesSizeIndices) {
                    subGenomesSizeIndices[*numSubGenomesSizeIndices] = result + Const::CellBasicBytes + cellFunctionFixedBytes + 1;
                    ++(*numSubGenomesSizeIndices);
                }
                auto subGenomeStartIndex = nodeAddress + Const::CellBasicBytes + cellFunctionFixedBytes + 3;
                auto subGenomeSize = getNextSubGenomeSize(genome, genomeSize, nodeAddress);
                if (subGenomeSize == Const::GenomeHeaderSize) {
                    if (considerZeroSubGenomes && data.numberGen1.randomBool()) {
                        result += Const::CellBasicBytes + cellFunctionFixedBytes + 3 + Const::GenomeHeaderSize;
                    } else {
                        if (numSubGenomesSizeIndices) {
                            --(*numSubGenomesSizeIndices);
                        }
                    }
                    break;
                }
                genomeSize = subGenomeSize;
                genome = genome + subGenomeStartIndex;
                randomRefIndex -= subGenomeStartIndex;
                result += Const::CellBasicBytes + cellFunctionFixedBytes + 3;
            }
        } else {
            break;
        }
    }
    return result;
}

__inline__ __device__ void GenomeDecoder::setRandomCellFunctionData(
    SimulationData& data,
    uint8_t* genome,
    int nodeAddress,
    CellFunction const& cellFunction,
    bool makeSelfCopy,
    int subGenomeSize)
{
    auto newCellFunctionSize = getCellFunctionDataSize(cellFunction, makeSelfCopy, subGenomeSize);
    data.numberGen1.randomBytes(genome + nodeAddress, newCellFunctionSize);
    if (cellFunction == CellFunction_Constructor || cellFunction == CellFunction_Injector) {
        auto cellFunctionFixedBytes = cellFunction == CellFunction_Constructor ? Const::ConstructorFixedBytes : Const::InjectorFixedBytes;
        genome[nodeAddress + cellFunctionFixedBytes] = makeSelfCopy ? 1 : 0;
        if (!makeSelfCopy) {
            writeWord(genome, nodeAddress + cellFunctionFixedBytes + 1, subGenomeSize);
        }
    }
}

__inline__ __device__ int GenomeDecoder::getNumNodes(uint8_t* genome, int genomeSize)
{
    int result = 0;
    int currentNodeAddress = Const::GenomeHeaderSize;
    for (; result < genomeSize && currentNodeAddress < genomeSize; ++result) {
        currentNodeAddress += Const::CellBasicBytes + getNextCellFunctionDataSize(genome, genomeSize, currentNodeAddress);
    }

    return result;
}

__inline__ __device__ int GenomeDecoder::getNodeAddress(uint8_t* genome, int genomeSize, int nodeIndex)
{
    int currentNodeAddress = Const::GenomeHeaderSize;
    for (int currentNodeIndex = 0; currentNodeIndex < nodeIndex; ++currentNodeIndex) {
        if (currentNodeAddress >= genomeSize) {
            break;
        }
        currentNodeAddress += Const::CellBasicBytes + getNextCellFunctionDataSize(genome, genomeSize, currentNodeAddress);
    }

    return currentNodeAddress;
}


__inline__ __device__ int GenomeDecoder::findStartNodeAddress(uint8_t* genome, int genomeSize, int refIndex)
{
    int currentNodeAddress = Const::GenomeHeaderSize;
    for (; currentNodeAddress <= refIndex;) {
        auto prevCurrentNodeAddress = currentNodeAddress;
        currentNodeAddress += Const::CellBasicBytes + getNextCellFunctionDataSize(genome, genomeSize, currentNodeAddress);
        if (currentNodeAddress > refIndex) {
            return prevCurrentNodeAddress;
        }
    }
    return Const::GenomeHeaderSize;
}

__inline__ __device__ int GenomeDecoder::getNextCellFunctionDataSize(uint8_t* genome, int genomeSize, int nodeAddress, bool withSubgenomes)
{
    auto cellFunction = getNextCellFunctionType(genome, nodeAddress);
    switch (cellFunction) {
    case CellFunction_Neuron:
        return Const::NeuronBytes;
    case CellFunction_Transmitter:
        return Const::TransmitterBytes;
    case CellFunction_Constructor: {
        if (withSubgenomes) {
            auto isMakeCopy = GenomeDecoder::convertByteToBool(genome[nodeAddress + Const::CellBasicBytes + Const::ConstructorFixedBytes]);
            if (isMakeCopy) {
                return Const::ConstructorFixedBytes + 1;
            } else {
                return Const::ConstructorFixedBytes + 3 + getNextSubGenomeSize(genome, genomeSize, nodeAddress);
            }
        } else {
            return Const::ConstructorFixedBytes;
        }
    }
    case CellFunction_Sensor:
        return Const::SensorBytes;
    case CellFunction_Nerve:
        return Const::NerveBytes;
    case CellFunction_Attacker:
        return Const::AttackerBytes;
    case CellFunction_Injector: {
        if (withSubgenomes) {
            auto isMakeCopy = GenomeDecoder::convertByteToBool(genome[nodeAddress + Const::CellBasicBytes + Const::InjectorFixedBytes]);
            if (isMakeCopy) {
                return Const::InjectorFixedBytes + 1;
            } else {
                return Const::InjectorFixedBytes + 3 + getNextSubGenomeSize(genome, genomeSize, nodeAddress);
            }
        } else {
            return Const::InjectorFixedBytes;
        }
    }
    case CellFunction_Muscle:
        return Const::MuscleBytes;
    case CellFunction_Defender:
        return Const::DefenderBytes;
    default:
        return 0;
    }
}

__inline__ __device__ int GenomeDecoder::getNextCellFunctionType(uint8_t* genome, int nodeAddress)
{
    return genome[nodeAddress] % CellFunction_Count;
}

__inline__ __device__ bool GenomeDecoder::isNextCellSelfCopy(uint8_t* genome, int nodeAddress)
{
    switch (getNextCellFunctionType(genome, nodeAddress)) {
    case CellFunction_Constructor:
        return GenomeDecoder::convertByteToBool(genome[nodeAddress + Const::CellBasicBytes + Const::ConstructorFixedBytes]);
    case CellFunction_Injector:
        return GenomeDecoder::convertByteToBool(genome[nodeAddress + Const::CellBasicBytes + Const::InjectorFixedBytes]);
    }
    return false;
}

__inline__ __device__ int GenomeDecoder::getNextCellColor(uint8_t* genome, int nodeAddress)
{
    return genome[nodeAddress + Const::CellColorPos] % MAX_COLORS;
}

__inline__ __device__ void GenomeDecoder::setNextCellFunctionType(uint8_t* genome, int nodeAddress, CellFunction cellFunction)
{
    genome[nodeAddress] = static_cast<uint8_t>(cellFunction);
}

__inline__ __device__ void GenomeDecoder::setNextCellColor(uint8_t* genome, int nodeAddress, int color)
{
    genome[nodeAddress + Const::CellColorPos] = color;
}

__inline__ __device__ void GenomeDecoder::setNextAngle(uint8_t* genome, int nodeAddress, uint8_t angle)
{
    genome[nodeAddress + Const::CellAnglePos] = angle;
}

__inline__ __device__ void GenomeDecoder::setNextRequiredConnections(uint8_t* genome, int nodeAddress, uint8_t angle)
{
    genome[nodeAddress + Const::CellRequiredConnectionsPos] = angle;
}

__inline__ __device__ void GenomeDecoder::setNextConstructionAngle1(uint8_t* genome, int nodeAddress, uint8_t angle)
{
    genome[nodeAddress + Const::CellBasicBytes + Const::ConstructorConstructionAngle1Pos] = angle;
}

__inline__ __device__ void GenomeDecoder::setNextConstructionAngle2(uint8_t* genome, int nodeAddress, uint8_t angle)
{
    genome[nodeAddress + Const::CellBasicBytes + Const::ConstructorConstructionAngle2Pos] = angle;
}

__inline__ __device__ void GenomeDecoder::setNextConstructorSeparation(uint8_t* genome, int nodeAddress, bool separation)
{
    genome[nodeAddress + Const::CellBasicBytes + Const::ConstructorFixedBytes + 3 + Const::ConstructorSeparation] = convertBoolToByte(separation);
}

__inline__ __device__ int GenomeDecoder::getNextSubGenomeSize(uint8_t* genome, int genomeSize, int nodeAddress)
{
    auto cellFunction = getNextCellFunctionType(genome, nodeAddress);
    auto cellFunctionFixedBytes = cellFunction == CellFunction_Constructor ? Const::ConstructorFixedBytes : Const::InjectorFixedBytes;
    auto subGenomeSizeIndex = nodeAddress + Const::CellBasicBytes + cellFunctionFixedBytes + 1;
    return max(min(GenomeDecoder::convertBytesToWord(genome[subGenomeSizeIndex], genome[subGenomeSizeIndex + 1]), genomeSize - (subGenomeSizeIndex + 2)), 0);
}

__inline__ __device__ int GenomeDecoder::getCellFunctionDataSize(CellFunction cellFunction, bool makeSelfCopy, int genomeSize)
{
    switch (cellFunction) {
    case CellFunction_Neuron:
        return Const::NeuronBytes;
    case CellFunction_Transmitter:
        return Const::TransmitterBytes;
    case CellFunction_Constructor: {
        return makeSelfCopy ? Const::ConstructorFixedBytes + 1 : Const::ConstructorFixedBytes + 3 + genomeSize;
    }
    case CellFunction_Sensor:
        return Const::SensorBytes;
    case CellFunction_Nerve:
        return Const::NerveBytes;
    case CellFunction_Attacker:
        return Const::AttackerBytes;
    case CellFunction_Injector: {
        return makeSelfCopy ? Const::InjectorFixedBytes + 1 : Const::InjectorFixedBytes + 3 + genomeSize;
    }
    case CellFunction_Muscle:
        return Const::MuscleBytes;
    case CellFunction_Defender:
        return Const::DefenderBytes;
    default:
        return 0;
    }
}

__inline__ __device__ bool GenomeDecoder::hasSelfCopy(uint8_t* genome, int genomeSize)
{
    int nodeAddress = 0;
    for (; nodeAddress < genomeSize;) {
        if (isNextCellSelfCopy(genome, nodeAddress)) {
            return true;
        }
        nodeAddress += Const::CellBasicBytes + getNextCellFunctionDataSize(genome, genomeSize, nodeAddress);
    }

    return false;
}

#pragma once

#include "cuda_runtime_api.h"
#include "sm_60_atomic_functions.h"

#include "Base.cuh"
#include "Map.cuh"
#include "ConstantMemory.cuh"
#include "ObjectFactory.cuh"
#include "SpotCalculator.cuh"

class ParticleProcessor
{
public:
    __inline__ __device__ static void updateMap(SimulationData& data);
    __inline__ __device__ static void movement(SimulationData& data);
    __inline__ __device__ static void collision(SimulationData& data);
    __inline__ __device__ static void transformation(SimulationData& data);
    __inline__ __device__ static void radiate(SimulationData& data, float2 pos, float2 vel, int color, float energy);  //return needed energy
};


/************************************************************************/
/* Implementation                                                       */
/************************************************************************/

__inline__ __device__ void ParticleProcessor::updateMap(SimulationData& data)
{
    auto partition = calcPartition(data.objects.particlePointers.getNumEntries(), blockIdx.x, gridDim.x);

    Particle** particlePointers = &data.objects.particlePointers.at(partition.startIndex);
    data.particleMap.set_block(partition.numElements(), particlePointers);
}

__inline__ __device__ void ParticleProcessor::movement(SimulationData& data)
{
    auto partition = calcPartition(
        data.objects.particlePointers.getNumEntries(), threadIdx.x + blockIdx.x * blockDim.x, blockDim.x * gridDim.x);

    for (int particleIndex = partition.startIndex; particleIndex <= partition.endIndex; ++particleIndex) {
        auto& particle = data.objects.particlePointers.at(particleIndex);
        particle->absPos = particle->absPos + particle->vel;
        data.particleMap.correctPosition(particle->absPos);
    }
}

__inline__ __device__ void ParticleProcessor::collision(SimulationData& data)
{
    auto partition = calcPartition(
        data.objects.particlePointers.getNumEntries(), threadIdx.x + blockIdx.x * blockDim.x, blockDim.x * gridDim.x);

    for (int particleIndex = partition.startIndex; particleIndex <= partition.endIndex; ++particleIndex) {
        auto& particle = data.objects.particlePointers.at(particleIndex);
        auto otherParticle = data.particleMap.get(particle->absPos);
        if (otherParticle && otherParticle != particle
            && Math::lengthSquared(particle->absPos - otherParticle->absPos) < 0.5) {

            SystemDoubleLock lock;
            lock.init(&particle->locked, &otherParticle->locked);
            if (lock.tryLock()) {

                if (particle->energy > NEAR_ZERO && otherParticle->energy > NEAR_ZERO) {
                    auto factor1 = particle->energy / (particle->energy + otherParticle->energy);
                    otherParticle->vel = particle->vel * factor1 + otherParticle->vel * (1.0f - factor1);
                    otherParticle->energy += particle->energy;
                    otherParticle->lastAbsorbedCell = nullptr;
                    particle->energy = 0;
                    particle = nullptr;
                }

                lock.releaseLock();
            }
        } else {
            if (auto cell = data.cellMap.getFirst(particle->absPos + particle->vel)) {
                if (cell->barrier) {
                    auto vr = particle->vel;
                    auto r = data.cellMap.getCorrectedDirection(particle->absPos - cell->pos);
                    auto dot_vr_r = Math::dot(vr, r);
                    if (dot_vr_r < 0) {
                        particle->vel = vr - r * 2 * dot_vr_r / Math::lengthSquared(r);
                    }
                } else {
                    if (particle->lastAbsorbedCell == cell) {
                        continue;
                    }
                    auto radiationAbsorption = SpotCalculator::calcParameter(
                        &SimulationParametersSpotValues::radiationAbsorption,
                        &SimulationParametersSpotActivatedValues::radiationAbsorption,
                        data,
                        cell->pos,
                        cell->color);

                    if (radiationAbsorption < NEAR_ZERO) {
                        continue;
                    }
                    if (!cell->tryLock()) {
                        continue;
                    }
                    if (particle->tryLock()) {

                        auto energyToTransfer = particle->energy * radiationAbsorption;
                        energyToTransfer *= max(0.0f, 1.0f - Math::length(cell->vel) * cudaSimulationParameters.radiationAbsorptionVelocityPenalty[cell->color]);


                        if (particle->energy < 1) {
                            energyToTransfer = particle->energy;
                        }
                        cell->energy += energyToTransfer;
                        particle->energy -= energyToTransfer;
                        bool killParticle = particle->energy < NEAR_ZERO;

                        particle->releaseLock();

                        if (killParticle) {
                            particle = nullptr;
                        } else {
                            particle->lastAbsorbedCell = cell;
                        }
                    }
                    cell->releaseLock();
                }
            }
        }
    }
}

__inline__ __device__ void ParticleProcessor::transformation(SimulationData& data)
{
    if (!cudaSimulationParameters.particleTransformationAllowed) {
        return;
    }
    auto const partition = calcAllThreadsPartition(data.objects.particlePointers.getNumOrigEntries());
    for (int particleIndex = partition.startIndex; particleIndex <= partition.endIndex; ++particleIndex) {
        if (auto& particle = data.objects.particlePointers.at(particleIndex)) {
            
            if (particle->energy >= cudaSimulationParameters.cellNormalEnergy[particle->color]) {
                ObjectFactory factory;
                factory.init(&data);
                auto cell = factory.createRandomCell(particle->energy, particle->absPos, particle->vel);
                cell->color = particle->color;

                particle = nullptr;
            }
        }
    }
}

__inline__ __device__ void ParticleProcessor::radiate(SimulationData& data, float2 pos, float2 vel, int color, float energy)
{
    if (cudaSimulationParameters.numParticleSources > 0) {
        auto sourceIndex = data.numberGen1.random(cudaSimulationParameters.numParticleSources - 1);
        pos.x = cudaSimulationParameters.particleSources[sourceIndex].posX;
        pos.y = cudaSimulationParameters.particleSources[sourceIndex].posY;

        auto const& source = cudaSimulationParameters.particleSources[sourceIndex];
        if (source.shapeType == RadiationSourceShapeType_Circular) {
            auto radius = max(1.0f, source.shapeData.circularRadiationSource.radius);
            float2 delta{0, 0};
            for (int i = 0; i < 10; ++i) {
                delta.x = data.numberGen1.random() * radius * 2 - radius;
                delta.y = data.numberGen1.random() * radius * 2 - radius;
                if (Math::length(delta) <= radius) {
                    break;
                }
            }
            pos += delta;
            if (source.useAngle) {
                vel = Math::unitVectorOfAngle(source.angle) * data.numberGen1.random(0.5f, 1.0f);
            } else {
                vel = Math::normalized(delta) * data.numberGen1.random(0.5f, 1.0f);
            }
        }
        if (source.shapeType == RadiationSourceShapeType_Rectangular) {
            auto const& rectangle = source.shapeData.rectangularRadiationSource;
            float2 delta;
            delta.x = data.numberGen1.random() * rectangle.width - rectangle.width / 2;
            delta.y = data.numberGen1.random() * rectangle.height - rectangle.height / 2;
            pos += delta;
            if (source.useAngle) {
                vel = Math::unitVectorOfAngle(source.angle) * data.numberGen1.random(0.5f, 1.0f);
            } else {
                auto roundSize = min(rectangle.width, rectangle.height) / 2;
                float2 corner1{-rectangle.width / 2, -rectangle.height / 2};
                float2 corner2{rectangle.width / 2, -rectangle.height / 2};
                float2 corner3{-rectangle.width / 2, rectangle.height / 2};
                float2 corner4{rectangle.width / 2, rectangle.height / 2};
                if (Math::lengthMax(corner1 - delta) <= roundSize) {
                    vel = Math::normalized(delta - (corner1 + float2{roundSize, roundSize}));
                } else if (Math::lengthMax(corner2 - delta) <= roundSize) {
                    vel = Math::normalized(delta - (corner2 + float2{-roundSize, roundSize}));
                } else if (Math::lengthMax(corner3 - delta) <= roundSize) {
                    vel = Math::normalized(delta - (corner3 + float2{roundSize, -roundSize}));
                } else if (Math::lengthMax(corner4 - delta) <= roundSize) {
                    vel = Math::normalized(delta - (corner4 + float2{-roundSize, -roundSize}));
                } else {
                    vel.x = 0;
                    vel.y = 0;
                    auto dx1 = rectangle.width / 2 + delta.x;
                    auto dx2 = rectangle.width / 2 - delta.x;
                    auto dy1 = rectangle.height / 2 + delta.y;
                    auto dy2 = rectangle.height / 2 - delta.y;
                    if (dx1 <= dy1 && dx1 <= dy2 && delta.x <= 0) {
                        vel.x = -1;
                    }
                    if (dy1 <= dx1 && dy1 <= dx2 && delta.y <= 0) {
                        vel.y = -1;
                    }
                    if (dx2 <= dy1 && dx2 <= dy2 && delta.x > 0) {
                        vel.x = 1;
                    }
                    if (dy2 <= dx1 && dy2 <= dx2 && delta.y > 0) {
                        vel.y = 1;
                    }
                }
                vel = vel * data.numberGen1.random(0.5f, 1.0f);
            }
        }
    }

    data.cellMap.correctPosition(pos);

    auto residualEnergyPerCell = *data.residualEnergy / max(static_cast<uint64_t>(1), data.objects.cells.getNumOrigEntries());
    auto cellFunctionConstructorEnergyFromRadiationFactor = residualEnergyPerCell < 0.2 ? cudaSimulationParameters.cellFunctionConstructorPumpEnergyFactor[color] / 3 : 0;
    auto particleEnergy = energy * (1.0f - cellFunctionConstructorEnergyFromRadiationFactor);
    if (particleEnergy > NEAR_ZERO) {
        ObjectFactory factory;
        factory.init(&data);
        data.cellMap.correctPosition(pos);
        factory.createParticle(particleEnergy, pos, vel, color);
    }

    atomicAdd(data.residualEnergy, energy * cellFunctionConstructorEnergyFromRadiationFactor);
}

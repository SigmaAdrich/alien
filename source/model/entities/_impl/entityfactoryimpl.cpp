#include "entityfactoryimpl.h"

#include "cellimpl.h"
#include "cellclusterimpl.h"
#include "model/entities/token.h"

#include "global/servicelocator.h"

namespace
{
    EntityFactoryImpl entityFactoryImpl;
}

EntityFactoryImpl::EntityFactoryImpl ()
{
    ServiceLocator::getInstance().registerService<EntityFactory>(this);
}

CellCluster* EntityFactoryImpl::buildCellCluster (SimulationContext* context) const
{
    return new CellClusterImpl(context);
}

CellCluster* EntityFactoryImpl::buildCellCluster (QList< Cell* > cells, qreal angle
    , QVector3D pos, qreal angularVel, QVector3D vel, SimulationContext* context) const
{
    return new CellClusterImpl(cells, angle, pos, angularVel, vel, context);
}

CellCluster* EntityFactoryImpl::buildCellClusterFromForeignCells (QList< Cell* > cells
    , qreal angle, SimulationContext* context) const
{
    return new CellClusterImpl(cells, angle, context);
}

Cell* EntityFactoryImpl::buildCell (SimulationContext* context) const
{
    return new CellImpl(context);
}

Cell* EntityFactoryImpl::buildCell (qreal energy, SimulationContext* context, int maxConnections
    , int tokenAccessNumber, QVector3D relPos) const
{
    return new CellImpl(energy, context, maxConnections, tokenAccessNumber, relPos);
}

Cell* EntityFactoryImpl::buildCellWithRandomData (qreal energy, SimulationContext* context) const
{
    return new CellImpl(energy, context, true);
}

Token* EntityFactoryImpl::buildToken () const
{
    return new Token();
}

EnergyParticle* EntityFactoryImpl::buildEnergyParticle(SimulationContext* context) const
{
    return new EnergyParticle(context);
}

EnergyParticle *EntityFactoryImpl::buildEnergyParticle(qreal energy, QVector3D pos, QVector3D vel
    , SimulationContext *context) const
{
    return new EnergyParticle(energy, pos, vel, context);
}

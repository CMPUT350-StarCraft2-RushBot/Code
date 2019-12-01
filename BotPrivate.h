size_t CountUnitType(UNIT_TYPEID unit_type) {
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}


bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Point2D location, bool isExpansion = false) {

    //std::cout << "TryBuildStructure started!\n";
    const ObservationInterface* observation = Observation();
    Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));

    //if we have no workers Don't build
    if (workers.empty()) {
        return false;
    }

    // Check to see if there is already a worker heading out to build it
    for (const auto& worker : workers) {
        for (const auto& order : worker->orders) {
            if (order.ability_id == ability_type_for_structure) {                return false;
            }
        }
    }

    // If no worker is already building one, get a random worker to build one
    const Unit* unit = GetRandomEntry(workers);
    // Check to see if unit can make it there
    if (Query()->PathingDistance(unit, location) < 0.1f) {
        return false;
    }
    if (!isExpansion) {
        for (const auto& expansion : expansions_) {
            if (Distance2D(location, Point2D(expansion.x, expansion.y)) < 7) {
                return false;
            }
        }
    }
    // Check to see if unit can build there
    if (Query()->Placement(ability_type_for_structure, location)) {
        Actions()->UnitCommand(unit, ability_type_for_structure, location);
        return true;
    }
    return false;

}


bool TryBuildSupplyDepot()  {
    const ObservationInterface* observation = Observation();

    // If we are not supply capped, don't build a supply depot.
    if (observation->GetFoodUsed() < observation->GetFoodCap() - 6) {
        return false;
    }

    if (observation->GetMinerals() < 100) {
        return false;
    }

    //check to see if there is already on building
    Units units = observation->GetUnits(Unit::Alliance::Self, IsUnits(supply_depot_types));
    if (observation->GetFoodUsed() < 40) {
        for (const auto& unit : units) {
            if (unit->build_progress != 1) {
                return false;
            }
        }
    }

    // Try and build a supply depot. Find a random SCV and give it the order.
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    Point2D build_location = Point2D(staging_location_.x + rx * 15, staging_location_.y + ry * 15);
    return TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, UNIT_TYPEID::TERRAN_SCV, build_location);
}


bool TryBuildRefinery() {
    const ObservationInterface* observation = Observation();
    Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
    if (CountUnitType(UNIT_TYPEID::TERRAN_REFINERY) >= observation->GetUnits(Unit::Alliance::Self, IsTownHall()).size() * 2) {
        return false;
    }

    for (const auto& base : bases) {
        if (base->assigned_harvesters >= base->ideal_harvesters) {
            if (base->build_progress == 1) {
                if (TryBuildGas(ABILITY_ID::BUILD_REFINERY, UNIT_TYPEID::TERRAN_SCV, base->pos)) {
                    isBuilt = 1;
                    
                    return true;
                }
            }
        }
    }
    return false;
}


const Unit* FindNearestMineralPatch(const Point2D& start) {
    Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
    float distance = std::numeric_limits<float>::max();
    const Unit* target = nullptr;
    for (const auto& u : units) {
        if (u->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD) {
            float d = DistanceSquared2D(u->pos, start);
            if (d < distance) {
                distance = d;
                target = u;
            }
        }
    }
    return target;
}


bool TryBuildGas(AbilityID build_ability, UnitTypeID worker_type, Point2D base_location) {
    const ObservationInterface* observation = Observation();
    Units geysers = observation->GetUnits(Unit::Alliance::Neutral, IsVespeneGeyser());

    //only search within this radius
    float minimum_distance = 15.0f;
    Tag closestGeyser = 0;
    for (const auto& geyser : geysers) {
        float current_distance = Distance2D(base_location, geyser->pos);
        if (current_distance < minimum_distance) {
            if (Query()->Placement(build_ability, geyser->pos)) {
                minimum_distance = current_distance;
                closestGeyser = geyser->tag;
            }
        }
    }

    // In the case where there are no more available geysers nearby
    if (closestGeyser == 0) {
        return false;
    }
    return TryBuildStructure(build_ability, worker_type, closestGeyser);

}


bool TryBuildBarracks() {
    const ObservationInterface* observation = Observation();

    // Wait until we have our quota of TERRAN_SCV's.
    if (CountUnitType(UNIT_TYPEID::TERRAN_SCV) < TargetSCVCount / 2)
        return false;

    // One build 1 barracks.
    if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) > 3)
        return false;

    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    
    return TryBuildStructure(ABILITY_ID::BUILD_BARRACKS, UNIT_TYPEID::TERRAN_SCV, Point2D(staging_location_.x+ rx * 5, staging_location_.y + ry * 5));
    
}


bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Tag location_tag) {
    
    const ObservationInterface* observation = Observation();
    Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
    const Unit* target = observation->GetUnit(location_tag);

    if (workers.empty()) {
        return false;
    }

    // Check to see if there is already a worker heading out to build it
    for (const auto& worker : workers) {
        for (const auto& order : worker->orders) {
            if (order.ability_id == ability_type_for_structure) {
                return false;
            }
        }
    }

    // If no worker is already building one, get a random worker to build one
    const Unit* unit = GetRandomEntry(workers);

    // Check to see if unit can build there
    if (Query()->Placement(ability_type_for_structure, target->pos)) {
        Actions()->UnitCommand(unit, ability_type_for_structure, target);
        return true;
    }
    return false;
}


bool TryBuildStructureRandom(AbilityID ability_type_for_structure, UnitTypeID unit_type) {
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    Point2D build_location = Point2D(staging_location_.x + rx * 15, staging_location_.y + ry * 15);
    
    
    if (staging_location_.x > 80) {
        build_location = Point2D(staging_location_.x - 6 + rx * 5, staging_location_.y + 6 + ry * 5);
    }
    else {
        build_location = Point2D(staging_location_.x + 6 + rx * 5, staging_location_.y - 6 + ry * 5);
    }
    
    Units units = Observation()->GetUnits(Unit::Self, IsStructure(Observation()));
    float distance = std::numeric_limits<float>::max();
    for (const auto& u : units) {
        if (u->unit_type == UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED) {
            continue;
        }
        float d = Distance2D(u->pos, build_location);
        if (d < distance) {
            distance = d;
        }
    }
    if (distance < 6) {
        return false;
    }
    return TryBuildStructure(ability_type_for_structure, unit_type, build_location);
}


void ManageWorkers(UNIT_TYPEID worker_type, AbilityID worker_gather_command, UNIT_TYPEID vespene_building_type) {
    const ObservationInterface* observation = Observation();
    Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
    Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(vespene_building_type));

    if (bases.empty()) {
        return;
    }

    for (const auto& base : bases) {
        //If we have already mined out or still building here skip the base.
        if (base->ideal_harvesters == 0 || base->build_progress != 1) {
            continue;
        }
        //if base is
        if (base->assigned_harvesters > base->ideal_harvesters) {
            Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(worker_type));

            for (const auto& worker : workers) {
                if (!worker->orders.empty()) {
                    if (worker->orders.front().target_unit_tag == base->tag) {
                        //This should allow them to be picked up by mineidleworkers()
                        MineIdleWorkers(worker, worker_gather_command,vespene_building_type);
                        return;
                    }
                }
            }
        }
    }
    Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(worker_type));
    for (const auto& geyser : geysers) {
        if (geyser->ideal_harvesters == 0 || geyser->build_progress != 1) {
            continue;
        }
        if (geyser->assigned_harvesters > geyser->ideal_harvesters) {
            for (const auto& worker : workers) {
                if (!worker->orders.empty()) {
                    if (worker->orders.front().target_unit_tag == geyser->tag) {
                        //This should allow them to be picked up by mineidleworkers()
                        MineIdleWorkers(worker, worker_gather_command, vespene_building_type);
                        return;
                    }
                }
            }
        }
        else if (geyser->assigned_harvesters < geyser->ideal_harvesters) {
            for (const auto& worker : workers) {
                if (!worker->orders.empty()) {
                    //This should move a worker that isn't mining gas to gas
                    const Unit* target = observation->GetUnit(worker->orders.front().target_unit_tag);
                    if (target == nullptr) {
                        continue;
                    }
                    if (target->unit_type != vespene_building_type) {
                        //This should allow them to be picked up by mineidleworkers()
                        MineIdleWorkers(worker, worker_gather_command, vespene_building_type);
                        return;
                    }
                }
            }
        }
    }
}


void MineIdleWorkers(const Unit* worker, AbilityID worker_gather_command, UnitTypeID vespene_building_type) {
    const ObservationInterface* observation = Observation();
    Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
    Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(vespene_building_type));

    const Unit* valid_mineral_patch = nullptr;

    if (bases.empty()) {
        return;
    }

    for (const auto& geyser : geysers) {
        if (geyser->assigned_harvesters < geyser->ideal_harvesters) {
            Actions()->UnitCommand(worker, worker_gather_command, geyser);
            return;
        }
    }
    //Search for a base that is missing workers.
    for (const auto& base : bases) {
        //If we have already mined out here skip the base.
        if (base->ideal_harvesters == 0 || base->build_progress != 1) {
            continue;
        }
        if (base->assigned_harvesters < base->ideal_harvesters) {
            valid_mineral_patch = FindNearestMineralPatch(base->pos);
            Actions()->UnitCommand(worker, worker_gather_command, valid_mineral_patch);
            return;
        }
    }

    if (!worker->orders.empty()) {
        return;
    }

    //If all workers are spots are filled just go to any base.
    const Unit* random_base = GetRandomEntry(bases);
    valid_mineral_patch = FindNearestMineralPatch(random_base->pos);
    Actions()->UnitCommand(worker, worker_gather_command, valid_mineral_patch);
}


void AttackWithUnitType(UnitTypeID unit_type, const ObservationInterface* observation) {
    Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
    for (const auto& unit : units) {
        AttackWithUnit(unit, observation);
    }
}


void AttackWithUnit(const Unit* unit, const ObservationInterface* observation) {
    //If unit isn't doing anything make it attack.
    Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
    if (enemy_units.empty()) {
        const GameInfo& game_info = Observation()->GetGameInfo();
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK_ATTACK, game_info.enemy_start_locations.front());
        return;
    }

    if (unit->orders.empty()) {
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
        return;
    }

    //If the unit is doing something besides attacking, make it attack.
    if (unit->orders.front().ability_id != ABILITY_ID::ATTACK) {
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
    }
}


bool TryBuildFactory()
{
    if (CountUnitType(UNIT_TYPEID::TERRAN_FACTORY) > 1)
    {
        return false;
    }
    const ObservationInterface* observation = Observation();
    if (Observation()->GetMinerals() > 150 && Observation()->GetVespene() > 100) {
        return TryBuildStructureRandom(ABILITY_ID::BUILD_FACTORY, UNIT_TYPEID::TERRAN_SCV);
    }
    return false;
}


bool TryBuildFactoryLab()
{
    const ObservationInterface* observation = Observation();
    Units factorys = observation->GetUnits(Unit::Self, IsUnits(factory_types));
    Units factorys_tech = observation->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_FACTORYTECHLAB));
    for (const auto& factory : factorys) {
        if (!factory->orders.empty()) {
            continue;
        }
        if (observation->GetUnit(factory->add_on_tag) == nullptr) {
            return TryBuildAddOn(ABILITY_ID::BUILD_TECHLAB_FACTORY, factory->tag);
        }
    }
    return false;
}


bool TryBuildAddOn(AbilityID ability_type_for_structure, Tag base_structure) {
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    const Unit* unit = Observation()->GetUnit(base_structure);

    if (unit->build_progress != 1) {
        return false;
    }

    Point2D build_location = Point2D(unit->pos.x + rx * 15, unit->pos.y + ry * 15);
 
    Units units = Observation()->GetUnits(Unit::Self, IsStructure(Observation()));

    if (Query()->Placement(ability_type_for_structure, unit->pos, unit)) {
        Actions()->UnitCommand(unit, ability_type_for_structure);
        return true;
    }

    float distance = std::numeric_limits<float>::max();
    for (const auto& u : units) {
        float d = Distance2D(u->pos, build_location);
        if (d < distance) {
            distance = d;
        }
    }
    if (distance < 6) {
        return false;
    }

    if(Query()->Placement(ability_type_for_structure, build_location, unit)){
        Actions()->UnitCommand(unit, ability_type_for_structure, build_location);
        return true;
    }
    return false;
    
}


bool FindEnemyPosition(Point2D& target_pos) {
    if (game_info_.enemy_start_locations.empty()) return false;
    target_pos = game_info_.enemy_start_locations.front();
    return true;
}


bool FindEnemyStructure(const ObservationInterface* observation, const Unit*& enemy_unit) {
    Units my_units = observation->GetUnits(Unit::Alliance::Enemy);
    for (const auto unit : my_units) {
        if (unit->unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER ||
            unit->unit_type == UNIT_TYPEID::TERRAN_SUPPLYDEPOT ||
            unit->unit_type == UNIT_TYPEID::TERRAN_BARRACKS) {
            enemy_unit = unit;
            return true;
        }
    }

    return false;
}


void ScoutWithMarines() {
    Units units = Observation()->GetUnits(Unit::Alliance::Self);
    for (const auto& unit : units) {
        UnitTypeID unit_type(unit->unit_type);
        if (unit_type != UNIT_TYPEID::TERRAN_MARINE)
            continue;

        if (!unit->orders.empty())
            continue;

        // Priority to attacking enemy structures.
        const Unit* enemy_unit = nullptr;
        if (FindEnemyStructure(Observation(), enemy_unit)) {
            Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_unit);
            return;
        }

        Point2D target_pos;
        // TODO: For efficiency, these queries should be batched.
        if (FindEnemyPosition(target_pos)) {
            Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
        }
    }
}

void ManageArmy() {
    const ObservationInterface* observation = Observation();
    Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
    Units army = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));
    size_t marine_count = CountUnitType(UNIT_TYPEID::TERRAN_MARINE);
    size_t tank_count = CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) + CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANKSIEGED);
    size_t medivac_count = CountUnitType(UNIT_TYPEID::TERRAN_MEDIVAC);
    size_t marauder_count = CountUnitType(UNIT_TYPEID::TERRAN_MARAUDER);
    
    if ( tank_count > 2 || marine_count > 20) {
    //if ( marauder_count > 6) {
        for (const auto& unit : army) {
            switch (unit->unit_type.ToType()) {
                case UNIT_TYPEID::TERRAN_SIEGETANK: {
                    if (!enemy_units.empty()) {
                        float distance = std::numeric_limits<float>::max();
                        for (const auto& u : enemy_units) {
                            float d = Distance2D(u->pos, unit->pos);
                            if (d < distance) {
                                distance = d;
                            }
                        }
                        if (distance < 11) {
                            Actions()->UnitCommand(unit, ABILITY_ID::MORPH_SIEGEMODE);
                        }
                        else {
                            AttackWithUnit(unit, observation);
                        }
                    }
                    else {
                        AttackWithUnit(unit, observation);
                    }
                    break;
                }
                case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED: {
                    if (!enemy_units.empty()) {
                        float distance = std::numeric_limits<float>::max();
                        for (const auto& u : enemy_units) {
                            float d = Distance2D(u->pos, unit->pos);
                            if (d < distance) {
                                distance = d;
                            }
                        }
                        if (distance > 13) {
                            Actions()->UnitCommand(unit, ABILITY_ID::MORPH_UNSIEGE);
                        }
                        else {
                            AttackWithUnit(unit, observation);
                        }
                        break;
                    }
                    else {
                        Actions()->UnitCommand(unit, ABILITY_ID::MORPH_UNSIEGE);
                        AttackWithUnit(unit, observation);
                    }
                }
                default: {
                    break;
                }
            }
        }
        for (const auto& unit : army) {
            switch (unit->unit_type.ToType()) {
                case UNIT_TYPEID::TERRAN_MARINE: {
                    AttackWithUnit(unit, observation);
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
    else {
        if (CountUnitType(UNIT_TYPEID::TERRAN_FACTORYTECHLAB) == 0) {
            ScoutWithMarines();
        }
        else if (CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) <= 2) {
            Point2D tankLocation = Point2D(staging_location_.x - 37, staging_location_.y + 27);
            Point2D marineLocation = Point2D(staging_location_.x - 37, staging_location_.y + 27);
            if (staging_location_.x > 80) {
                tankLocation = Point2D(staging_location_.x - 40, staging_location_.y + 30);
                marineLocation = Point2D(staging_location_.x - 37, staging_location_.y + 27);
            }
            else {
                tankLocation = Point2D(staging_location_.x + 40, staging_location_.y - 30);
                marineLocation = Point2D(staging_location_.x + 37, staging_location_.y - 27);
            }
            for (const auto& unit : army) {
                switch (unit->unit_type.ToType()) {
                    case UNIT_TYPEID::TERRAN_SIEGETANK: {
                        float dist = Distance2D(unit->pos, tankLocation);
                        if (dist < 2) {
                            if (!unit->orders.empty()) {
                                Actions()->UnitCommand(unit, ABILITY_ID::STOP);
                            }
                        }
                        else {
                            Actions()->UnitCommand(unit, ABILITY_ID::MOVE, tankLocation);
                        }
                        break;
                    }
                    case UNIT_TYPEID::TERRAN_MARINE: {
                        float dist = Distance2D(unit->pos, marineLocation);
                        if (dist < 2) {
                            if (!unit->orders.empty()) {
                                Actions()->UnitCommand(unit, ABILITY_ID::STOP);
                            }
                        }
                        else {
                            Actions()->UnitCommand(unit, ABILITY_ID::MOVE, marineLocation);
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
        }
    }
}


std::vector<UNIT_TYPEID> barrack_types = { UNIT_TYPEID::TERRAN_BARRACKS };
std::vector<UNIT_TYPEID> factory_types = { UNIT_TYPEID::TERRAN_FACTORY };
std::vector<UNIT_TYPEID> supply_depot_types = { UNIT_TYPEID::TERRAN_SUPPLYDEPOT, UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED };
std::vector<UNIT_TYPEID> siege_tank_types = { UNIT_TYPEID::TERRAN_SIEGETANK, UNIT_TYPEID::TERRAN_SIEGETANKSIEGED };
std::string last_action_text_;

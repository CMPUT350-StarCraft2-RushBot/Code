size_t CountUnitType(UNIT_TYPEID unit_type) {
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}


bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Point2D location, bool isExpansion = false) {

    const ObservationInterface* observation = Observation();
    Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));

    //if we have no workers Don't build
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

    Point2D defaultLocation;
    size_t supplyCount = CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT);

    //Find the default location we can start building on
    if (staging_location_.x > 80) {
        if (staging_location_.y < 80) {
            defaultLocation = Point2D(staging_location_.x - 4, staging_location_.y - 1);
        }
        else {
            defaultLocation = Point2D(staging_location_.x - 4, staging_location_.y);
        }
    }
    else {
        if (staging_location_.y > 80) {
            defaultLocation = Point2D(staging_location_.x + 3, staging_location_.y);
        }
        else {
            defaultLocation = Point2D(staging_location_.x + 3, staging_location_.y - 1);
        }
    }

    Point2D build_location;

    //Build next to what we already had built (If any)
    if (defaultLocation.x < staging_location_.x)
        build_location.x = defaultLocation.x - 2 * supplyCount;
    else
        build_location.x = defaultLocation.x + 2 * supplyCount;

    if (defaultLocation.y < staging_location_.y)
        build_location.y = defaultLocation.y - 2;
    else
        build_location.y = defaultLocation.y + 2;

    if (supplyCount > 5) {
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        build_location = Point2D(staging_location_.x + rx * 15, staging_location_.y + ry * 15);
    }
    return TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, UNIT_TYPEID::TERRAN_SCV, build_location);
}


bool TryBuildRefinery() {
    const ObservationInterface* observation = Observation();
    Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
    // Check for the refinery count
    if (CountUnitType(UNIT_TYPEID::TERRAN_REFINERY) >= observation->GetUnits(Unit::Alliance::Self, IsTownHall()).size() * 2) {
        return false;
    }
    // Find one free SCV
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
    // This function will find the nearest mineral
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
    // This function will find the build gas
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
    
    Point2D defaultLocation;
    int barrackCount = CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS);

    //Find a location that we can start building
    if (staging_location_.x > 80) {
        if (staging_location_.y < 80) {
            defaultLocation = Point2D(staging_location_.x - 3, staging_location_.y + 2);
        }
        else {
            defaultLocation = Point2D(staging_location_.x - 3, staging_location_.y - 3);
        }
    }
    else {
        if (staging_location_.y > 80) {
            defaultLocation = Point2D(staging_location_.x + 2, staging_location_.y - 3);
        }
        else {
            defaultLocation = Point2D(staging_location_.x + 2, staging_location_.y + 3);
        }
    }

    //Build next to what we already had built (If any)
    Point2D build_location;
    if (defaultLocation.x < staging_location_.x)
        build_location.x = defaultLocation.x - 3 * barrackCount;
    else
        build_location.x = defaultLocation.x + 3 * barrackCount;

    if (defaultLocation.y < staging_location_.y)
        build_location.y = defaultLocation.y - 2 ;
    else
        build_location.y = defaultLocation.y + 2 ;

    return TryBuildStructure(ABILITY_ID::BUILD_BARRACKS, UNIT_TYPEID::TERRAN_SCV, build_location);
    
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
    // This function will build structure at a random location give the specific unit
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    Point2D build_location = Point2D(staging_location_.x + rx * 15, staging_location_.y + ry * 15);
    
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
    // This function will manage the worker
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
    // This function can assist managing our SCV
    const ObservationInterface* observation = Observation();
    Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
    Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(vespene_building_type));

    const Unit* valid_mineral_patch = nullptr;

    if (bases.empty()) {
        return;
    }

    // Check for ideal SCV
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


void AttackWithUnit(const Unit* unit, const ObservationInterface* observation) {
    //If unit isn't doing anything make it attack.
    Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
    if (enemy_units.empty()) {
        // Attack based on the opponent's location
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK_ATTACK, game_info_.enemy_start_locations.front());
        return;
    }

    if (unit->orders.empty()) {
        // If the unit has no order now, make it attack
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
        return;
    }

    //If the unit is doing something besides attacking, make it attack.
    if (unit->orders.front().ability_id != ABILITY_ID::ATTACK) {
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
    }
}


bool TryBuildFactory() {
    // Check for exsisting factory
    if (CountUnitType(UNIT_TYPEID::TERRAN_FACTORY) > 1) {
        return false;
    }
    // Build the factory based on base's location
    const ObservationInterface* observation = Observation();
    if (Observation()->GetMinerals() > 150 && Observation()->GetVespene() > 100) {

        Point2D defaultLocation;
        int factoryCount = CountUnitType(UNIT_TYPEID::TERRAN_FACTORY);

        //Find default location we can start building
        if (staging_location_.x > 80) {
            if (staging_location_.y < 80) {
                defaultLocation = Point2D(staging_location_.x - 3, staging_location_.y + 8);
            }
            else {
                defaultLocation = Point2D(staging_location_.x - 3, staging_location_.y - 8);
            }
        }
        else {
            if (staging_location_.y > 80) {
                defaultLocation = Point2D(staging_location_.x + 2, staging_location_.y - 8);
            }
            else {
                defaultLocation = Point2D(staging_location_.x + 2, staging_location_.y + 8);
            }
        }

        Point2D build_location;
        //Build on top of what we already had built (If any)
        if (defaultLocation.x < staging_location_.x)
            build_location.x = defaultLocation.x - 3 ;
        else
            build_location.x = defaultLocation.x + 3 ;

        if (defaultLocation.y < staging_location_.y)
            build_location.y = defaultLocation.y - 3 * factoryCount;
        else
            build_location.y = defaultLocation.y + 3 * factoryCount;

        return TryBuildStructure(ABILITY_ID::BUILD_FACTORY, UNIT_TYPEID::TERRAN_SCV, build_location);


    }
    return false;
}


bool TryBuildFactoryLab() {
    // Get all factories first
    const ObservationInterface* observation = Observation();
    Units factorys = observation->GetUnits(Unit::Self, IsUnits(factory_types));
    Units factorys_tech = observation->GetUnits(Unit::Self, IsUnit(UNIT_TYPEID::TERRAN_FACTORYTECHLAB));
    for (const auto& factory : factorys) {
        if (!factory->orders.empty()) {
            continue;
        }
        // Check for factories' add on tag
        if (observation->GetUnit(factory->add_on_tag) == nullptr) {
            return TryBuildAddOn(ABILITY_ID::BUILD_TECHLAB_FACTORY, factory->tag);
        }
    }
    return false;
}


bool TryBuildAddOn(AbilityID ability_type_for_structure, Tag base_structure) {
    // This function will help to build on add on
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

void ManageArmy() {
    // This function will manage our army
    const ObservationInterface* observation = Observation();
    Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
    Units army = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));
    size_t marine_count = CountUnitType(UNIT_TYPEID::TERRAN_MARINE);
    size_t tank_count = CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) + CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANKSIEGED);
    
    // Calculate the gathering location for marine and tank
    Point2D tankLocation = Point2D(staging_location_.x - 23, staging_location_.y + 13);
    Point2D marineLocation = Point2D(staging_location_.x - 23, staging_location_.y + 13);
    if (staging_location_.x > 80) {
        if (staging_location_.y < 80) {
            tankLocation = Point2D(staging_location_.x - 38, staging_location_.y + 10);
            marineLocation = Point2D(staging_location_.x - 38, staging_location_.y + 10);
        }
        else {
            tankLocation = Point2D(staging_location_.x - 10, staging_location_.y - 38);
            marineLocation = Point2D(staging_location_.x - 10, staging_location_.y - 38);
        }
    }
    else {
        if (staging_location_.y > 80) {
            tankLocation = Point2D(staging_location_.x + 38, staging_location_.y - 10);
            marineLocation = Point2D(staging_location_.x + 38, staging_location_.y - 10);
        }
        else {
            tankLocation = Point2D(staging_location_.x + 10, staging_location_.y + 38);
            marineLocation = Point2D(staging_location_.x + 10, staging_location_.y + 38);
        }
    }

    // If the amount is reached, then attack
    if ( tank_count > 3 || marine_count > 20) {
        for (const auto& unit : army) {
            switch (unit->unit_type.ToType()) {
                case UNIT_TYPEID::TERRAN_SIEGETANK: {
                    // If enemy found, attack enemy
                    if (!enemy_units.empty()) {
                        float distance = std::numeric_limits<float>::max();
                        for (const auto& u : enemy_units) {
                            float d = Distance2D(u->pos, unit->pos);
                            if (d < distance) {
                                if (!u->is_flying)
                                    distance = d;
                            }
                        }
                        if (distance < 11 && unit->pos.z <= 10 || distance < 11 && unit->pos.z >= 11.95) {
                            Actions()->UnitCommand(unit, ABILITY_ID::MORPH_SIEGEMODE);
                        }
                        else {
                            AttackWithUnit(unit, observation);
                        }
                    }
                    // Attack the enemy's base
                    else {
                        AttackWithUnit(unit, observation);
                    }
                    break;
                }
                case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED: {
                    // If enemy found, attack enemy
                    if (!enemy_units.empty()) {
                        float distance = std::numeric_limits<float>::max();
                        for (const auto& u : enemy_units) {
                            float d = Distance2D(u->pos, unit->pos);
                            if (d < distance) {
                                if (!u->is_flying)
                                    distance = d;
                            }
                        }
                        if (unit->pos.z > 10 && unit->pos.z < 11.95 )
                        {
                            Actions()->UnitCommand(unit, ABILITY_ID::MORPH_UNSIEGE);
                        }
                        else if (distance > 13) {
                            Actions()->UnitCommand(unit, ABILITY_ID::MORPH_UNSIEGE);
                        }
                        else {
                            AttackWithUnit(unit, observation);
                        }
                        break;
                    }
                    else {
                        // Attack the enemy's base
                        Actions()->UnitCommand(unit, ABILITY_ID::MORPH_UNSIEGE);
                        AttackWithUnit(unit, observation);
                    }
                }
                case UNIT_TYPEID::TERRAN_MARINE: {
                    // Attack no matter what
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
        // Scouting with marine
        if (CountUnitType(UNIT_TYPEID::TERRAN_FACTORYTECHLAB) <= 1) {
            if (loccount < 3) {
                // Send out 3 marines for potential locations
                for (const auto& unit : army) {
                    UnitTypeID unit_type(unit->unit_type);
                    if (unit_type != UNIT_TYPEID::TERRAN_MARINE)
                        continue;
                    if (!unit->orders.empty())
                        continue;
                    Actions()->UnitCommand(unit, ABILITY_ID::ATTACK_ATTACK, game_info_.enemy_start_locations[loccount]);
                    loccount++;
                }
            }
            // If the marine come back, remove corresponding location
            for (const auto& unit : army) {
                switch (unit->unit_type.ToType()) {
                    case UNIT_TYPEID::TERRAN_MARINE: {
                        float d0 = Distance2D(unit->pos, Observation()->GetGameInfo().enemy_start_locations[0]);
                        float d1 = Distance2D(unit->pos, Observation()->GetGameInfo().enemy_start_locations[1]);
                        float d2 = Distance2D(unit->pos, Observation()->GetGameInfo().enemy_start_locations[2]);
                        if (d0 < 5) {
                            Actions()->UnitCommand(unit, ABILITY_ID::MOVE, marineLocation);
                            game_info_.enemy_start_locations.erase(std::remove(game_info_.enemy_start_locations.begin(), game_info_.enemy_start_locations.end(), Observation()->GetGameInfo().enemy_start_locations[0]), game_info_.enemy_start_locations.end());
                        }
                        if (d1 < 5) {
                            Actions()->UnitCommand(unit, ABILITY_ID::MOVE, marineLocation);
                            game_info_.enemy_start_locations.erase(std::remove(game_info_.enemy_start_locations.begin(), game_info_.enemy_start_locations.end(), Observation()->GetGameInfo().enemy_start_locations[1]), game_info_.enemy_start_locations.end());
                        }
                        if (d2 < 5) {
                            Actions()->UnitCommand(unit, ABILITY_ID::MOVE, marineLocation);
                            game_info_.enemy_start_locations.erase(std::remove(game_info_.enemy_start_locations.begin(), game_info_.enemy_start_locations.end(), Observation()->GetGameInfo().enemy_start_locations[2]), game_info_.enemy_start_locations.end());
                        }
                        break;
                    }
                    // If the tank is built, send to gathering location
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
                    default: {
                        break;
                    }
                }
            }
        }
        else if (CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) <= 3) {
            // Gather the army is the number is not reached
            for (const auto& unit : army) {
                switch (unit->unit_type.ToType()) {
                    case UNIT_TYPEID::TERRAN_SIEGETANK: {
                        float dist = Distance2D(unit->pos, tankLocation);
                        if (dist < 3) {
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
                        if (dist < 3) {
                            if (!unit->orders.empty()) {
                                Actions()->UnitCommand(unit, ABILITY_ID::STOP);
                            }
                        }
                        else {
                            Actions()->UnitCommand(unit, ABILITY_ID::MOVE, tankLocation);
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

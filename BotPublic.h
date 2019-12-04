

virtual void OnGameStart() final {
    staging_location_ = Observation()->GetStartLocation();
    game_info_ = Observation()->GetGameInfo();
}


virtual void OnStep() final {
    TryBuildRefinery();
    ManageWorkers(UNIT_TYPEID::TERRAN_SCV, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::TERRAN_REFINERY);
    TryBuildSupplyDepot();
    if (CountUnitType(UNIT_TYPEID::TERRAN_FACTORY) < 2) {
        TryBuildFactory();
    }
    if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) < 2) {
        TryBuildBarracks();
    }
    TryBuildFactoryLab();
    ManageArmy();
}


virtual void OnUnitIdle(const Unit* unit) final {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
            if (CountUnitType(UNIT_TYPEID::TERRAN_SCV) < 21) {
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SCV);
            }
            break;
        }
        case UNIT_TYPEID::TERRAN_SCV: {
            const Unit* mineral_target = FindNearestMineralPatch(unit->pos);
            if (!mineral_target) {
                break;
            }
            Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
            break;
        }
        case UNIT_TYPEID::TERRAN_BARRACKS: {
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
            break;
        }
        case UNIT_TYPEID::TERRAN_FACTORY: {
            size_t tank_count = CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) + CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANKSIEGED);
            if (tank_count < 7) {
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SIEGETANK);
            }
            break;
        }
        default: {
            break;
        }
    }
}

std::vector<Point3D> expansions_;
Point3D staging_location_;
GameInfo game_info_;

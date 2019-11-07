static int TargetSCVCount = 15;

static int isBuilt = 0;

struct IsAttackable {
    bool operator()(const Unit& unit) {
        switch (unit.unit_type.ToType()) {
            case UNIT_TYPEID::ZERG_OVERLORD: return false;
            case UNIT_TYPEID::ZERG_OVERSEER: return false;
            case UNIT_TYPEID::PROTOSS_OBSERVER: return false;
            default: return true;
        }
    }
};

struct IsFlying {
    bool operator()(const Unit& unit) {
        return unit.is_flying;
    }
};

//Ignores Overlords, workers, and structures
struct IsArmy {
    IsArmy(const ObservationInterface* obs) : observation_(obs) {}

    bool operator()(const Unit& unit) {
        auto attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
        for (const auto& attribute : attributes) {
            if (attribute == Attribute::Structure) {
                return false;
            }
        }
        switch (unit.unit_type.ToType()) {
            case UNIT_TYPEID::ZERG_OVERLORD: return false;
            case UNIT_TYPEID::PROTOSS_PROBE: return false;
            case UNIT_TYPEID::ZERG_DRONE: return false;
            case UNIT_TYPEID::TERRAN_SCV: return false;
            case UNIT_TYPEID::ZERG_QUEEN: return false;
            case UNIT_TYPEID::ZERG_LARVA: return false;
            case UNIT_TYPEID::ZERG_EGG: return false;
            case UNIT_TYPEID::TERRAN_MULE: return false;
            case UNIT_TYPEID::TERRAN_NUKE: return false;
            default: return true;
        }
    }

    const ObservationInterface* observation_;
};

struct IsTownHall {
    bool operator()(const Unit& unit) {
        switch (unit.unit_type.ToType()) {
            case UNIT_TYPEID::ZERG_HATCHERY: return true;
            case UNIT_TYPEID::ZERG_LAIR: return true;
            case UNIT_TYPEID::ZERG_HIVE : return true;
            case UNIT_TYPEID::TERRAN_COMMANDCENTER: return true;
            case UNIT_TYPEID::TERRAN_ORBITALCOMMAND: return true;
            case UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING: return true;
            case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS: return true;
            case UNIT_TYPEID::PROTOSS_NEXUS: return true;
            default: return false;
        }
    }
};

struct IsVespeneGeyser {
    bool operator()(const Unit& unit) {
        switch (unit.unit_type.ToType()) {
            case UNIT_TYPEID::NEUTRAL_VESPENEGEYSER: return true;
            case UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER: return true;
            case UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER: return true;
            default: return false;
        }
    }
};

struct IsStructure {
    IsStructure(const ObservationInterface* obs) : observation_(obs) {};

    bool operator()(const Unit& unit) {
        auto& attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
        bool is_structure = false;
        for (const auto& attribute : attributes) {
            if (attribute == Attribute::Structure) {
                is_structure = true;
            }
        }
        return is_structure;
    }

    const ObservationInterface* observation_;
};

int CountUnitType(const ObservationInterface* observation, UnitTypeID unit_type) {
    int count = 0;
    Units my_units = observation->GetUnits(Unit::Alliance::Self);
    for (const auto unit : my_units) {
        if (unit->unit_type == unit_type)
            ++count;
    }

    return count;
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

// This function gives me warning
/*
bool GetRandomUnit(const Unit*& unit_out, const ObservationInterface* observation, UnitTypeID unit_type) {
    Units my_units = observation->GetUnits(Unit::Alliance::Self);
    std::random_shuffle(my_units.begin(), my_units.end()); // Doesn't work, or doesn't work well.
    for (const auto unit : my_units) {
        if (unit->unit_type == unit_type) {
            unit_out = unit;
            return true;
        }
    }
    return false;
}*/

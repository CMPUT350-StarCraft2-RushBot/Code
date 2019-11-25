#include <iostream>
#include <string>
#include <algorithm>
#include <random>
#include <iterator>

#include "sc2api/sc2_api.h"
#include "sc2api/sc2_args.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2utils/sc2_arg_parser.h"

using namespace sc2;

#include "helpers.h"
// Helper functions

class Bot : public sc2::Agent {
public:

    virtual void OnGameStart() final {
        staging_location_ = Observation()->GetStartLocation();
        game_info_ = Observation()->GetGameInfo();
    }
    
    virtual void OnStep() final {
        //ScoutWithMarines();
        TryBuildRefinery();
        ManageWorkers(UNIT_TYPEID::TERRAN_SCV, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::TERRAN_REFINERY);
        TryBuildSupplyDepot();
        /*
        if (Observation()->GetMinerals() > 150 && Observation()->GetVespene() > 100) {
            TryBuildStructureRandom(ABILITY_ID::BUILD_STARPORT, UNIT_TYPEID::TERRAN_SCV);
        }*/
        TryBuildFactory();
        TryBuildBarracks();
        TryBuildFactoryLab();
        
    }

    virtual void OnUnitIdle(const Unit* unit) final {
        const ObservationInterface* observation = Observation();
        switch (unit->unit_type.ToType()) {
            case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
                if (CountUnitType(UNIT_TYPEID::TERRAN_SCV) < 20) {
                    Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SCV);
                }
                if (observation->GetMinerals() > 150) {
                    Actions()->UnitCommand(unit, ABILITY_ID::MORPH_ORBITALCOMMAND);
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
            case UNIT_TYPEID::TERRAN_MARINE: {
                if (CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) >= 3) {
                    AttackWithUnitType(UNIT_TYPEID::TERRAN_MARINE, observation);
                }
                break;
            }
            case UNIT_TYPEID::TERRAN_FACTORY: {
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SIEGETANK);
                /*
                if (CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) % 4 == 0) {
                    Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_THOR);
                }
                else
                {
                    Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SIEGETANK);
                }*/
            }
            case UNIT_TYPEID::TERRAN_SIEGETANK: {
                if (CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) >= 3) {
                    AttackWithUnitType(UNIT_TYPEID::TERRAN_SIEGETANK, observation);
                }
            }
            case UNIT_TYPEID::TERRAN_STARPORT: {
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_LIBERATOR);
            }
            case UNIT_TYPEID::TERRAN_LIBERATOR: {
                if (CountUnitType(UNIT_TYPEID::TERRAN_LIBERATOR) >= 3) {
                    AttackWithUnitType(UNIT_TYPEID::TERRAN_LIBERATOR, observation);
                }
            }
            default: {
                break;
            }
        }
    }
    
    std::vector<Point3D> expansions_;
    Point3D staging_location_;
    GameInfo game_info_;
    

private:
    
#include "BotPrivate.h"

};

int main(int argc, char* argv[]) {
    Coordinator coordinator;
    coordinator.LoadSettings(argc, argv);

    Bot bot;
    coordinator.SetParticipants({
        CreateParticipant(Race::Terran, &bot),
        CreateComputer(Race::Zerg, Difficulty::HardVeryHard)
    });

    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapBelShirVestigeLE);

    while (coordinator.Update()) {
    }

    return 0;
}

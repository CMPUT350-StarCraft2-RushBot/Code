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
        //const ObservationInterface* observation = Observation();
        TryBuildRefinery();
        ManageWorkers(UNIT_TYPEID::TERRAN_SCV, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::TERRAN_REFINERY);
        TryBuildSupplyDepot();
        TryBuildFactory();
        TryBuildBarracks();
        TryBuildBarrackReactor();
        TryBuildFactoryLab();
        ManageArmy();
    }

    virtual void OnUnitIdle(const Unit* unit) final {
        
        const ObservationInterface* observation = Observation();
        Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
        Units army = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));
        
        switch (unit->unit_type.ToType()) {
            case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
                if (CountUnitType(UNIT_TYPEID::TERRAN_SCV) < 20) {
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
                if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKSREACTOR) == 2) {
                    Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
                    if (CountUnitType(UNIT_TYPEID::TERRAN_SIEGETANK) > 3) {
                        Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
                    }
                }
                break;
            }
            case UNIT_TYPEID::TERRAN_FACTORY: {
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SIEGETANK);
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
        CreateComputer(Race::Protoss, Difficulty::Hard)
    });

    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapBelShirVestigeLE);

    while (coordinator.Update()) {
    }

    return 0;
}

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

class RushBot : public sc2::Agent {
    
public:
    
#include "BotPublic.h"
// Public functions
    
private:
    
#include "BotPrivate.h"
// Private functions
};

int main(int argc, char* argv[]) {
    Coordinator coordinator;
    coordinator.LoadSettings(argc, argv);

    RushBot mybot;
    coordinator.SetParticipants({
        CreateParticipant(Race::Terran, &mybot),
        CreateComputer(Race::Zerg, Difficulty::HardVeryHard)
    });

    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapBelShirVestigeLE);

    while (coordinator.Update()) {
    }

    return 0;
}

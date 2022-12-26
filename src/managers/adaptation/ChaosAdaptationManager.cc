#include "ChaosAdaptationManager.h"
#include "managers/adaptation/UtilityScorer.h"
#include "managers/execution/AllTactics.h"
#include "omnetpp.h"
#include <boost/log/trivial.hpp>

#define RT_THRESHOLD omnetpp::getSimulation()->getSystemModule()->par("responseTimeThreshold").doubleValue()


using namespace std;



Define_Module(ChaosAdaptationManager);


// Calculate the risk of a server failure
/* double risk = 1.0 - pow(1.0 - CHAOS_COEFFICIENT, noOfActiveServers);
 *
This formula calculates the probability of at least one server failure occurring in the system,
given the current chaos coefficient and the number of active servers.
It does this by raising the coefficient to the power of the number of active servers,
which represents the number of independent server failures that could occur.
The result is then subtracted from 1.0 to give the probability of at least one server failure occurring.

For example, if the chaos coefficient is 0.1 (10% probability of failure) and there are 10 active servers,
the risk of at least one server failure occurring would be approximately 63.1%.
If the coefficient is 0.01 (1% probability of failure) and there are 100 active servers,
the risk would be approximately 63.4%. The calculate_risk method returns
this probability as a double value between 0.0 and 1.0. This value can then be used to determine the appropriate response to the risk of server failure,
such as adding additional servers to the system or implementing other mitigation strategies.
*/


// Entry point
Tactic *ChaosAdaptationManager::evaluate()
{
    MacroTactic *pMacroTactic = new MacroTactic;
    Model *pModel = getModel();
    double chaos_coefficient = pModel -> getChaosCoefficient();
    int totalServers = pModel->getMaxServers();
    int noOfActiveServers = pModel->getActiveServers();
    const double dimmerStep = 1.0 / (pModel->getNumberOfDimmerLevels() - 1);
    double dimmer = pModel->getDimmerFactor();
    double spareServers = pModel->getConfiguration().getActiveServers() - pModel->getObservations().utilization;
    double spareUtilization = spareServers / (double)pModel->getConfiguration().getActiveServers();
    bool isServerBooting = pModel->getServers() > noOfActiveServers;
    double responseTime = pModel->getObservations().avgResponseTime;

    cout << "totalServers: " << totalServers << endl;
    cout << "noOfActiveServers: " << noOfActiveServers << endl;
    cout << "spareUtilization: " << spareUtilization << endl;

    //This is for adding chaos to the system
    // Generate a random number from 0 to RAND_MAX
    int random_number = rand();
    // Normalize the random number between 0 and 1
    // This is simulates chaos and removes a server like it is failed.
    double chance = static_cast<double>(random_number) / RAND_MAX;
    if (chaos_coefficient >= chance && pModel->getActiveServers() > 1) {
         pMacroTactic->addTactic(new RemoveServerTactic);
         cout << "Server removed --- " << endl;
         cout << "Servers left: " << pModel->getActiveServers() << endl;
         return pMacroTactic;
    }
    //

    //calculate risk
    double risk = 1.0 - pow(1.0 - chaos_coefficient, noOfActiveServers);
    cout << "Risk: " << risk << endl;


    // if response time is higher than threshold or risk is high we add servers
    if (responseTime > RT_THRESHOLD || risk > spareUtilization) {
        if (!isServerBooting
                        && pModel->getServers() < pModel->getMaxServers()) {
                    pMacroTactic->addTactic(new AddServerTactic);
        } else if (dimmer > 0.0) {
               dimmer = max(0.0, dimmer - dimmerStep);
                pMacroTactic->addTactic(new SetDimmerTactic(dimmer));
        }
    } else if (responseTime < RT_THRESHOLD && risk < spareUtilization) { // removing uncessary servers otherwise
        if (dimmer < 1.0) {
                    dimmer = min(1.0, dimmer + dimmerStep);
                    pMacroTactic->addTactic(new SetDimmerTactic(dimmer));
         } else if (!isServerBooting
                        && pModel->getServers() > 1) {
                    pMacroTactic->addTactic(new RemoveServerTactic);
         }
     }



    return pMacroTactic;
}

/*******************************************************************************
 * Simulator of Web Infrastructure and Management
 * Copyright (c) 2016 Carnegie Mellon University.
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," WITH NO WARRANTIES WHATSOEVER. CARNEGIE
 * MELLON UNIVERSITY EXPRESSLY DISCLAIMS TO THE FULLEST EXTENT PERMITTED BY LAW
 * ALL EXPRESS, IMPLIED, AND STATUTORY WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, AND NON-INFRINGEMENT OF PROPRIETARY RIGHTS.
 *
 * Released under a BSD license, please see license.txt for full terms.
 * DM-0003883
 *******************************************************************************/

#include "ChaosAdaptationManager.h"
#include "managers/adaptation/UtilityScorer.h"
#include "managers/execution/AllTactics.h"
#include "globals.h"
#include "omnetpp.h"

#include <cmath>

using namespace std;


Define_Module(ChaosAdaptationManager);

void perform_adaption(MacroTactic* pMacroTactic, int no_servers)
{
    if (current_extra_servers < no_servers) {
        int diff = abs(current_extra_servers - no_servers);
        while (diff != 0) {
            pMacroTactic->addTactic(new AddServerTactic);
            total_servers_created++;
            current_extra_servers++;
            diff--;
        }
    }
    else if (current_extra_servers > no_servers) {
        int diff = current_extra_servers - no_servers;
        while (diff != 0) {
            pMacroTactic->addTactic(new RemoveServerTactic);
            current_extra_servers--;
            diff--;
        }
    } else if (current_extra_servers == no_servers) return;
}

double calculate_risk(double bootDelay)
{
    double risk;
    double total_time = omnetpp::simTime().dbl();

    if(total_failed_servers > 0){
        double mtbf = total_time / total_failed_servers;
        double mttr = bootDelay;
        risk = 1  - (mtbf / (mtbf + mttr));
    }
    else{
        risk = 0.0;
    }

    return risk;

}


Tactic* ChaosAdaptationManager::evaluate() {
    MacroTactic* pMacroTactic = new MacroTactic;
    Model* pModel = getModel();
    double chaos_coefficient = pModel -> getChaosCoefficient();
    double boot_delay = pModel -> getBootDelay();
    const double dimmerStep = 1.0 / (pModel->getNumberOfDimmerLevels() - 1);
    double dimmer = pModel->getDimmerFactor();
    bool isServerBooting = pModel->getServers() > pModel->getActiveServers();
    double responseTime = pModel->getObservations().avgResponseTime;
    double spareUtilization =  pModel->getConfiguration().getActiveServers() - pModel->getObservations().utilization;



    int randomNumber = abs(rand() % 100) + 1;
    if (chaos_coefficient >= randomNumber && pModel->getServers() > 1) {
        pMacroTactic->addTactic(new RemoveServerTactic);
        time_at_failed_server = omnetpp::simTime().dbl();
        total_failed_servers++;
        cout << "Server removed --- " << "t=" << omnetpp::simTime() << endl;
        cout << "Servers left: " << pModel->getServers() << endl;
        return pMacroTactic;
    }

    const double risk = calculate_risk(boot_delay);
    cout << "\033[;31mRisk:\033[0m" << risk << endl;
    cout << "Spare utilization" << spareUtilization << endl;

    if (risk > 0.0 && risk < 0.2) {
        double level_of_adaption = ceil(pModel->getActiveServers() * 0.05);
        cout << "\033[;31mAdaptation level\033[0m : " << level_of_adaption << endl;
        perform_adaption(pMacroTactic, level_of_adaption);
    }
    else if (risk < 0.4) {
        double level_of_adaption = ceil(pModel->getActiveServers() * 0.10);
        cout << "\033[;31mAdaptation level\033[0m : " << level_of_adaption << endl;
        perform_adaption(pMacroTactic, level_of_adaption);
    }
    else if (risk < 0.6) {
        double level_of_adaption = ceil(pModel->getActiveServers() * 0.15);
        cout << "\033[;31mAdaptation level\033[0m : " << level_of_adaption << endl;
        perform_adaption(pMacroTactic, level_of_adaption);
    }
    else if (risk < 0.8) {
        double level_of_adaption = ceil(pModel->getActiveServers() * 0.20);
        cout << "\033[;31mAdaptation level\033[0m : " << level_of_adaption << endl;
        perform_adaption(pMacroTactic, level_of_adaption);
    }
    else {
        double level_of_adaption = ceil(pModel->getActiveServers() * 0.25);
        cout << "\033[;31mAdaptation level\033[0m : " << level_of_adaption << endl;
        perform_adaption(pMacroTactic, level_of_adaption);
    }


    if (responseTime > RT_THRESHOLD || risk > spareUtilization) {
        if (!isServerBooting
                        && pModel->getServers() < pModel->getMaxServers()) {
                    pMacroTactic->addTactic(new AddServerTactic);
                } else if (dimmer > 0.0) {
                    dimmer = max(0.0, dimmer - dimmerStep);
                    pMacroTactic->addTactic(new SetDimmerTactic(dimmer));
                }
    } else if (responseTime < RT_THRESHOLD && risk <= spareUtilization) { // can we increase dimmer or remove servers?
        if (spareUtilization > 1) {
                   if (dimmer < 1.0) {
                       dimmer = min(1.0, dimmer + dimmerStep);
                       pMacroTactic->addTactic(new SetDimmerTactic(dimmer));
                   } else if (!isServerBooting
                           && pModel->getServers() > 1) {
                       pMacroTactic->addTactic(new RemoveServerTactic);
                   }
               }
    }

    return pMacroTactic;
}

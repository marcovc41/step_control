#include "state_manager.hpp"

void StateManager::setAction(Action instruction)
{
    critical_section_enter_blocking(&stateManagerLock);
    instance->instructionBuffer = instruction;
    instance->machineData.state.actionRequired = true;
    critical_section_exit(&stateManagerLock);
}

void StateManager::setEmergencyStop()
{
    critical_section_enter_blocking(&stateManagerLock);
    instance->machineData.state.emergencyStop = true;
    instance->machineData.state.actionRequired = true;
    critical_section_exit(&stateManagerLock);
}

MachineState StateManager::getMachineState()
{
    critical_section_enter_blocking(&stateManagerLock);
    MachineState returnval = machineData.state.mode;
    critical_section_exit(&stateManagerLock);
    return returnval;
}

bool StateManager::isDataPending()
{
    critical_section_enter_blocking(&stateManagerLock);
    bool returnval = false;
    if(dataPending){
        returnval = true;
    }
    dataPending = false;
    critical_section_exit(&stateManagerLock);
    return returnval;
}

void StateManager::machineProcess()
{
    critical_section_enter_blocking(&stateManagerLock);

    // solo si action needed hacer algo
    // solo si paro de emergencia ignorar el resto
    switch (machineData.key)
    {
        case (EMERGENCY_ENABLED | static_cast<uint32_t>(MachineState::Off)):
        case (EMERGENCY_ENABLED | static_cast<uint32_t>(MachineState::PrepareMove)):
        case (EMERGENCY_ENABLED | static_cast<uint32_t>(MachineState::WaitStart)):
        case (EMERGENCY_ENABLED | static_cast<uint32_t>(MachineState::ExecuteProgram)):
        case (EMERGENCY_ENABLED | static_cast<uint32_t>(MachineState::Jog)):
            machineData.state.mode = MachineState::EmergencyStop;
            printf("EMERGENCY STOP\n");
            break;
        
        case (static_cast<uint32_t>(MachineState::Off)):
            switch (instructionBuffer)
            {
                case Action::Jog :
                    machineData.state.mode = MachineState::Jog;
                    printf("Off -> Jog\n");
                    break;
                case Action::Program :
                    machineData.state.mode = MachineState::PrepareMove;
                    printf("Off -> Program\n");
                    break;
                default:
                    break;
            }
            break;

        case (static_cast<uint32_t>(MachineState::Jog)):
            switch (instructionBuffer)
            {
                case Action::Cancel:
                    machineData.state.mode = MachineState::Off;
                    printf("Jog -> Off\n");
                    break;
                default:
                    break;
            }
            break;
            
        case (static_cast<uint32_t>(MachineState::PrepareMove)):
            switch (instructionBuffer)
            {
                case Action::Data:
                    printf("Program: Data\n");
                    dataPending = true;
                    break;
                case Action::Done:
                    machineData.state.mode = MachineState::WaitStart;
                    break;
                case Action::Cancel:
                    machineData.state.mode = MachineState::Off;
                    printf("Program -> Off\n");
                    break;
                default:
                    break;
            }
            break;

        case (static_cast<uint32_t>(MachineState::WaitStart)):
            switch (instructionBuffer)
            {
                case Action::Cancel:
                    machineData.state.mode = MachineState::Off;
                    printf("Waitstart -> Off\n");
                    break;
                case Action::Start:
                    machineData.state.mode = MachineState::ExecuteProgram;
                    printf("Waitstart -> Start\n");
                    break;
                default:
                    break;
            }
            break;

        case (static_cast<uint32_t>(MachineState::ExecuteProgram)):
            switch (instructionBuffer)
            {
                case Action::Next:
                    break;
                case Action::Done:
                    machineData.state.mode = MachineState::Off;
                    break;
                default:
                    break;
            }
            break;

        case (static_cast<uint32_t>(MachineState::EmergencyStop)):
            switch (instructionBuffer)
            {
                case Action::Done:
                    machineData.state.mode = MachineState::Off;
                    machineData.state.emergencyStop = false;
                    printf("Emer -> Off\n");
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
    machineData.state.actionRequired = false;

    critical_section_exit(&stateManagerLock);
}

StateManager * StateManager::instance{nullptr};

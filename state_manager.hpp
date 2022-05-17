#pragma once

#include <unordered_map>
#include <string>
#include <cstdint>
#include "trajectory_gen.hpp"

/* ---------------------------------------------------------------------------------------------- */
/*                                      Public Access Members                                     */
/* ---------------------------------------------------------------------------------------------- */

enum class Action : uint8_t
{
    Cancel = 0x00,
    Start = 0x01,
    Next = 0x02,
    Done = 0x03,
    Jog = 0x04,
    Program = 0x05,
    Data = 0x06,
    None = 0xFF
};

Action instructionBuffer = Action::None;

enum class MachineState : uint8_t
{
    Off = 0x00,
    PrepareMove = 0x01,
    WaitStart = 0x02,
    ExecuteProgram = 0x03,
    Jog = 0x04,
    EmergencyStop = 0xFF
};

typedef union
{
    struct
    {
        volatile bool actionNotRequired;
        volatile bool emergencyStop;
        volatile MachineState mode;
    } state;
    volatile uint32_t key;
} MachineData;

MachineData machineData =
{
    .state =
    {
        .actionNotRequired = true,
        .emergencyStop = false,
        .mode = MachineState::Off,
    }
};

/* ---------------------------------------------------------------------------------------------- */
/*                               State Manager Static Scope Members                               */
/* ---------------------------------------------------------------------------------------------- */

#define EMERGENCY_ENABLED (uint32_t)(1 << 8)
#define ACTION_NOT_NEEDED (uint32_t)(1 << 16)

static void machineProcess()
{
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
            break;
        
        case (static_cast<uint32_t>(MachineState::Off)):
            switch (instructionBuffer)
            {
                case Action::Jog :
                    machineData.state.mode = MachineState::Jog;
                    break;
                case Action::Program :
                    machineData.state.mode = MachineState::PrepareMove;
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
                    break;
                default:
                    break;
            }
            break;
            
        case (static_cast<uint32_t>(MachineState::PrepareMove)):
            switch (instructionBuffer)
            {
                case Action::Data:
                    break;
                case Action::Done:
                    machineData.state.mode = MachineState::WaitStart;
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
                    break;
                case Action::Start:
                    machineData.state.mode = MachineState::ExecuteProgram;
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
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
    machineData.state.actionNotRequired = true;
}


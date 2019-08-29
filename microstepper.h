/* INDI Driver for Milkyway Microstepper
 *
 * written 2019 by Juergen Ehnes (juergen at ehnes dot eu)
 *
 *  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef MICROSTEPPER_H
#define MICROSTEPPER_H

#pragma once

#include "indifocuser.h"


class MicroStepper : public INDI::Focuser
{
public:
    MicroStepper();
    virtual bool initProperties() override;
    const char *getDefaultName();
    virtual bool Handshake() override;
    bool updateProperties() override;
    bool saveConfigItems(FILE *fp) override;
    IPState MoveAbsFocuser(uint32_t targetTicks) override;
    IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks);
    bool ReverseFocuser(bool enabled);

    enum FocusDirectionState {
        NORMAL,
        REVERSE
    } ;

protected:

    bool readPosition();

    bool isAbsolute;
    bool isMoving;
    unsigned char isParked;
    bool isVcc12V;
    bool reverseFocus = false;

    ///////////////////////////////////////////////////////////////////////////////
    /// Utility Functions
    ///////////////////////////////////////////////////////////////////////////////
    /**
     * @brief sendCommand Send a string command to device.
     * @param cmd Command to be sent. Can be either NULL TERMINATED or just byte buffer.
     * @param res If not nullptr, the function will wait for a response from the device. If nullptr, it returns true immediately
     * after the command is successfully sent.
     * @param cmd_len if -1, it is assumed that the @a cmd is a null-terminated string. Otherwise, it would write @a cmd_len bytes from
     * the @a cmd buffer.
     * @param res_len if -1 and if @a res is not nullptr, the function will read until it detects the default delimeter DRIVER_STOP_CHAR
     *  up to DRIVER_LEN length. Otherwise, the function will read @a res_len from the device and store it in @a res.
     * @return True if successful, false otherwise.
     */
    bool sendCommand(const char * cmd, char * res = nullptr, int cmd_len = -1, int res_len = -1);

    /**
     * @brief hexDump Helper function to print non-string commands to the logger so it is easier to debug
     * @param buf buffer to format the command into hex strings.
     * @param data the command
     * @param size length of the command in bytes.
     * @note This is called internally by sendCommand, no need to call it directly.
     */
    void hexDump(char * buf, const char * data, int size);

    /////////////////////////////////////////////////////////////////////////////
    /// Static Helper Values
    /////////////////////////////////////////////////////////////////////////////
    static constexpr const char * STEPPING_TAB = "Stepping";
    // '#' is the stop char
    static const char DRIVER_STOP_CHAR { 0x23 };
    // Update temperature every 10x POLLMS. For 500ms, we would
    // update the temperature one every 5 seconds.
    static constexpr const uint8_t DRIVER_TEMPERATURE_FREQ {10};
    // Wait up to a maximum of 3 seconds for serial input
    static constexpr const uint8_t DRIVER_TIMEOUT {30};
    // Maximum buffer for sending/receving.
    static constexpr const uint8_t DRIVER_LEN {64};

    static constexpr const int32_t MAXIMUM_STEPS_PER_SEND { 1000};

    ISwitchVectorProperty SteppingModeSP;


};

#endif // MICROSTEPPER_H

/* INDI Driver for Milkyway Microstepper
 *
 * written 2019 by Juergen Ehnes (juergen at ehnes dot eu)
 * Used parts of the driver skeleton from libindi. Thanks to Jason
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

#include "microstepper.h"
#include "config.h"
#include "connectionplugins/connectionserial.h"
#include <memory>
#include <termios.h>
#include <cstring>

#include "indicom.h"

/**
 * @brief microStepper a microstepper instance as class member
 */
std::unique_ptr<MicroStepper> microStepper(new MicroStepper());

/**
 * @brief MicroStepper::MicroStepper
 */
MicroStepper::MicroStepper()
{
    FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | 
                      FOCUSER_CAN_REVERSE | FOCUSER_HAS_BACKLASH);
    isAbsolute = false;
    isParked = 0;
    isVcc12V = false;

    setVersion(MICROSTEPPER_VERSION_MAJOR, MICROSTEPPER_VERSION_MINOR);
}

/**
 * @brief MicroStepper::initProperties
 * @return true if successful
 */
bool MicroStepper::initProperties() {
    // Call Superclass Method
    INDI::Focuser::initProperties();

    // Set initial serial values
    serialConnection->setDefaultPort("/dev/ttyUSB0");
    serialConnection->setDefaultBaudRate(Connection::Serial::B_9600);
    setDefaultPollingPeriod(500);

    // Set limits as per documentation
    FocusAbsPosN[0].min  = 0;
    FocusAbsPosN[0].max  = 10000;
    FocusAbsPosN[0].step = 10;
    FocusAbsPosN[0].value = 5000;

    FocusRelPosN[0].min = 0;
    FocusRelPosN[0].max = 1000;
    FocusRelPosN[0].step = 10;
    FocusRelPosN[0].value= 10;

    FocusMaxPosN[0].value = 10000;
    FocusMaxPosN[0].max = 10000;
    FocusMaxPosN[0].min = 10000;

    // Focuser temperature
    IUFillNumber(&TemperatureN[0], "TEMPERATURE", "Celsius", "%6.2f", -100, 100, 0, 0);
    IUFillNumberVector(&TemperatureNP, TemperatureN, 1, getDeviceName(), "FOCUS_TEMPERATURE", "Temperature",
                       MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    return true;
}

/**
 * @brief MicroStepper::updateProperties
 * @return true if successful
 */
bool MicroStepper::updateProperties()
{

    if (isConnected())
    {
        // Read these values before defining focuser interface properties
        readPosition();

        if (readTemperature()) {
            defineNumber(&TemperatureNP);
        }
    }
    else
    {
        if (TemperatureNP.s == IPS_OK)
            deleteProperty(TemperatureNP.name);
    }
    INDI::Focuser::updateProperties();

    return true;
}

/**
 * Get default name
 *
 * return the default name
*/
const char* MicroStepper::getDefaultName(){
    return "MicroStepper";
}

/**
 * @brief ISGetProperties
 * @param dev
 */
void ISGetProperties (const char *dev) {
    microStepper->ISGetProperties(dev);
}

/**
 * @brief ISNewSwitch
 * @param dev
 * @param name
 * @param states
 * @param names
 * @param n
 */
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) {
    microStepper->ISNewSwitch(dev, name, states, names, n);
}

/**
 * @brief ISNewText
 * @param dev
 * @param name
 * @param texts
 * @param names
 * @param n
 */
void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) {
    microStepper->ISNewText(dev, name, texts, names, n);
}

/**
 * @brief ISNewNumber
 * @param dev
 * @param name
 * @param values
 * @param names
 * @param n
 */
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) {
    microStepper->ISNewNumber(dev, name, values, names, n);
}

/**
 * @brief ISNewBLOB
 * @param dev
 * @param name
 * @param sizes
 * @param blobsizes
 * @param blobs
 * @param formats
 * @param names
 * @param n
 */
void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char*blobs[], char *formats[], char *names[], int n) {
    microStepper->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

/**
 * @brief ISSnoopDevice
 * @param root
 */
void ISSnoopDevice (XMLEle *root) {
    INDI_UNUSED(root);
}

/**
 * @brief MicroStepper::sendCommand
 * @param cmd the command to send as string
 * @param res space for the result string
 * @param cmd_len the length of the command
 * @param res_len the possible length of the result
 * @return true if sending was successful
 */
bool MicroStepper::sendCommand(const char * cmd, char * res, int cmd_len, int res_len)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;

    tcflush(PortFD, TCIOFLUSH);

    if (cmd_len > 0)
    {
        char hex_cmd[DRIVER_LEN * 3] = {0};
        hexDump(hex_cmd, cmd, cmd_len);
        LOGF_DEBUG("CMD <%s>", hex_cmd);
        rc = tty_write(PortFD, cmd, cmd_len, &nbytes_written);
    }
    else
    {
        LOGF_DEBUG("CMD <%s>", cmd);
        rc = tty_write_string(PortFD, cmd, &nbytes_written);
    }

    if (rc != TTY_OK)
    {
        char errstr[MAXRBUF] = {0};
        tty_error_msg(rc, errstr, MAXRBUF);
        LOGF_ERROR("Serial write error: %s.", errstr);
        return false;
    }

    if (res == nullptr)
        return true;

    if (res_len > 0)
        rc = tty_read(PortFD, res, res_len, DRIVER_TIMEOUT, &nbytes_read);
    else
        rc = tty_nread_section(PortFD, res, DRIVER_LEN, DRIVER_STOP_CHAR, DRIVER_TIMEOUT, &nbytes_read);

    if (rc != TTY_OK)
    {
        char errstr[MAXRBUF] = {0};
        tty_error_msg(rc, errstr, MAXRBUF);
        LOGF_ERROR("Serial read error: %s.", errstr);
        return false;
    }

    if (res_len > 0)
    {
        char hex_res[DRIVER_LEN * 3] = {0};
        hexDump(hex_res, res, res_len);
        LOGF_DEBUG("RES <%s>", hex_res);
    }
    else
    {
        LOGF_DEBUG("RES <%s>", res);
    }

    tcflush(PortFD, TCIOFLUSH);

    return true;
}

/**
 * @brief MicroStepper::hexDump
 * @param buf
 * @param data
 * @param size
 */
void MicroStepper::hexDump(char * buf, const char * data, int size)
{
    for (int i = 0; i < size; i++)
        sprintf(buf + 3 * i, "%02X ", static_cast<uint8_t>(data[i]));

    if (size > 0)
        buf[3 * size - 1] = '\0';
}

/**
 * @brief MicroStepper::Handshake
 * @return
 */
bool MicroStepper::Handshake()
{
    // This functin is ensure that we have communication with the focuser
    // Below we send it GETPOS Command and check for 'P' as first return char

    char cmd[DRIVER_LEN] = {}, res[DRIVER_LEN] = {0};

    sprintf(cmd, "GETPOS#");

    bool rc = sendCommand(cmd, res, strlen(cmd), 5);
    if (rc == false)
        return false;

    char hex_res[DRIVER_LEN * 3] = {0};
    hexDump(hex_res,res, 5);
    LOGF_DEBUG("RES <%s>", hex_res);

    printf("RES <%s>", hex_res);

    return res[0] == 'P';
}

/**
 * @brief MicroStepper::readPosition
 * @return
 */
bool MicroStepper::readPosition()
{
    char cmd[DRIVER_LEN] = {0}, res[DRIVER_LEN] = {0};

    sprintf(cmd, "GETPOS#");

    if (sendCommand(cmd, res, 7, 7) == false)
        return false;

    int32_t pos = 1e6;
    sscanf(res+1, "%d", &pos);

    if (pos == 1e6)
        return false;

    FocusAbsPosN[0].value = pos;

    return true;
}

/**
 * @brief MicroStepper::saveConfigItems
 * @param fp
 * @return true if successful
 */
bool MicroStepper::saveConfigItems(FILE *fp)
{
    INDI::Focuser::saveConfigItems(fp);

    IUSaveConfigNumber(fp, &FocusRelPosNP);
    // We need to reserve and save stepping mode
    // so that the next time the driver is loaded, it is remembered and applied.

    //    IUSaveConfigNumber(fp, &FocusRelPosN);
    //    IUSaveConfigSwitch(fp, &reverseFocus);
    return true;
}

/**
 * Move focuser to absolute position.
 * @brief MicroStepper::MoveAbsFocuser
 * @param targetTicks
 * @return
 */
IPState MicroStepper::MoveAbsFocuser(uint32_t targetTicks)
{
    uint32_t currentPosition = FocusAbsPosN[0].value;

    int32_t diff = currentPosition - targetTicks;
    bool direction = diff < 0;
    diff = abs(diff);
    int32_t numberOfSteps = diff;

    if (diff > MAXIMUM_STEPS_PER_SEND) {
        numberOfSteps = MAXIMUM_STEPS_PER_SEND;
    }

    if (direction != oldDirection) {
        numberOfSteps += FocusBacklashN[0].value;
    }

    do {
        if (direction) {
            currentPosition += numberOfSteps;
        } else {
            currentPosition -= numberOfSteps;
        }

        oldDirection = direction;

        // Issue here the command necessary to move the focuser to targetTicks
        char cmd[DRIVER_LEN] = {0};
        int32_t newValue = currentPosition;
        sprintf(cmd, "GOTO:%d#", newValue);


        if (sendCommand(cmd, NULL, strlen(cmd),0) == false) {
            return IPS_ALERT;
        }

        diff = diff - numberOfSteps;

        if (diff < MAXIMUM_STEPS_PER_SEND) {
            numberOfSteps = diff;
        } else {
            numberOfSteps = MAXIMUM_STEPS_PER_SEND;
        }


    } while(diff > 0);

    readPosition();

    return IPS_OK;
}

/**
 * Move focuser relativ with direction dir and number of ticks
 * @brief MicroStepper::MoveRelFocuser
 * @param dir
 * @param ticks
 * @return
 */
IPState MicroStepper::MoveRelFocuser(FocusDirection dir, uint32_t ticks) {
    readPosition();

    int position = FocusAbsPosN[0].value;

    if (dir == FOCUS_INWARD) {
        if (!reverseFocus) {
            position -= ticks;
        } else {
            position += ticks;
        }

        if (position < FocusAbsPosN[0].min) {
            return IPS_ALERT;
        }
    } else if (dir == FOCUS_OUTWARD) {
        if (!reverseFocus) {
            position += ticks;
        } else {
            position -= ticks;
        }
        if (position > FocusAbsPosN[0].max) {
            return IPS_ALERT;
        }
    }

    return MoveAbsFocuser(position);
}

/**
 * If focuser is mounted in the other direction, reverse the direction of focuser movement
 * @brief MicroStepper::ReverseFocuser
 * @param enabled
 * @return
 */
bool MicroStepper::ReverseFocuser(bool enabled) {
    reverseFocus = enabled;
    return reverseFocus;
}

/**
 * @brief MicroStepper::readTemperature
 * @return
 */
bool MicroStepper::readTemperature() {
    char cmd[DRIVER_LEN] = {0}; char res[DRIVER_LEN] = {0};
    bool result = false;;
    sprintf(cmd, "GETTEMP#");

    if (sendCommand(cmd, res, strlen(cmd), 5)) {
        int8_t temperature = -127;
        sscanf(res+1, "%d", &temperature);

        if (temperature > -100) {
            TemperatureN[0].value = temperature;
            TemperatureNP.s = IPS_OK;
            result = true;
        }
    }

    return result;
}

/**
 * @brief MicroStepper::TimerHit
 */
void MicroStepper::TimerHit()
{
    if (isConnected() == false)
        return;

    // What is the last read position?
    double currentPosition = FocusAbsPosN[0].value;

    // Read the current position
    readPosition();

    // Check if we have a pending motion
    // if isMoving() is false, then we stopped, so we need to set the Focus Absolute
    // and relative properties to OK
    if ( (FocusAbsPosNP.s == IPS_BUSY || FocusRelPosNP.s == IPS_BUSY) && isMoving() == false)
    {
        FocusAbsPosNP.s = IPS_OK;
        FocusRelPosNP.s = IPS_OK;
        IDSetNumber(&FocusAbsPosNP, nullptr);
        IDSetNumber(&FocusRelPosNP, nullptr);
    }
    // If there was a different between last and current positions, let's update all clients
    else if (currentPosition != FocusAbsPosN[0].value)
    {
        IDSetNumber(&FocusAbsPosNP, nullptr);
    }

    // Read temperature periodically
    if (TemperatureNP.s == IPS_OK && m_TemperatureCounter++ == DRIVER_TEMPERATURE_FREQ)
    {
        m_TemperatureCounter = 0;
        if (readTemperature())
            IDSetNumber(&TemperatureNP, nullptr);
    }

    SetTimer(POLLMS);
}

/**
 * @brief MicroStepper::isMoving
 * @return
 */
bool MicroStepper::isMoving()
{
    return false;
}

/************************************************************************************
 *
************************************************************************************/
bool MicroStepper::SetFocuserBacklash(int32_t steps)
{
    INDI_UNUSED(steps);
    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool MicroStepper::SetFocuserBacklashEnabled(bool enabled)
{
    INDI_UNUSED(enabled);
    return true;
}


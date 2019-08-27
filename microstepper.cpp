/* INDI Driver for Milkyway Microstepper
 *
 * written 2019 by Juergen Ehnes (juergen@ehnes.eu)
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

std::unique_ptr<MicroStepper> microStepper(new MicroStepper());

MicroStepper::MicroStepper()
{
    FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_REVERSE);
    isAbsolute = false;
    isMoving = false;
    isParked = 0;
    isVcc12V = false;

    setVersion(MICROSTEPPER_VERSION_MAJOR, MICROSTEPPER_VERSION_MINOR);
}

bool MicroStepper::initProperties() {
    // Call Superclass Method
    INDI::Focuser::initProperties();

    serialConnection->setDefaultPort("/dev/ttyUSB0");
    serialConnection->setDefaultBaudRate(Connection::Serial::B_9600);
    setDefaultPollingPeriod(500);
    addDebugControl();
    DEBUG(INDI::Logger::DBG_SESSION, "updateProperties called");

    // Set limits as per documentation
    FocusAbsPosN[0].min  = 0;
    FocusAbsPosN[0].max  = 10000;
    FocusAbsPosN[0].step = 10;
    FocusAbsPosN[0].value = 5000;

    FocusRelPosN[0].min = 0;
    FocusRelPosN[0].max = 10000;
    FocusRelPosN[0].step = 10;
    FocusRelPosN[0].value= 5000;


   FocusMaxPosN[0].value = 10000;
   FocusMaxPosN[0].max = 10000;
    return true;
}


bool MicroStepper::updateProperties()
{

    if (isConnected())
    {
        // Read these values before defining focuser interface properties
        readPosition();
    }

    INDI::Focuser::updateProperties();

    return true;
}
const char* MicroStepper::getDefaultName(){
    return "MicroStepper";
}

void ISGetProperties (const char *dev) {
    microStepper->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) {
    microStepper->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) {
    microStepper->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) {
    microStepper->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char*blobs[], char *formats[], char *names[], int n) {
    microStepper->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

void ISSnoopDevice (XMLEle *root) {
    INDI_UNUSED(root);
}

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


void MicroStepper::hexDump(char * buf, const char * data, int size)
{
    for (int i = 0; i < size; i++)
        sprintf(buf + 3 * i, "%02X ", static_cast<uint8_t>(data[i]));

    if (size > 0)
        buf[3 * size - 1] = '\0';
}

bool MicroStepper::Handshake()
{
    // This functin is ensure that we have communication with the focuser
    // Below we send it 0x6 byte and check for 'S' in the return. Change this
    // to be valid for your driver. It could be anything, you can simply put this below
    // return readPosition()
    // since this will try to read the position and if successful, then communicatoin is OK.
    char cmd[DRIVER_LEN] = {}, res[DRIVER_LEN] = {0};


    cmd[0] = 'G';
    cmd[1] = 'E';
    cmd[2] = 'T';
    cmd[3] = 'P';
    cmd[4] = 'O';
    cmd[5] = 'S';
    cmd[6] = '#';

    bool rc = sendCommand(cmd, res, 6, 1);
    if (rc == false)
        return false;

    char hex_res[DRIVER_LEN * 3] = {0};
    hexDump(hex_res,res, 5);
    LOGF_DEBUG("RES <%s>", hex_res);

    printf("RES <%s>", hex_res);

    return res[0] == 'P';
}

bool MicroStepper::readPosition()
{
    char cmd[DRIVER_LEN] = {0}, res[DRIVER_LEN] = {0};

    cmd[0] = 'G';
    cmd[1] = 'E';
    cmd[2] = 'T';
    cmd[3] = 'P';
    cmd[4] = 'O';
    cmd[5] = 'S';
    cmd[6] = '#';


    // since the command above is not NULL-TERMINATED, we need to specify the number of bytes (3)
    // in the send command below. We also specify 7 bytes to be read which can be changed to any value.
    if (sendCommand(cmd, res, 7, 7) == false)
        return false;

    // For above, in case instead the response is terminated by DRIVER_STOP_CHAR, then the command would be
    // (sendCommand(cmd, res, 3) == false)
    //    return false;

    int32_t pos = 1e6;
    sscanf(res+1, "%d", &pos);

    if (pos == 1e6)
        return false;

    FocusAbsPosN[0].value = pos;

    return true;
}

bool MicroStepper::saveConfigItems(FILE *fp)
{
    INDI::Focuser::saveConfigItems(fp);

    // We need to reserve and save stepping mode
    // so that the next time the driver is loaded, it is remembered and applied.
    IUSaveConfigSwitch(fp, &SteppingModeSP);

    return true;
}

IPState MicroStepper::MoveAbsFocuser(uint32_t targetTicks)
{
    // Issue here the command necessary to move the focuser to targetTicks
    char cmd[DRIVER_LEN] = {0}, res[DRIVER_LEN] = {0};
    double currentValue =  FocusAbsPosN[0].value;
    int newValue = targetTicks;
    sprintf(cmd, "GOTO:%d#", newValue);


    if (sendCommand(cmd, NULL, strlen(cmd),0) == false)
        return IPS_ALERT;

    readPosition();

    return IPS_OK;
}

IPState MicroStepper::MoveRelFocuser(FocusDirection dir, uint32_t ticks) {
    readPosition();

    int position = FocusAbsPosN[0].value;

    if (dir == FOCUS_INWARD) {
        position -= ticks;

        if (position < FocusAbsPosN[0].min) {
            return IPS_ALERT;
        }
    } else if (dir == FOCUS_OUTWARD) {
        position += ticks;
        if (position > FocusAbsPosN[0].max) {
            return IPS_ALERT;
        }
    }

    char cmd[DRIVER_LEN] = {0}; char res[DRIVER_LEN] = {0};
    sprintf(cmd, "GOTO:%d#", position);

    return MoveAbsFocuser(position);
}

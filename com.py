import json
import serial
import time

from sw.host.penetrationtests.python.fi.host_scripts import fi_ibex_functions
from sw.host.penetrationtests.python.fi.communication.fi_ibex_commands import OTFIIbex
from sw.host.penetrationtests.python.util import targets
from sw.host.penetrationtests.python.util import utils
from sw.host.penetrationtests.python.util import common_library

def dump_uart(interface: serial.Serial) -> None:
    connection_live = True
    while connection_live:
        try:
            print(interface.readline())
        except:
            connection_live = False

def main():
    #uart_port="/dev/pts/15"
    #com_interface = serial.Serial(uart_port)
    #dump_uart(com_interface)
    target_cfg = targets.TargetConfig(
        target_type="chip",
        interface_type="hyperdebug",
        fw_bin="",
        opentitantool="",
        port="/dev/pts/20"
    )
    target = targets.Target(target_cfg)
    ibexfi = OTFIIbex(target)

    #dump_uart(target.com_interface)
    found = True
    while found:
        line = target.com_interface.readline()
        if "Running sw/device/tests/" in str(line):
            found = False
            print("Found the string!")

    target.write(json.dumps("IbexFi").encode("ascii"))
    target.write(json.dumps("Init").encode("ascii"))
    #
    #target.write(json.dumps(common_library.default_core_config).encode("ascii"))
    #target.write(json.dumps(common_library.default_sensor_config).encode("ascii"))
    #target.write(json.dumps(common_library.default_alert_config,).encode("ascii"))
#
    connection_live = True
    while connection_live:
        line = target.com_interface.readline()
        if "RESP_OK" in str(line):
            print(line)
            connection_live = False

    target.write(json.dumps("IbexFi").encode("ascii"))
    target.write(json.dumps("CharRegOpLoop").encode("ascii"))
    connection_live = True
    while connection_live:
        line = target.com_interface.readline()
        if "RESP_OK" in str(line):
            print(line)

    #response = target.read_response(max_tries=100000000)
    #print(response)
    #target.write(json.dumps("IbexFi").encode("ascii"))
    #target.write(json.dumps("CharRegOpLoop").encode("ascii"))
    #connection_live = True
    #while connection_live:
    #    line = target.com_interface.readline()
    #    if "RESP_OK" in str(line):
    #        print(line)
    #        connection_live = False

    #ibexfi = OTFIIbex(target)

    #device_id, sensors, alerts, owner_page, boot_log, boot_measurements, version = (
    #        ibexfi.init(alert_config=common_library.default_fpga_friendly_alert_config)
    #)
    #print(device_id)

    #device_id_json = json.loads(device_id)
    #sensors_json = json.loads(sensors)
    #alerts_json = json.loads(alerts)
    #owner_page_json = json.loads(owner_page)
    #boot_log_json = json.loads(boot_log)
    #boot_measurements_json = json.loads(boot_measurements)

    #print(device_id)

if __name__ == '__main__':
    main()
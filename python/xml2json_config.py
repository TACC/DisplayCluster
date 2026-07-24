#!/usr/bin/env python3

# Converts a pre-Qt6 configuration.xml into the configuration.json format
# this branch uses instead (Qt6 dropped QXmlQuery/QtXmlPatterns, so config
# parsing moved to JSON). Usage:
#
#   python3 xml2json_config.py configuration.xml > configuration.json

import sys
import json
import xml.etree.ElementTree as ET

if len(sys.argv) != 2:
    print("usage: xml2json_config.py <configuration.xml>", file=sys.stderr)
    sys.exit(1)

root = ET.parse(sys.argv[1]).getroot()

dims_el = root.find("dimensions")
dimensions = {
    "numTilesWidth": int(dims_el.get("numTilesWidth")),
    "numTilesHeight": int(dims_el.get("numTilesHeight")),
    "screenWidth": int(dims_el.get("screenWidth")),
    "screenHeight": int(dims_el.get("screenHeight")),
    "mullionWidth": int(dims_el.get("mullionWidth")),
    "mullionHeight": int(dims_el.get("mullionHeight")),
    "fullscreen": int(dims_el.get("fullscreen")),
}

processes = []
for process_el in root.findall("process"):
    screens = [
        {
            "x": int(screen_el.get("x")),
            "y": int(screen_el.get("y")),
            "i": int(screen_el.get("i")),
            "j": int(screen_el.get("j")),
        }
        for screen_el in process_el.findall("screen")
    ]
    processes.append({
        "host": process_el.get("host"),
        "display": process_el.get("display"),
        "screens": screens,
    })

config = {"dimensions": dimensions, "processes": processes}
print(json.dumps(config, indent=4))

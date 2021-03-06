import argparse

parser = argparse.ArgumentParser(
    "Remove garbage output from from detect markers")  # #/markerdetector.cpp:519:detect#Scale factor=X
parser.add_argument("detected_markers", help="The detect markers output")
parser.add_argument("--removeBad", help="remove bad (-99999) detections ", action="store_true")
args = parser.parse_args()

f = open(args.detected_markers, "r")

lines = f.readlines()

# print(lines)
f.close()

f = open(args.detected_markers, "w")

for line in lines:
    if 'markerdetector' in line:
        continue
    if args.removeBad:
        if '-999999' in line:
            continue
    f.write(line)

f.close()

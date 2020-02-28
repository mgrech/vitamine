import json
import os
import sys

if len(sys.argv) != 3:
	sys.exit("invalid args")

def loadTemplate():
	with open("template.hpp", "r") as file:
		return file.read()

def loadBlocks():
	with open("blocks.json", "r") as file:
		data = file.read()

	return json.loads(data)

def constantNameFromBlockName(name):
	return "BLOCKID_" + name.upper().replace(":", "_")

def generateConstants(blocks):
	str = ""

	for name, entry in blocks.items():
		for state in entry["states"]:
			if "default" in state and state["default"]:
				str += "\n\tconstexpr BlockId {} = {};".format(constantNameFromBlockName(name), state["id"])

	return str

blockConstantOutput = loadTemplate().replace("$$$", generateConstants(loadBlocks()))

outputDirPath = os.path.join(sys.argv[2], "generated")
os.makedirs(outputDirPath, exist_ok=True)

outputFilePath = os.path.join(outputDirPath, "ids.hpp")

with open(outputFilePath, "w") as file:
	file.write(blockConstantOutput)

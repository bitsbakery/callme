import shutil
import os

def cleanProjectDir(subDir: str):    
	for garbage in ["x64","Debug","Release"]:
		shutil.rmtree(os.path.join(subDir,garbage), ignore_errors=True)

for projectDir in ["UnitTests","Benchmark","pretty"]:
	cleanProjectDir(projectDir)

for garbage in ["build","out","x64","Debug","Release"]:
	shutil.rmtree(garbage, ignore_errors=True)

#!/usr/bin/env python3
from pathlib import Path
import re
#from itertools import groupby
from io import StringIO

def updatePathContents(path, newContents):
	originalContents = ''
	try:
		with path.open('r') as tio:
			originalContents = tio.read()
	except FileNotFoundError:
		pass
	if originalContents != newContents:
		with path.open('w') as tio:
			tio.write(newContents)

class Signature(object):
	def __init__(self, name, source, linenum):
		self.source = source
		self.name = name
		self.linenum = linenum

	@property
	def prototype(self):
		return 'extern void {}(void);'.format(self.name)

	@property
	def runCall(self):
		return '\tUnityDefaultTestRun({}, "{}", {});\n'.format(self.name, self.name, self.linenum)

class TestRun(object):
	def __init__(self, path):
		self.path = path

	@property
	def cSource(self):
		return self.path.parent / self.path.stem

	@property
	def testRunFunctionName(self):
		return self.cSource.stem
		
	def signatures(self):
		pattern = re.compile('void\s+(test[A-Z]+\w*)\s*\(\s*void\s*\)')
		for index, line in enumerate(self.cSource.open()):
			match = pattern.search(line)
			if match:
				yield Signature(match.group(1), self.cSource, index)

	def computeTestRunSource(self):
		buffer = StringIO()
		buffer.write('void {}(void) {{\n'.format(self.testRunFunctionName))
		buffer.write('\tUnity.TestFile = "{}";\n'.format(self.cSource))
		for signature in self.signatures():
			buffer.write(signature.runCall);
		buffer.write('}\n');
		return buffer.getvalue()

	def update(self):
		updatePathContents(self.path, self.computeTestRunSource())

class AllTestsRun(object):
	def __init__(self, path):
		self.path = path

	def update(self):
		runFunctions = []
		for runFile in self.path.glob('*.run'):
			testRun = TestRun(runFile)
			testRun.update()
			runFunctions.append(testRun.testRunFunctionName)
		buffer = StringIO()
		buffer.write('#include "unity.h"\n\n')
		for each in runFunctions:
			buffer.write('void {}(void);\n'.format(each))
		buffer.write('\nvoid allTestsRun(void *_) {\n')
		buffer.write('\tUnityBegin("");\n')
		for each in runFunctions:
			buffer.write('\t{}();\n'.format(each))
		buffer.write('\tUnityEnd();\n')
		buffer.write('}\n');
		updatePathContents(self.path / 'allTestsRun.c', buffer.getvalue())


if __name__ == '__main__':
	AllTestsRun(Path('.')).update()


const args = Module['arguments'];
// input file name is last argument
const inputFileName = args[args.length - 1];
const outputOptionFlagIndex = args.indexOf('-o');
// if output option was given, use that as output file name, otherwise use
// input file name with .mpy file extension
const outputFileName = outputOptionFlagIndex >= 0 ?
  args[outputOptionFlagIndex + 1] : inputFileName.replace('.py', '.mpy');

Module['preRun'] = () => {
  FS.writeFile(inputFileName, Module['inputFileContents']);
};

const collectedOut = [];

Module['print'] = (out) => {
  collectedOut.push(out);
}

const collectedErr = [];

Module['printErr'] = (err) => {
  collectedErr.push(err);
}

Module['onExit'] = (status) => {
  const mpy = status === 0 ? FS.readFile(outputFileName, { encoding: 'binary' }) : undefined;
  Module['callback'](status, collectedOut.join(), collectedErr.join(), mpy);
}

Module['quit'] = () => {
  // prevent node from calling process.exit()
}

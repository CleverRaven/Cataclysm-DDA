/***
Github: https://github.com/ampersand55/CddaTools/
Usage: node cddaUpdateJsonVolume.js "c:\Cataclysm-DDA\data\json"
 ***/

const FS = require('fs');
const PATH = require('path');

const pad = (str, n = 3) => String(str).padStart(n);
const L = (str, ...args) => console.info(pad(str, 30), ...args);

const matchVolume = /( *)("(?:min_|max_)?volume": )(\d+)(,\r?\n)/g;

if (process.argv.length !== 3) {
  L('must have exactly one argument');
  L('usage: "' + process.argv[0] + '" "' + process.argv[1] + '" <.json file or root directory>');
  process.exit(0);
}

if (!exists(process.argv[2])) {
  L('path not found:', process.argv[2]);
  process.exit(0);
}

handlePath(process.argv[2]);

function exists(path) {
  return FS.existsSync(path);
}

function isDir(path) {
  return FS.statSync(path).isDirectory();
}

function isJson(path) {
  return path.endsWith('.json');
}

function handlePath(p) {
  if (isJson(p)) {
    handleFile(p);
  } else if (isDir(p)) {
    handleDir(p);
  } else if (exists(p)) {
    L('path is not .json or directory:', p);
  } else {
    L('path not found:', p);
  }
}

function handleDir(dir) {
  FS.readdir(dir, (err, files) => {
    L('processing ' + pad(files.length) + ' files in:', dir);
    files
    .map(file => dir + PATH.sep + file)
    .forEach(p => {
      isJson(p) && handleFile(p);
      isDir(p) && handleDir(p);
    });
  });
}

function handleFile(file) {
  FS.readFile(file, 'utf8', (err, data) => {
    if (err)
      throw err;

    const matches = data.match(matchVolume);
    if (!matches) {
      L('no match(es) in:', file);
      return;
    }

    const newData = data.replace(matchVolume, fixVolume);
    if (newData !== data) {
      FS.writeFile(file, newData, 'utf-8', function (err) {
        if (err)
          throw err;
        L('updating ' + pad(matches.length) + ' match(es) in:', file);
      });
    } else {
      L('"Volume" instance skipped in:', file); // musical_instrument
    }
  });
}

function fixVolume(fullMatch, whiteSpace, key, volume, EOL, offset, fullText) {
  if (fullText.includes('musical_instrument') && whiteSpace.length === 6) { // ugly and not robust, but works for now
    return fullMatch;
  }

  const liters = Number(volume) / 4;
  let volumeStr = '';
  if (Number.isInteger(liters)) {
    volumeStr = '"' + liters + ' L"';
  } else {
    volumeStr = '"' + liters * 1000 + ' ml"';
  }
  return whiteSpace + key + volumeStr + EOL;
}

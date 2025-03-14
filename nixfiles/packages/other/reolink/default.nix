asdfds{ mkYarnPackage
}:
mkYarnPackage {
  name = "reolink-ctl";
  src = ./src;
  packageJSON = ./src/package.json;
  yarnLock = ./src/yarn.lock;
  yarnNix = ./src/yarn.nix;
};

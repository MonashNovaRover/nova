self: super:

{
  rtabmap = super.rtabmap.overrideAttrs ({ propagatedBuildInputs ? [ ], ... }: {
    propagatedBuildInputs = propagatedBuildInputs ++ (with self; [
      (pcl.override { vtk = vtkWithQt5; })
      octomap
      qt5.qtbase
    ]);
  });
}

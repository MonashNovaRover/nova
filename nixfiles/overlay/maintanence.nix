self: super:

{
  rtabmap = super.rtabmap.overrideAttrs (
    {
      propagatedBuildInputs ? [ ],
      ...
    }:
    {

      # Someone convert this to a nova fork. I wil be nice and not delete this commit
      src = super.fetchFromGitHub {
        owner = "KABILAN235";
        repo = "rtabmap";
        rev = "0136372e0d58658858d53a7a51b929a613888601";
        hash = "sha256-T4/HubpDnzvxeTeKynVABUu6CGqQQA0LKAOgik1jt7k=";
      };

      propagatedBuildInputs =
        propagatedBuildInputs
        ++ (with self; [
          (pcl.override { vtk = vtkWithQt5; })
          octomap
          qt5.qtbase
          opengv
        ]);
    }
  );
}

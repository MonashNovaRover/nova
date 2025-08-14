self: super:
{
  livox-sdk2 = super.stdenv.mkDerivation {
    pname = "livox-sdk2";
    version = "0.0.0";

    src = self.fetchgit {
      name = "livox-sdk2-source";
      url = "https://github.com/Livox-SDK/Livox-SDK2";
      rev = "6a940156dd7151c3ab6a52442d86bc83613bd11b";
      hash = "sha256-NGscO/vLiQ17yQJtdPyFzhhMGE89AJ9kTL5cSun/bpU=";
    };

    nativeBuildInputs = [ super.cmake ];

  };

  rosPackages = (super.rosPackages.appendDistroOverlay(
    rosSelf: rosSuper: {

      # Add packages here:
      livox-ros-driver2 = rosSuper.buildRosPackage {
        pname = "livox-ros-driver2";
        version = "0.0.0";

        src = self.fetchgit {
          name = "livox-ros-driver2-source";
          url = "https://github.com/Livox-SDK/livox_ros_driver2";
          rev= "6b9356cadf77084619ba406e6a0eb41163b08039";
          hash = "sha256-H2HBuTDkj5kcoANZU/MKZDt94a9oUd4KO73IBPOXBeU=";
        };

        patchPhase = ''
          # mv -f launch_ROS2 launch
          # mv -f package_ROS2.xml package.xml

          # They do this:
          cp -f package_ROS2.xml package.xml
          cp -rf launch_ROS2/ launch/
        '';

        buildType = "ament_cmake";
        
        checkInputs = with rosSuper; [ ament-cmake-copyright ament-cmake-cppcheck ament-cmake-uncrustify ament-lint-auto ament-lint-common ];
        
        nativeBuildInputs = with rosSuper; [ 
          ament-cmake 
          ament-cmake-auto
        ];

        buildInputs = with rosSuper; [ 
          ament-cmake 
          std-msgs
          builtin-interfaces
          rosidl-default-generators
          super.pcl
          self.livox-sdk2
        ];
        
        propagatedBuildInputs = with rosSuper; [ 
          rclcpp rclcpp-components pluginlib std-srvs 
          std-msgs
          builtin-interfaces
        ];
      
      };

    }
  )) super.rosPackages;
}
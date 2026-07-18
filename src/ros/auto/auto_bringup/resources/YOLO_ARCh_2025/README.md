This folder contains the YOLOv11 model we used at ARCh 2025.
This YOLO model detects Cubes that are Blue, Red, Green and White.
This model was only used in RGB mode and with object_localiser.py (in `rover/nav2_autonomous/nova_object_localisation`)

Trained using roboflow dataset and ultralytics by Joel Kruger
Converted for use on OAK-D LR using https://tools.luxonis.com/
Ran using ../../launch/yolo.launch.py

To use with nixos make sure to update the auto_bringup nix file to add nixstore path to the config files:
`rover/nix/auto_bringup/default.nix`
```nix
  ...
  postInstall = ''
    # Generate absolute nix store filepaths for JSON files
    jsonFilepath="$out/share/auto_bringup/resources/YOLO_ARCh_2025/best.json"
    jsonFile=$(cat $jsonFilepath)

    updatedJsonFile=$(echo "$jsonFile" | jq --arg out "$out" '. + {
      model: {
        bin: "\($out)/share/auto_bringup/resources/YOLO_ARCh_2025/best.bin",
        model_name: "\($out)/share/auto_bringup/resources/YOLO_ARCh_2025/best_openvino_2022.1_6shave.blob",
        xml: "\($out)/share/auto_bringup/resources/YOLO_ARCh_2025/best.xml",
        zoo: "path"
      }
    }')

    echo "$updatedJsonFile" > $jsonFilepath

    # Generate absolute nix store filepaths for YAML files
    yamlFilepath="$out/share/auto_bringup/params/oak.yaml"

    yq -y -i "
      .\"/oak\".ros__parameters.nn.i_nn_config_path = \"$jsonFilepath\"
    " $yamlFilepath
  '';
```

Details here: https://www.notion.so/Cube-Detection-19db713961718033b820fc406df6697f
import React, {useCallback, useEffect, useState} from "react";
import {Button, Card, CardBody, CardHeader, Tooltip} from "@nextui-org/react";
import {useBifrost} from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../../redux/RootState.ts";
import Perspective360CamCanvas from "./Perspective360CamCanvas.tsx";
import {RosService} from "../../../../ros/services/rosService.ts";
import {Save} from "react-feather";
import ExtendedDownloadButton from "../../components/ExtendedDownload.tsx";
import SegmentedPicker from "../../components/SegmentedPicker/SegmentedPicker.tsx";
import monkey from "../../../../assets/equirectangular.png";
import Panorama360CamCanvas from "./Panorama360CamCanvas.tsx";
import HeightCalculator from "./HeightCalculator.tsx";

function numsToBlobContent(data: number[]) {
  const byteString = atob("" + data);

  // write the bytes of the string to an ArrayBuffer
  const ab = new ArrayBuffer(data.length);
  const dw = new DataView(ab);

  for (let i = 0; i < data.length; i++) {
    dw.setUint8(i, byteString.charCodeAt(i));
  }

  return [ab]
}

export type AngleType = {low: number, high: number}

const Theta360CamWidget: React.FC = () => {
  const bifrost = useBifrost({
    topic: RosTopic.THETA_360_CAM_IMAGE,
    service: RosService.THETA_360_CAM_CAPTURE,
  });

  const imageMessage = {
    data: useSelector((state: RootState) => state.theta360CamStore.data),
    format: useSelector((state: RootState) => state.theta360CamStore.format),
  };
  //const imageRef = useRef<HTMLImageElement>(new Image(5376, 2388));
  //const imageRef = useRef<HTMLVideoElement>(null);
  //useWebcam(imageRef);
  // const imageRef = useImageTexture(monkey);
  //const image = useImageTexture(monkey);

  // const imageRef = useRef<HTMLImageElement>(null);\
  const image = new Image();
  image.src= imageMessage.data.length !== 0 ? `data:image/${imageMessage.format};base64,` + imageMessage.data : monkey;

  const [angles, setAngles] = useState({low: 0, high: 0});

  // Used to select between perspective and panoramic canvases
  const [canvasIndex, setCanvasIndex] = useState<number>(0);

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  // Update the image to contain the data from imageData whenever it changes
  // TODO: Test this, and performance test to ensure no unnecessary re-renders
  // To test this use this command: 
  //  mros2 run image_publisher image_publisher_node /mnt/c/Users/Anthony/Pictures/universetemple.jpg --ros-args -r /image_raw/compressed:=/science/theta360cam/image
  // This runs master build ros2's image_publisher node which the image u specify and the compressed topic remapped to the theta360cam topic

  // When called, will capture a new image
  const capture = useCallback(() => {
    bifrost.callService({}, { successToastMessage: "360 Camera image captured", responseToast: true });
  }, [bifrost]);

  // Used to construct HUD elements shared between the two canvas types
  const canvasChildren = (
    <>
      <Button onPress={capture} color="primary">Capture</Button>
      <Tooltip content="Download source image">
        <ExtendedDownloadButton
          fileContent={() => numsToBlobContent(imageMessage.data)}
          filename={`360cam-image.${imageMessage.format}`}
          fileType={`image/${imageMessage.format}`}
          isIconOnly
          isDisabled={imageMessage.format.length === 0}
        >
          <Save></Save>
        </ExtendedDownloadButton>
      </Tooltip>
    </>
  );

  // The webgl canvas for perspective projection. Used for stratigraphic profiles
  const perspective = (
    <Perspective360CamCanvas image={image} angles={angles} setAngles={setAngles}>
      {canvasChildren}
    </Perspective360CamCanvas>
  );

  const panorama = (
    <Panorama360CamCanvas image={image} angles={angles} setAngles={setAngles}>
      {canvasChildren}
    </Panorama360CamCanvas>
  );

  return (
    <Card className="absolute bottom-3 top-20 left-3 right-3">
      <CardHeader className="pb-0 flex flex-row gap-3">
        <div className="flex-grow">360 Camera</div>
        <SegmentedPicker selectedIndex={canvasIndex} onIndexChange={setCanvasIndex}>
          <>Perspective</>
          <>Panorama</>
        </SegmentedPicker>
      </CardHeader>
      <CardBody className="flex flex-col">
        { canvasIndex === 0 ? perspective : panorama }
        <HeightCalculator angles={angles} setAngles={setAngles}/>
      </CardBody>
    </Card>
  )
}

export default Theta360CamWidget;
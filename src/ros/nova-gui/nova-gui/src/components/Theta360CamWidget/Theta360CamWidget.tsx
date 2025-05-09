import React, {useCallback, useEffect, useLayoutEffect, useState} from "react";
import {Button, Card, CardBody, CardHeader, Tooltip} from "@nextui-org/react";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import Perspective360CamCanvas from "./Perspective360CamCanvas.tsx";
import {RosService} from "../../ros/services/rosService.ts";
import {Save} from "react-feather";
import ExtendedDownloadButton from "../shared/ExtendedDownload.tsx";
import SegmentedPicker from "../SegmentedPicker/SegmentedPicker.tsx";
import monkey from "../../assets/equirectangular.png";
import Panorama360CamCanvas from "./Panorama360CamCanvas.tsx";

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


const Theta360CamWidget: React.FC = () => {
  const bifrost = useBifrost({
    topic: RosTopic.THETA_360_CAM_IMAGE,
    service: RosService.THETA_360_CAM_CAPTURE,
  });

  const imageMessage = useSelector((state: RootState) => state.theta360CamStore);
  //const imageRef = useRef<HTMLImageElement>(new Image(5376, 2388));
  //const imageRef = useRef<HTMLVideoElement>(null);
  //useWebcam(imageRef);
  // const imageRef = useImageTexture(monkey);
  //const image = useImageTexture(monkey);

  // const imageRef = useRef<HTMLImageElement>(null);\
  const [image, setImage] = useState<HTMLImageElement>(() => new Image())

  const url = monkey;

  useLayoutEffect(() => {
    image.src = url;
  }, []);

  // Used to select between perspective and panoramic canvases
  const [canvasIndex, setCanvasIndex] = useState<number>(0);

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  // Update the image to contain the data from imageData whenever it changes
  // TODO: Test this, and performance test to ensure no unnecessary re-renders
  useEffect(() => {
    if (imageMessage.data.length == 0)
      return;

    const newImage = new Image();
    newImage.src = `data:image/${imageMessage.format};base64,` + imageMessage.data;
    setImage(newImage);
  }, [imageMessage]);

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
    <Perspective360CamCanvas image={image}>
      {canvasChildren}
    </Perspective360CamCanvas>
  );

  const panorama = (
    <Panorama360CamCanvas image={image}>
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
      </CardBody>
    </Card>
  )
}

export default Theta360CamWidget;








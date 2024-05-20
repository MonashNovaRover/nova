import React, {useCallback, useEffect, useRef} from "react";
import {Button, Card, CardBody, CardHeader, Tooltip} from "@nextui-org/react";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import Perspective360CamCanvas from "./Perspective360CamCanvas.tsx";
import {RosService} from "../../ros/services/rosService.ts";
import {Save} from "react-feather";
import ExtendedDownloadButton from "../shared/ExtendedDownload.tsx";

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
  const imageRef = useRef<HTMLImageElement>(new Image(5376, 2388));

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  // Update the image to contain the data from imageData whenever it changes
  useEffect(() => {
    if (!imageRef.current)
      return;

    imageRef.current.src = `data:image/${imageMessage.format};base64,` + imageMessage.data
  }, [imageMessage, imageRef]);

  // When called, will capture a new image
  const capture = useCallback(() => {
    bifrost.callService({}, { successToastMessage: "360 Camera image captured", responseToast: true });
  }, [bifrost]);

  return (
    <Card>
      <CardHeader className="pb-0">
        360 Camera
      </CardHeader>
      <CardBody className="">
        <Perspective360CamCanvas image={imageRef.current}>
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
        </Perspective360CamCanvas>
      </CardBody>
    </Card>
  )
}

export default Theta360CamWidget;








import React, {useCallback, useEffect, useRef} from "react";
import {Button, Card, CardBody, CardHeader} from "@nextui-org/react";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import Perspective360CamCanvas from "./Perspective360CamCanvas.tsx";
import {RosService} from "../../ros/services/rosService.ts";


const Theta360CamWidget: React.FC = () => {
  const bifrost = useBifrost({
    topic: RosTopic.THETA_360_CAM_IMAGE,
    service: RosService.THETA_360_CAM_CAPTURE,
  });
  const imageMessage = useSelector((state: RootState) => state.theta360CamStore);
  // const imageRef = useRef<HTMLImageElement>(null);
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
  }, [bifrost])


  // <img ref={imageRef} alt="360 Camera Image"></img>
  return (
    <Card>
      <CardHeader className="pb-0">
        360 Camera
      </CardHeader>
      <CardBody className="">
        <Perspective360CamCanvas image={imageRef.current}>
          <Button onPress={capture}>Capture</Button>
        </Perspective360CamCanvas>
      </CardBody>
    </Card>
  )
}

export default Theta360CamWidget;








import React, { useCallback, useState, useEffect } from "react";
import { Button, Input, CardProps, Card } from "@nextui-org/react";
import { useSelector } from "react-redux";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../ros/services/rosService.ts";
import { Square, Power, HelpCircle } from "react-feather";
import { OverlayedProgress } from "../OverlayedProgress/OverlayedProgress.tsx";
import { RosTopic } from "../../ros/topics/rosTopic.ts";
import { RootState } from "../../redux/RootState";
import KeyboardOverlayedCameraComponent from "../CameraComponent/special/KeyboardOverlayedCameraComponent.tsx";

interface IAutoTypingKeyEntryWidgetProps extends CardProps {
  showHelp: () => void;
  cameraSerial: string;
}

/**
 * Widget which takes input and controls the auto typing sequencer node.
 * @param cardProps
 * @constructor
 */
const AutoTypingKeyEntryWidget: React.FC<IAutoTypingKeyEntryWidgetProps> = (props) => {
  const startTypingBifrost = useBifrost({ service: RosService.START_AUTO_TYPING });
  const stopTypingBifrost = useBifrost({ service: RosService.STOP_AUTO_TYPING });
  const sequenceBifrost = useBifrost({ topic: RosTopic.TYPE_SEQUENCE})
  const overrideCornersBifrost = useBifrost({ service: RosService.OVERRIDE_CORNERS });

  const startTyping = (sequence: Array<string>) => startTypingBifrost.callServiceToRedux({ sequence: sequence });
  const stopTyping = () => stopTypingBifrost.callServiceToRedux({});

  const fullSequence = useSelector((state: RootState) => state.sequencerDataStore.sequence);
  const partialSequence = useSelector((state: RootState) => state.sequencerDataStore.partial_sequence);
  const currentKey = useSelector((state: RootState) => state.sequencerDataStore.current_key);

  useEffect(() => {
    sequenceBifrost.syncWithTopic();
  }, [sequenceBifrost]);

  const [sequence, setSequence] = useState([""]);
  const onInput = useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setSequence(event.target.value.replace(/\s+/g, ' ').trim().split(" ")) // split string by spaces
  }, [])

  const bifrost = useBifrost({ topic: RosTopic.KEYBOARD_DATA });
  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const keyboardPoints = useSelector((state: RootState) => state.keyboardDataStore.points);
  const camera_width = useSelector((state: RootState) => state.keyboardDataStore.width);
  const camera_height = useSelector((state: RootState) => state.keyboardDataStore.height);

  const [corners, setCorners] = useState([0, 0 ,0, 0, 0, 0, 0, 0]);
  const onCornerInput = useCallback((i: number) => (event: React.ChangeEvent<HTMLInputElement>) => {
    setCorners(prev => {
      const updated = [...prev]
      updated[i]=Number(event.target.value.trim())
      return updated
    })
  }, [])

  const overrideCorners = (corners: number[]) => overrideCornersBifrost.callServiceToRedux({ corners: corners });


  return (
    <div className="grid gap-y-3">
      <KeyboardOverlayedCameraComponent 
        cameraSerial={props.cameraSerial} 
        camera_width={camera_width} 
        camera_height={camera_height} 
        keyboardPoints={keyboardPoints} 
        overridePoints={corners}/>
      <Card className="row-4 gap-2 p-2">
        <div className="flex ">Separate keys with&nbsp;<b>spaces.</b>&nbsp;Type as seen on keyboard in&nbsp;<b>lowercase.</b>&nbsp;Click the&nbsp;<b>?</b>&nbsp;button for key info. Separate x,y with&nbsp;<b>space</b>.</div>
        <div className="flex flex-row justify-between gap-2">
          <Input size="lg" className="flex-grow w-6/12"
            placeholder="What do you want to type?"
            onChange={onInput}>
          </Input>
        </div>
        <div className="flex flex-row justify-between gap-2">
          <Button className="w-3/12 text-h1 h-12" color="success"
            onClick={() => startTyping(sequence)}>
            START TYPING
            <Power size="15" />
          </Button>
          <div className="w-5/12 content-center">
          <OverlayedProgress size="lg" radius="lg"
            value={partialSequence.length}
            maxValue={fullSequence.length}
            aria-label="Key Sequence"
            autoColor={false}
            disableAnimation={false}
            classNames={{
              indicator: "h-12",
              track: "h-full",  
            }}>
            <div className="grid grid-flow-col gap-3 auto-cols-fr text-large">
              <span>{partialSequence.join(" ")} <b>{currentKey}</b></span>
            </div>
          </OverlayedProgress>
          </div>
          <Button className="w-3/12 text-h1 h-12" color="danger"
            onClick={stopTyping}>
            STOP TYPING
            <Square size="15" fill="white" />
          </Button>
          <Button className="w-1/12 text-h1 h-12" color="primary"
            onClick={props.showHelp}>
            <HelpCircle />
          </Button>
        </div>
        <div className="flex flex-row gap-2">
          Top left:
          <Input size="lg" className="w-1/14"
            placeholder={keyboardPoints[0].toString()}
            onChange={onCornerInput(0)}>
          </Input>
          <Input size="lg" className="w-1/14"
            placeholder={keyboardPoints[1].toString()}
            onChange={onCornerInput(1)}>
          </Input>
          Top right:
          <Input size="lg" className="w-1/14"
            placeholder={keyboardPoints[2].toString()}
            onChange={onCornerInput(2)}>
          </Input>
          <Input size="lg" className="w-1/14"
            placeholder={keyboardPoints[3].toString()}
            onChange={onCornerInput(3)}>
          </Input>
          Bottom right:
          <Input size="lg" className="w-1/14"
            placeholder={keyboardPoints[4].toString()}
            onChange={onCornerInput(4)}>
          </Input>
          <Input size="lg" className="w-1/14"
            placeholder={keyboardPoints[5].toString()}
            onChange={onCornerInput(5)}>
          </Input>
          Bottom left:
          <Input size="lg" className="w-1/14"
            placeholder={keyboardPoints[6].toString()}
            onChange={onCornerInput(6)}>
          </Input>
          <Input size="lg" className="w-1/14"
            placeholder={keyboardPoints[7].toString()}
            onChange={onCornerInput(7)}>
          </Input>
          <Button className=" flex-grow w-1/4 text-h1 h-12" color="warning"
            onClick={() => overrideCorners(corners)}>
            Override<br/>Localiser
          </Button>
        </div>
      </Card>
    </div>
  );
}

export default AutoTypingKeyEntryWidget;


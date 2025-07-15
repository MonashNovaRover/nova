import React, { useCallback, useState, useEffect } from "react";
import { Button, Input, CardProps, Card } from "@nextui-org/react";
import { useSelector } from "react-redux";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { Square, Power, HelpCircle } from "react-feather";
import { OverlayedProgress } from "../../shared/components/OverlayedProgress/OverlayedProgress.tsx";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { RootState } from "../../../redux/RootState.ts";

interface IAutoTypingKeyEntryWidgetProps extends CardProps {
  showHelp: () => void;
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

  const startTyping = (sequence: Array<string>) => startTypingBifrost.callServiceToRedux({ sequence: sequence });
  const stopTyping = () => stopTypingBifrost.callServiceToRedux({});

  const fullSequence = useSelector((state: RootState) => state.sequencerDataStore.sequence);
  const partialSequence = useSelector((state: RootState) => state.sequencerDataStore.partial_sequence);
  const currentKey = useSelector((state: RootState) => state.sequencerDataStore.current_key);

  useEffect(() => {
    sequenceBifrost.syncWithTopic();
  }, [sequenceBifrost]);

  const onInput = useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setSequence(event.target.value.replace(/\s+/g, ' ').trim().split(" ")) // split string by spaces
  }, [])
  const [sequence, setSequence] = useState([""]);


  return (
    <Card className="row-3 gap-2 p-2">
      <div className="flex ">Separate keys with&nbsp;<b>spaces.</b>&nbsp;Type as seen on keyboard in&nbsp;<b>lowercase.</b>&nbsp;Click the&nbsp;<b>?</b>&nbsp;button for key info.</div>
      <div className="flex flex-row justify-between points: [0, 0, 0, 0, 0, 0, 0, 0],gap-2">
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
    </Card>
  );
}

export default AutoTypingKeyEntryWidget;


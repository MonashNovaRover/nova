import React, { useCallback, useState, useEffect, useRef } from "react";
import { Button, Input, CardProps, Card, Checkbox } from "@nextui-org/react";
import { useSelector } from "react-redux";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { Square, Power, HelpCircle } from "react-feather";
import toast from "react-hot-toast";
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
  const sequenceBifrost = useBifrost({ topic: RosTopic.TYPE_SEQUENCE })

  const startTyping = (sequence: Array<string>, relocalise: boolean) => startTypingBifrost.callServiceToRedux({ sequence: sequence, relocalise: relocalise });
  const stopTyping = () => stopTypingBifrost.callServiceToRedux({});

  const fullSequence = useSelector((state: RootState) => state.sequencerDataStore.sequence);
  const partialSequence = useSelector((state: RootState) => state.sequencerDataStore.partial_sequence);
  const currentKey = useSelector((state: RootState) => state.sequencerDataStore.current_key);
  const error = useSelector((state: RootState) => state.sequencerDataStore.error);

  const prevErrorRef = useRef<string>("");
  useEffect(() => {
    if (error && error !== prevErrorRef.current) {
      toast.error(error);
    }
    prevErrorRef.current = error;
  }, [error]);

  useEffect(() => {
    sequenceBifrost.syncWithTopic();
  }, [sequenceBifrost]);

  const isRunning = currentKey !== "";

  const [sequence, setSequence] = useState([""]);
  const [relocalise, setRelocalise] = useState(false);
  const onInput = useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setSequence(event.target.value.replace(/\s+/g, ' ').trim().split(" ")) // split string by spaces
  }, [])



  return (
    <Card className="row-3 gap-2 p-2">
      <div className="flex ">Separate keys with&nbsp;<b>spaces.</b>&nbsp;Type as seen on keyboard in&nbsp;<b>lowercase.</b>&nbsp;Click the&nbsp;<b>?</b>&nbsp;button for key info.</div>
      <div className="flex flex-row justify-between gap-2">
        <Input size="lg" className="flex-grow"
          placeholder="What do you want to type?"
          onChange={onInput}>
        </Input>
        <Checkbox
          size="lg"
          isSelected={relocalise}
          onValueChange={setRelocalise}
          className="w-4/12 justify-center"
        >
          Relocalise
        </Checkbox>
      </div>
      <div className="flex flex-row justify-between gap-2">
        <Button
          isDisabled
          className={`w-3/12 h-12 opacity-100 ${isRunning ? "bg-success" : "bg-content3"}`}>
          {isRunning ? "RUNNING" : "IDLE"}
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
            <div className="flex w-full text-large">
              {fullSequence.map((key, i) => (
                <span key={i} className="flex-1 text-center" style={{
                  fontWeight: key === currentKey ? 'bold' : 'normal',
                }}>
                  {key}
                </span>
              ))}
            </div>
          </OverlayedProgress>
        </div>
        <Button className="w-3/12 text-h1 h-12" color="primary"
          onPress={() => isRunning ? stopTyping() : startTyping(sequence, relocalise)}>
          {isRunning ? "STOP TYPING" : "START TYPING"}
          {isRunning ? <Square size="15" fill="white" /> : <Power size="15" />}
        </Button>
        <Button className="w-1/12 text-h1 h-12" color="primary"
          onPress={props.showHelp}>
          <HelpCircle />
        </Button>
      </div>
    </Card>
  );
}

export default AutoTypingKeyEntryWidget;


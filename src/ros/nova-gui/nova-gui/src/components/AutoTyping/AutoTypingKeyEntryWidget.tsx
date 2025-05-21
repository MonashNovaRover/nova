import React, {useCallback, useState} from "react";
import {Button, Input, CardProps} from "@nextui-org/react";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../ros/services/rosService.ts";
import { Square, Power } from "react-feather";

interface IAutoTypingKeyEntryWidgetProps extends CardProps {
}

/**
 * Widget controlling auto typing transforms on service "/arm/keyboard/transform_toggle"
 * @param cardProps
 * @constructor
 */
const AutoTypingKeyEntryWidget: React.FC<IAutoTypingKeyEntryWidgetProps> = () => {
  const startTypingBifrost = useBifrost({ service: RosService.START_AUTO_TYPING });
  const stopTypingBifrost = useBifrost({ service: RosService.STOP_AUTO_TYPING });

  const startTyping = (sequence:string) => startTypingBifrost.callServiceToRedux({sequence: sequence});
  const stopTyping = () => stopTypingBifrost.callServiceToRedux({});
  const onInput = useCallback((event: React.ChangeEvent<HTMLInputElement>)=>{
    setSequence(event.target.value)
  }, [])
  const [sequence, setSequence] = useState("");
  return (
    <div className="flex flex-row justify-between gap-5">
      <Button className="w-1/3 text-h1 h-12" color="primary" 
        onClick={() => startTyping(sequence)}>
          START TYPING
          <Power size="15"/>
      </Button>
      <Input size="lg" className="flex-grow w-1/3" 
        placeholder="What do you want to type?" 
        onChange={onInput}>
      </Input>
      <Button className="w-1/3 text-h1 h-12" color="danger"
        onClick={stopTyping}>
          STOP TYPING
          <Square size="15" fill="white"/>
      </Button>
    </div>
  );
}

export default AutoTypingKeyEntryWidget;

    
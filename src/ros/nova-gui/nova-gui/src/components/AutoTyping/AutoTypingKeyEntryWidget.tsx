import React, {useCallback, useState} from "react";
import {Button, Input, CardProps, Card} from "@nextui-org/react";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../ros/services/rosService.ts";
import { Square, Power, HelpCircle } from "react-feather";

interface IAutoTypingKeyEntryWidgetProps extends CardProps {
  showHelp: ()=>void;
}

/**
 * Widget which takes input and controls the auto typing sequencer node.
 * @param cardProps
 * @constructor
 */
const AutoTypingKeyEntryWidget: React.FC<IAutoTypingKeyEntryWidgetProps> = (props) => {
  const startTypingBifrost = useBifrost({ service: RosService.START_AUTO_TYPING });
  const stopTypingBifrost = useBifrost({ service: RosService.STOP_AUTO_TYPING });

  const startTyping = (sequence:Array<string>) => startTypingBifrost.callServiceToRedux({sequence: sequence});
  const stopTyping = () => stopTypingBifrost.callServiceToRedux({});
  const onInput = useCallback((event: React.ChangeEvent<HTMLInputElement>)=>{
    setSequence(event.target.value.replace(/\s+/g, ' ').trim().split(" ")) // split string by spaces
  }, [])
  const [sequence, setSequence] = useState([""]);


  return (
    <Card className="row-2 gap-2 p-2">
      <div className="flex ">Separate keys with&nbsp;<b>spaces.</b>&nbsp;Type as seen on keyboard in&nbsp;<b>lowercase.</b>&nbsp;Click the&nbsp;<b>?</b>&nbsp;button for key info.</div> 
      <div className="flex flex-row justify-between gap-2">
        <Button className="w-3/12 text-h1 h-12" color="success" 
          onClick={() => startTyping(sequence)}>
            START TYPING
            <Power size="15"/>
        </Button>
        <Input size="lg" className="flex-grow w-6/12" 
          placeholder="What do you want to type?" 
          onChange={onInput}>
        </Input>
        <Button className="w-3/12 text-h1 h-12" color="danger"
          onClick={stopTyping}>
            STOP TYPING
            <Square size="15" fill="white"/>
        </Button>
        <Button className="w-1/12 text-h1 h-12" color="primary"
          onClick={props.showHelp}>
            <HelpCircle/>
        </Button>
      </div>
    </Card>
  );
}

export default AutoTypingKeyEntryWidget;

    
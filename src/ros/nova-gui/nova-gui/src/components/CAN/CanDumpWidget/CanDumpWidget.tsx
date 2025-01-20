import {FC, useCallback, useEffect, useState} from "react";
import {Button, Card, CardBody, CardHeader, Textarea} from "@nextui-org/react";
import DownloadButton from "../../shared/Download.tsx";
import {Download} from "react-feather";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import {IRosNovaInterfacesCanMessage} from "../../../ros/rosTypes.ts";

type CanMessage = IRosNovaInterfacesCanMessage;

const MAX_MESSAGES = 50;

const CanDumpWidget: FC = () => {
  const bifrost = useBifrost({ topic: RosTopic.CAN_MESSAGE, service: RosService.SEND_CAN_MESSAGE });

  const [messages, setMessages] = useState<CanMessage[]>([]);

  const handleMessage = useCallback((message: CanMessage) => {
    setMessages((prevState) => {
      // Append message, and remove the first message in the array if we reached MAX_MESSAGES
      if (prevState.length < MAX_MESSAGES)
        return [...prevState, message]
      return [...prevState.slice(1), message]
    })
  }, [setMessages]);

  useEffect(() => {
    bifrost.syncWithTopic({handleMessage: (message) => handleMessage(message as CanMessage)});
  }, [bifrost, handleMessage]);

  const text = (
    messages.map((msg: CanMessage) => (`${msg.id.toString(16)} # ${
      msg.data.map(x => x.toString(16)).join()
    }\n`)).join()
  )
  return (
    <Card>
      <CardHeader className="pb-0">CAN Dump</CardHeader>
      <CardBody className="flex flex-col gap-2.5">
        <Textarea
          isReadOnly
          variant="bordered"
          labelPlacement="outside"
          placeholder="Output from the RFID scanner goes here"
          value={text}
          className="font-mono w-full flex-grow"
          fullWidth
        />

        <div className="flex flex-row justify-end gap-2">
          <Button className="grow" >
            Read
          </Button>
          <DownloadButton
            className="w-1/4"
            content={text}
            filename="can.txt"
          >
            <Download />
            Save
          </DownloadButton>
        </div>
      </CardBody>
    </Card>
  );
};

export default CanDumpWidget;
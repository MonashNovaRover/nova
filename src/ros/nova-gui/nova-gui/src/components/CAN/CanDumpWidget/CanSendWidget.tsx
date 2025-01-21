import {FC, useCallback, useEffect, useRef, useState} from "react";
import {Button, Card, CardBody, CardHeader, Input, Textarea} from "@nextui-org/react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import {IRosNovaInterfacesSendCanMessageRequest} from "../../../ros/rosTypes.ts";
import ContentEditable from "../../shared/ContentEditable/ContentEditable.tsx";
import Overlay from "../../shared/Overlay/Overlay.tsx";
import useContextEditable from "../../shared/ContentEditable/useContentEditable.ts";

const CanSendWidget: FC = () => {
  const bifrost = useBifrost({topic: RosTopic.CAN_MESSAGE, service: RosService.SEND_CAN_MESSAGE});

  const [canId, setCanId] = useState<string>("");
  const [data, setDataRaw] = useState<string>("");
  const [delimiter, setDelimiter] = useState<string>("");

  const setData = (value: string) => {
    const filteredValue = value.toUpperCase().replace(/[^A-F0-9]/g, "");
    setDataRaw(filteredValue.length > 16 ? filteredValue.slice(0, 16) : filteredValue);
  }


  const setFrame = (value: string) => {

    const sectionsRaw = value.split(/(?=#)/, 2).filter(x => x !== undefined);
    const sections = [...sectionsRaw, ...([...Array(2 - sectionsRaw.length)].map(() => ''))];

    console.log(sectionsRaw)
    console.log(sections)
    const frameCanId = sections[0].length > 3 ? sections[0].slice(0,3) : sections[0];
    const frameData = (sections[0].length > 3 ? sections[0].slice(3) : '') + sections[1].replace('#', '')

    const frameDelimiter = sections[1].startsWith('#') || frameData.length > 0 ? '#' : ''

    setCanId(frameCanId.toUpperCase().replace(/[^A-F0-9]/g, ""))
    setDelimiter(frameDelimiter);
    setData(frameData);
  }

  const splitData = data.match(/.{1,2}/g)?.map(match => match.toString()) ?? [];

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const send = useCallback(() => {
    const msg = {
      bus: 0,
      id: Number(`0x${canId}`),
      data: splitData.map(chunk => Number(`0x${chunk}`)),
    } as IRosNovaInterfacesSendCanMessageRequest;

    bifrost.callService(msg);
  }, [bifrost, canId, data]);

  const dataSpan = (
    <>
      {splitData.map(chunk => (
        <span className="ps-[0.5ch] font-mono">
          {chunk}
        </span>
      ))}
      {
        splitData.length === 0 && <span className="w-min-4">{" "}</span>
      }
    </>
  );

  const inputField = (
    <Overlay
      className="font-mono bg-default-100 px-3 rounded-xl grow h-10"

      overlay={
        <div className="flex flex-row items-center w-full px-3 pt-0.5 h-full text-mono text-small">
          <span className="text-default-400 me-[0.25ch] text-small font-mono">0x</span>
          <span className="mx-[0.25ch] w-[3.0ch] inline-flex justify-start text-foreground-500">
            {"0".repeat(3 - canId.length)}
          </span>
          <span className={"text-default-400 ms-[0.25ch] " + (delimiter.length > 0 ? "opacity-0" : "")}>#</span>
          <span className="opacity-0">{dataSpan}</span>
          {data.length === 0 && (
            <span className={"text-foreground-500 ps-[0.5ch]"}>00</span>
          )}
          {data.length % 2 !== 0 && (
            <span className={"text-foreground-500"}>0</span>
          )}
        </div>
      }
      noCentering
    >
      <div className="flex flex-row items-center h-10 pt-0.5">
        <span className="text-default-400 me-[0.25ch] text-small font-mono">0x</span>
        <ContentEditable
          className="text-mono text-small grow items-center block outline-none"
          autocorrect="off" autocapitalize="off" spellcheck="false"
          onValueChange={setFrame}
        >
              <span className="mx-[0.25ch] w-[3.0ch] inline-flex justify-end">
                {canId}
              </span>
          {delimiter.length > 0 && (
            <span className="text-foreground-500 ms-[0.25ch]">{delimiter}</span>
          )}
          {dataSpan}
        </ContentEditable>
      </div>
    </Overlay>
  );

  return (
    <Card className="col-span-2">
      <CardHeader className="pb-0">CAN Send</CardHeader>
      <CardBody className="flex flex-col gap-2.5">
        <div className="flex flex-row justify-end gap-2">
          {inputField}
          <Button
            className="w-1/4"
            onPress={send}
          >
            Send
          </Button>
        </div>
      </CardBody>
    </Card>
  );
};

export default CanSendWidget;
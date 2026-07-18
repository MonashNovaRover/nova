import {
  Button,
  Card,
  CardBody,
  CardHeader,
  CardProps,
  Textarea,
} from "@nextui-org/react";
import React, { useEffect } from "react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { IRosStdSrvsTriggerResponse } from "../../../ros/rosTypes.ts";
import toast from "react-hot-toast";
import DownloadButton from "../../shared/components/Download.tsx";
import { Download } from "react-feather";

const RFIDWidget: React.FC<CardProps> = (props) => {
  const bifrost = useBifrost({
    topic: RosTopic.RFID_DATA,
    service: RosService.READ_RFID,
  });
  const rfidData = useSelector((state: RootState) => state.rfidDataStore.data);

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const read = () => {
    bifrost.callService(
      {},
      {
        // TODO: Fix typing for the handle response function
        // eslint-disable-next-line @typescript-eslint/ban-ts-comment
        // @ts-ignore
        handleResponse: (response: IRosStdSrvsTriggerResponse) => {
          if (response.success)
            toast.success(
              response.message.length > 0
                ? response.message
                : "RFID request succeeded!"
            );
          else
            toast.error(
              response.message.length > 0
                ? response.message
                : "RFID request failed!"
            );
        },
      }
    );
  };

  return (
    <Card {...props}>
      <CardHeader className="pb-0">RFID</CardHeader>
      <CardBody className="flex flex-col gap-2.5">
        <Textarea
          isReadOnly
          variant="bordered"
          labelPlacement="outside"
          placeholder="Output from the RFID scanner goes here"
          value={rfidData}
          className="font-mono w-full flex-grow"
          fullWidth
        />

        <div className="flex flex-row justify-end gap-2">
          <Button className="grow" onPress={read}>
            Read
          </Button>
          <DownloadButton
            className="w-1/4"
            content={rfidData}
            filename="rfid_data.txt"
          >
            <Download />
            Save
          </DownloadButton>
        </div>
      </CardBody>
    </Card>
  );
};

export default RFIDWidget;

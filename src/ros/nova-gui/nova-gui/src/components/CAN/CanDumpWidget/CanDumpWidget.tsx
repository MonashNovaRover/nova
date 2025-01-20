import {FC} from "react";
import {Button, Card, CardBody, CardHeader, Textarea} from "@nextui-org/react";
import DownloadButton from "../../shared/Download.tsx";
import {Download} from "react-feather";

const CanDumpWidget: FC = () => {
  const text = "";

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
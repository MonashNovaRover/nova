import React from "react";
import {Button, Card, CardBody, CardHeader, Input} from "@nextui-org/react";

const ToolRotatorWidget: React.FC = () => {


  const section = (name: string) => (
    <div className="flex flex-col gap-3">
      <Input
        type="number"
        label={name}
        labelPlacement="outside"
        endContent={
          <div className="pointer-events-none flex items-center">
            <span className="text-default-400 text-small">°</span>
          </div>
        }
        value={"0"}
      />

      <div className="grid grid-cols-2 gap-3">
        <Button color="primary">Set Preset</Button>
        <Button>Go To</Button>
      </div>
    </div>
  )

  return (
    <Card>
      <CardHeader>
        Tool Rotator
      </CardHeader>

      <CardBody>
        <div className="grid grid-cols-3 gap-3">
          {section("Microscope")}
          {section("NIR Probe")}
          {section("Sweeper")}
        </div>

        <div>

        </div>
      </CardBody>
    </Card>
  )
}

export default ToolRotatorWidget
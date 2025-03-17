import React from "react";
import {CameraComponent, CameraComponentProps} from "../CameraComponent/CameraComponent.tsx";
import {Card, CardBody, Input, Tab, Tabs} from "@nextui-org/react";
import MicroscopeThresholdWidget from "./MicroscopeThresholdWidget.tsx";
import {CameraSerials} from "../../views/shared/CamerasPage/CameraPageConstants.tsx";
import {useGenericStore} from "../../hooks/useGenericStore.ts";

const MicroscopeWidget: React.FC<CameraComponentProps> = () => {
  const [ilmeniteMLResult, setIlmeniteMLResult] = useGenericStore<string>("ilmeniteMLResult")

  return (
    <Card>
      <CardBody>
        <Tabs
          aria-label="NIR-Probe-Options"
          classNames={{
            tabList: "gap-6 w-full relative rounded-none p-0 border-b border-divider",
            cursor: "w-full bg-[#22d3ee]",
            tab: "max-w-fit px-0 h-12",
            tabContent: "group-data-[selected=true]:text-[#06b6d4]",
          }}
          color="primary"
          variant="underlined"
        >
          <Tab title="Camera Feed">
            <CameraComponent cameraSerial={CameraSerials.SCIENCE_MICROSCOPE}/>
            <Input
              className="px-1 pt-4"
              label="ML Output"
              labelPlacement="outside"
              value={ilmeniteMLResult}
              onValueChange={setIlmeniteMLResult}
            />
          </Tab>
          <Tab title="Thresholding">
            <MicroscopeThresholdWidget cameraSerial={CameraSerials.SCIENCE_MICROSCOPE}/>
          </Tab>
        </Tabs>
      </CardBody>
    </Card>
  )
}

export default MicroscopeWidget

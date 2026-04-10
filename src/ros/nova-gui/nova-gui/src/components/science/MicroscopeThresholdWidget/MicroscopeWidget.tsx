import React from "react";
import {CameraComponentProps} from "../../cameras/CameraComponent/CameraComponent.tsx";
import {Card, CardBody, Tab, Tabs} from "@nextui-org/react";
import MicroscopeThresholdWidget from "./MicroscopeThresholdWidget.tsx";
import {CameraSerials} from "../../../views/shared/CamerasPage/CameraPageConstants.tsx";
import MicroscopeCamerasWidget from "./MicroscopeCamerasWidget.tsx";
import {ARCNIRProbeWidgetOneCol} from "../NIRProbe/ARCNIRProbeWidget.tsx";

const MicroscopeWidget: React.FC<CameraComponentProps> = () => {

  return (
    <Card>
      <CardBody>
        <Tabs
          aria-label="NIR-Probe-Options"
          classNames={{
            tabList: "gap-6 w-full relative rounded-none p-0 border-b border-divider",
            cursor: "w-full bg-[#ECBAC4]",
            tab: "max-w-fit px-0 h-12",
            tabContent: "group-data-[selected=true]:text-[#ECBAC4]",
          }}
          color="primary"
          variant="underlined"
        >
          <Tab title="NIR Probe" className="p-0 pt-3">
            <ARCNIRProbeWidgetOneCol/>
          </Tab>
          <Tab title="Camera Feed" className="p-0 pt-3">
            <MicroscopeCamerasWidget/>
          </Tab>
          <Tab title="Thresholding" className="p-0 pt-3">
            <MicroscopeThresholdWidget cameraSerial={CameraSerials.SCIENCE_MICROSCOPE}/>
          </Tab>
        </Tabs>
      </CardBody>
    </Card>
  )
}

export default MicroscopeWidget

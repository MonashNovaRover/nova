import React from "react";
import {CameraComponentProps} from "../../cameras/CameraComponent/CameraComponent.tsx";
import {Card, CardContent, Tab, TabList, TabPanel, Tabs} from "@heroui/react";
import MicroscopeThresholdWidget from "./MicroscopeThresholdWidget.tsx";
import {CameraSerials} from "../../../views/shared/CamerasPage/CameraViewConstants.tsx";
import MicroscopeCamerasWidget from "./MicroscopeCamerasWidget.tsx";
import {ARCNIRProbeWidgetOneCol} from "../NIRProbe/ARCNIRProbeWidget.tsx";

const MicroscopeWidget: React.FC<CameraComponentProps> = () => {

  return (
    <Card>
      <CardContent>
        <Tabs
          aria-label="NIR-Probe-Options"
          variant="primary"
        >
          <TabList className="gap-6 w-full relative rounded-none p-0 border-b border-divider">
            <Tab id="nir-probe">NIR Probe</Tab>
            <Tab id="camera-feed">Camera Feed</Tab>
            <Tab id="thresholding">Thresholding</Tab>
          </TabList>
          <TabPanel id="nir-probe" className="p-0 pt-3">
            <ARCNIRProbeWidgetOneCol/>
          </TabPanel>
          <TabPanel id="camera-feed" className="p-0 pt-3">
            <MicroscopeCamerasWidget/>
          </TabPanel>
          <TabPanel id="thresholding" className="p-0 pt-3">
            <MicroscopeThresholdWidget cameraSerial={CameraSerials.SCIENCE_MICROSCOPE}/>
          </TabPanel>
        </Tabs>
      </CardContent>
    </Card>
  )
}

export default MicroscopeWidget

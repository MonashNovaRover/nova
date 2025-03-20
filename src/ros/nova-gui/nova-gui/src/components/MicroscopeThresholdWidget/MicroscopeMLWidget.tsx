import {CameraComponent} from "../CameraComponent/CameraComponent.tsx";
import {CameraSerials} from "../../views/shared/CamerasPage/CameraPageConstants.tsx";
import {Button, Input} from "@nextui-org/react";
import {useGenericStore} from "../../hooks/useGenericStore.ts";
import {Site} from "../../redux/models/genericStores/CurrentSiteStore.ts";
import {SiteDataState} from "../../redux/models/genericStores/SiteDataState.ts";
import {useCallback} from "react";
import SiteSelectWidget from "../SiteSelectWidget/SiteSelectWidget.tsx";

/**
 * Widget for microscope camera feed and ML output
 * @constructor
 */
function MicroscopeMLWidget () {
  // current site as provided by the site selector
  const [currentSite, _] = useGenericStore<Site>("currentSite");

  // data related to each site
  const [siteData, setSiteData] = useGenericStore<SiteDataState>("siteData");
  const MLOutput = siteData[currentSite].MLOutput

  // function to update the current space resource entries
  const setMLOutput = useCallback((v: string) => {
    setSiteData({
      ...siteData,
      [currentSite]: {
        ...siteData[currentSite],
        MLOutput: v,
      }
    } as SiteDataState)
  }, [currentSite, siteData, setSiteData]);

  return (
    <div className="mx-1 mt-1">
      <CameraComponent cameraSerial={CameraSerials.SCIENCE_MICROSCOPE}/>
      <SiteSelectWidget pickerClassName="mt-4 mb-1"/>
      <div className="flex flex-row gap-3 content-end">
        <Input
          className="px-1 pt-1"
          label="ML Output"
          labelPlacement="outside"
          value={MLOutput}
          onValueChange={setMLOutput}
          type="number"
        />
        <Button className="self-end" onClick={() => setMLOutput("")}>
          Clear
        </Button>
      </div>
    </div>
  )
}

export default MicroscopeMLWidget;

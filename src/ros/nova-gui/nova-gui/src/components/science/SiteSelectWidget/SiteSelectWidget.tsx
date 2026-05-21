import React from "react";
import SegmentedPicker from "../../shared/components/SegmentedPicker/SegmentedPicker.tsx";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";

export interface SiteSelectWidgetProps {
  pickerClassName?: string,
  // how many sites to display in the picker, default 4, should be between 1 and 4.
  numSites?: number,
}

const siteColours = ["text-primary", "text-warning", "text-sky-300", "text-violet-300"]

const SiteSelectWidget: React.FC<SiteSelectWidgetProps> = (
  {
    pickerClassName, numSites
  }: SiteSelectWidgetProps) => {

  const [currentSite, setCurrentSite] = useGenericStore<Site>("currentSite");

  return (
    <SegmentedPicker
      fullWidth
      className={"grow " + (pickerClassName ?? "")}
      onIndexChange={setCurrentSite}
      selectedIndex={currentSite}
    >
      {siteColours.filter((_, i) => i < (numSites ? numSites : 4))
        .map((colour, index) => (
          <div className={colour}>Site {" " + (index + 1)}</div>
        ))

      }
    </SegmentedPicker>
  );
};

export default SiteSelectWidget;